#include <Arduino.h>
/*
ESPink-4.2
----------------------
https://remoteqth.com

 ___               _        ___ _____ _  _
| _ \___ _ __  ___| |_ ___ / _ \_   _| || |  __ ___ _ __
|   / -_) '  \/ _ \  _/ -_) (_) || | | __ |_/ _/ _ \ '  \
|_|_\___|_|_|_\___/\__\___|\__\_\|_| |_||_(_)__\___/_|_|_|


This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Based on
Display test for LaskaKit ESPink-4.2"
-------- ESPink pinout -------
MOSI/SDI 23
CLK/SCK 18
SS 5 //CS
DC 17
RST 16
BUSY 4
-------------------------------
Email:podpora@laskakit.cz
Web:laskakit.cz

HARDWARE ESP32 Dev Module
IDE 1.8.19
Použití knihovny GxEPD2 ve verzi 1.5.2 v adresáři: /home/dan/Arduino/libraries/GxEPD2
Použití knihovny Adafruit_GFX_Library ve verzi 1.11.3 v adresáři: /home/dan/Arduino/libraries/Adafruit_GFX_Library
Použití knihovny Adafruit_BusIO ve verzi 1.14.1 v adresáři: /home/dan/Arduino/libraries/Adafruit_BusIO
Použití knihovny Wire ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Wire
Použití knihovny SPI ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/SPI
Použití knihovny WiFi ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/WiFi
Použití knihovny AsyncTCP ve verzi 1.1.1 v adresáři: /home/dan/Arduino/libraries/AsyncTCP
Použití knihovny ESPAsyncWebServer ve verzi 1.2.3 v adresáři: /home/dan/Arduino/libraries/ESPAsyncWebServer
Použití knihovny FS ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/FS
Použití knihovny AsyncElegantOTA ve verzi 2.2.7 v adresáři: /home/dan/Arduino/libraries/AsyncElegantOTA
Použití knihovny Update ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Update
Použití knihovny PubSubClient ve verzi 2.8 v adresáři: /home/dan/Arduino/libraries/PubSubClient

Changelog:
2023-08-17 - First init

mosquitto_pub -h 54.38.157.134 -t OK1HRA/0/ROT/Azimuth -m '83'
mosquitto_sub -v -h 54.38.157.134 -t 'OK1HRA/0/ROT/#'

*/
//-------------------------------------------------------------------------------------------------------

#define REV 20230821
#define OTAWEB                    // enable upload firmware via web
#define MQTT                      // enable MQTT
#include <esp_adc_cal.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
// source https://oleddisplay.squix.ch/ - must copy via clipboard!
#include "Open_Sans_Condensed_Light_80.h"
#include "Open_Sans_Condensed_Light_16.h"
#include "Open_Sans_Condensed_Bold_20.h"
// #include "Open_Sans_Condensed_Light_60.h"
// #include "Open_Sans_Condensed_Light_40.h"
// #include "Open_Sans_Condensed_Light_20.h"
// #include "Open_Sans_Condensed_Bold_40.h"

// https://rop.nl/truetype2gfx/
// #include "Logisoso250pt7b.h"
// display.setFont(&Logisoso250pt7b);

// #define SLEEP                    // Uncomment so board goes to sleep after printing on display
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 10          // Time ESP32 will go to sleep (in seconds)
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4)); // GDEW042T2 400x300, UC8176 (IL0398)
//GxEPD2_3C<GxEPD2_420c_Z21, GxEPD2_420c_Z21::HEIGHT> display(GxEPD2_420c_Z21(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEQ042Z21 400x300, UC8276

const String mainHWdevice[3][2] = {
    {"/ROT/", "IP rotator"},  // 0
    {"/WX/", "WX station"},   // 1
    {"topic", "name"},        // 2
};
int mainHWdeviceSelect = -1;  //0 = IP rotator, 1 = WX station
String TOPIC = "";        // same as 'location' on IP rotator
String ROT_TOPIC = "";    // mainHWdevice[mainHWdeviceSelect][0]
String WX_TOPIC = "";    // mainHWdevice[mainHWdeviceSelect][0]
byte mqttBroker[4]={0,0,0,0}; // MQTT broker IP address
int MQTT_PORT = 0;         // MQTT broker port
IPAddress mqtt_server_ip(mqttBroker[0], mqttBroker[1], mqttBroker[2], mqttBroker[3]);       // MQTT broker IP address
String SSID = "";
String PSWD = "";
unsigned int eInkRotation = 1; // 1 USB TOP, 3 USB DOWN | 0 default, 1 90°CW, 2 180°CW, 3 90°CCW
int OfflineTimeout = 5;   // minutes
bool eInkNegativ = true;  // not implemented!
int DesignSkin = 0;       // not implemented!
// #define SDTEST_TEXT_PADDING 25
#define SD_CS 27
SPIClass spiSD(HSPI); // Use HSPI for SD card
File myFile;
String ConfigFile="/setup.cfg";
char charConfigFile[11]; // length +1
int microSDlines = 0;

// ROT
int Azimuth       = -42;
int AzimuthTmp    = -42;
int AzimuthStart  = 0;
String Name       = "";
int Status        = 4;
bool eInkNeedRefresh = true;
bool eInkOfflineDetect = false;
long RxMqttTimer=0;

//WX
float Temperature = 7.3;
float HumidityRel = 0;
float Pressure = 0;
int WindDir = 0;
float WindSpeedAvg = 0;
float WindSpeedMaxPeriod = 0;

const int SdCardPresentPin = 33;
bool SdCardPresentStatus   = false;

int Az=0;
char buf[21];
#define RAD_TO_DEG 57.295779513082320876798154814105

// ntp
#include "time.h"
const char* ntpServer = "pool.ntp.org";
// const char* ntpServer = "tik.cesnet.cz";
// const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

int DebuggingOutput = 1;  // 0-off | 1-Serial

#define WIFI
#include <WiFi.h>
// #include <ETH.h>
// int SsidPassSize = (sizeof(SsidPass)/sizeof(char *))/2; //array size
// int SelectSsidPass = -1;
#define wifi_max_try 10             // Number of try
unsigned long WifiTimer = 0;
unsigned long WifiReconnect = 30000;

unsigned int RunApp = 255;
#if defined(OTAWEB)
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <AsyncElegantOTA.h>
  AsyncWebServer OTAserver(80);
#endif

#if defined(MQTT)
  #include <PubSubClient.h>
  // #include "PubSubClient.h" // lokalni verze s upravou #define MQTT_MAX_PACKET_SIZE 128
  WiFiClient espClient;
  PubSubClient mqttClient(espClient);
  // PubSubClient mqttClient(server, 1883, callback, ethClient);
  long lastMqttReconnectAttempt = 0;
  boolean MQTT_LOGIN      = 0;          // enable MQTT broker login
  // char MQTT_USER= 'login';    // MQTT broker user login
  // char MQTT_PASS= 'passwd';   // MQTT broker password
  const int MqttBuferSize = 1000; // 1000
  char mqttTX[MqttBuferSize];
  char mqttPath[MqttBuferSize];
  long MqttStatusTimer[2]{1500,1000};
#endif

//-------------------------------------------------------------------------------------------------------
void setup(void) {
  pinMode(SdCardPresentPin, INPUT);
  pinMode(2, OUTPUT);    // Set epaper transistor as output
  digitalWrite(2, HIGH); // Surn on epaper transistor
  delay(100);            // Delay so it has time to turn on
  display.init();
  display.setRotation(eInkRotation); // 1 USB TOP, 3 USB DOWN | 0 default, 1 90°CW, 2 180°CW, 3 90°CCW

  Serial.begin(115200);
  Serial.println();
  Serial.print("e-ink rev ");
  Serial.println(REV);

  SdCardPresentStatus=!digitalRead(SdCardPresentPin);
  Serial.println("Check microSD ");

  if(SdCardPresentStatus != true) {
    Serial.print("micro SD card is not inserted");
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&Open_Sans_Condensed_Bold_20);
    display.setCursor(30, 120);
    display.println("!");
    display.setCursor(30, 160);
    display.println("micro SD card is not inserted");
    display.setCursor(30, 190);
    display.println("Please insert card with");
    display.setCursor(30, 220);
    display.println("setup.cfg config file");
    display.display(false);
    while(SdCardPresentStatus != true) {
      delay(1000);
      SdCardPresentStatus=!digitalRead(SdCardPresentPin);
    }
  }
  SDtest();

  display.fillScreen(GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&Open_Sans_Condensed_Bold_20);
  display.setCursor(70, 150);
  display.println("Connecting");
  display.setFont(&Open_Sans_Condensed_Light_16);
  display.setCursor(90, 190);
  display.println("MicroSD import "+String(microSDlines)+" values");
    display.fillCircle(80, 190-7, 3, GxEPD_WHITE);
  display.setCursor(90, 220);
  display.println("WiFi...");
    display.fillCircle(80, 220-7, 3, GxEPD_WHITE);
  display.setFont(&Open_Sans_Condensed_Light_16);
  display.setCursor(200, 385);
  display.print(REV);
  display.display(false);

  // display.fillScreen(GxEPD_BLACK);

  #if defined(WIFI)
    // WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID.c_str(), PSWD.c_str());
    Serial.print("Connecting ssid "+String(SSID)+" ");
    int count_try = 0;
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      count_try++;    // Increase try counter
      if ( count_try >= wifi_max_try ) {
        Serial.println("\n");
        Serial.println("Impossible to establish WiFi connexion");

        print_wifi_error();
      }
    }
    Serial.println("");
    Serial.print("WIFI connected with IP ");
    Serial.println(WiFi.localIP());
    Serial.print("WIFI dBm: ");
    Serial.println(WiFi.RSSI());

    #if defined(MQTT)
      if (MQTT_LOGIN == true){
        // if (mqttClient.connect("esp32gwClient", MQTT_USER, MQTT_PASS)){
        //   AfterMQTTconnect();
        // }
      }else{
        mqttClient.setServer(mqtt_server_ip, MQTT_PORT);
        Serial.println("EthEvent-MQTTclient");
        mqttClient.setCallback(MqttRx);
        Serial.println("EthEvent-MQTTcallback");
        lastMqttReconnectAttempt = 0;

        char charbuf[50];
         WiFi.macAddress().toCharArray(charbuf, 18);
        if (mqttClient.connect(charbuf)){
          Serial.print("EthEvent-maccharbuf ");
          Serial.println(charbuf);
          mqttReconnect();
        }
      }
    #endif

  #endif

  #if defined(OTAWEB)
    OTAserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "e-ink rev "+String(REV)+" | PSE QSY to /update");
    });
    AsyncElegantOTA.begin(&OTAserver);    // Start ElegantOTA
    OTAserver.begin();
  #endif
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

//-------------------------------------------------------------------------------------------------------
void loop(void) {
  Watchdog();
  Mqtt();
  eInkRefresh();

  #if defined(OTAWEB)
   AsyncElegantOTA.loop();
  #endif
}
//-------------------------------------------------------------------------------------------------------
void eInkRefresh(){
  static long eInkRefreshTimer = -5000;
  // ROT
  if( mainHWdeviceSelect==0 && eInkNeedRefresh==true && millis()-eInkRefreshTimer > 5000 && Azimuth!=-42 && Name != "" ){
      display.fillScreen(GxEPD_BLACK);

      if(Azimuth>=0){
        DirectionalRosette(AzimuthShifted(Azimuth), 150, 145, 130);
      }

      display.setTextColor(GxEPD_WHITE);
      display.setFont(&Open_Sans_Condensed_Light_80);
      if(Azimuth>=0){
        if(AzimuthShifted(Azimuth)>99){
          display.setCursor(170, 355);
        }else if(AzimuthShifted(Azimuth)<100 && AzimuthShifted(Azimuth)>9){
          display.setCursor(200, 355);
        }else if(AzimuthShifted(Azimuth)<10){
          display.setCursor(230, 355);
        }
        display.println(AzimuthShifted(Azimuth));
        display.setCursor(270, 310);
        display.setFont(&Open_Sans_Condensed_Bold_20);
        display.println("o");
      }else{
        display.setCursor(170, 355);
        display.println("n/a");
      }
      int ZZshift=2;
      display.setFont(&Open_Sans_Condensed_Light_16);
      display.setCursor(15, 285+4*ZZshift);
      display.println(String(SSID)+" "+String(WiFi.RSSI())+" dBm");
      display.setCursor(15, 310+3*ZZshift);
      display.print(WiFi.localIP());
      display.setFont(&Open_Sans_Condensed_Bold_20);
      display.setCursor(15, 335+2*ZZshift);
      display.println(Name);
      display.setFont(&Open_Sans_Condensed_Light_16);
      display.setCursor(15, 360+ZZshift);
      display.println(String(TOPIC)+"#");
      display.setCursor(15, 385);
      UtcTime(1).toCharArray(buf, 21);
      display.println("UTC "+String(buf));
      if(eInkOfflineDetect==true){
        display.setCursor(185, 385);
        display.setFont(&Open_Sans_Condensed_Bold_20);
        display.print("OFF >"+String(OfflineTimeout)+"min");
      }else{
        display.setCursor(200, 385);
        display.print(REV);
      }
      display.display(false);
      eInkNeedRefresh=false;
      eInkRefreshTimer=millis();
  // WX
  }else if( mainHWdeviceSelect==1 && eInkNeedRefresh==true && millis()-eInkRefreshTimer > 10000 ){
        display.fillScreen(GxEPD_BLACK);

        display.setTextColor(GxEPD_WHITE);
        display.setFont(&Open_Sans_Condensed_Light_80);
        int Xshift=0;
        if(Temperature<0){
          Xshift=-20;
        }else{
          Xshift=0;
        }
        if(abs(Temperature)>99){
          display.setCursor(40+Xshift, 80);
        }else if(abs(Temperature)<100 && abs(Temperature)>9){
          display.setCursor(70+Xshift, 80);
        }else if(abs(Temperature)<10){
          display.setCursor(100+Xshift, 80);
        }
        String str = String(Temperature);
        String subStr = str.substring(0, str.length() - 1);
        display.println(String(subStr)+" C");
        display.setCursor(190, 30);
        display.setFont(&Open_Sans_Condensed_Bold_20);
        display.println("o");

        display.drawLine(15, 100, 285, 100, 2);
        int XX = (285-15)/100*HumidityRel+15;
        display.fillCircle(XX, 100, 3, GxEPD_WHITE);

        display.setFont(&Open_Sans_Condensed_Light_16);
        display.setCursor(15, 125);
        display.println("Relative humidity "+String((int)HumidityRel)+"%");
        display.setCursor(15, 150);
        display.println("Pressure "+String((int)Pressure)+" hpa");

        display.setFont(&Open_Sans_Condensed_Light_80);
        if(abs(WindSpeedMaxPeriod)>99){
          display.setCursor(0, 260);
        }else if(abs(WindSpeedMaxPeriod)<100 && abs(WindSpeedMaxPeriod)>9){
          display.setCursor(30, 260);
        }else if(abs(WindSpeedMaxPeriod)<10){
          display.setCursor(60, 260);
        }
        if(WindSpeedMaxPeriod>0){
          display.println((int)WindSpeedMaxPeriod);
          // display.setFont(&Open_Sans_Condensed_Bold_20);
          display.setFont(&Open_Sans_Condensed_Light_16);
          display.setCursor(40, 285);
          display.println("gust m/s");
        }

        // int Pressure = 0;
        // int WindSpeedAvg = 0;

        DirectionalRosette(WindDir, 200, 270, 80);

        int ZZshift=2;
        // display.setFont(&Open_Sans_Condensed_Bold_20);
        // display.println(Name);
        display.setFont(&Open_Sans_Condensed_Light_16);
        display.setCursor(15, 285+4*ZZshift);
        display.setCursor(15, 310+3*ZZshift);
        display.println(String(SSID)+" "+String(WiFi.RSSI())+" dBm");
        display.setCursor(15, 335+2*ZZshift);
        display.print(WiFi.localIP());
        display.setFont(&Open_Sans_Condensed_Light_16);
        display.setCursor(15, 360+ZZshift);
        display.println(String(TOPIC)+"#");
        display.setCursor(15, 385);
        UtcTime(1).toCharArray(buf, 21);
        display.println("UTC "+String(buf));
        if(eInkOfflineDetect==true){
          display.setCursor(185, 385);
          display.setFont(&Open_Sans_Condensed_Bold_20);
          display.print("OFF >"+String(OfflineTimeout)+"min");
        }else{
          display.setCursor(200, 385);
          display.print(REV);
        }

        display.display(false);
        eInkNeedRefresh=false;
        eInkRefreshTimer=millis();
  }
}
//------------------------------------------------------------------------------
int AzimuthShifted(int DEG){
  DEG=DEG+AzimuthStart; // 246 + 390 > 236
  if(DEG>359){
    DEG=DEG-360;
  }
  return DEG;
}

//-------------------------------------------------------------------------------------------------------
void Watchdog(){

  static unsigned long SdCardTimer = millis();
  if(millis()-SdCardTimer > 1000){
    SdCardPresentStatus=!digitalRead(SdCardPresentPin);
    // Serial.println(" microSD "+String(SdCardPresentStatus));
    SdCardTimer = millis();
  }

  if( (millis()-RxMqttTimer) > OfflineTimeout*60000 && eInkOfflineDetect == false ){
    eInkNeedRefresh=true;
    eInkOfflineDetect = true;
    Serial.print(millis());
    Serial.print(" | ");
    Serial.print(OfflineTimeout);
    Serial.println(" minutes offline timeout");
  }

  #if defined(WIFI)
    unsigned long currentMillis = millis();
    // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - WifiTimer >=WifiReconnect)) {
      Serial.print(millis());
      Serial.println(" Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      WifiTimer = currentMillis;
    }
  #endif
}

//-------------------------------------------------------------------------------------------------------
  void DirectionalRosette(int deg, int X, int Y, int R){
    int dot1;
    int dot2;
    if(R>100){
      dot1=2;
      dot2=5;
      display.setFont(&Open_Sans_Condensed_Bold_20);
    }else{
      dot1=1.5;
      dot2=3;
      display.setFont(&Open_Sans_Condensed_Light_16);
    }
    if(R>100){
      display.setCursor(Xcoordinate(0,X-5,R-10), Ycoordinate(0,Y,R-10));
      display.println("N");
      display.setCursor(Xcoordinate(90,X+5,R-10), Ycoordinate(90,Y+8,R-10));
      display.println("E");
      display.setCursor(Xcoordinate(180,X-6,R-10), Ycoordinate(180,Y+13,R-10));
      display.println("S");
      display.setCursor(Xcoordinate(270,X-16,R-10), Ycoordinate(270,Y+8,R-10));
      display.println("W");
    }
    for (int j=0; j<36; j++) {
      if(j % 9 == 0){
        if(R<100){
          display.fillCircle(Xcoordinate(j*10,X,R), Ycoordinate(j*10,Y,R), dot2, GxEPD_WHITE);
        }
      }else{
        display.fillCircle(Xcoordinate(j*10,X,R), Ycoordinate(j*10,Y,R), dot1, GxEPD_WHITE);
      }
    }
    if( (WindSpeedMaxPeriod>0 && mainHWdeviceSelect==1) || mainHWdeviceSelect==0){
      Arrow(deg,X,Y,R*0.9);
    }
}

//-------------------------------------------------------------------------------------------------------
void Arrow(int deg, int X, int Y, int r){
  int deg2 = deg+130;
  int deg3 = deg+230;
  display.fillTriangle(Xcoordinate(deg,X,r), Ycoordinate(deg,Y,r), Xcoordinate(deg2,X,r/2), Ycoordinate(deg2,Y,r/2), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), GxEPD_WHITE);
  display.fillTriangle(Xcoordinate(deg,X,r), Ycoordinate(deg,Y,r), Xcoordinate(deg3,X,r/2), Ycoordinate(deg3,Y,r/2), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), GxEPD_WHITE);
  display.fillTriangle(Xcoordinate(deg+180,X,r), Ycoordinate(deg+180,Y,r), Xcoordinate(deg3,X,r/10), Ycoordinate(deg3,Y,r/10), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), GxEPD_WHITE);
  display.fillTriangle(Xcoordinate(deg+180,X,r), Ycoordinate(deg+180,Y,r), Xcoordinate(deg2,X,r/10), Ycoordinate(deg2,Y,r/10), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), GxEPD_WHITE);
}

//-------------------------------------------------------------------------------------------------------
  float Xcoordinate(int dir, int Center, int r){
    float x = Center + sin(dir/RAD_TO_DEG) * r;
    return x;
  }
//-------------------------------------------------------------------------------------------------------

  float Ycoordinate(int dir, int Center, int r){
    float y = Center - cos(dir/RAD_TO_DEG) * r;
    return y;
  }

//-------------------------------------------------------------------------------------------------------
void print_wifi_error(){
  switch(WiFi.status())
  {
    case WL_IDLE_STATUS : Serial.println("WL_IDLE_STATUS"); break;
    case WL_NO_SSID_AVAIL : Serial.println("WL_NO_SSID_AVAIL"); break;
    case WL_CONNECT_FAILED : Serial.println("WL_CONNECT_FAILED"); break;
    case WL_DISCONNECTED : Serial.println("WL_DISCONNECTED"); break;
    default : Serial.printf("No know WiFi error"); break;
  }
}

//-------------------------------------------------------------------------------------------------------
void Mqtt(){
  #if defined(MQTT)
    if (millis()-MqttStatusTimer[0]>MqttStatusTimer[1]){
      if(!mqttClient.connected()){
        long now = millis();
        if (now - lastMqttReconnectAttempt > 5000) {
          lastMqttReconnectAttempt = now;
          Serial.print("Attempt to MQTT reconnect | ");
          Serial.println(millis()/1000);
          Status = 4; // reset
          if (mqttReconnect()) {
            lastMqttReconnectAttempt = 0;
          }
        }
      }else{
        // Client connected
        mqttClient.loop();
      }
      MqttStatusTimer[0]=millis();
    }
  #endif
}

//-------------------------------------------------------------------------------------------------------

#if defined(MQTT)
bool mqttReconnect() {
    char charbuf[50];
    WiFi.macAddress().toCharArray(charbuf, 18);
    if (mqttClient.connect(charbuf)) {
      Serial.println("mqttReconnect-connected");

      // resubscribe
      if(mainHWdeviceSelect==0){
        String topic = String(TOPIC) + "AzimuthStop";
        topic.reserve(50);
        const char *cstr0 = topic.c_str();
        if(mqttClient.subscribe(cstr0)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr0));
        }

        topic = String(TOPIC) + "Name";
        topic.reserve(50);
        const char *cstr1 = topic.c_str();
        if(mqttClient.subscribe(cstr1)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr1));
        }

        topic = String(TOPIC) + "StartAzimuth";
        topic.reserve(50);
        const char *cstr2 = topic.c_str();
        if(mqttClient.subscribe(cstr2)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr2));
        }

        // topic = String(TOPIC) + "Status";
        // topic.reserve(50);
        // const char *cstr3 = topic.c_str();
        // if(mqttClient.subscribe(cstr3)==true){
          //   Serial.print("mqttReconnect-subscribe ");
          //   Serial.println(String(cstr3));
          // }
          // MqttPubString("get", "4eink", false);


      }else if( mainHWdeviceSelect==1){
        String topic = String(TOPIC) + "Temperature-Celsius-HTU21D";
        topic.reserve(50);
        const char *cstr4 = topic.c_str();
        if(mqttClient.subscribe(cstr4)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr4));
        }

        topic = String(TOPIC) + "HumidityRel-Percent-HTU21D";
        topic.reserve(50);
        const char *cstr5 = topic.c_str();
        if(mqttClient.subscribe(cstr5)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr5));
        }

        topic = String(TOPIC) + "Pressure-hPa-BMP280";
        topic.reserve(50);
        const char *cstr6 = topic.c_str();
        if(mqttClient.subscribe(cstr6)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr6));
        }

        topic = String(TOPIC) + "WindDir-azimuth";
        topic.reserve(50);
        const char *cstr7 = topic.c_str();
        if(mqttClient.subscribe(cstr7)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr7));
        }

        topic = String(TOPIC) + "WindSpeedAvg-mps";
        topic.reserve(50);
        const char *cstr8 = topic.c_str();
        if(mqttClient.subscribe(cstr8)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr8));
        }

        topic = String(TOPIC) + "WindSpeedMaxPeriod-mps";
        topic.reserve(50);
        const char *cstr9 = topic.c_str();
        if(mqttClient.subscribe(cstr9)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr9));
        }


      }
      display.fillScreen(GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&Open_Sans_Condensed_Bold_20);
      display.setCursor(70, 150);
      display.println("Connecting");
      display.setFont(&Open_Sans_Condensed_Light_16);
      display.setCursor(90, 190);
      display.println("MicroSD import "+String(microSDlines)+" values");
      display.fillCircle(80, 190-7, 3, GxEPD_WHITE);
      display.setCursor(90, 220);
      display.println("WiFi "+String(SSID)+" "+String(WiFi.RSSI())+" dBm");
      display.fillCircle(80, 220-7, 3, GxEPD_WHITE);
      display.setCursor(90, 250);
      display.println(WiFi.localIP());
      display.fillCircle(80, 250-7, 3, GxEPD_WHITE);
      display.setCursor(90, 280);
      display.println("MQTT "+String(TOPIC)+"#");
      display.fillCircle(80, 280-7, 3, GxEPD_WHITE);
      display.setCursor(90, 310);
      display.println(mainHWdevice[mainHWdeviceSelect][1]);
      display.fillCircle(80, 310-7, 3, GxEPD_WHITE);
      display.setFont(&Open_Sans_Condensed_Light_16);
      display.setCursor(15, 385);
      UtcTime(1).toCharArray(buf, 21);
      display.println("UTC "+String(buf));
      display.setCursor(200, 385);
      display.print(REV);
      display.display(false);
      MqttPubString("get", "4eink", false);
    }
    return mqttClient.connected();
}
#endif

//------------------------------------------------------------------------------------
void MqttRx(char *topic, byte *payload, unsigned int length) {
  #if defined(MQTT)
    String CheckTopicBase;
    CheckTopicBase.reserve(100);
    byte* p = (byte*)malloc(length);
    memcpy(p,payload,length);
    // static bool HeardBeatStatus;
    Serial.print("RXmqtt < ");

    if( mainHWdeviceSelect==0){

      CheckTopicBase = String(TOPIC) + "AzimuthStop";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        if(NR>360){
          Azimuth=NR-(NR/360);
        }
        Azimuth=NR;
        Serial.println(String(Azimuth)+"°");
        RxMqttTimer=millis();
        if( (AzimuthTmp!=Azimuth && abs(Azimuth-AzimuthTmp)>3) || eInkOfflineDetect==true){
          eInkNeedRefresh=true;
          AzimuthTmp=Azimuth;
        }
        eInkOfflineDetect = false;
      }

      // CheckTopicBase = String(TOPIC) + "Status";
      // if ( CheckTopicBase.equals( String(topic) )){
      //   int NR = 0;
      //   unsigned long exp = 1;
      //   for (int i = length-1; i >=0 ; i--) {
      //     // Numbers only
      //     if(p[i]>=48 && p[i]<=58){
      //       NR = NR + (p[i]-48)*exp;
      //       exp = exp*10;
      //     }
      //   }
      //   Status=NR;
      //   Serial.println("Status "+String(Status));
      // }

      CheckTopicBase = String(TOPIC) + "StartAzimuth";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        AzimuthStart=NR;
        Serial.println("AzimuthStart "+String(AzimuthStart)+"°");
        // eInkNeedRefresh=true;
        // RxMqttTimer=millis();
      }

      CheckTopicBase = String(TOPIC) + "Name";
      if ( CheckTopicBase.equals( String(topic) )){
        String buf = "";
        for (int i = 0; i <=length-1 ; i++) {
          buf = buf+String((char)p[i]);
        }
        Name = buf;
        Serial.println(String(Name));
        eInkNeedRefresh=true;
        // RxMqttTimer=millis();
      }

    }else if( mainHWdeviceSelect==1){

      CheckTopicBase = String(TOPIC) + "Temperature-Celsius-HTU21D";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        bool NEGATIV = false;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          if(p[i]==45){
            NEGATIV = true;
          }
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        if(NEGATIV==true){
          NR=-NR;
        }
        Temperature=(float)NR/100.0;
        Serial.println("Temperature-Celsius-HTU21D "+String(Temperature)+"°");
        RxMqttTimer=millis();
        eInkNeedRefresh=true;
        eInkOfflineDetect = false;
      }

      CheckTopicBase = String(TOPIC) + "HumidityRel-Percent-HTU21D";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        HumidityRel=(float)NR/100.0;
        Serial.println("HumidityRel-Percent-HTU21D "+String(HumidityRel)+"%");
        RxMqttTimer=millis();
        eInkNeedRefresh=true;
        eInkOfflineDetect = false;
      }

      CheckTopicBase = String(TOPIC) + "Pressure-hPa-BMP280";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        Pressure=(float)NR/100.0;
        Serial.println("Pressure-hPa-BMP280 "+String(Pressure)+" hpa");
        RxMqttTimer=millis();
        eInkNeedRefresh=true;
        eInkOfflineDetect = false;
      }

      CheckTopicBase = String(TOPIC) + "WindDir-azimuth";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        WindDir=NR;
        Serial.println("WindDir-azimuth "+String(WindDir)+"°az");
        RxMqttTimer=millis();
        eInkNeedRefresh=true;
        eInkOfflineDetect = false;
      }

      CheckTopicBase = String(TOPIC) + "WindSpeedAvg-mps";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        WindSpeedAvg=(float)NR/100.0;
        Serial.println("WindSpeedAvg-mps "+String(WindSpeedAvg)+" m/s");
        RxMqttTimer=millis();
        eInkNeedRefresh=true;
        eInkOfflineDetect = false;
      }

      CheckTopicBase = String(TOPIC) + "WindSpeedMaxPeriod-mps";
      if ( CheckTopicBase.equals( String(topic) )){
        int NR = 0;
        unsigned long exp = 1;
        for (int i = length-1; i >=0 ; i--) {
          // Numbers only
          if(p[i]>=48 && p[i]<=58){
            NR = NR + (p[i]-48)*exp;
            exp = exp*10;
          }
        }
        WindSpeedMaxPeriod=(float)NR/100.0;
        Serial.println("WindSpeedMaxPeriod-mps "+String(WindSpeedMaxPeriod)+" m/s");
        RxMqttTimer=millis();
        eInkNeedRefresh=true;
        eInkOfflineDetect = false;
      }

    }

  #endif
} // MqttRx END

//-----------------------------------------------------------------------------------
void MqttPubString(String TOPICEND, String DATA, bool RETAIN){
  char charbuf[50];
   // memcpy( charbuf, mac, 6);
   WiFi.macAddress().toCharArray(charbuf, 18);
  // if(EnableEthernet==1 && MQTT_ENABLE==1 && EthLinkStatus==1 && mqttClient.connected()==true){
  if(mqttClient.connected()==true){
    if (mqttClient.connect(charbuf)) {
      Serial.print("TXmqtt > ");
      String topic = String(TOPIC)+String(TOPICEND);
      topic.toCharArray( mqttPath, 50 );
      DATA.toCharArray( mqttTX, 50 );
      mqttClient.publish(mqttPath, mqttTX, RETAIN);
      Serial.print(mqttPath);
      Serial.print(" ");
      Serial.println(mqttTX);
    }
  }
}

//-------------------------------------------------------------------------------------------------------
String UtcTime(int format){
  tm timeinfo;
  char buf[50]; //50 chars should be enough
  // if(WiFi.status() == WL_CONNECTED) {
  //   strcpy(buf, "n/a");
  // }else{
    if(!getLocalTime(&timeinfo)){
      strcpy(buf, "n/a");
    }else{
      if(format==1){
        strftime(buf, sizeof(buf), "%Y-%b-%d %H:%M:%S", &timeinfo);
      }else if(format==2){
        strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
      }else if(format==3){
        strftime(buf, sizeof(buf), "%Y", &timeinfo);
      }
    }
  // }
  // Serial.println(buf);
  return String(buf);
}

//-------------------------------------------------------------------------------------------------------
int readFile(fs::FS &fs, const char *path)
{
	uint16_t lines = 0;
	File file = fs.open(path);
	if (!file){
		return -1;
	}while (file.available()){
		if (file.read() == '\n'){
			lines++;
		}
	}
	file.close();
	return lines;
}

//-------------------------------------------------------------------------------------------------------
int SDtestInit()
{
	char disp[50];
	uint8_t cardType;
	uint64_t cardSize;
	if (!SD.begin(SD_CS, spiSD)){
			return -1;
		}
	cardType = SD.cardType();
	if (cardType == CARD_NONE){
		return -1;
	}
	cardSize = SD.cardSize() / (1024 * 1024);
  Serial.print("SD Card Type: ");
	if (cardType == CARD_MMC){
		Serial.print("MMC");
	}
	else if (cardType == CARD_SD){
		Serial.print("SDSC");
	}
	else if (cardType == CARD_SDHC){
    Serial.print("SDHC");
	}
	else {
    Serial.print("UNKNOWN");
	}
  Serial.print(", size: ");
  Serial.print(cardSize);
  Serial.println(" Mb");
	return 0;
}

//-------------------------------------------------------------------------------------------------------
void SDtest(){
  static int intCount = 0;
	// if(SDtestInit()==-1){
  	while (SDtestInit()){
      if(intCount==0){
        Serial.println("SD card not found");
      }
      if(intCount==1){
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&Open_Sans_Condensed_Bold_20);
        display.setCursor(40, 120);
        display.println("!");
        display.setCursor(40, 160);
        display.println("SD card not found");
        display.setCursor(40, 190);
        display.println("insert formated SD card");
        display.setCursor(40, 220);
        display.println("with setup.cfg file");
        display.setCursor(40, 250);
        display.println("and reboot the device");
        display.display(false);
      }
      Serial.print(".");
      delay(1000);
      intCount++;
  		// SD.end();
  		// return;
      // while(true);
  	}


	Serial.println("Init SDcard done");

  ConfigFile.toCharArray(charConfigFile, sizeof(charConfigFile));
  if(!SD.exists(charConfigFile)){
    Serial.print(ConfigFile);
    Serial.println(" not found");
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&Open_Sans_Condensed_Bold_20);
    display.setCursor(40, 120);
    display.println("!");
    display.setCursor(40, 160);
    display.println("setup.cfg");
    display.setCursor(40, 190);
    display.println("not found on microSD");
    display.setCursor(40, 220);
    display.println("please upload the file");
    display.setCursor(40, 250);
    display.println("and reboot the device");
    display.display(false);
    while(!SD.exists(charConfigFile)){
      Serial.print(".");
      delay(1000);
    }
  }
  Serial.print("load ");
  Serial.println(ConfigFile);

  display.fillScreen(GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&Open_Sans_Condensed_Bold_20);
  display.setCursor(70, 150);
  display.println("Connecting");
  display.setFont(&Open_Sans_Condensed_Light_16);
  display.setCursor(90, 190);
  display.println("MicroSD...");
    display.fillCircle(80, 190-7, 3, GxEPD_WHITE);
  display.setFont(&Open_Sans_Condensed_Light_16);
  display.setCursor(200, 385);
  display.print(REV);
  display.display(false);

  readSDSettings();
}

//-------------------------------------------------------------------------------------------------------
void readSDSettings(){
  char character;
  String settingName;
  settingName.reserve(20);
  String settingValue;
  settingValue.reserve(20);
  myFile = SD.open(charConfigFile);
  if (myFile){
    while (myFile.available()){
    	character = myFile.read();
    	while((myFile.available()) && (character != '[')){
    		character = myFile.read();
    	}
    	character = myFile.read();
    	while((myFile.available()) && (character != '=')){
    		settingName = settingName + character;
    		character = myFile.read();
    	}
    	character = myFile.read();
    	while((myFile.available()) && (character != ']')){
    		settingValue = settingValue + character;
    		character = myFile.read();
    	}
    	if(character == ']'){

      Serial.println("   ["+String(settingName)+"="+String(settingValue)+"]");

        if(settingName == "mainHWdeviceSelect"){
          mainHWdeviceSelect = (int)settingValue.toInt();  //0 = IP rotator, 1 = WX station
          // Serial.println("   mainHWdeviceSelect="+String(mainHWdeviceSelect));
          microSDlines++;
        }else if(settingName == "ROT_TOPIC" && mainHWdeviceSelect==0){
          TOPIC = String(settingValue)+String(mainHWdevice[mainHWdeviceSelect][0]);
          // Serial.println("   TOPIC="+String(TOPIC));
          microSDlines++;
        }else if(settingName == "WX_TOPIC" && mainHWdeviceSelect==1){
          TOPIC = String(settingValue)+String(mainHWdevice[mainHWdeviceSelect][0]);
          // Serial.println("   TOPIC="+String(TOPIC));
          microSDlines++;
        }else if(settingName == "mqttBroker0"){
          mqttBroker[0] = (int)settingValue.toInt();
          // Serial.println("   mqttBroker0="+String(mqttBroker[0]));
          microSDlines++;
        }else if(settingName == "mqttBroker1"){
          mqttBroker[1] = (int)settingValue.toInt();
          // Serial.println("   mqttBroker1="+String(mqttBroker[1]));
          microSDlines++;
        }else if(settingName == "mqttBroker2"){
          mqttBroker[2] = (int)settingValue.toInt();
          // Serial.println("   mqttBroker2="+String(mqttBroker[2]));
          microSDlines++;
        }else if(settingName == "mqttBroker3"){
          mqttBroker[3] = (int)settingValue.toInt();
          // Serial.println("   mqttBroker3="+String(mqttBroker[3]));
          mqtt_server_ip = mqttBroker;
          microSDlines++;
        }else if(settingName == "MQTT_PORT"){
          MQTT_PORT = (int)settingValue.toInt();
          // Serial.println("   MQTT_PORT="+String(MQTT_PORT));
          microSDlines++;
        }else if(settingName == "SSID"){
          SSID = settingValue;
          // Serial.println("   SSID="+String(SSID));
          microSDlines++;
          // char buff[20];
          // settingValue.toCharArray(buff, 20);
          // SSID=buff;
        }else if(settingName == "PSWD"){
          PSWD = settingValue;
          // Serial.println("   PSWD=****");
          microSDlines++;
          // char buff[20];
          // settingValue.toCharArray(buff, 20);
          // PSWD=buff;
        }else if(settingName == "eInkRotation"){
          eInkRotation = (unsigned int)settingValue.toInt();
          display.setRotation(eInkRotation); // 1 USB TOP, 3 USB DOWN | 0 default, 1 90°CW, 2 180°CW, 3 90°CCW
          // Serial.println("   eInkRotation="+String(eInkRotation));
          microSDlines++;
        }else if(settingName == "OfflineTimeout"){
          OfflineTimeout = (int)settingValue.toInt();
          // Serial.println("   OfflineTimeout="+String(OfflineTimeout));
          microSDlines++;
        }else if(settingName == "eInkNegativ"){
          eInkNegativ = (bool)settingValue.toInt();
          // Serial.println("   eInkNegativ="+String(eInkNegativ));
          microSDlines++;
        }else if(settingName == "DesignSkin"){
          DesignSkin = (int)settingValue.toInt();
          // Serial.println("   DesignSkin="+String(DesignSkin));
          microSDlines++;
        }
    		settingName = "";
    		settingValue = "";
    	}
    } // end while
    myFile.close();
  }else{
    // if the file didn't open, print an error:
    //Serial.println("error opening settings.txt");
  }
  SD.end();
}
