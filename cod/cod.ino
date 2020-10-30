#include <RtcDS3231.h>
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
//char *html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Desk Link - Dashboard</title></head><body><center>Conectat</center><center><a href='/on'>Porneste buzzer </a><br/><a href='/off'> Opreste buzzer</a></center></body></html>";
DNSServer server;
IPAddress apIP(192, 168, 4, 1);
const byte DNS_PORT = 53;

//variabile ESPUI
int graphId;
int webTemp;
int webUmi;
int webLum;
int webVent;

//declarare module I2C
RtcDS3231<TwoWire> Ceas(Wire);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

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
// Ventilatie -- todo

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

void setup(){
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  IPAddress serverIp = WiFi.softAPIP();
  Serial.println(serverIp);

  server.start(DNS_PORT, "*", apIP);
  Serial.println("Server initializat");

  uint16_t tabCtrl =  ESPUI.addControl(ControlType::Tab, "Interactiune","Interactiune"); 
  uint16_t tabMediu = ESPUI.addControl(ControlType::Tab, "Mediu", "Mediu");
  uint16_t tabAlarm = ESPUI.addControl(ControlType::Tab, "Alarma", "Alarma");

  //tabCtrl
  webVent = ESPUI.addControl(ControlType::Switcher, "Control Ventilatie", "", ControlColor::Peterriver, tabCtrl, &controlVentilatie);
  
  //tabMediu
  webTemp = ESPUI.addControl(ControlType::Label, "Temperatura","", ControlColor::Wetasphalt, tabMediu);
  webUmi = ESPUI.addControl(ControlType::Label, "Umiditate", "", ControlColor::Wetasphalt, tabMediu);
  
  ESPUI.begin("Desk Link");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  
  dht.begin();

  pinMode(BUTONspate, INPUT);
  pinMode(BUTONenter, INPUT);
  pinMode(BUTONfata, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, LOW);
}

void loop(){

  ESPUI.print(webTemp, String(dht.readTemperature())+" °C");
  ESPUI.print(webUmi, String(dht.readHumidity())+" %");
  
  server.processNextRequest();

}
