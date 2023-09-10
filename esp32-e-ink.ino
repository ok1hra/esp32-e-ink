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
Použití knihovny FS ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/FS
Použití knihovny SD ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/SD
Použití knihovny SPI ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/SPI
Použití knihovny GxEPD2 ve verzi 1.5.2 v adresáři: /home/dan/Arduino/libraries/GxEPD2
Použití knihovny Adafruit_GFX_Library ve verzi 1.11.3 v adresáři: /home/dan/Arduino/libraries/Adafruit_GFX_Library
Použití knihovny Adafruit_BusIO ve verzi 1.14.1 v adresáři: /home/dan/Arduino/libraries/Adafruit_BusIO
Použití knihovny Wire ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Wire
Použití knihovny WiFiClientSecure ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/WiFiClientSecure
Použití knihovny WiFi ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/WiFi
Použití knihovny jsonlib ve verzi 0.1.1 v adresáři: /home/dan/Arduino/libraries/jsonlib
Použití knihovny AsyncTCP ve verzi 1.1.1 v adresáři: /home/dan/Arduino/libraries/AsyncTCP
Použití knihovny ESPAsyncWebServer ve verzi 1.2.3 v adresáři: /home/dan/Arduino/libraries/ESPAsyncWebServer
Použití knihovny AsyncElegantOTA ve verzi 2.2.7 v adresáři: /home/dan/Arduino/libraries/AsyncElegantOTA
Použití knihovny Update ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Update
Použití knihovny PubSubClient ve verzi 2.8 v adresáři: /home/dan/Arduino/libraries/PubSubClient

mosquitto_pub -h 54.38.157.134 -t OK1HRA/0/ROT/Azimuth -m '83'
mosquitto_sub -v -h 54.38.157.134 -t 'OK1HRA/0/ROT/#'

*/
//-------------------------------------------------------------------------------------------------------

#define REV 20230910
#define OTAWEB                    // enable upload firmware via web
#define MQTT                      // enable MQTT
#define APRSFI                    // enable get from aprs.fi
#include <esp_adc_cal.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
// #define BMPMAP
#if defined(BMPMAP)
  #include "ok.h"
#endif
#if defined(APRSFI)
  // #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
  const char*  server = "api.aprs.fi";  // Server URL
  WiFiClientSecure client;

  // #include <ArduinoJson.h>
  String jsonString = "";
  #include <jsonlib.h>    // https://github.com/wyolum/jsonlib/tree/master/

  // ISRG Root X1 root .pem certificate for aprs.fi valid to Mon, 04 Jun 2035 11:04:38 GMT
  const char* rootCACertificate = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
  "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
  "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
  "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
  "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
  "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
  "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
  "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
  "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
  "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
  "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
  "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
  "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
  "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
  "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
  "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
  "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
  "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
  "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
  "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
  "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
  "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
  "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
  "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
  "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
  "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
  "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
  "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
  "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
  "-----END CERTIFICATE-----\n";
#endif

// source https://oleddisplay.squix.ch/ - must copy via clipboard!
// #include "Open_Sans_Condensed_Light_80.h"
// #include "Open_Sans_Condensed_Bold_20.h"
// #include "Open_Sans_Condensed_Light_16.h"

// https://rop.nl/truetype2gfx/
#include "Logisoso8pt7b.h"
#include "Logisoso10pt7b.h"
#include "Logisoso50pt7b.h"
// display.setFont(&Logisoso250pt7b);

// #define SLEEP                    // Uncomment so board goes to sleep after printing on display
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 10          // Time ESP32 will go to sleep (in seconds)
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4)); // GDEW042T2 400x300, UC8176 (IL0398)
//GxEPD2_3C<GxEPD2_420c_Z21, GxEPD2_420c_Z21::HEIGHT> display(GxEPD2_420c_Z21(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEQ042Z21 400x300, UC8276

const String mainHWdevice[4][2] = {
    {"/ROT/", "IP rotator"},  // 0
    {"/WX/", "WX station"},   // 1
    {"", "aprs.fi"},          // 2
    {"topic", "name"},        // 3
};
int mainHWdeviceSelect = -1;  //0 = IP rotator, 1 = WX station, 2 = aprs.fi source
String TOPIC = "";        // same as 'location' on IP rotator
String ROT_TOPIC = "";    // mainHWdevice[mainHWdeviceSelect][0]
String WX_TOPIC = "";    // mainHWdevice[mainHWdeviceSelect][0]
byte mqttBroker[4]={0,0,0,0}; // MQTT broker IP address
int MQTT_PORT = 0;         // MQTT broker port
IPAddress mqtt_server_ip(mqttBroker[0], mqttBroker[1], mqttBroker[2], mqttBroker[3]);       // MQTT broker IP address
String SSID = "";
String PSWD = "";
String APRS_FI_NAME = "";
String APRS_FI_APIKEY = "";

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
bool eInkNeedRefresh = false;
bool eInkOfflineDetect = false;
long RxMqttTimer=0;

//WX
float Temperature = 7.3;
float RainToday = 0.0;
float HumidityRel = 0;
float DewPoint = 0;
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
    display.setFont(&Logisoso10pt7b);
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
  display.setFont(&Logisoso10pt7b);
  display.setCursor(70, 150);
  display.println("Connecting");
  display.setFont(&Logisoso8pt7b);
  display.setCursor(90, 190);
  display.println("MicroSD import "+String(microSDlines)+" values");
    display.fillCircle(80, 190-7, 3, GxEPD_WHITE);
  display.setCursor(90, 220);
  display.println("WiFi "+String(SSID)+"...");
    display.fillCircle(80, 220-7, 3, GxEPD_WHITE);
  display.setFont(&Logisoso8pt7b);
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

    if(mainHWdeviceSelect==2){
      display.fillScreen(GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&Logisoso10pt7b);
      display.setCursor(70, 150);
      display.println("Connecting");
      display.setFont(&Logisoso8pt7b);
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
      display.fillCircle(80, 280-7, 3, GxEPD_WHITE);
      display.println(String(mainHWdevice[mainHWdeviceSelect][1])+"/"+String(APRS_FI_NAME)+"...");
      display.setFont(&Logisoso8pt7b);
      display.setCursor(15, 385);
      UtcTime(1).toCharArray(buf, 21);
      display.println("UTC "+String(buf));
      display.setCursor(200, 385);
      display.print(REV);
      display.display(false);
    }

    #if defined(MQTT)
    if(mainHWdeviceSelect==0 || mainHWdeviceSelect==1){
      if (MQTT_LOGIN == true){
        // if (mqttClient.connect("esp32gwClient", MQTT_USER, MQTT_PASS)){
          //   AfterMQTTconnect();
          // }
        }else{
          if(mainHWdeviceSelect==0 || mainHWdeviceSelect==1 ){
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
    Serial.println("OTAserver start");
  #endif
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  #if defined(APRSFI)
    jsonString.reserve(900);
  #endif
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

void GetHttps(){
  #if defined(APRSFI)
  /*
    WiFiClientSecure *client = new WiFiClientSecure;
    Serial.println("[https] start");
    if(client) {
      client -> setCACert(rootCACertificate);

      {
        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
        HTTPClient https;

        Serial.print("[https] begin...\n");
        if (https.begin(*client, "https://api.aprs.fi/api/get?name="+String(APRS_FI_NAME)+"&what=wx&apikey="+String(APRS_FI_APIKEY)+"&format=json")) {
          Serial.print("[https] GET...\n");
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[https] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              jsonString = https.getString();
              Serial.print("[https] ");
              Serial.println(jsonString);
              RxMqttTimer=millis();
            }
          } else {
            Serial.printf("[https] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          }
          https.end();
        } else {
          Serial.printf("[https] Unable to connect\n");
        }
        // End extra scoping block
      }
      delete client;
    } else {
      Serial.println("[https] Unable to create client");
    }
    */
  #endif
}
//-------------------------------------------------------------------------------------------------------
void eInkRefresh(){
  static long eInkRefreshTimer = -5000;
  // ROT
  if( mainHWdeviceSelect==0 && eInkNeedRefresh==true && millis()-eInkRefreshTimer > 5000 && Azimuth!=-42 && Name != "" ){
      display.fillScreen(GxEPD_BLACK);

      #if defined(BMPMAP)
        display.drawBitmap(0, 0, ok, 300, 300, GxEPD_WHITE);
      #endif

      if(Azimuth>=0){
        DirectionalRosette(AzimuthShifted(Azimuth), 150, 145, 130);
      }

      display.setTextColor(GxEPD_WHITE);
      display.setFont(&Logisoso50pt7b);
      /*
      char to - (minus) width 39px
      char width 33px
      char to char width 44px
      dot+char width 68px
      */
      if(Azimuth>=0){
        if(AzimuthShifted(Azimuth)>=100){
          display.setCursor(175-44, 360);
        }else if(AzimuthShifted(Azimuth)<100 && AzimuthShifted(Azimuth)>=10){
          display.setCursor(175, 360);
        }else if(AzimuthShifted(Azimuth)<10){
          display.setCursor(175+44, 360);
        }
        display.println(AzimuthShifted(Azimuth));
        // display.setCursor(270, 310);
        // display.setFont(&Logisoso10pt7b);
        // display.println("o");
        display.fillCircle(275, 300, 6, GxEPD_WHITE);
      }else{
        display.setCursor(170, 355);
        display.println("n/a");
      }
      int ZZshift=2;
      display.setFont(&Logisoso8pt7b);
      display.setCursor(15, 285+4*ZZshift);
      display.println(String(SSID)+" "+String(WiFi.RSSI())+" dBm");
      display.setCursor(15, 310+3*ZZshift);
      display.print(WiFi.localIP());
      display.setFont(&Logisoso10pt7b);
      display.setCursor(15, 335+2*ZZshift);
      display.println(Name);
      display.setFont(&Logisoso8pt7b);
      display.setCursor(15, 360+ZZshift);
      display.println(String(TOPIC)+"#");
      display.setCursor(15, 385);
      UtcTime(1).toCharArray(buf, 21);
      display.println("UTC "+String(buf));
      if(eInkOfflineDetect==true){
        display.setCursor(185, 385);
        display.setFont(&Logisoso10pt7b);
        display.print("OFF >"+String(OfflineTimeout)+"min");
      }else{
        display.setCursor(200, 385);
        display.print(REV);
      }
      display.display(false);
      eInkNeedRefresh=false;
      eInkRefreshTimer=millis();
    // WX
    }else if( (mainHWdeviceSelect==1 || mainHWdeviceSelect==2) && eInkNeedRefresh==true && millis()-eInkRefreshTimer > 10000 ){
        display.fillScreen(GxEPD_BLACK);

        display.setTextColor(GxEPD_WHITE);
        display.setFont(&Logisoso50pt7b);
        int Xshift=0;
        if(Temperature<0){
          Xshift=-39;
        }else{
          Xshift=0;
        }
        if(abs(Temperature)>=10){
          display.setCursor(64+Xshift, 85);
        }else if(abs(Temperature)<10){
          display.setCursor(97+Xshift, 85);
        }
        String str = String(Temperature);
        String subStr = str.substring(0, str.length() - 1);
        display.println(String(subStr));
        display.setCursor(242, 85);
        display.println("C");
        display.fillCircle(230, 25, 6, GxEPD_WHITE);

        display.drawLine(15, 100, 285, 100, 2);
        float XX = (285.0-15.0)/100.0*HumidityRel+15.0;
        display.fillCircle((int)XX, 100, 3, GxEPD_WHITE);

        display.setFont(&Logisoso8pt7b);
        display.setCursor(15, 125);
        display.print("Relative humidity ");
        display.setFont(&Logisoso10pt7b);
        display.print(String((int)HumidityRel)+"%  ");
        display.setFont(&Logisoso8pt7b);
        display.print("Dew point ");
        display.setFont(&Logisoso10pt7b);
        display.print(String((int)DewPoint)+" C");

        display.setCursor(15, 150);
        display.setFont(&Logisoso8pt7b);
        display.print("Pressure ");
        display.setFont(&Logisoso10pt7b);
        display.print(String((int)Pressure)+" hpa");
        Triangle(Pressure, 983.0, 1043.0);  // 1013 +-30
        display.drawLine(15, 200, 20, 200, 2);

        if(RainToday>0){
          str = String(RainToday);
          subStr = str.substring(0, str.length() - 1);
          display.print("  RAIN "+String(subStr)+" mm");
          int ten = (int)RainToday % 10;
          if(RainToday>0 && RainToday<1){
            display.fillCircle(285-1*(11+1), 170, 3+1, GxEPD_WHITE);
          }
          for (int j=ten; j>0; j--) {
            display.fillCircle(285-j*(11+j), 170, 3+j, GxEPD_WHITE);
          }
          int tens = (int)(RainToday/10);
          for (int j=tens; j>0; j--) {
            display.fillCircle(j*30-5, 170, 13, GxEPD_WHITE);
          }
        }

        display.setFont(&Logisoso50pt7b);
        if(abs(WindSpeedMaxPeriod)>=10){
          display.setCursor(6+4, 265);
        }else if(abs(WindSpeedMaxPeriod)<10){
          display.setCursor(50+4, 265);
        }
        if(WindSpeedMaxPeriod>0){
          display.println((int)WindSpeedMaxPeriod);
          // display.setFont(&Logisoso10pt7b);
          display.setFont(&Logisoso8pt7b);
          display.setCursor(35, 290);
          display.println("gust m/s");
        }

        // int Pressure = 0;
        // int WindSpeedAvg = 0;

        DirectionalRosette(WindDir, 200, 270, 80);

        int ZZshift=2;
        // display.setFont(&Logisoso10pt7b);
        // display.println(Name);
        display.setFont(&Logisoso8pt7b);
        display.setCursor(15, 285+4*ZZshift);
        display.setCursor(15, 310+3*ZZshift);
        display.println(String(SSID)+" "+String(WiFi.RSSI())+" dBm");
        display.setCursor(15, 335+2*ZZshift);
        display.print(WiFi.localIP());
        display.setFont(&Logisoso8pt7b);
        display.setCursor(15, 360+ZZshift);
        if(mainHWdeviceSelect==1){
          display.println(String(TOPIC)+"#");
        }else if(mainHWdeviceSelect==2){
          display.println(String(mainHWdevice[mainHWdeviceSelect][1])+"/"+String(APRS_FI_NAME));
        }
        display.setCursor(15, 385);
        UtcTime(1).toCharArray(buf, 21);
        display.println("UTC "+String(buf));
        if(eInkOfflineDetect==true){
          display.setCursor(185, 385);
          display.setFont(&Logisoso10pt7b);
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
void Triangle(float VALUE, float MIN, float MAX){
  float YY = 400.0-(VALUE - MIN) * (400.0/(MAX-MIN));
  display.fillTriangle(0, (int)YY-5, 14, (int)YY, 0, (int)YY+5, GxEPD_WHITE);
  // Serial.println("Triangle Ypx: "+String(YY));
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

  #if defined(APRSFI)
    static long aprsfiTimer = -900000;
    if(millis()-aprsfiTimer > 900000 && mainHWdeviceSelect==2){
      aprsfiTimer=millis();

      // Serial.println("[json] start");

      // Use https://arduinojson.org/v6/assistant to compute the capacity.
      // StaticJsonDocument<600> doc;

      // {"command":"get","result":"ok","found":1,"what":"wx","entries":[{"name":"OK1HRA-8","time":"1694332396","temp":"20.0","pressure":"1007.5","humidity":"60","wind_direction":"247","wind_speed":"3.1","wind_gust":"3.6","rain_mn":"0.0"}]}
      // jsonString = "{\"command\":\"get\",\"result\":\"ok\",\"found\":1,\"what\":\"wx\",\"entries\":[{\"name\":\"OK1HRA-8\",\"time\":\"1693643356\",\"temp\":\"17.8\",\"pressure\":\"1008.2\",\"humidity\":\"81\",\"wind_direction\":\"270\",\"wind_speed\":\"1.3\",\"wind_gust\":\"2.2\",\"rain_mn\":\"0.7\"}]}";
      // GetHttps();
      // DeserializationError error = deserializeJson(doc, jsonString);
      // Test if parsing succeeds.
      // if (error) {
      //   Serial.print("[json] deserializeJson() failed: ");
      //   Serial.println(error.f_str());
      //   return;
      // }

      client.setCACert(rootCACertificate);

      Serial.println("\n[https] Starting connection to server...");
      if (!client.connect(server, 443))
        Serial.println("[https] Connection failed!");
      else {
        Serial.println("[https] Connected to server!");
        // Make a HTTP request:
        client.println( "GET /api/get?name="+String(APRS_FI_NAME)+"&what=wx&apikey="+String(APRS_FI_APIKEY)+"&format=json HTTP/1.1" );
        client.println("Host: api.aprs.fi");
        client.println("Connection: close");
        client.println();

        while (client.connected()) {
          String line = client.readStringUntil('\n');
          if (line == "\r") {
            Serial.println("[https] headers received");
            break;
          }
        }

        jsonString = "";
        while (client.available()) {
          char c = client.read();
          jsonString = jsonString + String(c);
          // Serial.write(c);
        }
        int indexStart = jsonString.indexOf("{");
        int indexEnd = jsonString.indexOf("]");
        jsonString = jsonString.substring(indexStart,indexEnd+2);
        Serial.print("[https RX] ");
        Serial.println(jsonString);
        RxMqttTimer=millis();

        client.stop();
        Serial.println("[https] stop");

        Serial.println("[json] start extract");
        String posStr = jsonExtract(jsonString, "entries");
        Serial.println("[json] extract temp");
        Temperature = jsonExtract(posStr, "temp").toFloat();
        Serial.println("[json] extract rain");
        RainToday = jsonExtract(posStr, "rain_mn").toFloat();
        HumidityRel = jsonExtract(posStr, "humidity").toFloat();
        DewPoint = (float)Temperature - (100.0 - constrain(HumidityRel, 0, 100)) / 5.0;
        Pressure = jsonExtract(posStr, "pressure").toFloat();
        WindDir = jsonExtract(posStr, "wind_direction").toInt();
        WindSpeedMaxPeriod = jsonExtract(posStr, "wind_gust").toFloat();

        // Temperature = doc["entries"][0]["temp"];
        // RainToday = doc["entries"][0]["rain_mn"];
        // HumidityRel = doc["entries"][0]["humidity"];
        // Pressure = doc["entries"][0]["pressure"];
        // WindDir = doc["entries"][0]["wind_direction"];
        // WindSpeedMaxPeriod = doc["entries"][0]["wind_gust"];

        Serial.println("[json] Temperature: "+String(Temperature));
        Serial.println("[json] RainToday: "+String(RainToday));
        Serial.println("[json] HumidityRel: "+String(HumidityRel));
        Serial.println("[json] DewPoint: "+String(DewPoint));
        Serial.println("[json] Pressure: "+String(Pressure));
        Serial.println("[json] WindDir: "+String(WindDir));
        Serial.println("[json] WindSpeedMaxPeriod: "+String(WindSpeedMaxPeriod));

        eInkNeedRefresh=true;
        RxMqttTimer=millis();
      }

    }
  #endif

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
      display.setFont(&Logisoso10pt7b);
    }else{
      dot1=1.5;
      dot2=3;
      display.setFont(&Logisoso8pt7b);
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
    if( (WindSpeedMaxPeriod>0 && mainHWdeviceSelect==1) || mainHWdeviceSelect==2){
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
    if (millis()-MqttStatusTimer[0]>MqttStatusTimer[1] && (mainHWdeviceSelect==0 || mainHWdeviceSelect==1)){
      if(!mqttClient.connected()){
        long now = millis();
        if (now - lastMqttReconnectAttempt > 10000) {
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

      String topic = String(ROT_TOPIC) + "mainHWdeviceSelect";
      topic.reserve(50);
      const char *cstrr = topic.c_str();
      if(mqttClient.subscribe(cstrr)==true){
        Serial.print("mqttReconnect-subscribe ");
        Serial.println(String(cstrr));
      }

      topic = String(WX_TOPIC) + "mainHWdeviceSelect";
      topic.reserve(50);
      const char *cstrw = topic.c_str();
      if(mqttClient.subscribe(cstrw)==true){
        Serial.print("mqttReconnect-subscribe ");
        Serial.println(String(cstrw));
      }

      // resubscribe
      // if(mainHWdeviceSelect==0){
        topic = String(ROT_TOPIC) + "AzimuthStop";
        topic.reserve(50);
        const char *cstr0 = topic.c_str();
        if(mqttClient.subscribe(cstr0)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr0));
        }

        topic = String(ROT_TOPIC) + "Name";
        topic.reserve(50);
        const char *cstr1 = topic.c_str();
        if(mqttClient.subscribe(cstr1)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr1));
        }

        topic = String(ROT_TOPIC) + "StartAzimuth";
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


      // }else if( mainHWdeviceSelect==1){
        topic = String(WX_TOPIC) + "Temperature-Celsius";
        topic.reserve(50);
        const char *cstr4 = topic.c_str();
        if(mqttClient.subscribe(cstr4)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr4));
        }

        topic = String(WX_TOPIC) + "HumidityRel-Percent-HTU21D";
        topic.reserve(50);
        const char *cstr5 = topic.c_str();
        if(mqttClient.subscribe(cstr5)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr5));
        }

        topic = String(WX_TOPIC) + "Pressure-hPa-BMP280";
        topic.reserve(50);
        const char *cstr6 = topic.c_str();
        if(mqttClient.subscribe(cstr6)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr6));
        }

        topic = String(WX_TOPIC) + "WindDir-azimuth";
        topic.reserve(50);
        const char *cstr7 = topic.c_str();
        if(mqttClient.subscribe(cstr7)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr7));
        }

        topic = String(WX_TOPIC) + "WindSpeedAvg-mps";
        topic.reserve(50);
        const char *cstr8 = topic.c_str();
        if(mqttClient.subscribe(cstr8)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr8));
        }

        topic = String(WX_TOPIC) + "WindSpeedMaxPeriod-mps";
        topic.reserve(50);
        const char *cstr9 = topic.c_str();
        if(mqttClient.subscribe(cstr9)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr9));
        }

        topic = String(WX_TOPIC) + "DewPoint-Celsius-HTU21D";
        topic.reserve(50);
        const char *cstr10 = topic.c_str();
        if(mqttClient.subscribe(cstr10)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr10));
        }

        topic = String(WX_TOPIC) + "RainToday-mm";
        topic.reserve(50);
        const char *cstr11 = topic.c_str();
        if(mqttClient.subscribe(cstr11)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr11));
        }

      // }
      display.fillScreen(GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&Logisoso10pt7b);
      display.setCursor(70, 150);
      display.println("Connecting");
      display.setFont(&Logisoso8pt7b);
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
      if(mainHWdeviceSelect==0 || mainHWdeviceSelect==1){
        display.println("MQTT "+String(TOPIC)+"#");
      }else{
        display.println("MQTT disable");
      }
      display.fillCircle(80, 280-7, 3, GxEPD_WHITE);
      display.setCursor(90, 310);
      display.println(String(mainHWdevice[mainHWdeviceSelect][1])+"...");
      display.fillCircle(80, 310-7, 3, GxEPD_WHITE);
      display.setFont(&Logisoso8pt7b);
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

float payloadToFloat(byte *payload, unsigned int length){
  byte* p = (byte*)malloc(length);
  memcpy(p,payload,length);
  int intBuf=0;
  int mult=1;
  bool negativ = false;
  bool decimals = true;
  int decimal=1;
  for (int j=length-1; j>=0; j--){
    // 0-9 || ,-.
    if( (p[j]>=48 && p[j]<=57) || (p[j]>=44 && p[j]<=46) ){
      if(p[j]==45){
        negativ = true;
      }else if(p[j]==44 || p[j]==46){
        decimals=false;
      }else{
        intBuf = intBuf + ((p[j]-48)*mult);
        mult = mult*10;
        if(decimals==true){
          decimal= decimal*10;
        }
      }
    }
  }
  if(negativ==true){
    intBuf=-intBuf;
  }
  if(decimals==false){
    return (float)intBuf/(float)decimal;
  }else{
    return (float)intBuf;
  }

}
//------------------------------------------------------------------------------------
void MqttRx(char *topic, byte *payload, unsigned int length) {
  #if defined(MQTT)
    String CheckTopicBase;
    CheckTopicBase.reserve(100);
    byte* p = (byte*)malloc(length);
    memcpy(p,payload,length);
    // static bool HeardBeatStatus;
    Serial.print("RXmqtt < ");

    CheckTopicBase = String(TOPIC) + "mainHWdeviceSelect";
    if ( CheckTopicBase.equals( String(topic) )){
      // int intBuf=(int)payloadToFloat(payload, length);
      // if(intBuf==0 || intBuf==1){
      if(mainHWdeviceSelect==0){
        mainHWdeviceSelect=1;
        TOPIC = WX_TOPIC;
      }else if(mainHWdeviceSelect==1){
        mainHWdeviceSelect=0;
        TOPIC = ROT_TOPIC;
      }
        // mainHWdeviceSelect=intBuf;
        Serial.println(String(mainHWdeviceSelect));
        Serial.println("  TOPIC= "+String(TOPIC));
        MqttPubString("get", "4eink", false);
      // }
      RxMqttTimer=millis();
      eInkOfflineDetect = false;
      eInkNeedRefresh=true;
    }

    if( mainHWdeviceSelect==0){

      CheckTopicBase = String(ROT_TOPIC) + "AzimuthStop";
      if ( CheckTopicBase.equals( String(topic) )){
        Azimuth=(int)payloadToFloat(payload, length);
        Serial.println(String(Azimuth)+"°");
        RxMqttTimer=millis();
        if( (AzimuthTmp!=Azimuth && abs(Azimuth-AzimuthTmp)>3) || eInkOfflineDetect==true){
          eInkNeedRefresh=true;
          AzimuthTmp=Azimuth;
        }
        eInkOfflineDetect = false;
      }

      CheckTopicBase = String(ROT_TOPIC) + "StartAzimuth";
      if ( CheckTopicBase.equals( String(topic) )){
        AzimuthStart=(int)payloadToFloat(payload, length);;
        Serial.println("AzimuthStart "+String(AzimuthStart)+"°");
        // eInkNeedRefresh=true;
        // RxMqttTimer=millis();
      }

      CheckTopicBase = String(ROT_TOPIC) + "Name";
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

      CheckTopicBase = String(WX_TOPIC) + "Temperature-Celsius";
      if ( CheckTopicBase.equals( String(topic) )){
        Temperature = payloadToFloat(payload, length);
        Serial.println("Temperature-Celsius "+String(Temperature)+"°");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "RainToday-mm";
      if ( CheckTopicBase.equals( String(topic) )){
        RainToday = payloadToFloat(payload, length);
        Serial.println("RainToday-mm "+String(RainToday)+" mm");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "HumidityRel-Percent-HTU21D";
      if ( CheckTopicBase.equals( String(topic) )){
        HumidityRel=payloadToFloat(payload, length);
        Serial.println("HumidityRel-Percent-HTU21D "+String(HumidityRel)+"%");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "DewPoint-Celsius-HTU21D";
      if ( CheckTopicBase.equals( String(topic) )){
        DewPoint=payloadToFloat(payload, length);
        Serial.println("DewPoint-Celsius-HTU21D "+String(DewPoint)+"°");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "Pressure-hPa-BMP280";
      if ( CheckTopicBase.equals( String(topic) )){
        Pressure=payloadToFloat(payload, length);
        Serial.println("Pressure-hPa-BMP280 "+String(Pressure)+" hpa");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(TOPIC) + "WindDir-azimuth";
      if ( CheckTopicBase.equals( String(topic) )){
        WindDir=(int)payloadToFloat(payload, length);
        Serial.println("WindDir-azimuth "+String(WindDir)+"°az");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "WindSpeedAvg-mps";
      if ( CheckTopicBase.equals( String(topic) )){
        WindSpeedAvg=payloadToFloat(payload, length);
        Serial.println("WindSpeedAvg-mps "+String(WindSpeedAvg)+" m/s");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "WindSpeedMaxPeriod-mps";
      if ( CheckTopicBase.equals( String(topic) )){
        WindSpeedMaxPeriod=payloadToFloat(payload, length);
        Serial.println("WindSpeedMaxPeriod-mps "+String(WindSpeedMaxPeriod)+" m/s");
        RxMqttTimer=millis();
        eInkOfflineDetect = false;
        // eInkNeedRefresh=true;
      }

    }

  #endif
} // MqttRx END

//-----------------------------------------------------------------------------------
void MqttPubString(String TOPICEND, String DATA, bool RETAIN){
  #if defined(MQTT)
    char charbuf[50];
     // memcpy( charbuf, mac, 6);
     WiFi.macAddress().toCharArray(charbuf, 18);
    // if(EnableEthernet==1 && MQTT_ENABLE==1 && EthLinkStatus==1 && mqttClient.connected()==true){
    if(mqttClient.connected()==true && (mainHWdeviceSelect==0 || mainHWdeviceSelect==1)){
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
  #endif
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
        // strftime(buf, sizeof(buf), "%Y-%b-%d %H:%M:%S", &timeinfo);
        strftime(buf, sizeof(buf), "%Y-%b-%d %H:%M", &timeinfo);
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
int SDtestInit(){
  /*
  .SD Card Type: SDSC, size: 124 Mb


  */
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
        display.setFont(&Logisoso10pt7b);
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
    display.setFont(&Logisoso10pt7b);
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
  display.setFont(&Logisoso10pt7b);
  display.setCursor(70, 150);
  display.println("Connecting");
  display.setFont(&Logisoso8pt7b);
  display.setCursor(90, 190);
  display.println("MicroSD...");
    display.fillCircle(80, 190-7, 3, GxEPD_WHITE);
  display.setFont(&Logisoso8pt7b);
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
        }else if(settingName == "ROT_TOPIC"){
          ROT_TOPIC = String(settingValue)+String(mainHWdevice[0][0]);
          microSDlines++;
        }else if(settingName == "WX_TOPIC"){
          WX_TOPIC = String(settingValue)+String(mainHWdevice[1][0]);
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
        }else if(settingName == "APRS_FI_NAME"){
          #if defined(APRSFI)
            APRS_FI_NAME = settingValue;
          #endif
          microSDlines++;
        }else if(settingName == "APRS_FI_APIKEY"){
          #if defined(APRSFI)
            APRS_FI_APIKEY = settingValue;
          #endif
          microSDlines++;
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
        if(mainHWdeviceSelect==0){
          TOPIC = ROT_TOPIC;
        }else if(mainHWdeviceSelect==1){
          TOPIC = WX_TOPIC;
        }
    		settingName = "";
    		settingValue = "";
    	}
    } // end while
    myFile.close();
    // Serial.println("   ROT_TOPIC="+String(ROT_TOPIC));
    // Serial.println("   WX_TOPIC="+String(WX_TOPIC));
    // Serial.println("   TOPIC="+String(TOPIC));
  }else{
    // if the file didn't open, print an error:
    //Serial.println("error opening settings.txt");
  }
  SD.end();
}
