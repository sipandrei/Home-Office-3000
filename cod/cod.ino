// de adaugat afisare oled

#include <Button2.h>
#include <cmath> 

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

#include <sunset.h>
#include <TimeLib.h>

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
int webAlarmOra, oraacum;
int webAlarmMin, minacum;
int webAlarmSwitch, webAlarm, webAlarmTimer, webPauza, secBirou, pauzaSec = 0, pauzaMin = 0; //se memoreaza secundele din pauza de masa
int webOra, webMin, webSec, webLat, webLon, webUtc, webApus, webRasarit;
bool vent = LOW, anulare = false, alarma = false, alarmaActiva = false, repetare = false, timerInceput = false, firstRun = true, inPauza = false;
uint8_t oraAlarma, minAlarma, offMin = -1, offSec = -2, numVent = 28, ziCurenta;
uint16_t minAdd = 32, oraAdd = 64, birouAdd = 10;
unsigned long milisecundeTrecut, milisecundeTrecutPauza = 0;

//declarare module 
RtcDS3231<TwoWire> Ceas(Wire);
EepromAt24c32<TwoWire> EepromCeas(Wire);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
RtcDateTime acum = Ceas.GetDateTime();

// variabile locatie
float latitudine, longitudine;
int UTC = 2, latAdd = 128, lonAdd = 140, utcAdd = 100;
SunSet sun;

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
  if(btn == butonEnter)
   {
    anulare = true;
   }
  //de adaugat pagina
  if(btn == butonEnter)
   {
    Serial.println("Buton enter apasat"); 
    if(vent == LOW)
      vent = HIGH;
      else
      vent = LOW;}

   if(btn == butonEnter)
   {
    Serial.println("Buton enter apasat"); 
    if(inPauza == false)
    {
      inPauza = true;
      pauzaMin = 0;
      pauzaSec = 0;
      }
    else
    {
      inPauza = false;
      }
    }
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
void afisarePauzaMasa( Control* sender, int value){
      ESPUI.print(webPauza,String(pauzaMin) + ":" + String(pauzaSec));
  }

//Ajustare ceas
int ora = 0,minu = 0,sec = 0;
void ajustareOra( Control* sender, int value){
  ora = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(acum.Year(), acum.Month(), acum.Day(), ora, minu, sec));
  Serial.println(ora+":");
}
void ajustareMinut( Control* sender, int value){
  minu = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(acum.Year(), acum.Month(), acum.Day(), ora, minu, sec));
  Serial.print(minu+":");
}
void ajustareSecunda( Control* sender, int value){
   sec = sender->value.toInt();
   Ceas.SetDateTime(RtcDateTime(acum.Year(), acum.Month(), acum.Day(), ora, minu, sec));
   Serial.print(sec);
}

void ajustareLatitudine( Control* sender, int value){
  latitudine = sender->value.toFloat();
  sun.setPosition(latitudine, longitudine, UTC);
  sun.setTZOffset(UTC);
  EepromCeas.SetMemory(latAdd, latitudine);
}
void ajustareLongitudine( Control* sender, int value){
  longitudine = sender->value.toFloat();
  sun.setPosition(latitudine, longitudine, UTC);
  sun.setTZOffset(UTC);
  EepromCeas.SetMemory(lonAdd, longitudine);
}
void ajustareZona( Control* sender, int value){
  UTC = sender->value.toInt();
  sun.setPosition(latitudine, longitudine, UTC);
  sun.setTZOffset(UTC);
  EepromCeas.SetMemory(utcAdd, UTC);
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
  EepromCeas.Begin(); 
  
  uint16_t tabCtrl =  ESPUI.addControl(ControlType::Tab, "Principal", "Principal");
  uint16_t tabAlarm = ESPUI.addControl(ControlType::Tab, "Alarma", "Alarma");
  uint16_t tabCeas = ESPUI.addControl(ControlType::Tab, "Ajustare Ceas si Locatie", "Ajustare Ceas si Locatie");

  //tabPrincipal
  webVent = ESPUI.addControl(ControlType::Switcher, "Control Ventilatie", "", ControlColor::Wetasphalt, tabCtrl, &controlVentilatie);
  webVentNum = ESPUI.addControl(ControlType::Number, "Introduceti temperatura la care porneste ventilatorul", String(numVent), ControlColor::Wetasphalt, tabCtrl, &tempVentilatie);
  webTemp = ESPUI.addControl(ControlType::Label, "Temperatura", "", ControlColor::Wetasphalt, tabCtrl);
  webUmi = ESPUI.addControl(ControlType::Label, "Umiditate", "", ControlColor::Wetasphalt, tabCtrl);
  webRasarit = ESPUI.addControl(ControlType::Label, "Rasarit", "", ControlColor::Wetasphalt, tabCtrl);
  webApus = ESPUI.addControl(ControlType::Label, "Apus", "", ControlColor::Wetasphalt, tabCtrl);

  //tabAlarm
  webAlarmOra = ESPUI.addControl(ControlType::Number, "Ora alarma", "", ControlColor::Emerald, tabAlarm, &inputOraAlarma);
  webAlarmMin = ESPUI.addControl(ControlType::Number, "Minut alarma", "", ControlColor::Emerald, tabAlarm, &inputMinAlarma);
  webAlarmSwitch = ESPUI.addControl(ControlType::Switcher, "Stare alarma", "", ControlColor::Emerald, tabAlarm, &activareAlarma);
  webAlarmTimer = ESPUI.addControl(ControlType::Number, "Introduceti numarul de secunde pentru a ajunge la birou", String(EepromCeas.GetMemory(birouAdd)), ControlColor::Emerald, tabAlarm, &timerBirou);
  webAlarm = ESPUI.addControl(ControlType::Label, "Timp Alarma", "", ControlColor::Emerald, tabAlarm, &timpAlarma);
  webPauza = ESPUI.addControl(ControlType::Label, "Timp petrecut in pauza de masa", "", ControlColor::Emerald, tabAlarm, &afisarePauzaMasa);
   
  //tabCeas
  webOra = ESPUI.addControl(ControlType::Number, "Ore", "", ControlColor::Carrot, tabCeas, &ajustareOra);
  webMin = ESPUI.addControl(ControlType::Number, "Minute", "", ControlColor::Carrot, tabCeas, &ajustareMinut);
  webSec = ESPUI.addControl(ControlType::Number, "Secunde", "", ControlColor::Carrot, tabCeas, &ajustareSecunda);
  webTimp = ESPUI.addControl(ControlType::Label, "Ora", "", ControlColor::Carrot, tabCeas);
  webLat = ESPUI.addControl(ControlType::Text, "Latitudine", "", ControlColor::Carrot, tabCeas, &ajustareLatitudine);
  webLon = ESPUI.addControl(ControlType::Text, "Longitudine", "", ControlColor::Carrot, tabCeas, &ajustareLongitudine);
  webUtc = ESPUI.addControl(ControlType::Text, "Zona de timp (UTC)", "", ControlColor::Carrot, tabCeas, &ajustareZona);

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
  
  dht.begin();
  
  Ceas.Enable32kHzPin(false);
  Ceas.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

  //adrese ora si minut alarma
  EepromCeas.SetMemory(0, minAdd);
  EepromCeas.SetMemory(1,oraAdd);

  latitudine = EepromCeas.GetMemory(latAdd);
  longitudine = EepromCeas.GetMemory(latAdd);
  UTC = EepromCeas.GetMemory(utcAdd);

  sun.setPosition(latitudine, longitudine, UTC);
  sun.setTZOffset(UTC);
    
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  pinMode(VENT, OUTPUT);
  digitalWrite(BUZZ, LOW);
  
  butonEnter.setTapHandler(pornireVentButon);
  
}

void loop() {
  acum = Ceas.GetDateTime();
  ESPUI.print(webTemp, String(dht.readTemperature()) + " °C");
  ESPUI.print(webUmi, String(dht.readHumidity()) + " %");
  printTimp(acum);
   
  //citire ora si minut alarma
  if(firstRun){
    secBirou = EepromCeas.GetMemory(birouAdd);
    minAlarma = EepromCeas.GetMemory(minAdd);
    oraAlarma = EepromCeas.GetMemory(oraAdd);
    ESPUI.print(webPauza,String(pauzaMin) + ":" + String(pauzaSec));
    ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
  firstRun = false;
  }

  if(ziCurenta != day()){
    sun.setCurrentDate(year(), month(), day());
  }
  
  ventilatieAuto();
   
  butonSpate.loop(); 
  butonEnter.loop();
  butonFata.loop();

  alarmare();
  mancand();
  soare();

  if(repetare == false)
    ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
  
  server.processNextRequest();

}

void alarmare(){
    if(alarma == true)
  {
    if(oraacum == oraAlarma && minacum == minAlarma)
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
  if(timerInceput == false)
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

  oraacum = dt.Hour();
  minacum = dt.Minute();
  ziCurenta = dt.Day();
  ESPUI.print(webTimp, datestring);
}

void mancand(){
 if(inPauza == true)
 {
    unsigned long milisecundePauza = millis();
    
    if(milisecundePauza - milisecundeTrecutPauza >= 1000)
    {
    milisecundeTrecutPauza = milisecundePauza;
    pauzaSec++;
    }
    
    if(pauzaSec>=60)
    {
      pauzaSec=0;
      ++pauzaMin;
    } 
    ESPUI.print(webPauza,String(pauzaMin) + ":" + String(pauzaSec));
  }
 
}

void soare(){
  int apus, rasarit;
  rasarit = abs(static_cast<int>(sun.calcSunrise()));
  apus = static_cast<int>(sun.calcSunset());
  ESPUI.print(webRasarit, String(rasarit/60)+":"+String(rasarit%60)+" am");
  ESPUI.print(webApus, String(apus/60)+":"+String(apus%60)+" pm");
}
