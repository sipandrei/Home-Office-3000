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
int webVent, webVentNum;
int webTimp;
int webAlarmOra;
int webAlarmMin;
int webAlarmSwitch, webAlarm;
int webOra, webMin, webSec;
bool vent = false;
uint8_t webNumAlarmOra, webNumAlarmMin, offMin = -1, offSec = -2, numVent = 28;
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
DS3231AlarmOne alarmOff(
  now.Day(),
  offMin,
  offSec,
  now.Second(),
  DS3231AlarmOneControl_HoursMinutesSecondsMatch);

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
  if(dht.readTemperature() >= numVent && vent == false)
     digitalWrite(VENT,HIGH);
    else if(vent == false)
      digitalWrite(VENT,LOW);
}

void pornireVentButon()
{
  if(digitalRead(BUTONenter) == HIGH)
    if(vent == LOW)
      vent = HIGH;
      else
      vent = LOW;
}

// Configurare alarma
void timpAlarma(Control* sender, int type)
{
  ESPUI.print(webAlarm, String(webNumAlarmOra) + ":" + String(webNumAlarmMin));
}
void oraAlarma( Control* sender, int type) {
  webNumAlarmOra = sender->value.toInt();
  DS3231AlarmOne alarm1(
    now.Day(),
    webNumAlarmOra,
    webNumAlarmMin,
    now.Second(),
    DS3231AlarmOneControl_HoursMinutesSecondsMatch);
  Ceas.SetAlarmOne(alarm1);
  Serial.println(webNumAlarmOra + " ora alarma");
  ESPUI.print(webAlarm, String(webNumAlarmOra) + ":" + String(webNumAlarmMin));
}
void minAlarma( Control* sender, int type) {
  webNumAlarmMin = sender->value.toInt();
  DS3231AlarmOne alarm1(
    now.Day(),
    webNumAlarmOra,
    webNumAlarmMin,
    now.Second(),
    DS3231AlarmOneControl_HoursMinutesSecondsMatch);
  Ceas.SetAlarmOne(alarm1);
  Serial.println(webNumAlarmMin + " minut alarma");
  ESPUI.print(webAlarm, String(webNumAlarmOra) + ":" + String(webNumAlarmMin));
}
void activareAlarma(Control *sender, int value)
{
  switch (value) {
    case S_ACTIVE:
      Ceas.SetAlarmOne(alarm1);
      Serial.println("Alarma activata!");
      break;

    case S_INACTIVE:
      Ceas.SetAlarmOne(alarmOff);
      Serial.println("Alarma dezactivata!");
      break;
  }
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
  webAlarmOra = ESPUI.addControl(ControlType::Number, "Ora alarma", "", ControlColor::Emerald, tabAlarm, &oraAlarma);
  webAlarmMin = ESPUI.addControl(ControlType::Number, "Minut alarma", "", ControlColor::Emerald, tabAlarm, &minAlarma);
  webAlarmSwitch = ESPUI.addControl(ControlType::Switcher, "Stare alarma", "", ControlColor::Emerald, tabAlarm, &activareAlarma);
  webAlarm = ESPUI.addControl(ControlType::Label, "Timp alarma", "", ControlColor::Emerald, tabAlarm, &timpAlarma);

  //tabCeas
  webOra = ESPUI.addControl(ControlType::Number, "Ore", "", ControlColor::Carrot, tabCeas, &ajustareOra);
  webMin = ESPUI.addControl(ControlType::Number, "Minute", "", ControlColor::Carrot, tabCeas, &ajustareMinut);
  webSec = ESPUI.addControl(ControlType::Number, "Secunde", "", ControlColor::Carrot, tabCeas, &ajustareSecunda);
  webTimp = ESPUI.addControl(ControlType::Label, "Ora", "", ControlColor::Carrot, tabCeas);

  ESPUI.begin("Desk Link");
  ESPUI.print(webAlarm, String(webNumAlarmOra) + ":" + String(webNumAlarmMin));

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

  pinMode(BUTONspate, INPUT);
  pinMode(BUTONenter, INPUT);
  pinMode(BUTONfata, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  pinMode(VENT, OUTPUT);
  digitalWrite(BUZZ, LOW);

  Ceas.LatchAlarmsTriggeredFlags();
}

void loop() {
  now = Ceas.GetDateTime();
  ESPUI.print(webTemp, String(dht.readTemperature()) + " °C");
  ESPUI.print(webUmi, String(dht.readHumidity()) + " %");
  printTimp(now);
  ventilatieAuto();
  pornireVentButon();
  if(vent == true)
    digitalWrite(VENT, HIGH);
   else
    digitalWrite(VENT, LOW);
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
