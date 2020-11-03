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

// Declarare pini
#define BUZZ 15
#define BUTONspate 14
#define BUTONenter 12
#define BUTONfata 13
#define LDR A0
#define LED 2
#define VENT 16


//date pentru transformare citire fotorezistor in lux
#define VIN 5             //voltaj fotorezistor
#define R 10000           // rezistenta rezistor 


//variabile server
const char *ssid = "Desk Link";
const char *pass = ""; 
DNSServer server;
IPAddress apIP(192, 168, 4, 1);
const byte DNS_PORT = 53;

//variabile ESPUI
int graphId;
int webTemp;
int webUmi;
int webLum;
int webVent;
int webTimp;
int webAlarmOra;
int webAlarmMin;
int webAlarmSwitch, webAlarm;
int webOra, webMin, webSec;

uint8_t webNumAlarmOra, webNumAlarmMin, webNumOra,webNumMin, webNumSec;
//declarare module I2C
RtcDS3231<TwoWire> Ceas(Wire);     
Adafruit_SSD1306 display(128, 64, &Wire, -1);
RtcDateTime now = Ceas.GetDateTime();
   DS3231AlarmOne alarm1(
            now.Day(),
            webNumAlarmOra,
            webNumAlarmMin, 
            now.Second(),
            DS3231AlarmOneControl_HoursMinutesSecondsMatch);   
   
// Preluare temperatura si umiditate 
#define SIGdht 0
DHT dht(SIGdht, DHT11);

// Prelucrare citire LDR
 int luminozitate(){
           int raw = analogRead(LDR);
           float Vout = float(raw) * (VIN / float(1023));
           float RLDR = (R * (VIN - Vout))/Vout;
           int luxi=500/(RLDR/1000);
           return luxi;
        }

// Functii interfata web
void controlVentilatie(Control *sender, int stare)
{
  switch(stare){
    case S_ACTIVE: 
      digitalWrite(VENT,HIGH); 
      Serial.println("Motor Ventilatie Pornit");
      break;

    case S_INACTIVE:
      digitalWrite(VENT, LOW);
      Serial.println("Motor Ventilatie Oprit");
      break;
  }  
}

// Configurare alarma 
void oraAlarma( Control* sender, int type){
  webNumAlarmOra = sender->value.toInt();
   DS3231AlarmOne alarm1(
            now.Day(),
            webNumAlarmOra,
            webNumAlarmMin, 
            now.Second(),
            DS3231AlarmOneControl_HoursMinutesSecondsMatch);
            Ceas.SetAlarmOne(alarm1);
  Serial.println(webNumAlarmOra + " ora alarma");
}
void minAlarma( Control* sender, int type){
  webNumAlarmMin = sender->value.toInt();
    DS3231AlarmOne alarm1(
            now.Day(),
            webNumAlarmOra,
            webNumAlarmMin, 
            now.Second(),
            DS3231AlarmOneControl_HoursMinutesSecondsMatch);
            Ceas.SetAlarmOne(alarm1);
  Serial.println(webNumAlarmMin + " minut alarma");
}

void setup(){ 
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
  Ceas.SetAlarmOne(alarm1); 
  
  uint16_t tabCtrl =  ESPUI.addControl(ControlType::Tab, "Interactiune","Interactiune"); 
  uint16_t tabMediu = ESPUI.addControl(ControlType::Tab, "Mediu", "Mediu");
  uint16_t tabAlarm = ESPUI.addControl(ControlType::Tab, "Alarma", "Alarma");
  uint16_t tabCeas = ESPUI.addControl(ControlType::Tab, "Ajustare Ceas", "Ajustare Ceas");

  //tabCtrl
  webVent = ESPUI.addControl(ControlType::Switcher, "Control Ventilatie", "", ControlColor::Peterriver, tabCtrl, &controlVentilatie);
  
  //tabMediu
  webTemp = ESPUI.addControl(ControlType::Label, "Temperatura","", ControlColor::Wetasphalt, tabMediu);
  webUmi = ESPUI.addControl(ControlType::Label, "Umiditate", "", ControlColor::Wetasphalt, tabMediu);
  webTimp = ESPUI.addControl(ControlType::Label, "Ora","", ControlColor::Wetasphalt, tabMediu);

  //tabAlarm
  webAlarmOra = ESPUI.addControl(ControlType::Number, "Ora alarma", "", ControlColor::Emerald, tabAlarm, &oraAlarma);
  webAlarmMin = ESPUI.addControl(ControlType::Number, "Minut alarma", "", ControlColor::Emerald, tabAlarm, &minAlarma);
  webAlarmSwitch = ESPUI.addControl(ControlType::Switcher, "Stare alarma","", ControlColor::Emerald, tabAlarm);
  webAlarm = ESPUI.addControl(ControlType::Label, "Timp alarma", "", ControlColor::Emerald, tabAlarm);
  

  //tabCeas
  webOra = ESPUI.addControl(ControlType::Number, "Ore","", ControlColor::Carrot, tabCeas);
  webMin = ESPUI.addControl(ControlType::Number, "Minute","", ControlColor::Carrot, tabCeas);
  webSec = ESPUI.addControl(ControlType::Number, "Secunde","", ControlColor::Carrot, tabCeas);
  
  
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
        }
      else
        {
          Serial.println("Ceasul a pierdut timpul");
          Ceas.SetDateTime(RtcDateTime(__DATE__,__TIME__));  
        }
       }

  dht.begin();

  pinMode(BUTONspate, INPUT);
  pinMode(BUTONenter, INPUT);
  pinMode(BUTONfata, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  pinMode(VENT, OUTPUT);
  digitalWrite(BUZZ, LOW);

  
}

void loop(){
  now = Ceas.GetDateTime(); 
  ESPUI.print(webTemp, String(dht.readTemperature())+" °C");
  ESPUI.print(webUmi, String(dht.readHumidity())+" %");
  printTimp(now);
  server.processNextRequest();

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
    ESPUI.print(webTimp, datestring);
}
