// pauza masa
// de adaugat afisare oled
// de adaugat calculare faze soare

#include <Button2.h>
#include <RtcDateTime.h>
#include <RtcDS3231.h>
#include <RtcUtility.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPtimeESP.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include <EepromAT24C32.h>

// Declarare pini
#define BUZZ 15
#define LDR A0
#define LED 2
#define VENT 16
#define oprireAlarma1 14
#define oprireAlarma2 13

Button2 butonSpate = Button2(14);
Button2 butonEnter = Button2(12);
Button2 butonFata = Button2(13);

//date pentru transformare citire fotorezistor in lux
#define VIN 5             //voltaj fotorezistor
#define R 10000           // rezistenta rezistor 


//variabile server
const char *ssid = "Desk Link";
const char *pass = "";
DNSServer server;
IPAddress apIP(192, 168, 4, 1);
const byte DNS_PORT = 53;

//variabile interfata web
int graphId;
int webTemp;
int webUmi;
int webLum;
int webVent, webVentNum;
int webTimp;
int webAlarmOra, oraNow;
int webAlarmMin, minNow;
int webAlarmSwitch, webAlarm, webAlarmTimer, secBirou;
int webOra, webMin, webSec;
bool vent = LOW, anulare = false, alarma = false, alarmaActiva = false, repetare = false, timerInceput = false, firstRun = true;
uint8_t oraAlarma, minAlarma, offMin = -1, offSec = -2, numVent = 28;
uint16_t minAdd = 32, oraAdd = 64, birouAdd = 10;
unsigned long milisecundeTrecut;

//declarare module I2C
RtcDS3231<TwoWire> Ceas(Wire);
EepromAt24c32<TwoWire> EepromCeas(Wire);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
RtcDateTime now = Ceas.GetDateTime();

//variabile pozitie solara


// Preluare temperatura si umiditate
#define SIGdht 0
DHT dht(SIGdht, DHT11);

// Luminozitate
int luminozitate() {
  int raw = analogRead(LDR);
  float Vout = float(raw) * (VIN / float(1023));
  float RLDR = (R * (VIN - Vout)) / Vout;
  int luxi = 500 / (RLDR / 1000);
  return luxi;
}

// Functii interfata web
// Ventilatie
void controlVentilatie(Control *sender, int stare)
{
  switch (stare) {
    case S_ACTIVE:
      //digitalWrite(VENT, HIGH);
      Serial.println("Motor Ventilatie Pornit");
      vent = true;
      break;

    case S_INACTIVE:
      //digitalWrite(VENT, LOW);
      Serial.println("Motor Ventilatie Oprit");
      vent = false;
      break;
  }
}

void tempVentilatie(Control* sender, int value)
{
  numVent = sender -> value.toInt();
}

void ventilatieAuto()
{
   digitalWrite(VENT, vent);
   if(dht.readTemperature() >= numVent && anulare == false)
      {
        vent = HIGH;   
      }
      else if(dht.readTemperature() < numVent)
      anulare = false;
}

void pornireVentButon(Button2& btn)
{
  //de adaugat pagina
  if(btn == butonEnter)
   {
    anulare = true;
    Serial.println("Buton enter apasat"); 
    if(vent == LOW)
      vent = HIGH;
      else
      vent = LOW;}
}

// Configurare alarma
void timpAlarma( Control* sender, int value){
      ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
  }
void inputOraAlarma( Control* sender, int value) {
  oraAlarma = sender->value.toInt();
  EepromCeas.SetMemory(oraAdd, oraAlarma);
  
  Serial.println(EepromCeas.GetMemory(oraAdd) + " ora alarma");
      ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
}
void inputMinAlarma( Control* sender, int value) {
  minAlarma = sender->value.toInt();
  EepromCeas.SetMemory(minAdd, minAlarma);
   
  Serial.println(EepromCeas.GetMemory(minAdd) + " minut alarma");
     ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
}
void activareAlarma(Control *sender, int value)
{
  switch (value) {
    case S_ACTIVE:
      alarma = true;
      EepromCeas.SetMemory(3, 1);
      Serial.println("Alarma activata!");
      break;

    case S_INACTIVE:
      alarma = false;
      EepromCeas.SetMemory(3, 0);
      Serial.println("Alarma dezactivata!");
      break;
  }
}
void timerBirou(Control *sender, int value){
  secBirou = sender->value.toInt()*1000;
  EepromCeas.SetMemory(birouAdd,secBirou);
}

//Ajustare ceas
int ora = 0,minu = 0,sec = 0;
void ajustareOra( Control* sender, int value){
  ora = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(now.Year(), now.Month(), now.Day(), ora, minu, sec));
  Serial.println(ora+":");
}
void ajustareMinut( Control* sender, int value){
  minu = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(now.Year(), now.Month(), now.Day(), ora, minu, sec));
  Serial.print(minu+":");
}
void ajustareSecunda( Control* sender, int value){
   sec = sender->value.toInt();
   Ceas.SetDateTime(RtcDateTime(now.Year(), now.Month(), now.Day(), ora, minu, sec));
   Serial.print(sec);
}

void setup() {
  secBirou = EepromCeas.GetMemory(10)*1000;
  
  ESPUI.setVerbosity(Verbosity::Verbose);
  Serial.begin(115200);
  Serial.println();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  IPAddress serverIp = WiFi.softAPIP();
  Serial.println(serverIp);

  server.start(DNS_PORT, "*", apIP);
  Serial.println("Server initializat");

  Wire.begin();

  uint16_t tabCtrl =  ESPUI.addControl(ControlType::Tab, "Ventilatie", "Ventilatie");
  uint16_t tabMediu = ESPUI.addControl(ControlType::Tab, "Mediu", "Mediu");
  uint16_t tabAlarm = ESPUI.addControl(ControlType::Tab, "Alarma", "Alarma");
  uint16_t tabCeas = ESPUI.addControl(ControlType::Tab, "Ajustare Ceas", "Ajustare Ceas");

  //tabVentilatie
  webVent = ESPUI.addControl(ControlType::Switcher, "Control Ventilatie", "", ControlColor::Peterriver, tabCtrl, &controlVentilatie);
  webVentNum = ESPUI.addControl(ControlType::Number, "Introduceti temperatura la care porneste ventilatorul", String(numVent), ControlColor::Peterriver, tabCtrl, &tempVentilatie);

  //tabMediu
  webTemp = ESPUI.addControl(ControlType::Label, "Temperatura", "", ControlColor::Wetasphalt, tabMediu);
  webUmi = ESPUI.addControl(ControlType::Label, "Umiditate", "", ControlColor::Wetasphalt, tabMediu);

  //tabAlarm
  webAlarmOra = ESPUI.addControl(ControlType::Number, "Ora alarma", "", ControlColor::Emerald, tabAlarm, &inputOraAlarma);
  webAlarmMin = ESPUI.addControl(ControlType::Number, "Minut alarma", "", ControlColor::Emerald, tabAlarm, &inputMinAlarma);
  webAlarmSwitch = ESPUI.addControl(ControlType::Switcher, "Stare alarma", "", ControlColor::Emerald, tabAlarm, &activareAlarma);
  webAlarmTimer = ESPUI.addControl(ControlType::Number, "Introduceti numarul de secunde pentru a ajunge la birou", String(EepromCeas.GetMemory(birouAdd)), ControlColor::Emerald, tabAlarm, &timerBirou);
  webAlarm = ESPUI.addControl(ControlType::Label, "Timp Alarma", "", ControlColor::Emerald, tabAlarm, &timpAlarma);

  //tabCeas
  webOra = ESPUI.addControl(ControlType::Number, "Ore", "", ControlColor::Carrot, tabCeas, &ajustareOra);
  webMin = ESPUI.addControl(ControlType::Number, "Minute", "", ControlColor::Carrot, tabCeas, &ajustareMinut);
  webSec = ESPUI.addControl(ControlType::Number, "Secunde", "", ControlColor::Carrot, tabCeas, &ajustareSecunda);
  webTimp = ESPUI.addControl(ControlType::Label, "Ora", "", ControlColor::Carrot, tabCeas);

  ESPUI.begin("Desk Link");
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  Ceas.Begin();
  if (!Ceas.IsDateTimeValid())
  {
    if (Ceas.LastError() != 0)
    {
      Serial.print("Eroare comunicare ceas = ");
      Serial.println(Ceas.LastError());
      if (Ceas.LastError() == 4);
      ESPUI.print(webTimp, "Verifica conexiune ceas");
    }
    else
    {
      Serial.println("Ceasul a pierdut timpul");
      Ceas.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    }
  }
  EepromCeas.Begin();
  dht.begin();
  
  Ceas.Enable32kHzPin(false);
  Ceas.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

  //adrese ora si minut alarma
  EepromCeas.SetMemory(0, minAdd);
  EepromCeas.SetMemory(1,oraAdd);
  
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  pinMode(VENT, OUTPUT);
  digitalWrite(BUZZ, LOW);
  
  butonEnter.setTapHandler(pornireVentButon);
  
}

void loop() {
  now = Ceas.GetDateTime();
  ESPUI.print(webTemp, String(dht.readTemperature()) + " °C");
  ESPUI.print(webUmi, String(dht.readHumidity()) + " %");
  printTimp(now);
   
  //citire ora si minut alarma
  if(firstRun){
    secBirou = EepromCeas.GetMemory(birouAdd);
    minAlarma = EepromCeas.GetMemory(minAdd);
    oraAlarma = EepromCeas.GetMemory(oraAdd);
    ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
  firstRun = false;
  }
  
  ventilatieAuto();
   
  butonSpate.loop(); 
  butonEnter.loop();
  butonFata.loop();

  alarmare();

  if(repetare == false)
    ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
  
  server.processNextRequest();

}

void alarmare(){
    if(alarma == true)
  {
    if(oraNow == oraAlarma && minNow == minAlarma)
     {
      if(repetare == false)
        alarmaActiva = true;
     }
     else if(secBirou/1000 == 0 && repetare == true && EepromCeas.GetMemory(birouAdd)!=0)
     {
      tone(BUZZ, 500, 10000);
      secBirou = EepromCeas.GetMemory(birouAdd);
      ESPUI.print(webAlarmTimer, String(secBirou/1000));
      repetare = false;
     }
      
    if(alarmaActiva == true)
    {
      if((digitalRead(oprireAlarma1) == HIGH && digitalRead(oprireAlarma2) == HIGH) or (luminozitate() >= 1 && oraAlarma<=7))
        {
          repetare = true;
          alarmaActiva = false;
          Serial.println("Alarma inchisa");
        }
      tone(BUZZ, 1500,2000);
    }

    if(repetare == true)
    {
      timpBirou();
    }
  }

}

void timpBirou(){
  unsigned long milisecundeBirou = millis();
  if(timerInceput = false)
  {
    milisecundeTrecut = milisecundeBirou;
    timerInceput = true;
  }
  
  if(milisecundeBirou - milisecundeTrecut >= 1000 && secBirou/1000>0)
  {
    milisecundeTrecut = milisecundeBirou;
    secBirou-=1000;
    }

  ESPUI.print(webAlarmTimer, String(secBirou/1000));
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printTimp(const RtcDateTime& dt)
{
  char datestring[14];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u:%02u:%02u"),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );

  oraNow = dt.Hour();
  minNow = dt.Minute();
  ESPUI.print(webTimp, datestring);
}
