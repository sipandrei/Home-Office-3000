#include <Button2.h>
#include <cmath> 

#include <RtcDateTime.h>
#include <RtcDS3231.h>
#include <RtcUtility.h>

#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <SSD1306Wire.h>

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
#define SIGdht 0

Button2 butonSpate = Button2(14);
Button2 butonEnter = Button2(12);
Button2 butonFata = Button2(13);

//date pentru transformare citire fotorezistor in lux
#define VIN 5             //voltaj fotorezistor
#define R 10000           // rezistenta rezistor 


//variabile server
const char *ssid = "Home Office 3000";
const char *pass = "";
DNSServer server;
IPAddress apIP(192, 168, 4, 1);
const byte DNS_PORT = 53;

int ora = 0,minu = 0,sec = 0;
int webTemp, webUmi;
int webVent, webVentNum;
int webTimp, webAlarmMin, minacum, secacum, webAlarmOra, oraacum;
int webAlarmSwitch, webAlarmStare, webAlarm, webAlarmTimer, webPauza, secBirou, pauzaSec = 0, pauzaMin = 0;
int webAn, webLuna, webOra, webMin, webSec, webLat, webLon, webUtc, webApus, webRasarit;
bool ecran = true, vent = LOW, anulare = false, alarma = false, alarmaActiva = false, repetare = false, timerInceput = false, firstRun = true, inPauza = false;
uint8_t oraAlarma, minAlarma, offMin = -1, offSec = -2, numVent = 28, ziCurenta;
uint16_t minAdd = 32, oraAdd = 64, birouAdd = 10;
unsigned long milisecundeTrecut, milisecundeTrecutPauza = 0, debounce = 0;
int tempe, humi, timer = 100, r, a, an, luna;
int pagina = 1;

//declarare module 
RtcDS3231<TwoWire> Ceas(Wire);
EepromAt24c32<TwoWire> EepromCeas(Wire);
SSD1306Wire display(0x3c, SDA, SCL);
RtcDateTime acum = Ceas.GetDateTime();
DHT dht(SIGdht, DHT11);

// variabile locatie
float latitudine, longitudine;
int UTC = 2, latAdd = 128, lonAdd = 140, utcAdd = 100;
SunSet sun;

// Se calculeaza luminozitatea din camera
long long raw, luxi;
double Vout, RLDR;
int luminozitate() {
  timer--;
  if(timer<=0)
  {
    raw = analogRead(LDR);
    Vout = double(raw) * (VIN / double(1023));
    RLDR = (R * (VIN - Vout)) / Vout;
    luxi = 500 / (RLDR / 1000);
    timer = 100;
  }
  return luxi;
}

//afisare
void SchimbarePag()
{
    if(pagina<1)
      pagina = 4;
       else if(pagina>4)
      pagina = 1;
  
  
    switch(pagina)
    {
      case 1:
        pagina1();        // pe prima pagina se afiseaza ora actuala si timpul petrecut in pauza de masa - se poate porni numaratoarea pentru pauza
        break;
      case 2:
        pagina2();      // pe a doua pagina se afiseaza rasaritul, apusul, temperatura, umiditatea si luminozitatea - se poate porni motorul pentru ventilatie
        break;
      case 3:
        pagina3();    // pe a treia pagina se afiseaza ora si minutul alarmei, starea alarmei si numarul de secunde pentru a ajunge la birou, dupa dezactivarea alarmei
        break;
      case 4: 
        pagina4();  // pe a patra pagina se afiseaza adresa ip
        break;
    }
}

void pagina1(){
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawHorizontalLine(0,18,128);
  display.drawString(64, 22, String(oraacum)+" : "+String(minacum)+" : "+String(secacum));
  display.drawHorizontalLine(0,44,128);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64,3,String(pauzaMin)+":"+String(pauzaSec));
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0,3,"Pauza");
  display.drawCircle(54,62,1);
  display.fillCircle(61,62,1);
  display.fillCircle(67,62,1);
  display.fillCircle(74,62,1);
}
void pagina2(){
  display.drawVerticalLine(64,0,40);
  display.drawHorizontalLine(0,15,128);
  display.drawHorizontalLine(0,40,128);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(32,20,String(tempe)+"°C");
  display.drawString(96,20,String(humi)+"%");
  display.setFont(ArialMT_Plain_10);
  display.drawString(32,2,String(r/60)+":"+String(r%60)+"am");
  display.drawString(96,2,String(a/60)+":"+String(a%60)+"pm");
  display.drawString(64,45,String(luminozitate())+" lux");
  display.fillCircle(54,62,1);
  display.drawCircle(61,62,1);
  display.fillCircle(67,62,1);
  display.fillCircle(74,62,1);
}
void pagina3(){
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0,10,"Alarma");
  display.drawString(0,37,"T.Birou");
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);  
  display.drawString(64, 9,String(oraAlarma)+" : "+ String(minAlarma));
  display.drawHorizontalLine(0,30,128);
  display.drawString(64,37, String(secBirou/1000));
  display.fillCircle(54,62,1);
  display.fillCircle(61,62,1);
  display.drawCircle(67,62,1);
  display.fillCircle(74,62,1);
  if(alarma == true)
    display.fillCircle(123,16,2);
  else
    display.drawCircle(123,16,2);
}
void pagina4(){
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64,22, "192.168.4.1");
  display.fillCircle(54,62,1);
  display.fillCircle(61,62,1);
  display.fillCircle(67,62,1);
  display.drawCircle(74,62,1);
}

// oprire si pornire ecran 
void pornireEcran()
{
    if((millis() - debounce) > 50)
    {
    if(digitalRead(oprireAlarma1) == HIGH && digitalRead(oprireAlarma2) == HIGH && oraacum != oraAlarma && minacum != minAlarma) 
      if(ecran == false)
        ecran = true;
       else
        ecran = false;
       debounce = millis();
    }
    if(ecran == false)
      display.displayOff();
    else
      display.displayOn();
}
//

void paginaSpate(Button2& btn){
  pagina--; display.clear();
}
void paginaFata(Button2& btn){
  pagina++; display.clear();
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
  
  switch (pagina){
    case 1:
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
    break;
    
    case 2:
    if(btn == butonEnter)
    {
    Serial.println("Buton enter apasat"); 
    if(vent == LOW)
      vent = HIGH;
      else
      vent = LOW;
    if( dht.readTemperature() >= numVent)
      anulare = true;
   }
    break;
    
    case 3:
    if(btn == butonEnter)
    {
      alarma = !alarma;
    }
    break;
  }
}
  //
 // Configurare alarma
void stareAlarma(Control* sender, int value){
  if(alarma == true)
    ESPUI.print(webAlarmStare, "Alarma Activata!");
  else
    ESPUI.print(webAlarmStare, "Alarma Dezactivata!");
}
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
    case B_DOWN:
      alarma = !alarma;
      EepromCeas.SetMemory(3, alarma);
      Serial.println("Alarma -  stare schimbata!");
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
   //
  //Ajustare ceas
void ajustareOra( Control* sender, int value){
  ora = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(acum.Year(), acum.Month(), acum.Day(), ora, minacum, secacum));
  Serial.println(ora+":");
}
void ajustareMinut( Control* sender, int value){
  minu = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(acum.Year(), acum.Month(), acum.Day(), oraacum, minu, secacum));
  Serial.print(minu+":");
}
void ajustareSecunda( Control* sender, int value){
   sec = sender->value.toInt();
   Ceas.SetDateTime(RtcDateTime(acum.Year(), acum.Month(), acum.Day(), oraacum, minacum, sec));
   Serial.print(sec);
}
void ajustareAn(Control* sender, int value){
  an = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(an, acum.Month(), acum.Day(), oraacum, minacum, secacum));
}
void ajustareLuna(Control* sender, int value){
  luna = sender->value.toInt();
  Ceas.SetDateTime(RtcDateTime(acum.Year(), luna, acum.Day(), oraacum, minacum, secacum));
}
  //
 // ajustare pozitie geografica
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
 //
//

void setup() {   
  Serial.begin(115200);
  Serial.println();

  // configurare si initializare punct de acces 
  ESPUI.setVerbosity(Verbosity::Verbose);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  IPAddress serverIp = WiFi.softAPIP();
  Serial.println(serverIp);

  server.start(DNS_PORT, "*", apIP);
  Serial.println("Server initializat");
  // 
 
  Wire.begin();
  EepromCeas.Begin(); 
  
  secBirou = EepromCeas.GetMemory(10)*1000;
  
  // configurare interfata web
    //taburi 
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
  webAlarmSwitch = ESPUI.addControl(ControlType::Button, "Activare alarma", "", ControlColor::Emerald, tabAlarm, &activareAlarma);
  webAlarmStare = ESPUI.addControl(ControlType::Label, "Stare alarma", "", ControlColor::Emerald, tabAlarm, &stareAlarma);
  webAlarmTimer = ESPUI.addControl(ControlType::Number, "Introduceti numarul de secunde pentru a ajunge la birou", String(EepromCeas.GetMemory(birouAdd)), ControlColor::Emerald, tabAlarm, &timerBirou);
  webAlarm = ESPUI.addControl(ControlType::Label, "Timp Alarma", "", ControlColor::Emerald, tabAlarm, &timpAlarma);
  webPauza = ESPUI.addControl(ControlType::Label, "Timp petrecut in pauza de masa", "", ControlColor::Emerald, tabAlarm, &afisarePauzaMasa);
   
    //tabCeas
  webTimp = ESPUI.addControl(ControlType::Label, "Ora", "", ControlColor::Carrot, tabCeas);
  webOra = ESPUI.addControl(ControlType::Number, "Ore", "", ControlColor::Carrot, tabCeas, &ajustareOra);
  webMin = ESPUI.addControl(ControlType::Number, "Minute", "", ControlColor::Carrot, tabCeas, &ajustareMinut);
  webSec = ESPUI.addControl(ControlType::Number, "Secunde", "", ControlColor::Carrot, tabCeas, &ajustareSecunda);
  webAn = ESPUI.addControl(ControlType::Number, "An", "", ControlColor::Carrot, tabCeas, &ajustareAn);
  webLuna = ESPUI.addControl(ControlType::Number, "Luna", "", ControlColor::Carrot, tabCeas, &ajustareLuna);
  webLat = ESPUI.addControl(ControlType::Number, "Latitudine", "", ControlColor::Carrot, tabCeas, &ajustareLatitudine);
  webLon = ESPUI.addControl(ControlType::Number, "Longitudine", "", ControlColor::Carrot, tabCeas, &ajustareLongitudine);
  webUtc = ESPUI.addControl(ControlType::Number, "Zona de timp (UTC)", "", ControlColor::Carrot, tabCeas, &ajustareZona);

  ESPUI.begin("Home Office 3000");
  //
  
  //initializare ecran
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  //

  // initializare si verificare ceas
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
 
  Ceas.Enable32kHzPin(false);
  Ceas.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  //
  
  dht.begin();
  
  //adrese ora si minut alarma
  EepromCeas.SetMemory(0, minAdd);
  EepromCeas.SetMemory(1,oraAdd);
  //
  
  // pozitionare geografica
  latitudine = EepromCeas.GetMemory(latAdd);
  longitudine = EepromCeas.GetMemory(latAdd);
  UTC = EepromCeas.GetMemory(utcAdd);

  sun.setPosition(latitudine, longitudine, UTC);
  sun.setTZOffset(UTC);
  //
    
  // setare mod pini
  pinMode(LED, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  pinMode(VENT, OUTPUT);
  digitalWrite(BUZZ, LOW);
  //
   
  // configurare butoane
  butonEnter.setTapHandler(pornireVentButon);
  butonFata.setTapHandler(paginaFata);
  butonSpate.setTapHandler(paginaSpate);
  //
}

void loop() {
  acum = Ceas.GetDateTime();                      // ora actuala
  tempe = dht.readTemperature();                  // temperatura actuala
  humi = dht.readHumidity();                      // umiditatea actuala
  ESPUI.print(webTemp, String(tempe) + " °C");    // afisare temperatura
  ESPUI.print(webUmi, String(humi) + " %");       // afisare umiditate
  printTimp(acum);                                // afisare ora
   
  // citire ora si minut alarma
  if(firstRun){
    secBirou = EepromCeas.GetMemory(birouAdd);
    minAlarma = EepromCeas.GetMemory(minAdd);
    oraAlarma = EepromCeas.GetMemory(oraAdd);
    ESPUI.print(webPauza,String(pauzaMin) + ":" + String(pauzaSec));
    ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));
  firstRun = false;
  }
  //

  if(alarma == true)
    ESPUI.print(webAlarmStare, "Alarma Activata!");
  else
    ESPUI.print(webAlarmStare, "Alarma Dezactivata!");
  
  if(ziCurenta != day()){
    sun.setCurrentDate(an, luna, day());
  }
   
  butonSpate.loop(); 
  butonEnter.loop();
  butonFata.loop();
  
  ventilatieAuto(); // ventilatie automata
  alarmare();       // alarma
  mancand();        // pauza de masa
  soare();          // apus si rasarit

  // apelare functii afisare ecran
  SchimbarePag();
  display.display();
  pornireEcran();
  //

  if(repetare == false)
    ESPUI.print(webAlarm, String(oraAlarma) + ":" + String(minAlarma));

  display.clear();
  server.processNextRequest();
}

void alarmare(){
    if(alarma == true)                                           // daca este alarma pornita
  {
    if(oraacum == oraAlarma && minacum == minAlarma)           // daca ora actuala este ora alarmei
     {
      if(repetare == false)                                  // daca nu a fost oprita alarma
        alarmaActiva = true;                                // alarma porneste
     }
     else if(secBirou/1000 == 0 && repetare == true && EepromCeas.GetMemory(birouAdd)!=0) // daca a trecut timpul de ajuns la birou dupa oprirea alarmei
     {
      tone(BUZZ, 500, 5000);                                            // emite sunete
      secBirou = EepromCeas.GetMemory(birouAdd);                       // cronometrul e resetat
      ESPUI.print(webAlarmTimer, String(secBirou/1000));                                  
      repetare = false;                                                              
     }
      
    if(alarmaActiva == true)
    {
      if((digitalRead(oprireAlarma1) == HIGH && digitalRead(oprireAlarma2) == HIGH) or (luminozitate() >= 1 && oraAlarma<=7)) // oprire alarma utilizand doua butoane sau lumina din camera - dimineata
        {
          repetare = true;                       // nu se mai repeta de data asta
          alarmaActiva = false;                 // alarma oprita
          Serial.println("Alarma inchisa");
        }
      tone(BUZZ, 1500,2000); // ton alarma
    }

    if(repetare == true)  // daca a fost alarma oprita
    {
      timpBirou();      // cronometru
    }
  }

}

// cronometrul efectiv
void timpBirou(){
  unsigned long milisecundeBirou = millis();
  if(timerInceput == false && secBirou == 0) 
  {
    milisecundeTrecut = milisecundeBirou;
    timerInceput = true;
  }
  
  if(milisecundeBirou - milisecundeTrecut >= 1000 && secBirou/1000>0) 
  {
    milisecundeTrecut = milisecundeBirou;
    secBirou-=1000;
    }
    
  if(timerInceput == true && secBirou == 0)
  {
    timerInceput = false, tone(BUZZ, 500, 500), secBirou = EepromCeas.GetMemory(birouAdd);;
  }
  ESPUI.print(webAlarmTimer, String(secBirou/1000));
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

// afisare ora , minutul si secunda
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
  secacum = dt.Second();
  ziCurenta = dt.Day();
  an = dt.Year();
  luna = dt.Month();
  ESPUI.print(webTimp, datestring);
}

// pauza de masa
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

// rasarit si apus
void soare(){
  int rasarit, apus;
  rasarit = abs(static_cast<int>(sun.calcSunrise()));
  apus = static_cast<int>(sun.calcSunset());
  r = rasarit;
  a = apus;
  ESPUI.print(webRasarit, String(rasarit/60)+":"+String(rasarit%60)+" am");
  ESPUI.print(webApus, String(apus/60)+":"+String(apus%60)+" pm");
}
