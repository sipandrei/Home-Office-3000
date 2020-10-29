#include <RtcDS3231.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPtimeESP.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPUI.h>

// Declarare pini
#define BUZZ 15
#define BUTONspate 14
#define BUTONenter 12
#define BUTONfata 13
#define LDR A0
#define LED 16


//date pentru transformare citire fotorezistor zin lux
#define VIN 5 //voltaj fotorezistor
#define R 10000 // rezistenta rezistor 

//variabile server
const char *ssid = "Desk Link";
const char *pass = ""; 
AsyncWebServerRequest *request;
char *html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Desk Link - Dashboard</title></head><body><center>Conectat</center><center><a href='/on'>Porneste buzzer </a><a href='/off'> Opreste buzzer</a></center></body></html>";
AsyncWebServer server(80);

//declarare module I2C
RtcDS3231<TwoWire> Ceas(Wire);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Preluare temperatura si umiditate 
#define SIGdht 0
DHT dht(SIGdht, DHT11);

// Prelucrare citire LDR
 int luminozitate(int raw){
           float Vout = float(raw) * (VIN / float(1023));
           float RLDR = (R * (VIN - Vout))/Vout;
           int luxi=500/(RLDR/1000);
           return luxi;
        }

void handleRoot(AsyncWebServerRequest *request) {
 Serial.println("You called root page");
 request -> send(200, "text/html", html); //Send web page
}

void buzzOn(AsyncWebServerRequest *request){
  Serial.println("Buzz on");
  request -> send(200, "text/html", html);
  tone(BUZZ, 3000);
}

void buzzOff(AsyncWebServerRequest *request){
  Serial.println("Buzz off");
  request -> send(200, "text/html", html);
  digitalWrite(BUZZ, LOW);
}

void setup(){
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  IPAddress serverIp = WiFi.softAPIP();
  Serial.println(serverIp);
  server.on("/", handleRoot);
  server.on("/on", buzzOn);
  server.on("/off", buzzOff);
  server.begin();
  Serial.println("Server initializat");

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
  int citireLDR = analogRead(LDR);
  int lux = luminozitate(citireLDR);
  ArduinoOTA.handle();
  Serial.println(lux);
  delay(1000);

}
