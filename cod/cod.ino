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
#define LED 16


//date pentru transformare citire fotorezistor in lux
#define VIN 5             //voltaj fotorezistor
#define R 10000           // rezistenta rezistor 

//variabile server
const char *ssid = "Desk Link";
const char *pass = ""; 
//char *html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Desk Link - Dashboard</title></head><body><center>Conectat</center><center><a href='/on'>Porneste buzzer </a><br/><a href='/off'> Opreste buzzer</a></center></body></html>";
DNSServer server;
IPAddress apIP(192, 168, 1, 1);
const byte DNS_PORT = 53;

//variabile ESPUI
int graphId;
int webTemp;
int webUmi;
int webLum;

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
void butonGrafice(Control *sender, int stare)
{
  switch(stare){
    case B_DOWN: 
    ESPUI.clearGraph(graphId); 
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

  //graphId = ESPUI.graph("Temperatura", ControlColor::Wetasphalt);
  //ESPUI.button("", &butonGrafice,ControlColor::Wetasphalt, "Sterge grafic");
  webTemp = ESPUI.label("Temperatura", ControlColor::Wetasphalt);
  webUmi = ESPUI.label("Umiditate", ControlColor::Wetasphalt);
  webLum = ESPUI.label("Luminozitate", ControlColor::Wetasphalt);
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
  /*
  int citireLDR = analogRead(LDR);
  int lux = luminozitate(citireLDR);
  Serial.println(lux);
  delay(1000);
  */
  //ESPUI.addGraphPoint(graphId, dht.readHumidity());
  ESPUI.print(webTemp, String(dht.readTemperature())+" °C");
  ESPUI.print(webUmi, String(dht.readHumidity())+" %");
  ESPUI.print(webLum, "test");
  server.processNextRequest();

}
