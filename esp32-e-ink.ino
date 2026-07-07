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

HARDWARE ESP32 Dev Module - No OTA (2 MB APP/2MB SPIFFS)

- Zvýšit REV
- Arduino IDE → Export compiled Binary.
- ./tools/gh-pages.sh --publish.

IDE 1.8.19
Použití knihovny FS ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/FS
Použití knihovny SD ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/SD
Použití knihovny SPI ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/SPI
Použití knihovny GxEPD2 ve verzi 1.5.2 v adresáři: /home/dan/Arduino/libraries/GxEPD2
Použití knihovny Adafruit_GFX_Library ve verzi 1.11.3 v adresáři: /home/dan/Arduino/libraries/Adafruit_GFX_Library
Použití knihovny Adafruit_BusIO ve verzi 1.14.1 v adresáři: /home/dan/Arduino/libraries/Adafruit_BusIO
Použití knihovny Wire ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Wire
Použití knihovny WiFi ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/WiFi
Použití knihovny AsyncTCP ve verzi 1.1.1 v adresáři: /home/dan/Arduino/libraries/AsyncTCP
Použití knihovny ESPAsyncWebServer ve verzi 1.2.3 v adresáři: /home/dan/Arduino/libraries/ESPAsyncWebServer
Použití knihovny AsyncElegantOTA ve verzi 2.2.7 v adresáři: /home/dan/Arduino/libraries/AsyncElegantOTA
Použití knihovny Update ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Update
Použití knihovny PubSubClient ve verzi 2.8 v adresáři: /home/dan/Arduino/libraries/PubSubClient

mosquitto_pub -h 54.38.157.134 -t OK1HRA/0/ROT/Azimuth -m '83'
mosquitto_sub -v -h 54.38.157.134 -t 'OK1HRA/0/ROT/#'

New firmware version (GitHub Pages USB web flasher)
  1. Increase REV value below.
  2. Arduino IDE 1.8.19: Sketch/Export compiled Binary
     (board "ESP32 Dev Module", Partition Scheme "No OTA (2MB APP/2MB SPIFFS)")
     -> produces esp32-e-ink.ino.esp32.bin in this sketch folder.
  3a. Build the web flasher locally (no publish): $ ./tools/gh-pages.sh
  3b. Build AND publish to GitHub Pages:          $ ./tools/gh-pages.sh --publish
      (--publish wipes the gh-pages branch, so only the latest firmware stays online)
  4. git commit with the Release number and push.

Web UI only (no new firmware)
  - edit data/*.html/css/js, then rebuild the SPIFFS image: $ ./tools/build_spiffs_image.sh
  - flash it over USB: esptool.py --chip esp32 write_flash 0x210000 build/spiffs.bin

*/
//-------------------------------------------------------------------------------------------------------

#define REV 20260707
#define WIFI
#define MQTT                      // enable MQTT
#define WDT         // watchdog timer
// #define APRSFI                 // enable get from aprs.fi - not work
#include <esp_adc_cal.h>
#include <SPI.h>
#define ENABLE_GxEPD2_GFX 1   // make GxEPD2_BW/3C derive from GxEPD2_GFX for runtime panel selection
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_GFX.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <TrxNet.h>      // P2P alternative to MQTT (runtime-selectable)
// #define BMPMAP

// Display panel type is selected at runtime (NVS "disp") and instantiated in initDisplay().
// All settings live in NVS via Preferences; the config web UI is served from SPIFFS.
// 0 = GDEW042T2 (UC8176)  1 = GDEY042T81 (SSD1683)  2 = GDEQ042Z21 (UC8276, 3-color)
GxEPD2_GFX *gfx = nullptr;
uint8_t dispType = 0;
Preferences prefs;
const char* NVS_NS = "eink";
/*
  Configuration is stored in NVS (Preferences, namespace "eink"). Keys:
    apmode bool | ssid0..3 / pass0..3 String | devsel int | topic String
    mqip0..3 uchar | mqport ushort | rot uchar | neg bool | offto uchar
    units bool | tzh int (hours) | dst bool | ntp String | disp uchar
    proto uchar (0=MQTT 1=TrxNet) | udpport ushort | devid String
  First boot (no config) -> AP mode. WiFi-connect failure -> set apmode and reboot.
*/


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

// source https://oledgfx->squix.ch/ - must copy via clipboard!
// #include "Open_Sans_Condensed_Light_80.h"
// #include "Open_Sans_Condensed_Bold_20.h"
// #include "Open_Sans_Condensed_Light_16.h"

// https://rop.nl/truetype2gfx/
#include "Logisoso8pt7b.h"
#include "Logisoso10pt7b.h"
#include "Logisoso20pt7b.h"   // "More sources" medium value size
#include "Logisoso30pt7b.h"   // "More sources" large-ish value size (index 3, added after 40pt)
#include "Logisoso40pt7b.h"   // "More sources" large value size
#include "Logisoso50pt7b.h"
// gfx->setFont(&Logisoso250pt7b);
uint16_t colorB = GxEPD_BLACK;
uint16_t colorW = GxEPD_WHITE;

// #define SLEEP                    // Uncomment so board goes to sleep after printing on display
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 10          // Time ESP32 will go to sleep (in seconds)

const String mainHWdevice[4][2] = {
    {"/ROT/", "IP rotator"},  // 0
    {"/WX/", "WX station"},   // 1
    {"", "aprs.fi"},          // 2
    {"topic", "name"},        // 3
};
int mainHWdeviceSelect = -1;  //0 = IP rotator, 1 = WX station, 2 = aprs.fi source
String MQTT_TOPIC = "";        // same as 'location' on IP rotator
String TOPIC = "";        // same as 'location' on IP rotator
String ROT_TOPIC = "";    // mainHWdevice[mainHWdeviceSelect][0]
String WX_TOPIC = "";    // mainHWdevice[mainHWdeviceSelect][0]
byte mqttBroker[4]={0,0,0,0}; // MQTT broker IP address
int MQTT_PORT = 0;         // MQTT broker port
#define WALL_WS_PORT 1884  // broker WebSocket port for the MQTT wall page (must have a WS listener)
IPAddress mqtt_server_ip(mqttBroker[0], mqttBroker[1], mqttBroker[2], mqttBroker[3]);       // MQTT broker IP address
String SSID = "";   // active connection (the network we joined this boot)
String PSWD = "";
String APRS_FI_NAME = "";
String APRS_FI_APIKEY = "";

// up to 4 stored WiFi networks; boot scans and joins the strongest reachable known one
String wifiSSID[4] = {"","","",""};
String wifiPSWD[4] = {"","","",""};

String topicBase = "";    // single topic base from the web UI; /ROT/ or /WX/ is appended per devsel
bool usUnits = false;     // false = metric, true = US units (was #define USunits)

unsigned int eInkRotation = 1; // 1 USB TOP, 3 USB DOWN | 0 default, 1 90°CW, 2 180°CW, 3 90°CCW
int OfflineTimeout = 5;   // minutes
bool eInkNegativ = false;
bool eInkNegativTmp = false;
int DesignSkin = 0;       // not implemented!

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

int Az=0;
char buf[21];
#define RAD_TO_DEG 57.295779513082320876798154814105

// ntp (configurable via web; derived from tzHours/dst/ntp in NVS)
#include "time.h"
String ntpServer = "pool.ntp.org";
long  gmtOffset_sec = 0;
int   daylightOffset_sec = 0;

int DebuggingOutput = 1;  // 0-off | 1-Serial

#if defined(WDT)
  // 1000 seconds WDT (WatchDogTimer)
  #include <esp_task_wdt.h>
  #define WDT_TIMEOUT 1000
  long WdtTimer=0;
#endif

#if defined(WIFI)
  #include <WiFi.h>
  // #include <ETH.h>
  // int SsidPassSize = (sizeof(SsidPass)/sizeof(char *))/2; //array size
  // int SelectSsidPass = -1;
  #define wifi_max_try 20             // Number of try
  unsigned long WifiTimer = 0;
  unsigned long WifiReconnect = 30000;
  String MACString;

  #include <ESPmDNS.h>

  const char* ssidAP     = "esp32-e-ink-AP";
  const char* passwordAP = "remoteqth";
  bool APmode = false;
  #include <WebServer.h>
  WebServer ajaxserver(80);
  DNSServer dnsServer;            // captive portal in AP mode
  const byte DNS_PORT = 53;
  bool FsMounted = false;         // true after a successful SPIFFS.begin()
#endif

unsigned int RunApp = 255;

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

// ---- TrxNet (runtime-selectable alternative to MQTT) ----
// Both protocols are compiled in; only the one selected by NVS "proto" is started at boot.
WiFiUDP  trxUdp;
TrxNet   trxNet(trxUdp);
int      proto      = 0;     // 0 = MQTT, 1 = TrxNet  (NVS "proto")
uint16_t udpPort    = 5683;  // TrxNet UDP/CoAP port  (NVS "udpport")
String   deviceId   = "";    // numeric ID only, e.g. "01"  (NVS "devid")
String   trxSource  = "";    // peer we mirror, e.g. "WX.01" (configured, or auto-picked at runtime)
String   trxOwnName = "";    // our own TrxNet name, e.g. "INK.01" (or INK.<MAC> when no devid)
// Priority prefixes: peers whose name starts with one of these keep a slot when the peer
// table (TRXNET_MAX_PEERS) fills on a busy network — they evict the stalest non-priority
// peer instead of being dropped. This matters even though we only subscribe: a dropped
// sender has no peer entry, so its name can't be resolved (TrxNet.cpp) and its telemetry
// stops matching a source row. Space-separated (e.g. "ROT WX"); empty -> derived from the
// device type in trxBegin() (ROT rotator -> "ROT", WX station -> "WX", others -> none).
String   prioPrefixRaw = "";        // NVS "prioPfx"
#define  PRIO_MAX 6
static char        prioStore[PRIO_MAX][TRXNET_MAX_DEVICE_NAME];  // stable storage the library keeps referencing
static const char* prioPtr[PRIO_MAX];                            // pointer array handed to setPriorityPrefixes()
// --- runtime source auto-select (never persisted to NVS) ---
String   trxCfgSource = "";  // the configured source to honour/reclaim ("" = none configured -> pure auto)
int      trxCfgSelect = 1;   // configured device type (layout) to prefer when auto-picking
bool     trxAutoPicked= false;// true when trxSource came from the network scan, not from config
int      trxCurSelect = -1;  // device type currently subscribed (-1 = nothing subscribed yet)
uint32_t trxStartMs   = 0;   // millis() at trxBegin(), for the configured-source grace window

// ---- More sources (devsel==3): user-defined list of label/value rows ----
// A variable-length list configured in setup ("More sources" tab) and persisted to NVS as one
// RS/US-encoded string ("srcCfg"). Each row mirrors one value from MQTT (text payload) or TrxNet
// (raw bytes decoded per the row's format). Both network stacks run at once in this mode.
#define SRC_MAX 12
struct Source {
  String  label;        // left description; may contain '\n' for multiple lines
  uint8_t proto;        // 0 = MQTT, 1 = TrxNet
  String  topic;        // MQTT: full path  |  TrxNet: "DEV.ID/path" (e.g. WX.01/temp)
  uint8_t format;       // TrxNet decode: 0 int16/100, 1 int16, 2 uint16, 3 int32, 4 float, 5 ASCII, 6 uint8, 7 int8, 8 uint32
  uint8_t decimals;     // displayed decimal places (0-2); rounds the numeric transform (and TrxNet decode)
  uint8_t size;         // value font: 0=10pt 1=20pt 2=40pt
  String  suffix;       // unit chars drawn after the value (same size), e.g. "°C"
  bool    refreshAfter; // refresh the display immediately on a new value (before the global interval)
  // ---- display formatting (applyFormat), all optional / backward compatible ----
  bool    numeric;      // apply the numeric transform: extract number, y = x*mul + add, round to decimals
  float   mul;          // numeric gain   (default 1)
  float   add;          // numeric offset (default 0)
  String  thousands;    // thousands-group separator inserted every 3 integer digits ("" = off), e.g. "."
  String  decsep;       // decimal separator replacing "." ("" = keep "."), e.g. "," -> 123.456,78
  String  replaceMap;   // literal from->to pairs: "from"\x1c"to", pairs joined by \x1d (no regex)
  String  rawText;      // last raw MQTT payload (unformatted) - kept so live preview can re-format
  String  value;        // last received/decoded value as text (after applyFormat)
  bool    haveValue;    // a value has arrived at least once
  uint8_t rawBuf[8];    // last raw TrxNet payload (for live re-decode in the setup preview)
  uint8_t rawLen;
};
Source   sources[SRC_MAX];
int      sourceCount = 0;
String   srcCfgRaw = "";          // raw NVS string (RS/US encoded)
uint16_t srcRefreshSec = 60;      // global refresh interval in seconds (NVS "srcRefr", UI mm:ss)
bool     srcUseMqtt = false;      // any MQTT row present -> start MQTT stack in devsel==3
bool     srcUseTrx  = false;      // any TrxNet row present -> start TrxNet stack in devsel==3
unsigned long srcRefreshTimer = 0;// millis() of last forced periodic refresh
// TrxNet callbacks carry no path, so each subscribed path gets its own trampoline slot.
String   srcTrxSubPath[8];        // path subscribed in slot i (e.g. "/temp")
int      srcTrxSubCount = 0;

// ---- Low power (battery) mode ----------------------------------------------------------------------
// Fixed-interval deep sleep + pull-latest-on-wake. The e-ink image is bistable so it stays visible
// while the board sleeps. On each timer wake we reconnect (fast, from RTC-cached AP), pull the latest
// reading (MQTT retained / TrxNet greet), refresh only if it changed, then sleep again.
// Hardware: LaskaKit ESPink-42 v2.x -> battery divider on GPIO34, e-paper supply transistor on GPIO2.
#define LP_BAT_PIN        34            // ADC1_CH6, safe to read with Wi-Fi on
#define LP_DIVIDER_RATIO  1.7693877551f // 1 MOhm + 1.3 MOhm divider (LaskaKit ESPink42_V2)
#define LP_POWER_PIN      2             // e-paper supply transistor (HIGH = panel on)
#define LP_GRACE_MS       90000UL       // cold-boot window kept awake for web config
#define LP_WAKE_BUDGET_MS 15000UL       // max awake time on a timer wake before giving up
#define LP_BAT_WARN       3.50f         // below -> show "recharge" marker, keep running
#define LP_BAT_CRIT       3.30f         // below -> park (final screen + long sleep) to protect the cell
#define LP_BAT_RESUME     3.45f         // hysteresis: leave park only above this
#define LP_I_ACTIVE_MA    90.0f         // assumed average current while awake (Wi-Fi on), for runtime estimate
#define LP_I_SLEEP_MA     0.05f         // assumed deep-sleep current (e-paper supply off)
bool     lowPower    = false;  // NVS "lowpwr"
uint16_t lpInterval  = 15;     // wake interval in minutes (NVS "lpint")
uint16_t batCapacity = 0;      // battery capacity in mAh for the runtime estimate (NVS "batcap", 0 = off)
bool     timerWake   = false;  // this boot was an interval wake (vs cold boot / RESET)
bool     lpDataHandled = false;// fresh reading received and shown (or suppressed) this wake
float    lpVbat      = 0.0f;   // last battery reading (volts)

// Persisted across deep sleep (RTC slow memory survives the reboot a timer wake performs)
RTC_DATA_ATTR uint32_t rtcBootCount   = 0;
RTC_DATA_ATTR uint8_t  rtcBssid[6]    = {0};
RTC_DATA_ATTR uint8_t  rtcChannel     = 0;
RTC_DATA_ATTR int      rtcSlot        = -1;    // wifi slot of the cached AP
RTC_DATA_ATTR bool     rtcBssidValid  = false;
RTC_DATA_ATTR uint32_t rtcShownHash   = 0;     // hash of the values last drawn (refresh suppression)
RTC_DATA_ATTR bool     rtcParked      = false; // critical-battery park latched
RTC_DATA_ATTR uint32_t rtcAwakeAvgMs  = 0;     // EMA of the measured awake-window length on timer wakes

#include "esp_sleep.h"
#include "driver/gpio.h"

//-------------------------------------------------------------------------------------------------------
// Forward declarations (Arduino auto-prototype generation fails for this sketch)
void print_wifi_error();
void MqttRx(char *topic, byte *payload, unsigned int length);
bool mqttReconnect();
String UtcTime(int format);
void Watchdog();
void Mqtt();
void eInkRefresh();
int AzimuthShifted(int DEG);
void DirectionalRosette(int deg, int X, int Y, int R);
void Triangle(float VALUE, float MIN, float MAX);
void Arrow(int deg, int X, int Y, int r);
float Xcoordinate(int dir, int Center, int r);
float Ycoordinate(int dir, int Center, int r);
void MqttPubString(String TOPICEND, String DATA, bool RETAIN);
void loadConfig();
void applyDerivedConfig();
void initDisplay();
bool connectWifi();
void startAPmode();
void handleApiConfig();
void handleApiConfigSave();
void handleFactoryReset();
void handleRoot();
void handleStatic();
bool streamSpiffsFile(const String& path);
String webContentType(const String& path);
String jsonEsc(const String& s);
void trxBegin();
String footerId();
void TrxLoop();
void recomputeDewPoint();
void handleApiPeers();
void onTrxPeer(const TrxPeer* p);
void trxSubscribeFor(int type);
void trxMaintainSource();
void drawTrxWaiting();
float lpBatteryVolts();
void lpInit();
void lpDrawRecharge(float vb);
void lpEnterDeepSleep();
void lowPowerManage();
void lpDrawBattery();
uint32_t lpWxHash();
float lpEstimateDays();
// "More sources" (devsel==3)
void parseSources();
String decodeTrx(uint8_t fmt, const uint8_t* d, size_t l, uint8_t decimals = 2);
void srcMark(bool immediate);
void handleTrxSource(int slot, const char* from, const uint8_t* d, size_t l);
void srcTrxSubscribe();
void drawSourcesScreen();
const GFXfont* srcValueFont(uint8_t size);
int srcValueHeight(uint8_t size);
int srcCountLines(const String& s);
void handleApiSrcPreview();
void handleApiWallCfg();

//-------------------------------------------------------------------------------------------------------
void setup(void){

  Serial.begin(115200);
  Serial.println();
  Serial.println("-------- DivaDroid International --------");
  Serial.print(" ink| rev ");
  Serial.println(REV);
  Serial.println(" ink| press '?' for network status");

  loadConfig();          // read all settings from NVS into globals (sets APmode on first boot)

  // Power up the e-paper supply (GPIO2 transistor) BEFORE initialising the panel. In low-power mode
  // the supply is parked LOW during deep sleep (the e-ink image is bistable, so it stays visible with
  // no power) - release that hold first or GPIO2 would stay LOW and the panel init would talk to a
  // dead bus.
  gpio_hold_dis((gpio_num_t)LP_POWER_PIN);
  gpio_deep_sleep_hold_dis();
  pinMode(2, OUTPUT);    // Set epaper transistor as output
  digitalWrite(2, HIGH); // Turn on epaper transistor
  delay(100);            // Delay so it has time to turn on

  initDisplay();         // instantiate gfx for the selected panel (NVS "disp")
  gfx->init();
  gfx->setRotation(eInkRotation); // 1 USB TOP, 3 USB DOWN | 0 default, 1 90°CW, 2 180°CW, 3 90°CCW
  Serial.println(" LCD| init rotation "+String(eInkRotation));

  applyDerivedConfig();   // colors from eInkNegativ, ROT/WX/TOPIC from topicBase, tz from tzHours/dst

  lpInit();   // low-power: classify wake cause, release sleep GPIO hold, battery protection (may sleep)

  // mount SPIFFS (web UI assets). Keep an existing good FS; format empty if the partition is fresh.
  FsMounted = SPIFFS.begin(false);
  if(!FsMounted){
    Serial.println("FS | SPIFFS mount failed - formatting to partition geometry");
    FsMounted = SPIFFS.begin(true);
  }
  if(FsMounted){
    Serial.print("FS | SPIFFS used "); Serial.print(SPIFFS.usedBytes());
    Serial.print("/"); Serial.print(SPIFFS.totalBytes()); Serial.println(" B");
  }else{
    Serial.println("FS | SPIFFS unavailable - flash spiffs.bin over USB");
  }

  // web routes - served in both AP and STA mode (server started after WiFi is up, below)
  ajaxserver.on("/", handleRoot);
  ajaxserver.on("/setup", handleRoot);
  ajaxserver.on("/api/config", HTTP_GET, handleApiConfig);
  ajaxserver.on("/api/config", HTTP_POST, handleApiConfigSave);
  ajaxserver.on("/api/peers", HTTP_GET, handleApiPeers);
  ajaxserver.on("/api/srcpreview", HTTP_GET, handleApiSrcPreview);
  ajaxserver.on("/api/wallcfg", HTTP_GET, handleApiWallCfg);
  ajaxserver.on("/factoryreset", HTTP_POST, handleFactoryReset);
  ajaxserver.onNotFound(handleStatic);   // static SPIFFS files (gzip-aware) + captive-portal catch-all

  #if defined(WIFI)
    if(APmode==true){
      startAPmode();
    }else{
      if(!connectWifi()){
        // low-power timer wake: never fall back to AP mode (it would drain the battery). One short
        // retry, then sleep and try again next interval - the e-ink keeps its retained image.
        if(lowPower && timerWake){
          Serial.println("WIFI| timer wake connect failed - short retry then sleep");
          delay(1000);
          if(!connectWifi()) lpEnterDeepSleep();   // never returns
        }else{
          prefs.begin(NVS_NS, false);
          prefs.putBool("apmode", true);   // no known network reachable -> reconfigure in AP mode
          prefs.end();
          Serial.println("WIFI| no known AP reachable - rebooting to AP mode...");
          delay(3000);
          ESP.restart();
        }
      }
      MACString = WiFi.macAddress();
      if (!MDNS.begin("esp32eink")) {
        Serial.println("mDNS| responder failed");
      }else{
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS| responder started");
      }
    }
    ajaxserver.begin();
    Serial.println("HTTP| web server started");
  #endif


  if(APmode==false){

    #if defined(HTTP)
      server = WiFiServer(HTTP_CAT_PORT);
      server.begin();

      // Add service to MDNS-SD
      MDNS.addService("http", "tcp", 81);
      MDNS.addService("http", "tcp", 80);
    #endif

    #if defined(MQTT)
      // if(mqttEnable==true){
        if (MQTT_LOGIN == true){
        // if (mqttClient.connect("esp32gwClient", MQTT_USER, MQTT_PASS)){
          //   AfterMQTTconnect();
          // }
        }else if( (proto==0 && (mainHWdeviceSelect==0 || mainHWdeviceSelect==1)) || (mainHWdeviceSelect==3 && srcUseMqtt && mqttBroker[0]!=0) ){
            mqtt_server_ip = IPAddress(mqttBroker[0], mqttBroker[1], mqttBroker[2], mqttBroker[3]);       // MQTT broker IP address (set global)
            mqttClient.setServer(mqtt_server_ip, MQTT_PORT);
            Serial.print("MQTT| Connect to ");
            Serial.print(mqtt_server_ip);
            Serial.print(":");
            Serial.println(MQTT_PORT);
            mqttClient.setCallback(MqttRx);
            Serial.println("MQTT| Callback");
            lastMqttReconnectAttempt = 0;

            char charbuf[50];
            WiFi.macAddress().toCharArray(charbuf, 18);
              Serial.print("MQTT| maccharbuf ");
              Serial.println(charbuf);
              mqttReconnect();
        }
      // }
    #endif
    if(proto==1 || (mainHWdeviceSelect==3 && srcUseTrx)){   // start TrxNet; skip splash on a low-power timer wake (keep retained image)
      trxBegin();
      if(!(lowPower && timerWake) && mainHWdeviceSelect!=3) drawTrxWaiting();
    }
    if(mainHWdeviceSelect==3){ srcRefreshTimer=millis(); eInkNeedRefresh=true; }  // draw the first (possibly empty) frame
    Serial.println("AP-MODE OFF");
    Serial.println("");
    Serial.print("WIFI connected with IP ");
    Serial.println(WiFi.localIP());
    Serial.print("WIFI dBm: ");
    Serial.println(WiFi.RSSI());

  }



  

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  #if defined(APRSFI)
    jsonString.reserve(900);
  #endif

  #if defined(WDT)
    // WDT
    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch
    WdtTimer=millis();
  #endif
}

//-------------------------------------------------------------------------------------------------------
// Serial console: press '?' (or 'i') to print the current network status.
void serialMenu(){
  while(Serial.available()){
    char c = Serial.read();
    if(c=='?' || c=='i' || c=='I'){
      Serial.println();
      Serial.println("--------------- e-ink status ---------------");
      Serial.println(" FW REV  : "+String(REV));
      Serial.println(" MAC     : "+MACString);
      if(APmode){
        Serial.println(" mode    : AP (setup)");
        Serial.println(" AP SSID : "+String(ssidAP));
        Serial.println(" IP      : "+WiFi.softAPIP().toString());
        Serial.println(" URL     : http://esp32eink.local  or  http://"+WiFi.softAPIP().toString());
      }else{
        Serial.println(" mode    : client (STA)");
        Serial.println(" SSID    : "+SSID+"  ("+String(WiFi.RSSI())+" dBm)");
        Serial.println(" IP      : "+WiFi.localIP().toString());
        Serial.println(" URL     : http://esp32eink.local  or  http://"+WiFi.localIP().toString());
        if(proto==1){
          Serial.println(" TrxNet  : "+trxOwnName+" :"+String(udpPort)+"  peers "+String(trxNet.peerCount()));
          Serial.println(" source  : "+trxSource);
        }else{
          #if defined(MQTT)
            Serial.println(" MQTT    : "+mqtt_server_ip.toString()+":"+String(MQTT_PORT)+"  "+(mqttClient.connected()?"connected":"down"));
            Serial.println(" topic   : "+TOPIC);
          #endif
        }
      }
      Serial.println("--------------------------------------------");
    }
  }
}

void loop(void) {
  serialMenu();

  if(APmode==true){
    dnsServer.processNextRequest();   // captive portal
    ajaxserver.handleClient();
    // eInkRefresh();
    #if defined(WDT)
      if(millis()-WdtTimer > 60000){
        esp_task_wdt_reset();
        WdtTimer=millis();
      }
    #endif

  }else{
    Watchdog();
    Mqtt();
    TrxLoop();
    // More sources: force a periodic full refresh even without new data (global Refresh time)
    if(mainHWdeviceSelect==3 && srcRefreshSec>0 && millis()-srcRefreshTimer > (unsigned long)srcRefreshSec*1000UL){
      srcRefreshTimer = millis();
      eInkNeedRefresh = true;
    }
    eInkRefresh();
    ajaxserver.handleClient();
    lowPowerManage();   // low-power: end the awake window and deep sleep when done
  }
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
  bool lpWake = (lowPower && timerWake);   // low-power timer wake: refresh immediately, no throttle wait
  if( mainHWdeviceSelect==0 && eInkNeedRefresh==true && (millis()-eInkRefreshTimer > 5000 || lpWake) && Azimuth!=-42 && Name != "" ){
      gfx->fillScreen(colorB);

      #if defined(BMPMAP)
        gfx->drawBitmap(0, 0, ok, 300, 300, colorW);
      #endif

      if(Azimuth>=0){
        DirectionalRosette(AzimuthShifted(Azimuth), 150, 145, 130);
      }

      gfx->setTextColor(colorW);
      gfx->setFont(&Logisoso50pt7b);
      /*
      char to - (minus) width 39px
      char width 33px
      char to char width 44px
      dot+char width 68px
      */
      if(Azimuth>=0){
        if(AzimuthShifted(Azimuth)>=100){
          gfx->setCursor(175-44, 360);
        }else if(AzimuthShifted(Azimuth)<100 && AzimuthShifted(Azimuth)>=10){
          gfx->setCursor(175, 360);
        }else if(AzimuthShifted(Azimuth)<10){
          gfx->setCursor(175+44, 360);
        }
        gfx->println(AzimuthShifted(Azimuth));
        // gfx->setCursor(270, 310);
        // gfx->setFont(&Logisoso10pt7b);
        // gfx->println("o");
        gfx->fillCircle(275, 300, 6, colorW);
      }else{
        gfx->setCursor(170, 355);
        gfx->println("n/a");
      }
      int ZZshift=2;
      gfx->setFont(&Logisoso8pt7b);
      gfx->setCursor(15, 285+4*ZZshift);
      gfx->println(String(SSID)+" "+String(WiFi.RSSI())+" dBm");
      gfx->setCursor(15, 310+3*ZZshift);
      gfx->print(WiFi.localIP());
      gfx->setFont(&Logisoso10pt7b);
      gfx->setCursor(15, 335+2*ZZshift);
      gfx->println(Name);
      gfx->setFont(&Logisoso8pt7b);
      gfx->setCursor(15, 360+ZZshift);
      gfx->println(footerId());
      gfx->setCursor(15, 385);
      UtcTime(1).toCharArray(buf, 21);
      gfx->println("UTC "+String(buf));
      if(eInkOfflineDetect==true){
        gfx->setCursor(185, 385);
        gfx->setFont(&Logisoso10pt7b);
        gfx->print("OFF >"+String(OfflineTimeout)+"min");
      }else{
        gfx->setCursor(200, 385);
        gfx->print(REV);
      }
      lpDrawBattery();
      gfx->display(false);
      eInkNeedRefresh=false;
      eInkRefreshTimer=millis();
      lpDataHandled=true;
    // WX
    }else if( (mainHWdeviceSelect==1 || mainHWdeviceSelect==2) && eInkNeedRefresh==true && (millis()-eInkRefreshTimer > 10000 || lpWake) ){
      // Low-power: skip the slow panel refresh when nothing visible changed since the last drawn frame.
      if(lpWake && lpWxHash() == rtcShownHash){
        Serial.println("LP | WX unchanged - skip refresh");
        eInkNeedRefresh=false; eInkRefreshTimer=millis(); lpDataHandled=true;
        return;
      }
      // Serial.println("eInk eInkNegativ "+String(eInkNegativ));
      // Serial.println("eInk colorB "+String(colorB));
      // Serial.println("eInk colorW "+String(colorW));
        gfx->fillScreen(colorB);

        // local display copies so unit conversion never mutates the source globals (would compound on each refresh)
        float dispTemperature = Temperature;
        float dispDewPoint = DewPoint;
        float dispRainToday = RainToday;
        float dispWindSpeedMaxPeriod = WindSpeedMaxPeriod;
        if(usUnits){
          dispTemperature = (Temperature*1.8)+32;
          dispDewPoint = (DewPoint*1.8)+32;
          dispRainToday = RainToday/25.4;
          dispWindSpeedMaxPeriod = WindSpeedMaxPeriod*3.281;
        }

        gfx->setTextColor(colorW);
        gfx->setFont(&Logisoso50pt7b);
        int Xshift=0;
        if(dispTemperature<0){
          Xshift=-39;
        }else{
          Xshift=0;
        }
        if(abs(dispTemperature)>=10){
          gfx->setCursor(64+Xshift, 85);
        }else if(abs(dispTemperature)<10){
          gfx->setCursor(97+Xshift, 85);
        }
        String str = String(dispTemperature);
        String subStr = str.substring(0, str.length() - 1);
        gfx->println(String(subStr));
        gfx->setCursor(242, 85);
        gfx->println(usUnits ? "F" : "C");
        gfx->fillCircle(230, 25, 6, colorW);

        gfx->drawLine(15, 100, 285, 100, colorW);
        float XX = (285.0-15.0)/100.0*HumidityRel+15.0;
        gfx->fillCircle((int)XX, 100, 3, colorW);

        gfx->setFont(&Logisoso8pt7b);
        gfx->setCursor(15, 125);
        gfx->print("Relative humidity ");
        gfx->setFont(&Logisoso10pt7b);
        gfx->print(String((int)HumidityRel)+"%  ");
        gfx->setFont(&Logisoso8pt7b);
        gfx->print("Dew point ");
        gfx->setFont(&Logisoso10pt7b);
        gfx->print(String((int)dispDewPoint)+" ");
        gfx->println(usUnits ? "F" : "C");

        gfx->setCursor(15, 150);
        gfx->setFont(&Logisoso8pt7b);
        gfx->print("Pressure ");
        gfx->setFont(&Logisoso10pt7b);
        gfx->print(String((int)Pressure)+" hpa");
        Triangle(Pressure, 983.0, 1043.0);  // 1013 +-30
        gfx->drawLine(15, 200, 20, 200, colorW);

        if(dispRainToday>0){
          str = String(dispRainToday);
          subStr = str.substring(0, str.length() - 1);
          gfx->print("  RAIN "+String(subStr)+" ");
          gfx->println(usUnits ? "in" : "mm");
          int ten = (int)dispRainToday % 10;
          if(dispRainToday>0 && dispRainToday<1){
            gfx->fillCircle(285-1*(11+1), 170, 3+1, colorW);
          }
          for (int j=ten; j>0; j--) {
            gfx->fillCircle(285-j*(11+j), 170, 3+j, colorW);
          }
          int tens = (int)(dispRainToday/10);
          for (int j=tens; j>0; j--) {
            gfx->fillCircle(j*30-5, 170, 13, colorW);
          }
        }

        gfx->setFont(&Logisoso50pt7b);
        if(abs(dispWindSpeedMaxPeriod)>=10){
          gfx->setCursor(6+4, 265);
        }else if(abs(dispWindSpeedMaxPeriod)<10){
          gfx->setCursor(50+4, 265);
        }
        if(dispWindSpeedMaxPeriod>0){
          gfx->println((int)dispWindSpeedMaxPeriod);
          // gfx->setFont(&Logisoso10pt7b);
          gfx->setFont(&Logisoso8pt7b);
          gfx->setCursor(35, 290);
          gfx->println(usUnits ? "gust ft/s" : "gust m/s");

        }

        // int Pressure = 0;
        // int WindSpeedAvg = 0;

        DirectionalRosette(WindDir, 200, 270, 80);

        int ZZshift=2;
        // gfx->setFont(&Logisoso10pt7b);
        // gfx->println(Name);
        gfx->setFont(&Logisoso8pt7b);
        gfx->setCursor(15, 285+4*ZZshift);
        gfx->setCursor(15, 310+3*ZZshift);
        gfx->println(String(SSID)+" "+String(WiFi.RSSI())+" dBm");
        gfx->setCursor(15, 335+2*ZZshift);
        gfx->print(WiFi.localIP());
        gfx->setFont(&Logisoso8pt7b);
        gfx->setCursor(15, 360+ZZshift);
        if(mainHWdeviceSelect==1){
          gfx->println(footerId());
        }else if(mainHWdeviceSelect==2){
          gfx->println(String(mainHWdevice[mainHWdeviceSelect][1])+"/"+String(APRS_FI_NAME));
        }
        gfx->setCursor(15, 385);
        UtcTime(1).toCharArray(buf, 21);
        gfx->println("UTC "+String(buf));
        if(eInkOfflineDetect==true){
          gfx->setCursor(185, 385);
          gfx->setFont(&Logisoso10pt7b);
          gfx->print("OFF >"+String(OfflineTimeout)+"min");
        }else{
          gfx->setCursor(200, 385);
          gfx->print(REV);
        }

        lpDrawBattery();
        gfx->display(false);
        eInkNeedRefresh=false;
        eInkRefreshTimer=millis();
        lpDataHandled=true;
        if(lowPower) rtcShownHash = lpWxHash();   // seed/refresh suppression baseline (incl. cold boot)
    // More sources
    }else if( mainHWdeviceSelect==3 && eInkNeedRefresh==true && (millis()-eInkRefreshTimer > 3000 || lpWake) ){
        drawSourcesScreen();
        eInkNeedRefresh=false;
        eInkRefreshTimer=millis();
        lpDataHandled=true;
  }
}
//------------------------------------------------------------------------------
void Triangle(float VALUE, float MIN, float MAX){
  float YY = 400.0-(VALUE - MIN) * (400.0/(MAX-MIN));
  gfx->fillTriangle(0, (int)YY-5, 14, (int)YY, 0, (int)YY+5, colorW);
  // Serial.println("Triangle Ypx: "+String(YY));
}

//------------------------------------------------------------------------------
int AzimuthShifted(int DEG){
  DEG=DEG+AzimuthStart; // 246 + 390 > 236
  DEG=((DEG % 360) + 360) % 360; // normalize to 0..359 for any input
  return DEG;
}

//-------------------------------------------------------------------------------------------------------
void WDTimer(){
  eInkOfflineDetect = false;
  // WDT
  esp_task_wdt_reset();
}

//-------------------------------------------------------------------------------------------------------
void Watchdog(){

  // // WDT
  // if(millis()-WdtTimer > 960000){
  //   esp_task_wdt_reset();
  //   WdtTimer=millis();
  //   Serial.print("WDT reset ");
  //   Serial.println(UtcTime(1));
  // }

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

  static bool eInkOfflineDetectTmp = false;
  if( (millis()-RxMqttTimer) > OfflineTimeout*60000 && eInkOfflineDetect == false ){
    eInkNeedRefresh=true;
    eInkOfflineDetect = true;
    Serial.print(millis());
    Serial.print(" | ");
    Serial.print(OfflineTimeout);
    Serial.println(" minutes offline timeout");
  }

  if(eInkOfflineDetect!=eInkOfflineDetectTmp){
    eInkNegativ = !eInkNegativ;
    eInkOfflineDetectTmp = eInkOfflineDetect;
  }

  // static unsigned long eInkNegativTimer = millis();
  // if(millis()-eInkNegativTimer > 10000 || eInkNeedRefresh==true){
    if(eInkNegativ!=eInkNegativTmp){
      if(eInkNegativ==true){
        colorB = GxEPD_BLACK;
        colorW = GxEPD_WHITE;
        eInkNegativTmp=true;
      }else{
        colorB = GxEPD_WHITE;
        colorW = GxEPD_BLACK;
        eInkNegativTmp=false;
      }
      // Serial.println("wd eInkNegativ "+String(eInkNegativ));
      // Serial.println("wd colorB "+String(colorB));
      // Serial.println("wd colorW "+String(colorW));
    }
  // }

  // #if defined(WIFI)
  //   unsigned long currentMillis = millis();
  //   // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  //   if ((WiFi.status() != WL_CONNECTED) && (currentMillis - WifiTimer >=WifiReconnect)) {
  //     Serial.print(millis());
  //     Serial.println(" Reconnecting to WiFi...");
  //     WiFi.disconnect();
  //     WiFi.reconnect();
  //     WifiTimer = currentMillis;
  //   }
  // #endif
  // WIFI status
  #if defined(WIFI)
    unsigned long currentMillis = millis();
    // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - WifiTimer >=WifiReconnect)) {
      Serial.print(millis());
      Serial.println("cReconnecting...");
      WiFi.disconnect();
      WiFi.reconnect();
      WifiTimer = currentMillis;
    }
  #endif

  // WDT
  #if defined(WDT)
    if(millis()-WdtTimer > 60000){
      esp_task_wdt_reset();
      WdtTimer=millis();
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
      gfx->setFont(&Logisoso10pt7b);
    }else{
      dot1=1.5;
      dot2=3;
      gfx->setFont(&Logisoso8pt7b);
    }
    if(R>100){
      gfx->setCursor(Xcoordinate(0,X-5,R-10), Ycoordinate(0,Y,R-10));
      gfx->println("N");
      gfx->setCursor(Xcoordinate(90,X+5,R-10), Ycoordinate(90,Y+8,R-10));
      gfx->println("E");
      gfx->setCursor(Xcoordinate(180,X-6,R-10), Ycoordinate(180,Y+13,R-10));
      gfx->println("S");
      gfx->setCursor(Xcoordinate(270,X-16,R-10), Ycoordinate(270,Y+8,R-10));
      gfx->println("W");
    }
    for (int j=0; j<36; j++) {
      if(j % 9 == 0){
        if(R<100){
          gfx->fillCircle(Xcoordinate(j*10,X,R), Ycoordinate(j*10,Y,R), dot2, colorW);
        }
      }else{
        gfx->fillCircle(Xcoordinate(j*10,X,R), Ycoordinate(j*10,Y,R), dot1, colorW);
      }
    }
    if( mainHWdeviceSelect==0 || (mainHWdeviceSelect==1 && WindSpeedMaxPeriod>0) || mainHWdeviceSelect==2){
      Arrow(deg,X,Y,R*0.9);
      Serial.println("Arrow");
    }
}

//-------------------------------------------------------------------------------------------------------
void Arrow(int deg, int X, int Y, int r){
  int deg2 = deg+130;
  int deg3 = deg+230;
  gfx->fillTriangle(Xcoordinate(deg,X,r), Ycoordinate(deg,Y,r), Xcoordinate(deg2,X,r/2), Ycoordinate(deg2,Y,r/2), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), colorW);
  gfx->fillTriangle(Xcoordinate(deg,X,r), Ycoordinate(deg,Y,r), Xcoordinate(deg3,X,r/2), Ycoordinate(deg3,Y,r/2), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), colorW);
  gfx->fillTriangle(Xcoordinate(deg+180,X,r), Ycoordinate(deg+180,Y,r), Xcoordinate(deg3,X,r/10), Ycoordinate(deg3,Y,r/10), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), colorW);
  gfx->fillTriangle(Xcoordinate(deg+180,X,r), Ycoordinate(deg+180,Y,r), Xcoordinate(deg2,X,r/10), Ycoordinate(deg2,Y,r/10), Xcoordinate(deg+180,X,0), Ycoordinate(deg+180,Y,0), colorW);
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
    case WL_IDLE_STATUS : Serial.println("WiFi| WL_IDLE_STATUS"); break;
    case WL_NO_SSID_AVAIL : Serial.println("WiFi| WL_NO_SSID_AVAIL"); break;
    case WL_CONNECT_FAILED : Serial.println("WiFi| WL_CONNECT_FAILED"); break;
    case WL_DISCONNECTED : Serial.println("WiFi| WL_DISCONNECTED"); break;
    default : Serial.printf("WiFi| No know WiFi error"); break;
  }
}

//-------------------------------------------------------------------------------------------------------
void Mqtt(){
  #if defined(MQTT)
    if ( millis()-MqttStatusTimer[0]>MqttStatusTimer[1] && ((proto==0 && (mainHWdeviceSelect==0 || mainHWdeviceSelect==1)) || (mainHWdeviceSelect==3 && srcUseMqtt)) ){
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

      // More sources: subscribe each MQTT row's topic, then we are done (no ROT/WX topics, no splash)
      if(mainHWdeviceSelect==3){
        for(int i=0;i<sourceCount;i++){
          if(sources[i].proto!=0 || sources[i].topic.length()==0) continue;
          if(mqttClient.subscribe(sources[i].topic.c_str())){
            Serial.println("mqttReconnect-subscribe "+sources[i].topic);
          }
        }
        return mqttClient.connected();
      }

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

        topic = String(WX_TOPIC) + "HumidityRel-Percent";
        topic.reserve(50);
        const char *cstr5 = topic.c_str();
        if(mqttClient.subscribe(cstr5)==true){
          Serial.print("mqttReconnect-subscribe ");
          Serial.println(String(cstr5));
        }

        topic = String(WX_TOPIC) + "Pressure-hPa";
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

        topic = String(WX_TOPIC) + "DewPoint-Celsius";
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
      // low-power timer wake: skip the "Connecting" splash to keep the retained image (saves a ~3 s refresh)
      if(!(lowPower && timerWake)){
      gfx->fillScreen(colorB);
      gfx->setTextColor(colorW);
      gfx->setFont(&Logisoso10pt7b);
      gfx->setCursor(70, 150);
      gfx->println("Connecting");
      gfx->setFont(&Logisoso8pt7b);
      gfx->setCursor(90, 190);
      gfx->println(String(mainHWdevice[mainHWdeviceSelect][1]));
      gfx->fillCircle(80, 190-7, 3, colorW);
      gfx->setCursor(90, 220);
      gfx->println("WiFi "+String(SSID)+" "+String(WiFi.RSSI())+" dBm");
      gfx->fillCircle(80, 220-7, 3, colorW);
      gfx->setCursor(90, 250);
      gfx->println(WiFi.localIP());
      gfx->fillCircle(80, 250-7, 3, colorW);
      gfx->setCursor(90, 280);
      if(mainHWdeviceSelect==0 || mainHWdeviceSelect==1){
        gfx->println("MQTT "+String(TOPIC)+"#");
      }else{
        gfx->println("MQTT disable");
      }
      gfx->fillCircle(80, 280-7, 3, colorW);
      gfx->setCursor(90, 310);
      gfx->println(String(mainHWdevice[mainHWdeviceSelect][1])+"...");
      gfx->fillCircle(80, 310-7, 3, colorW);
      gfx->setFont(&Logisoso8pt7b);
      gfx->setCursor(15, 385);
      UtcTime(1).toCharArray(buf, 21);
      gfx->println("UTC "+String(buf));
      gfx->setCursor(200, 385);
      gfx->print(REV);
      gfx->display(false);
      }   // end skip-splash guard
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
  free(p);
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

    // More sources: store the raw text payload into every matching row
    if(mainHWdeviceSelect==3){
      String tp = String(topic);
      String pv = ""; for(unsigned int i=0;i<length;i++) pv += (char)p[i];
      Serial.println(tp+" = "+pv);
      for(int i=0;i<sourceCount;i++){
        if(sources[i].proto!=0 || sources[i].topic!=tp) continue;
        sources[i].rawText = pv;                          // keep raw for live re-format in preview
        sources[i].value = applyFormat(sources[i], pv);
        sources[i].haveValue = true;
        srcMark(sources[i].refreshAfter);
      }
      free(p);
      return;
    }

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
      WDTimer();
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
        WDTimer();
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
        WDTimer();
        eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "RainToday-mm";
      if ( CheckTopicBase.equals( String(topic) )){
        RainToday = payloadToFloat(payload, length);
        Serial.println("RainToday-mm "+String(RainToday)+" mm");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "HumidityRel-Percent";
      if ( CheckTopicBase.equals( String(topic) )){
        HumidityRel=payloadToFloat(payload, length);
        Serial.println("HumidityRel-Percent "+String(HumidityRel)+"%");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "DewPoint-Celsius";
      if ( CheckTopicBase.equals( String(topic) )){
        DewPoint=payloadToFloat(payload, length);
        Serial.println("DewPoint-Celsius "+String(DewPoint)+"°");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "Pressure-hPa";
      if ( CheckTopicBase.equals( String(topic) )){
        Pressure=payloadToFloat(payload, length);
        Serial.println("Pressure-hPa "+String(Pressure)+" hpa");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "WindDir-azimuth";
      if ( CheckTopicBase.equals( String(topic) )){
        WindDir=(int)payloadToFloat(payload, length);
        Serial.println("WindDir-azimuth "+String(WindDir)+"°az");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "WindSpeedAvg-mps";
      if ( CheckTopicBase.equals( String(topic) )){
        WindSpeedAvg=payloadToFloat(payload, length);
        Serial.println("WindSpeedAvg-mps "+String(WindSpeedAvg)+" m/s");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

      CheckTopicBase = String(WX_TOPIC) + "WindSpeedMaxPeriod-mps";
      if ( CheckTopicBase.equals( String(topic) )){
        WindSpeedMaxPeriod=payloadToFloat(payload, length);
        Serial.println("WindSpeedMaxPeriod-mps "+String(WindSpeedMaxPeriod)+" m/s");
        RxMqttTimer=millis();
        WDTimer();
        // eInkNeedRefresh=true;
      }

    }

    free(p);
  #endif
} // MqttRx END

//-----------------------------------------------------------------------------------
void MqttPubString(String TOPICEND, String DATA, bool RETAIN){
  #if defined(MQTT)
    // if(EnableEthernet==1 && MQTT_ENABLE==1 && EthLinkStatus==1 && mqttClient.connected()==true){
    if(mqttClient.connected()==true && (mainHWdeviceSelect==0 || mainHWdeviceSelect==1)){
      Serial.print("TXmqtt > ");
      String topic = String(TOPIC)+String(TOPICEND);
      topic.toCharArray( mqttPath, 50 );
      DATA.toCharArray( mqttTX, 50 );
      mqttClient.publish(mqttPath, mqttTX, RETAIN);
      Serial.print(mqttPath);
      Serial.print(" ");
      Serial.println(mqttTX);
    }
  #endif
}

//-----------------------------------------------------------------------------------
// TrxNet receive path -----------------------------------------------------------------
// All telemetry topics carry raw little-endian scaled integers (see TrxNet README).
// We accept only messages whose sender name matches the configured source (e.g. WX.01).

static int16_t  trxRd16(const uint8_t* d){ int16_t v;  memcpy(&v, d, 2); return v; }
static uint16_t trxRdU16(const uint8_t* d){ uint16_t v; memcpy(&v, d, 2); return v; }

// Magnus-formula dew point from current temperature + relative humidity (TrxNet has no /dewpoint).
void recomputeDewPoint(){
  float rh = constrain(HumidityRel, 1.0f, 100.0f);
  const float a = 17.62f, b = 243.12f;
  float g = log(rh/100.0f) + (a*Temperature)/(b+Temperature);
  DewPoint = (b*g)/(a-g);
}

static bool trxFromSource(const char* from){
  return trxSource.length()==0 || strcmp(from, trxSource.c_str())==0;
}
static void trxMark(){          // mirror what MqttRx does on every accepted message
  RxMqttTimer = millis();
  WDTimer();
  eInkNeedRefresh = true;
}

// ---- "More sources" (devsel==3) runtime --------------------------------------------------------------
// Mark a new value: reset the offline watchdog like trxMark, but only force a redraw when the row
// asked for it ("Refresh after receive"); otherwise the global srcRefreshSec timer repaints later.
void srcMark(bool immediate){
  RxMqttTimer = millis();
  WDTimer();
  if(immediate) eInkNeedRefresh = true;
}

// Decode a raw TrxNet payload to display text per the row's format.
String decodeTrx(uint8_t fmt, const uint8_t* d, size_t l, uint8_t decimals){
  if(decimals > 2) decimals = 2;
  switch(fmt){
    case 0: if(l<2) return "?"; return String(trxRd16(d)/100.0f, (int)decimals); // int16 / 100
    case 1: if(l<2) return "?"; return String(trxRd16(d));             // int16
    case 2: if(l<2) return "?"; return String(trxRdU16(d));            // uint16
    case 3: { if(l<4) return "?"; int32_t v; memcpy(&v,d,4); return String(v); }
    case 4: { if(l<4) return "?"; float v;   memcpy(&v,d,4); return String(v,(int)decimals); }
    case 5: { String s; for(size_t i=0;i<l;i++) s += (char)d[i]; return s; } // ASCII text
    case 6: if(l<1) return "?"; return String((unsigned)d[0]);        // uint8 (byte, e.g. CI-V mode)
    case 7: if(l<1) return "?"; return String((int)(int8_t)d[0]);     // int8
    case 8: { if(l<4) return "?"; uint32_t v; memcpy(&v,d,4); return String((unsigned long)v); } // uint32 (e.g. /hz)
  }
  return "?";
}

// Group the integer part of a plain numeric string ("123456.78") in threes with `sep`, and
// swap the decimal "." for `dsep` if given -> e.g. sep="." dsep="," yields "123.456,78".
static String groupThousands(const String& num, const String& sep, const String& dsep){
  int dot = num.indexOf('.');
  String ip = (dot < 0) ? num : num.substring(0, dot);      // integer part
  String fp = (dot < 0) ? ""  : num.substring(dot);         // ".78" (leading dot kept)
  bool neg = ip.startsWith("-");
  if(neg) ip = ip.substring(1);
  if(sep.length() && ip.length() > 3){
    String g; int c = 0;
    for(int i = ip.length()-1; i >= 0; i--){
      g = String((char)ip[i]) + g;
      if(++c % 3 == 0 && i > 0) g = sep + g;
    }
    ip = g;
  }
  if(dsep.length() && fp.length()) fp = dsep + fp.substring(1);   // replace leading "." with dsep
  return (neg ? String("-") : String("")) + ip + fp;
}

// Apply the per-row display formatting to a decoded/received text value.
//  numeric on : extract the leading number (toFloat ignores any trailing unit text),
//               apply the affine transform y = x*mul + add, round to `decimals`, then
//               optionally group thousands / swap the decimal separator.
//  numeric off: pass the text through unchanged.
//  Finally run the literal replace table (from \x1c to, pairs joined by \x1d) via
//  String::replace() - covers bool/enum mapping and sed-s/// (literal). No regex engine
//  on purpose: std::regex would cost tens of kB flash + a lot of RAM on the ESP32.
String applyFormat(const Source& o, const String& raw){
  String out = raw;
  if(o.numeric){
    float x = raw.toFloat();                    // leading number; ignores trailing unit chars
    float y = x * o.mul + o.add;
    out = String(y, (int)(o.decimals > 2 ? 2 : o.decimals));
    if(o.thousands.length() || o.decsep.length()) out = groupThousands(out, o.thousands, o.decsep);
  }
  if(o.replaceMap.length()){
    const char GS = (char)0x1d, FS = (char)0x1c;   // pair separator / from-to separator
    int start = 0;
    while(start <= (int)o.replaceMap.length()){
      int gs = o.replaceMap.indexOf(GS, start);
      String pair = (gs<0) ? o.replaceMap.substring(start) : o.replaceMap.substring(start, gs);
      int fs = pair.indexOf(FS);
      if(fs >= 0){
        String from = pair.substring(0, fs);
        if(from.length()) out.replace(from, pair.substring(fs + 1));
      }
      if(gs < 0) break;
      start = gs + 1;
    }
  }
  return out;
}

// Parse the RS/US-encoded NVS string into sources[]. Fields per row (US-separated):
//   label | proto | topic | format | size | suffix | refreshAfter | decimals | numeric | mul | add
//         | replaceMap | thousands | decsep
// Fields were appended over time (decimals=7, numeric/mul/add/replaceMap=8..11, thousands/decsep=12..13);
// rows from older configs omit trailing fields and fall back to their prior defaults.
// Rows are RS-separated. label may carry embedded '\n' (no collision with RS=0x1e/US=0x1f).
void parseSources(){
  sourceCount = 0;
  srcUseMqtt = false; srcUseTrx = false;
  const char RS = (char)0x1e, US = (char)0x1f;
  int start = 0;
  while(start <= (int)srcCfgRaw.length() && sourceCount < SRC_MAX){
    int rs = srcCfgRaw.indexOf(RS, start);
    String rec = (rs<0) ? srcCfgRaw.substring(start) : srcCfgRaw.substring(start, rs);
    if(rec.length() > 0){
      String f[14]; int fi = 0, p = 0;
      while(fi < 14){
        int us = rec.indexOf(US, p);
        f[fi++] = (us<0) ? rec.substring(p) : rec.substring(p, us);
        if(us<0) break;
        p = us + 1;
      }
      Source& o = sources[sourceCount];
      o.label        = f[0];
      o.proto        = (uint8_t)f[1].toInt();
      o.topic        = f[2];
      o.format       = (uint8_t)f[3].toInt();
      o.size         = (uint8_t)constrain(f[4].toInt(), 0, 3);
      o.suffix       = f[5];
      o.refreshAfter = (f[6] == "1");
      o.decimals     = (uint8_t)(f[7].length() ? constrain(f[7].toInt(), 0, 2) : 2);  // absent (old cfg) -> 2 (prior behavior)
      o.numeric      = (f[8] == "1");
      o.mul          = f[9].length()  ? f[9].toFloat()  : 1.0f;  // absent -> identity
      o.add          = f[10].length() ? f[10].toFloat() : 0.0f;
      o.replaceMap   = f[11];
      o.thousands    = f[12];
      o.decsep       = f[13];
      o.value = ""; o.rawText = ""; o.haveValue = false; o.rawLen = 0;
      if(o.proto == 0) srcUseMqtt = true; else srcUseTrx = true;
      sourceCount++;
    }
    if(rs < 0) break;
    start = rs + 1;
  }
  Serial.println("SRC| parsed "+String(sourceCount)+" source(s) mqtt="+String(srcUseMqtt)+" trx="+String(srcUseTrx));
}

// Store a freshly received TrxNet value into every row that matches this slot's path + sender.
void handleTrxSource(int slot, const char* from, const uint8_t* d, size_t l){
  if(slot < 0 || slot >= srcTrxSubCount) return;
  const String& path = srcTrxSubPath[slot];
  for(int i=0;i<sourceCount;i++){
    Source& o = sources[i];
    if(o.proto != 1) continue;
    int sl = o.topic.indexOf('/');
    if(sl < 0) continue;
    if(o.topic.substring(sl) != path) continue;        // path part "/temp"
    if(o.topic.substring(0, sl) != String(from)) continue;  // device part "WX.01"
    o.rawLen = (uint8_t)(l > sizeof(o.rawBuf) ? sizeof(o.rawBuf) : l);
    memcpy(o.rawBuf, d, o.rawLen);
    o.value = applyFormat(o, decodeTrx(o.format, d, l, o.decimals));
    o.haveValue = true;
    srcMark(o.refreshAfter);
  }
}
// Eight fixed trampolines so each TrxNet path maps to a distinct callback (path index baked in).
static void srcTrx0(const char* f,const uint8_t* d,size_t l){ handleTrxSource(0,f,d,l); }
static void srcTrx1(const char* f,const uint8_t* d,size_t l){ handleTrxSource(1,f,d,l); }
static void srcTrx2(const char* f,const uint8_t* d,size_t l){ handleTrxSource(2,f,d,l); }
static void srcTrx3(const char* f,const uint8_t* d,size_t l){ handleTrxSource(3,f,d,l); }
static void srcTrx4(const char* f,const uint8_t* d,size_t l){ handleTrxSource(4,f,d,l); }
static void srcTrx5(const char* f,const uint8_t* d,size_t l){ handleTrxSource(5,f,d,l); }
static void srcTrx6(const char* f,const uint8_t* d,size_t l){ handleTrxSource(6,f,d,l); }
static void srcTrx7(const char* f,const uint8_t* d,size_t l){ handleTrxSource(7,f,d,l); }
static TrxNetCallback srcTrxCbs[8] = { srcTrx0,srcTrx1,srcTrx2,srcTrx3,srcTrx4,srcTrx5,srcTrx6,srcTrx7 };

// Subscribe each unique TrxNet path (capped at 8 / TRXNET_MAX_SUBS) to its trampoline.
void srcTrxSubscribe(){
  srcTrxSubCount = 0;
  for(int i=0;i<sourceCount && srcTrxSubCount<8;i++){
    if(sources[i].proto != 1) continue;
    int sl = sources[i].topic.indexOf('/');
    if(sl < 0) continue;
    String pth = sources[i].topic.substring(sl);
    bool dup = false;
    for(int k=0;k<srcTrxSubCount;k++) if(srcTrxSubPath[k] == pth){ dup = true; break; }
    if(dup) continue;
    srcTrxSubPath[srcTrxSubCount] = pth;
    trxNet.subscribe(pth.c_str(), srcTrxCbs[srcTrxSubCount]);
    srcTrxSubCount++;
  }
  Serial.println("SRC| TrxNet subscribed "+String(srcTrxSubCount)+" path(s)");
}

// ---- "More sources" drawing --------------------------------------------------------------------------
// Value-size index: 0=10pt 1=20pt 2=40pt 3=30pt. 30pt was appended as index 3 to keep
// older saved configs (which used 0/1/2) rendering at their original sizes.
const GFXfont* srcValueFont(uint8_t size){
  if(size == 3) return &Logisoso30pt7b;
  if(size == 2) return &Logisoso40pt7b;
  if(size == 1) return &Logisoso20pt7b;
  return &Logisoso10pt7b;
}
int srcValueHeight(uint8_t size){   // approx glyph box height for fit/layout (px)
  if(size == 3) return 43;
  if(size == 2) return 56;
  if(size == 1) return 30;
  return 18;
}
int srcCountLines(const String& s){
  int lines = 1;
  for(unsigned i=0;i<s.length();i++) if(s[i]=='\n') lines++;
  return lines;
}
// Left description: smallest font, left-aligned, lines stacked, block vertically centered in the row.
static void drawSourceLabel(const String& label, int x, int yTop, int rh){
  gfx->setFont(&Logisoso8pt7b);
  const int lineH = 17;
  int lines = srcCountLines(label);
  int y0 = yTop + (rh - lines*lineH)/2 + 13;   // +ascent so the first baseline sits below the top
  int start = 0, li = 0;
  while(start <= (int)label.length()){
    int nl = label.indexOf('\n', start);
    String line = (nl<0) ? label.substring(start) : label.substring(start, nl);
    gfx->setCursor(x, y0 + li*lineH);
    gfx->print(line);
    if(nl<0) break;
    start = nl + 1; li++;
  }
}
// True at a UTF-8 degree sign (U+00B0 = 0xC2 0xB0), which the Logisoso fonts don't contain.
static bool isDegreeAt(const String& s, unsigned i){
  return (uint8_t)s[i]==0xC2 && i+1 < s.length() && (uint8_t)s[i+1]==0xB0;
}
// Value (+suffix): chosen font, right-aligned against xRight, vertically centered in the row.
// The font has no "°" glyph, so any degree sign in the suffix is drawn as a geometric ring
// scaled to the font's cap height and placed like a superscript.
static void drawSourceValue(Source& o, int xRight, int yTop, int rh){
  gfx->setFont(srcValueFont(o.size));
  String full = (o.haveValue ? o.value : String("--")) + o.suffix;
  int16_t bx, by; uint16_t bw, bh;

  // Cap height from a digit → ring radius, stroke, and reserved advance per degree sign.
  gfx->getTextBounds("0", 0, 0, &bx, &by, &bw, &bh);
  int capH = bh;
  int degR = (int)(capH * 0.12f + 0.5f); if(degR < 2) degR = 2;   // ~1/4 smaller
  int degStroke = capH/11; if(degStroke < 2) degStroke = 2;       // 2x thicker ring
  if(degStroke > degR-1) degStroke = degR-1;                      // keep a hole
  int degW = 3*degR;                       // reserved horizontal space: gap + ring + gap

  // Split into text segments around the degree marks; the box excludes the (missing) "°".
  const int MAXSEG = 8; String seg[MAXSEG]; int nseg = 0, nDeg = 0;
  { String cur = "";
    for(unsigned i=0;i<full.length();i++){
      if(isDegreeAt(full,i)){ if(nseg<MAXSEG) seg[nseg++]=cur; cur=""; nDeg++; i++; }
      else cur += full[i];
    }
    if(nseg<MAXSEG) seg[nseg++]=cur;
  }

  // Total advance = printable text widths + one reserved slot per degree sign.
  int totalW = nDeg*degW;
  for(int i=0;i<nseg;i++) if(seg[i].length()){
    gfx->getTextBounds(seg[i], 0, 0, &bx, &by, &bw, &bh); totalW += bx + bw;
  }

  // Vertical placement from the whole (degree-free) string, as before.
  gfx->getTextBounds(full, 0, 0, &bx, &by, &bw, &bh);
  int cy = yTop + (rh - (int)bh)/2 - by;   // baseline
  int capTop = cy + by;                    // top of the glyph box (by is negative)

  int x = xRight - totalW;                 // right-aligned start
  for(int i=0;i<nseg;i++){
    if(seg[i].length()){
      gfx->setCursor(x, cy); gfx->print(seg[i]);
      gfx->getTextBounds(seg[i], 0, 0, &bx, &by, &bw, &bh); x += bx + bw;
    }
    if(i < nseg-1){                        // a degree mark follows this segment
      int rcx = x + degR + degR/2, rcy = capTop + degR;
      gfx->fillCircle(rcx, rcy, degR, colorW);
      gfx->fillCircle(rcx, rcy, degR - degStroke, colorB);   // hollow it out → ring
      x += degW;
    }
  }
}
// Bottom status row: IP, last refresh time, and (in low-power) battery voltage / offline marker.
static void drawSourceStatus(int W, int H){
  gfx->drawLine(15, H - 22, W - 15, H - 22, colorW);   // separator above the status row
  gfx->setFont(&Logisoso8pt7b);
  gfx->setTextColor(colorW);
  String s = WiFi.localIP().toString() + " | " + UtcTime(1);   // date + time
  if(lowPower && lpVbat > 2.5f) s += " | " + String(lpVbat, 2) + "V";
  if(eInkOfflineDetect)         s += " | OFF>" + String(OfflineTimeout) + "m";
  gfx->setCursor(15, H - 6);
  gfx->print(s);
}
void drawSourcesScreen(){
  int W = gfx->width(), H = gfx->height();
  gfx->fillScreen(colorB);
  gfx->setTextColor(colorW);

  const int leftM = 15, rightM = 15, topM = 6, statusH = 24;
  int n = sourceCount;
  if(n <= 0){
    gfx->setFont(&Logisoso10pt7b);
    gfx->setCursor(20, H/2);
    gfx->print("No sources configured");
    drawSourceStatus(W, H);
    gfx->display(false);
    return;
  }
  if(n > SRC_MAX) n = SRC_MAX;

  // Row heights from the taller of value font vs. stacked label lines.
  int rowH[SRC_MAX]; int totalRows = 0;
  for(int i=0;i<n;i++){
    int vH = srcValueHeight(sources[i].size);
    int lH = srcCountLines(sources[i].label) * 17;
    rowH[i] = (vH > lH) ? vH : lH;
    totalRows += rowH[i];
  }
  // Spread leftover space as n+1 equal gaps (above each row + below the last), evenly down the panel.
  int avail = H - statusH - topM;
  int gap = (avail - totalRows) / (n + 1);
  if(gap < 0) gap = 0;

  int y = topM + gap;
  for(int i=0;i<n;i++){
    drawSourceLabel(sources[i].label, leftM, y, rowH[i]);
    drawSourceValue(sources[i], W - rightM, y, rowH[i]);
    y += rowH[i];
    if(i < n-1){                                   // 1px separator centered in the gap
      int ly = y + gap/2;
      gfx->drawLine(leftM, ly, W - rightM, ly, colorW);
      y += gap;
    }
  }
  drawSourceStatus(W, H);
  lpDrawBattery();
  gfx->display(false);
}

void onTrxTemp(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  Temperature = trxRd16(d)/100.0f;  recomputeDewPoint();
  Serial.println("RXtrx < /temp "+String(Temperature)+"C");
  trxMark();
}
void onTrxHum(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  HumidityRel = trxRdU16(d)/100.0f;  recomputeDewPoint();
  Serial.println("RXtrx < /hum "+String(HumidityRel)+"%");
  trxMark();
}
void onTrxPress(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  Pressure = trxRdU16(d)/10.0f;
  Serial.println("RXtrx < /press "+String(Pressure)+"hPa");
  trxMark();
}
void onTrxRain(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  RainToday = trxRdU16(d)/100.0f;
  Serial.println("RXtrx < /rain "+String(RainToday)+"mm");
  trxMark();
}
void onTrxWindDir(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  WindDir = trxRdU16(d);
  Serial.println("RXtrx < /winddir "+String(WindDir));
  trxMark();
}
void onTrxWindAvg(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  WindSpeedAvg = trxRdU16(d)/100.0f;
  Serial.println("RXtrx < /windavg "+String(WindSpeedAvg)+"m/s");
  trxMark();
}
void onTrxWindMax(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  WindSpeedMaxPeriod = trxRdU16(d)/100.0f;
  Serial.println("RXtrx < /windmax "+String(WindSpeedMaxPeriod)+"m/s");
  trxMark();
}
void onTrxAzimuth(const char* from, const uint8_t* d, size_t l){
  if(!trxFromSource(from) || l<2) return;
  Azimuth = trxRdU16(d);
  Serial.println("RXtrx < /azimuth "+String(Azimuth));
  RxMqttTimer = millis();
  WDTimer();
  if( (AzimuthTmp!=Azimuth && abs(Azimuth-AzimuthTmp)>3) || eInkOfflineDetect==true ){
    eInkNeedRefresh = true;
    AzimuthTmp = Azimuth;
  }
}

// Join the network and subscribe to the topic set for the selected device type.
// E-ink is a pure receiver: it publishes nothing; the source station sends a state
// snapshot to us on join via its onPeerAdded handler.
void onTrxPeer(const TrxPeer* p){   // discovery diagnostics on Serial
  if(p) Serial.println("TRX | peer + "+String(p->name)+" "+p->ip.toString());
}

// Subscribe to the topic set for one device type, dropping the other type's set first.
// No-op when already subscribed to that type. Used both at boot and on a runtime
// type switch by the auto-select logic.
void trxSubscribeFor(int type){
  if(type == trxCurSelect) return;
  if(type == 3){ srcTrxSubscribe(); trxCurSelect = type; return; }  // More sources: per-row paths
  trxNet.unsubscribe("/azimuth");
  trxNet.unsubscribe("/temp");    trxNet.unsubscribe("/hum");
  trxNet.unsubscribe("/press");   trxNet.unsubscribe("/rain");
  trxNet.unsubscribe("/winddir"); trxNet.unsubscribe("/windavg");
  trxNet.unsubscribe("/windmax");
  if(type==0){
    trxNet.subscribe("/azimuth", onTrxAzimuth);
  }else{
    trxNet.subscribe("/temp",    onTrxTemp);
    trxNet.subscribe("/hum",     onTrxHum);
    trxNet.subscribe("/press",   onTrxPress);
    trxNet.subscribe("/rain",    onTrxRain);
    trxNet.subscribe("/winddir", onTrxWindDir);
    trxNet.subscribe("/windavg", onTrxWindAvg);
    trxNet.subscribe("/windmax", onTrxWindMax);
  }
  trxCurSelect = type;
}

// Parse the configured (or device-type-default) priority prefixes into stable storage and
// register them with the library. Called from trxBegin() before begin(). Space-separated;
// empty input falls back to the mirrored source's type so ROT/WX displays are protected
// out of the box (other types get none — a busy network can't starve a single source they
// don't have). Capped at PRIO_MAX tokens / TRXNET_MAX_DEVICE_NAME chars each.
void trxApplyPriorityPrefixes(){
  String s = prioPrefixRaw; s.trim();
  if(s.length()==0){                          // empty -> default per device type
    if(mainHWdeviceSelect==0)      s = "ROT";
    else if(mainHWdeviceSelect==1) s = "WX";
  }
  uint8_t n = 0;
  int i = 0, len = s.length();
  while(i < len && n < PRIO_MAX){
    while(i < len && s[i]==' ') i++;          // skip leading spaces
    int j = i;
    while(j < len && s[j]!=' ') j++;          // to end of token
    if(j > i){
      String tok = s.substring(i, j);
      strncpy(prioStore[n], tok.c_str(), TRXNET_MAX_DEVICE_NAME-1);
      prioStore[n][TRXNET_MAX_DEVICE_NAME-1] = '\0';
      prioPtr[n] = prioStore[n];
      n++;
    }
    i = j;
  }
#if TRXNET_VERSION >= 0x0104
  trxNet.setPriorityPrefixes(n ? prioPtr : nullptr, n);
#endif
  Serial.println("TRX | priority prefixes: "+String(n)+(n? " ("+s+")":""));
}

void trxBegin(){
  // Own identity: INK.<devid> only in "More sources" mode (devsel==3), where devid IS
  // this display's chosen name. In ROT/WX mode devid selects the *source* to mirror
  // (e.g. WX.01), NOT this display's name -> deriving the name from it would make two
  // displays mirroring the same source collide (same name -> merged into one peer on
  // the WX, one steals the other's updates). So ROT/WX always uses the MAC-derived name.
  // The fallback uses the last 2 MAC bytes (INK.<4 hex>): ~1:65k collision between units.
  if(mainHWdeviceSelect==3 && deviceId.length()>0){
    trxOwnName = "INK." + deviceId;
  }else{
    uint8_t mac[6]; WiFi.macAddress(mac);
    char b[8]; sprintf(b, "%02X%02X", mac[4], mac[5]);
    trxOwnName = "INK." + String(b);
  }
  trxNet.setPort(udpPort);
  trxNet.onPeerAdded(onTrxPeer);
  trxApplyPriorityPrefixes();            // protect mirrored sources from peer-table eviction
  trxNet.begin(trxOwnName.c_str());
  trxStartMs = millis();
  Serial.println("TRX | begin name="+trxOwnName+" port="+String(udpPort)
                 +" cfgSource="+(trxCfgSource.length()? trxCfgSource : String("(auto)")));
  trxSubscribeFor(mainHWdeviceSelect);   // subscribe configured/default type up front
}

// Status line under the data: MQTT shows the subscribed topic with its "#" wildcard;
// TrxNet shows source-client (e.g. "WX.01-INK.D9D0") since "#" is MQTT-only.
String footerId(){
  if(proto==1){
    String src = trxSource.length() ? trxSource : String("(auto)");
    return src + "-" + trxOwnName;
  }
  return String(TOPIC) + "#";
}

// "WiFi connected, waiting for data" splash for TrxNet mode — shown after WiFi is up
// so the stale "Connecting WiFi" screen is replaced until the first telemetry arrives.
void drawTrxWaiting(){
  char buf[24];
  gfx->fillScreen(colorB);
  gfx->setTextColor(colorW);
  gfx->setFont(&Logisoso10pt7b);
  gfx->setCursor(30, 90);  gfx->println("WiFi connected");
  gfx->setFont(&Logisoso8pt7b);
  gfx->setCursor(30, 140); gfx->println("waiting for data...");
  gfx->setCursor(90, 200); gfx->println(String(mainHWdevice[mainHWdeviceSelect][1]));
  gfx->fillCircle(80, 200-7, 3, colorW);
  gfx->setCursor(90, 230); gfx->println("WiFi "+String(SSID)+" "+String(WiFi.RSSI())+" dBm");
  gfx->fillCircle(80, 230-7, 3, colorW);
  gfx->setCursor(90, 260); gfx->println(WiFi.localIP());
  gfx->fillCircle(80, 260-7, 3, colorW);
  gfx->setCursor(90, 290); gfx->println("TrxNet "+trxOwnName);
  gfx->fillCircle(80, 290-7, 3, colorW);
  gfx->setCursor(90, 320); gfx->println("src "+(trxSource.length()? trxSource : String("(auto)")));
  gfx->fillCircle(80, 320-7, 3, colorW);
  gfx->setCursor(15, 385); UtcTime(1).toCharArray(buf, 21); gfx->println("UTC "+String(buf));
  gfx->setCursor(200, 385); gfx->print(REV);
  gfx->display(false);
}

// --- runtime source auto-select helpers -------------------------------------------
static int trxTypeOfPeer(const char* name){      // -> layout type, or -1 if unsupported
  if(strncmp(name, "WX.",  3)==0) return 1;
  if(strncmp(name, "ROT.", 4)==0) return 0;
  return -1;
}
static bool trxPeerPresent(const String& name){  // is an active peer with this exact name?
  if(name.length()==0) return false;
  for(int i=0;i<trxNet.peerCount();i++){
    const TrxPeer* p = trxNet.peer(i);
    if(p && name == p->name) return true;
  }
  return false;
}
// Pick the best supported peer: prefer one matching preferType, else any; tie-break on
// the lexicographically lowest name so the choice is stable across reboots.
static String trxChooseBest(int preferType){
  String best=""; int bestType=-1;
  for(int i=0;i<trxNet.peerCount();i++){
    const TrxPeer* p = trxNet.peer(i);
    if(!p) continue;
    int t = trxTypeOfPeer(p->name);
    if(t<0) continue;
    String nm = String(p->name);
    if(best.length()==0){ best=nm; bestType=t; continue; }
    bool candMatch = (t==preferType), bestMatch = (bestType==preferType);
    if(candMatch && !bestMatch){ best=nm; bestType=t; }
    else if(candMatch==bestMatch && nm < best){ best=nm; bestType=t; }
  }
  return best;
}
// Apply a source selection: re-subscribe/flip layout, update display label (~ marker
// when auto-picked) and trigger a refresh.
static void trxSetSource(const String& name, int type, bool autoPicked){
  trxSubscribeFor(type);
  mainHWdeviceSelect = type;
  trxSource     = name;
  trxAutoPicked = autoPicked;
  String disp   = (autoPicked && name.length()) ? ("~"+name) : name;
  TOPIC = disp;
  if(type==0) Name = disp;          // ROT layout draws Name (and needs it != "")
  // No forced redraw here: globals hold no data for the new source yet, so the layout
  // is repainted by the first accepted message (trxMark) — until then the waiting
  // splash / previous data stays rather than flashing empty/stale values.
  Serial.println("TRX | source -> "+disp+(autoPicked?" (auto)":" (cfg)"));
}

// Run from TrxLoop(): keep trxSource pointing at the configured peer when present,
// otherwise auto-pick a supported peer from the scan (after a short grace for the
// configured one). Pure RAM — nothing here is written to NVS.
void trxMaintainSource(){
  static uint32_t lastChk=0;
  if(millis()-lastChk < 1000) return;
  lastChk = millis();

  // 1) configured source present -> use it / reclaim it from an auto-pick
  if(trxCfgSource.length()>0 && trxPeerPresent(trxCfgSource)){
    if(trxAutoPicked || trxSource!=trxCfgSource || mainHWdeviceSelect!=trxCfgSelect)
      trxSetSource(trxCfgSource, trxCfgSelect, false);
    return;
  }
  // configured but absent: hold for the grace window before the first fallback
  if(trxCfgSource.length()>0 && !trxAutoPicked && (millis()-trxStartMs < 10000UL))
    return;

  // 2) keep a still-valid auto-pick
  if(trxAutoPicked && trxPeerPresent(trxSource)) return;

  // 3) (re)pick the best supported peer; if none, go offline/clear
  String best = trxChooseBest(trxCfgSelect);
  if(best.length()>0){
    if(best!=trxSource || !trxAutoPicked)
      trxSetSource(best, trxTypeOfPeer(best.c_str()), true);
  }else if(trxAutoPicked){
    // our auto-pick vanished and nothing else is available -> back to the waiting splash
    trxSource=""; trxAutoPicked=false; TOPIC="";
    if(mainHWdeviceSelect==0) Name="";
    drawTrxWaiting();
  }
}

void TrxLoop(){
  bool srcTrx = (mainHWdeviceSelect==3 && srcUseTrx);
  if(proto!=1 && !srcTrx) return;
  trxNet.loop();
  if(mainHWdeviceSelect!=3) trxMaintainSource();   // auto-source picking is only for ROT/WX layouts
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
// Low power (battery) mode -----------------------------------------------------------------------------

// Battery voltage in volts. analogReadMilliVolts() already applies the chip's factory eFuse ADC
// calibration; we multisample to denoise. Needs the e-paper supply (GPIO2) HIGH, set in setup().
float lpBatteryVolts(){
  uint32_t mv = 0;
  for(int i=0;i<8;i++) mv += analogReadMilliVolts(LP_BAT_PIN);
  return (mv / 8.0f) * LP_DIVIDER_RATIO / 1000.0f;
}

// Draw a battery indicator (voltage + small gauge) in the top-right corner. Called from eInkRefresh
// just before display() so it rides along on the same panel update.
void lpDrawBattery(){
  if(!lowPower || lpVbat < 2.5f) return;   // skip when no real battery (USB-only reads near 0)
  char b[12];
  dtostrf(lpVbat, 0, 2, b);
  gfx->setFont(&Logisoso8pt7b);
  gfx->setTextColor(colorW);
  // gauge body
  gfx->drawRect(338, 6, 22, 11, colorW);
  gfx->fillRect(360, 9, 2, 5, colorW);
  int fill = (int)((constrain(lpVbat, 3.30f, 4.20f) - 3.30f) / (4.20f - 3.30f) * 18.0f);
  if(fill > 0) gfx->fillRect(340, 8, fill, 7, colorW);
  gfx->setCursor(296, 16);
  gfx->print(String(b) + "V");
  if(lpVbat < LP_BAT_WARN){
    gfx->setCursor(250, 16);
    gfx->print("!");
  }
  float days = lpEstimateDays();
  if(days > 0.0f){
    gfx->setCursor(300, 30);
    gfx->print("~" + String(days < 10 ? days : (float)(int)days, days < 10 ? 1 : 0) + "d");
  }
}

// Final low-battery screen, then park: long sleep until recharged + reset/checked again.
void lpDrawRecharge(float vb){
  char b[12]; dtostrf(vb, 0, 2, b);
  gfx->fillScreen(colorB);
  gfx->setTextColor(colorW);
  gfx->setFont(&Logisoso50pt7b);
  gfx->setCursor(40, 110); gfx->println("LOW");
  gfx->setCursor(40, 175); gfx->println("BATT");
  gfx->setFont(&Logisoso10pt7b);
  gfx->setCursor(40, 230); gfx->println(String(b) + " V");
  gfx->setCursor(40, 270); gfx->println("Recharge the battery");
  gfx->setFont(&Logisoso8pt7b);
  gfx->setCursor(200, 385); gfx->print(REV);
  gfx->display(false);
  gfx->hibernate();
}

// Rough battery-runtime estimate in days. No fuel gauge on this board, so this is a model, not a
// measurement: the awake-window length is measured (EMA in RTC), the active/sleep currents are
// assumed constants. Good for an order-of-magnitude figure and for comparing wake intervals.
float lpEstimateDays(){
  if(!lowPower || batCapacity == 0) return 0.0f;
  float awakeMs    = (rtcAwakeAvgMs > 0) ? (float)rtcAwakeAvgMs : 6000.0f;   // 6 s default until measured
  float intervalMs = (float)lpInterval * 60000.0f;
  if(awakeMs > intervalMs) awakeMs = intervalMs;
  float avg_mA = (awakeMs * LP_I_ACTIVE_MA + (intervalMs - awakeMs) * LP_I_SLEEP_MA) / intervalMs;
  if(avg_mA <= 0.0f) return 0.0f;
  return (float)batCapacity / avg_mA / 24.0f;
}

// Power down the e-paper controller and enter deep sleep for the configured interval. Holds the
// e-paper supply pin across sleep (LaskaKit pattern) so the retained image stays clean. Never returns.
void lpEnterDeepSleep(){
  // record this awake window (time since boot) for the runtime estimate - only representative timer
  // wakes count, not the long cold-boot grace window or the parked state.
  if(timerWake && !rtcParked){
    uint32_t aw = millis();
    rtcAwakeAvgMs = (rtcAwakeAvgMs == 0) ? aw : (rtcAwakeAvgMs * 3 + aw) / 4;
  }
  uint64_t mins = rtcParked ? 360ULL : (uint64_t)lpInterval;  // parked: re-check every 6 h
  if(mins < 1) mins = 1;
  Serial.println("LP | deep sleep " + String((unsigned long)mins) + " min (boot #" + String(rtcBootCount) + ")");
  Serial.flush();
  gfx->hibernate();
  pinMode(LP_POWER_PIN, OUTPUT);
  digitalWrite(LP_POWER_PIN, LOW);             // power the e-paper supply OFF; the bistable image stays
  gpio_hold_en((gpio_num_t)LP_POWER_PIN);      // pin a clean LOW across deep sleep (and through reset)
  gpio_deep_sleep_hold_en();
  WiFi.disconnect(true, false);
  esp_sleep_enable_timer_wakeup(mins * 60ULL * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

// Called once in setup() after the panel + config are up: classify the wake, release the sleep GPIO
// hold, and enforce battery protection before any Wi-Fi is started.
void lpInit(){
  timerWake = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER);
  rtcBootCount++;
  // (the GPIO2 sleep-hold was already released at the top of setup, before the panel init)

  if(!lowPower || APmode) return;        // battery protection only in configured low-power operation

  lpVbat = lpBatteryVolts();
  Serial.println("LP | wake=" + String(timerWake ? "timer" : "cold") + " Vbat=" + String(lpVbat, 2) + " parked=" + String(rtcParked));

  // Below ~2.5 V means no battery / USB-only (divider reads near 0) - don't park on a bad reading.
  bool valid = (lpVbat > 2.5f);
  bool critical = valid && ((lpVbat < LP_BAT_CRIT) || (rtcParked && lpVbat < LP_BAT_RESUME));
  if(critical){
    rtcParked = true;
    lpDrawRecharge(lpVbat);
    lpEnterDeepSleep();                  // never returns
  }
  rtcParked = false;                     // recovered (or never parked) -> normal operation
}

// Run every loop() in STA mode: decide when the awake window is done and go to sleep.
//  - timer wake: sleep as soon as a fresh reading is shown, or when the budget runs out.
//  - cold boot/RESET: stay awake the grace window (web config reachable), then sleep.
void lowPowerManage(){
  if(!lowPower || APmode) return;
  static uint32_t t0 = millis();
  if(timerWake){
    if(lpDataHandled || millis() - t0 > LP_WAKE_BUDGET_MS) lpEnterDeepSleep();
  }else{
    if(millis() - t0 > LP_GRACE_MS) lpEnterDeepSleep();
  }
}

// Hash of the WX values currently in the globals, used to skip a (slow) panel refresh when nothing
// the user can see has changed since the last drawn frame. Battery (0.1 V) and offline state are
// included so those still trigger a redraw.
uint32_t lpWxHash(){
  uint32_t h = 2166136261UL;
  int v[8] = {
    (int)lroundf(Temperature*10), (int)lroundf(HumidityRel), (int)lroundf(DewPoint),
    (int)lroundf(Pressure), WindDir, (int)lroundf(WindSpeedMaxPeriod*10),
    (int)lroundf(RainToday*10), (int)lroundf(lpVbat*10)
  };
  for(int i=0;i<8;i++){ h ^= (uint32_t)v[i]; h *= 16777619UL; }
  h ^= (uint32_t)(eInkOfflineDetect?1:0); h *= 16777619UL;
  return h;
}

// Configuration (NVS via Preferences) ------------------------------------------------------------------

void loadConfig(){
  prefs.begin(NVS_NS, true);   // read-only
  APmode = prefs.getBool("apmode", true);   // fresh NVS (first boot) -> AP mode
  for(int i=0;i<4;i++){
    wifiSSID[i] = prefs.getString(("ssid"+String(i)).c_str(), "");
    wifiPSWD[i] = prefs.getString(("pass"+String(i)).c_str(), "");
  }
  mainHWdeviceSelect = prefs.getInt("devsel", 1);
  topicBase          = prefs.getString("topic", "");
  mqttBroker[0]      = prefs.getUChar("mqip0", 0);
  mqttBroker[1]      = prefs.getUChar("mqip1", 0);
  mqttBroker[2]      = prefs.getUChar("mqip2", 0);
  mqttBroker[3]      = prefs.getUChar("mqip3", 0);
  MQTT_PORT          = prefs.getUShort("mqport", 1883);
  eInkRotation       = prefs.getUChar("rot", 1);
  eInkNegativ        = prefs.getBool("neg", true);
  OfflineTimeout     = prefs.getUChar("offto", 6);
  usUnits            = prefs.getBool("units", false);
  int  tzh           = prefs.getInt("tzh", 0);
  bool dst           = prefs.getBool("dst", false);
  ntpServer          = prefs.getString("ntp", "pool.ntp.org");
  dispType           = prefs.getUChar("disp", 0);
  proto              = prefs.getUChar("proto", 0);
  udpPort            = prefs.getUShort("udpport", 5683);
  deviceId           = prefs.getString("devid", "");
  lowPower           = prefs.getBool("lowpwr", false);
  lpInterval         = prefs.getUShort("lpint", 15);
  batCapacity        = prefs.getUShort("batcap", 0);
  srcCfgRaw          = prefs.getString("srcCfg", "");
  srcRefreshSec      = prefs.getUShort("srcRefr", 60);
  prioPrefixRaw      = prefs.getString("prioPfx", "");
  prefs.end();

  parseSources();    // populate sources[] + srcUseMqtt/srcUseTrx from srcCfgRaw

  gmtOffset_sec      = (long)tzh * 3600L;
  daylightOffset_sec = dst ? 3600 : 0;
  Serial.println("CFG| loaded (apmode="+String(APmode)+" devsel="+String(mainHWdeviceSelect)+" disp="+String(dispType)+")");
}

void applyDerivedConfig(){
  mqtt_server_ip = IPAddress(mqttBroker[0], mqttBroker[1], mqttBroker[2], mqttBroker[3]);
  if(eInkNegativ){ colorB = GxEPD_BLACK; colorW = GxEPD_WHITE; eInkNegativTmp = true; }
  else           { colorB = GxEPD_WHITE; colorW = GxEPD_BLACK; eInkNegativTmp = false; }
  // single topic base from the UI; firmware appends /ROT/ or /WX/ per device type
  ROT_TOPIC = topicBase + String(mainHWdevice[0][0]);
  WX_TOPIC  = topicBase + String(mainHWdevice[1][0]);
  if(mainHWdeviceSelect==0)      TOPIC = ROT_TOPIC;
  else if(mainHWdeviceSelect==1) TOPIC = WX_TOPIC;

  // TrxNet: build the configured source/identity. trxOwnName (which may need the MAC
  // fallback) is finalised in trxBegin() once WiFi is up. With no devid there is no
  // configured source -> trxCfgSource stays empty and the runtime auto-select takes over.
  if(proto==1){
    String pfx   = (mainHWdeviceSelect==0) ? "ROT." : "WX.";
    trxCfgSource = (deviceId.length()>0) ? (pfx + deviceId) : String("");
    trxCfgSelect = mainHWdeviceSelect;
    trxSource    = trxCfgSource;     // start on the configured source (may be empty)
    trxAutoPicked= false;
    TOPIC = trxSource;               // display shows device ID in place of the MQTT topic
    if(mainHWdeviceSelect==0) Name = trxSource;  // ROT refresh gate needs Name != ""
  }
}

// Instantiate the e-paper driver for the panel selected in NVS ("disp"). Different panels are
// different GxEPD2 template types, so they share the GxEPD2_GFX base via a heap pointer.
void initDisplay(){
  switch(dispType){
    case 1:
      gfx = new GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>(GxEPD2_420_GDEY042T81(/*CS=*/SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));
      break;
    case 2:
      gfx = new GxEPD2_3C<GxEPD2_420c_Z21, GxEPD2_420c_Z21::HEIGHT/2>(GxEPD2_420c_Z21(/*CS=*/SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));
      break;
    default:
      gfx = new GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT>(GxEPD2_420(/*CS=*/SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));
      break;
  }
}

//-------------------------------------------------------------------------------------------------------
// WiFi -------------------------------------------------------------------------------------------------

// Scan, then connect to the strongest reachable stored network; on failure try the next strongest.
// Returns false if no stored network could be joined (caller reboots into AP mode).
bool connectWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  // Low-power timer wake: skip the connecting splash (keeps the retained image; a full refresh costs
  // ~3 s) and try a direct connect to the AP cached in RTC memory, skipping the scan entirely.
  bool lpWake = (lowPower && timerWake);
  if(!lpWake){
    gfx->fillScreen(colorB);
    gfx->setTextColor(colorW);
    gfx->setFont(&Logisoso10pt7b);
    gfx->setCursor(30, 120);
    gfx->println("Connecting WiFi");
    gfx->setFont(&Logisoso8pt7b);
    gfx->setCursor(200, 385);
    gfx->print(REV);
    gfx->display(false);
  }else if(rtcBssidValid && rtcSlot>=0 && rtcSlot<4 && wifiSSID[rtcSlot].length()>0){
    SSID = wifiSSID[rtcSlot];
    PSWD = wifiPSWD[rtcSlot];
    Serial.println("WIFI| fast reconnect to '"+SSID+"' ch"+String(rtcChannel)+" (cached BSSID)");
    WiFi.begin(SSID.c_str(), PSWD.c_str(), rtcChannel, rtcBssid);
    int tryc = 0;
    while(WiFi.status()!=WL_CONNECTED && tryc<wifi_max_try){ delay(300); Serial.print("."); tryc++; }
    Serial.println();
    if(WiFi.status()==WL_CONNECTED){
      Serial.println("WIFI| connected (fast), IP "+WiFi.localIP().toString());
      return true;
    }
    Serial.println("WIFI| fast reconnect failed - full scan");
    WiFi.disconnect(true); delay(200);
  }

  Serial.println("WIFI| scanning...");
  int n = WiFi.scanNetworks();

  struct Cand { int slot; int rssi; };
  Cand cands[4]; int nc = 0;
  for(int s=0;s<4;s++){
    if(wifiSSID[s].length()==0) continue;
    int bestRssi = -999; bool seen = false;
    for(int i=0;i<n;i++){
      if(WiFi.SSID(i) == wifiSSID[s]){ seen = true; if(WiFi.RSSI(i) > bestRssi) bestRssi = WiFi.RSSI(i); }
    }
    if(seen){ cands[nc].slot = s; cands[nc].rssi = bestRssi; nc++; }
  }
  for(int a=0;a<nc;a++) for(int b=a+1;b<nc;b++) if(cands[b].rssi > cands[a].rssi){ Cand t=cands[a]; cands[a]=cands[b]; cands[b]=t; }
  WiFi.scanDelete();

  if(nc==0){ Serial.println("WIFI| no known network visible"); return false; }

  for(int c=0;c<nc;c++){
    int s = cands[c].slot;
    SSID = wifiSSID[s];
    PSWD = wifiPSWD[s];
    Serial.print("WIFI| connecting to '"+SSID+"' ("+String(cands[c].rssi)+" dBm) ");
    WiFi.begin(SSID.c_str(), PSWD.c_str());
    int tryc = 0;
    while(WiFi.status()!=WL_CONNECTED && tryc<wifi_max_try){ delay(500); Serial.print("."); tryc++; }
    Serial.println();
    if(WiFi.status()==WL_CONNECTED){
      Serial.println("WIFI| connected, IP "+WiFi.localIP().toString()+"  "+String(WiFi.RSSI())+" dBm");
      // cache this AP so a low-power timer wake can reconnect without scanning
      if(WiFi.BSSID()){ memcpy(rtcBssid, WiFi.BSSID(), 6); rtcChannel = WiFi.channel(); rtcSlot = s; rtcBssidValid = true; }
      return true;
    }
    Serial.println("WIFI| failed, trying next known network");
    WiFi.disconnect(true); delay(200);
  }
  return false;
}

void startAPmode(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();
  MACString = WiFi.softAPmacAddress();
  Serial.println(" AP | SSID '"+String(ssidAP)+"'  IP "+IP.toString());

  dnsServer.start(DNS_PORT, "*", IP);   // captive portal: resolve every host to us
  if(MDNS.begin("esp32eink")) MDNS.addService("http", "tcp", 80);

  gfx->fillScreen(colorB);
  gfx->setTextColor(colorW);
  gfx->setFont(&Logisoso50pt7b);
  gfx->setCursor(30, 88);  gfx->println("AP");
  gfx->setFont(&Logisoso10pt7b);
  gfx->setCursor(130, 38); gfx->println("e-ink");
  gfx->setCursor(130, 63); gfx->println("WiFi");
  gfx->setCursor(130, 88); gfx->println("setup");
  gfx->setCursor(30, 140); gfx->println("Connect phone/PC to WiFi");
  gfx->setCursor(30, 170); gfx->println("'"+String(ssidAP)+"'");
  gfx->setCursor(30, 200); gfx->println("password '"+String(passwordAP)+"'");
  gfx->setCursor(30, 245); gfx->println("then open in a browser:");
  gfx->setCursor(30, 280); gfx->println("http://esp32eink.local");
  gfx->setCursor(30, 310); gfx->println("or http://"+IP.toString());
  gfx->setFont(&Logisoso8pt7b);
  gfx->setCursor(200, 385); gfx->print(REV);
  gfx->display(false);
}

//-------------------------------------------------------------------------------------------------------
// Web UI (served from SPIFFS, gzip-aware) --------------------------------------------------------------

String jsonEsc(const String& s){
  String o; o.reserve(s.length()+8);
  for(unsigned int i=0;i<s.length();i++){
    char c = s[i];
    if(c=='"'||c=='\\'){ o+='\\'; o+=c; }
    else if(c=='\n') o+="\\n";
    else if(c=='\r') {}
    else if(c=='\t') o+="\\t";
    else if((unsigned char)c < 0x20){ char b[8]; sprintf(b,"\\u%04x",(unsigned char)c); o += b; }
    else o+=c;
  }
  return o;
}

String webContentType(const String& path){
  String p = path;
  if(p.endsWith(".gz")) p = p.substring(0, p.length()-3);
  if(p.endsWith(".html")||p.endsWith(".htm")) return "text/html";
  if(p.endsWith(".css"))  return "text/css";
  if(p.endsWith(".js"))   return "application/javascript";
  if(p.endsWith(".json")) return "application/json";
  if(p.endsWith(".svg"))  return "image/svg+xml";
  if(p.endsWith(".png"))  return "image/png";
  if(p.endsWith(".ico"))  return "image/x-icon";
  return "text/plain";
}

// Stream a SPIFFS file, preferring a .gz sibling (the core auto-sets Content-Encoding: gzip).
bool streamSpiffsFile(const String& path){
  if(!FsMounted) return false;
  String gz = path + ".gz";
  if(SPIFFS.exists(gz)){
    File f = SPIFFS.open(gz, "r");
    if(!f) return false;
    ajaxserver.streamFile(f, webContentType(path));
    f.close();
    return true;
  }
  if(SPIFFS.exists(path)){
    File f = SPIFFS.open(path, "r");
    if(!f) return false;
    ajaxserver.streamFile(f, webContentType(path));
    f.close();
    return true;
  }
  return false;
}

static const char SETUP_FALLBACK_HTML[] PROGMEM =
  "<!doctype html><meta charset=utf-8><title>e-ink</title>"
  "<body style='font-family:sans-serif;background:#333;color:#ddd;text-align:center;padding:2em'>"
  "<h2>e-ink setup</h2><p>Web UI not in flash yet.<br>Flash <code>build/spiffs.bin</code> to the SPIFFS"
  " partition over USB.</p></body>";

void handleRoot(){
  if(streamSpiffsFile("/setup.html")) return;
  ajaxserver.send_P(200, "text/html", SETUP_FALLBACK_HTML);
}

void handleStatic(){
  String path = ajaxserver.uri();
  if(path.endsWith("/")) path += "setup.html";
  if(streamSpiffsFile(path)) return;
  int slash = path.lastIndexOf('/');
  if(path.indexOf('.', slash) < 0 && streamSpiffsFile(path + ".html")) return;
  if(APmode){   // captive portal: send every unknown request to the setup page
    ajaxserver.sendHeader("Location", String("http://")+WiFi.softAPIP().toString()+"/setup", true);
    ajaxserver.send(302, "text/plain", "");
    return;
  }
  ajaxserver.send(404, "text/plain", "404: not found");
}

// GET /api/config - current settings + diagnostics, hand-built JSON.
void handleApiConfig(){
  String j; j.reserve(900);
  j  = "{";
  for(int i=0;i<4;i++){
    j += "\"ssid"+String(i)+"\":\"" + jsonEsc(wifiSSID[i]) + "\",";
    j += "\"pass"+String(i)+"\":\"" + jsonEsc(wifiPSWD[i]) + "\",";
  }
  j += "\"devsel\":" + String(mainHWdeviceSelect) + ",";
  j += "\"topic\":\"" + jsonEsc(topicBase) + "\",";
  j += "\"mqttIp\":\"" + String(mqttBroker[0])+"."+String(mqttBroker[1])+"."+String(mqttBroker[2])+"."+String(mqttBroker[3]) + "\",";
  j += "\"mqttPort\":" + String(MQTT_PORT) + ",";
  j += "\"rotation\":" + String(eInkRotation) + ",";
  j += "\"negativ\":" + String(eInkNegativ ? "true":"false") + ",";
  j += "\"units\":"   + String(usUnits ? "true":"false") + ",";
  j += "\"offline\":" + String(OfflineTimeout) + ",";
  j += "\"tzHours\":" + String(gmtOffset_sec/3600) + ",";
  j += "\"dst\":"     + String(daylightOffset_sec!=0 ? "true":"false") + ",";
  j += "\"ntp\":\""   + jsonEsc(ntpServer) + "\",";
  j += "\"dispType\":"+ String(dispType) + ",";
  j += "\"proto\":"   + String(proto) + ",";
  j += "\"udpPort\":" + String(udpPort) + ",";
  j += "\"devid\":\"" + jsonEsc(deviceId) + "\",";
  j += "\"lowpwr\":"  + String(lowPower ? "true":"false") + ",";
  j += "\"lpint\":"   + String(lpInterval) + ",";
  j += "\"batcap\":"  + String(batCapacity) + ",";
  j += "\"srcRefr\":" + String(srcRefreshSec) + ",";
  j += "\"srcCfg\":\""+ jsonEsc(srcCfgRaw) + "\",";
  j += "\"prioPfx\":\""+ jsonEsc(prioPrefixRaw) + "\",";
  j += "\"vbat\":"    + String(lowPower ? lpBatteryVolts() : 0.0f, 2) + ",";
  j += "\"days\":"    + String(lpEstimateDays(), 1) + ",";
  // diagnostics
  j += "\"rev\":\"" + String(REV) + "\",";
  j += "\"mac\":\"" + jsonEsc(MACString) + "\",";
  j += "\"apmode\":" + String(APmode ? "true":"false") + ",";
  j += "\"ip\":\"" + (APmode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  j += "\"rssi\":" + String(APmode ? 0 : WiFi.RSSI()) + ",";
  j += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  j += "\"fsTotal\":" + String(FsMounted ? SPIFFS.totalBytes() : 0) + ",";
  j += "\"fsUsed\":"  + String(FsMounted ? SPIFFS.usedBytes()  : 0) + ",";
  j += "\"trxPeers\":" + String((proto==1 && APmode==false) ? trxNet.peerCount() : 0) + ",";
  j += "\"trxName\":\"" + jsonEsc(trxOwnName) + "\"";   // resolved TrxNet identity (INK.<devid|MAC>) - two units must differ
  j += "}";
  ajaxserver.sendHeader("Cache-Control", "no-store");
  ajaxserver.send(200, "application/json", j);
}

// POST /api/config - persist every provided field to NVS, then reboot to apply.
void handleApiConfigSave(){
  prefs.begin(NVS_NS, false);
  for(int i=0;i<4;i++){
    String ks = "ssid"+String(i), kp = "pass"+String(i);
    if(ajaxserver.hasArg(ks)) prefs.putString(ks.c_str(), ajaxserver.arg(ks));
    if(ajaxserver.hasArg(kp)) prefs.putString(kp.c_str(), ajaxserver.arg(kp));
  }
  if(ajaxserver.hasArg("devsel"))   prefs.putInt("devsel", ajaxserver.arg("devsel").toInt());
  if(ajaxserver.hasArg("topic"))    prefs.putString("topic", ajaxserver.arg("topic"));
  if(ajaxserver.hasArg("mqttIp")){
    String s = ajaxserver.arg("mqttIp"); int p = 0;
    const char* keys[4] = {"mqip0","mqip1","mqip2","mqip3"};
    for(int k=0;k<4;k++){
      int dot = s.indexOf('.', p);
      String part = (dot<0) ? s.substring(p) : s.substring(p, dot);
      prefs.putUChar(keys[k], (uint8_t)constrain(part.toInt(), 0, 255));
      if(dot<0) break;
      p = dot+1;
    }
  }
  if(ajaxserver.hasArg("mqttPort")) prefs.putUShort("mqport", (uint16_t)ajaxserver.arg("mqttPort").toInt());
  if(ajaxserver.hasArg("rotation")) prefs.putUChar("rot", (uint8_t)ajaxserver.arg("rotation").toInt());
  if(ajaxserver.hasArg("negativ"))  prefs.putBool("neg", ajaxserver.arg("negativ")=="1");
  if(ajaxserver.hasArg("units"))    prefs.putBool("units", ajaxserver.arg("units")=="1");
  if(ajaxserver.hasArg("offline"))  prefs.putUChar("offto", (uint8_t)ajaxserver.arg("offline").toInt());
  if(ajaxserver.hasArg("tzHours"))  prefs.putInt("tzh", ajaxserver.arg("tzHours").toInt());
  if(ajaxserver.hasArg("dst"))      prefs.putBool("dst", ajaxserver.arg("dst")=="1");
  if(ajaxserver.hasArg("ntp"))      prefs.putString("ntp", ajaxserver.arg("ntp"));
  if(ajaxserver.hasArg("dispType")) prefs.putUChar("disp", (uint8_t)ajaxserver.arg("dispType").toInt());
  if(ajaxserver.hasArg("proto"))    prefs.putUChar("proto", (uint8_t)ajaxserver.arg("proto").toInt());
  if(ajaxserver.hasArg("udpPort"))  prefs.putUShort("udpport", (uint16_t)ajaxserver.arg("udpPort").toInt());
  if(ajaxserver.hasArg("devid"))    prefs.putString("devid", ajaxserver.arg("devid"));
  if(ajaxserver.hasArg("lowpwr"))   prefs.putBool("lowpwr", ajaxserver.arg("lowpwr")=="1");
  if(ajaxserver.hasArg("lpint"))    prefs.putUShort("lpint", (uint16_t)constrain(ajaxserver.arg("lpint").toInt(), 1, 1440));
  if(ajaxserver.hasArg("batcap"))   prefs.putUShort("batcap", (uint16_t)constrain(ajaxserver.arg("batcap").toInt(), 0, 65535));
  if(ajaxserver.hasArg("srcCfg"))   prefs.putString("srcCfg", ajaxserver.arg("srcCfg"));
  if(ajaxserver.hasArg("srcRefr"))  prefs.putUShort("srcRefr", (uint16_t)constrain(ajaxserver.arg("srcRefr").toInt(), 0, 65535));
  if(ajaxserver.hasArg("prioPfx"))  prefs.putString("prioPfx", ajaxserver.arg("prioPfx"));
  prefs.putBool("apmode", false);   // configured -> boot into client mode
  prefs.end();

  ajaxserver.send(200, "text/plain", "saved");
  delay(1500);
  ESP.restart();
}

// GET /api/peers - JSON array of TrxNet peer names currently visible on the network.
// Only populated when booted in TrxNet + STA mode; empty otherwise (e.g. AP setup).
void handleApiPeers(){
  String j = "[";
  if(proto==1 && APmode==false){
    int n = trxNet.peerCount();
    for(int i=0;i<n;i++){
      const TrxPeer* p = trxNet.peer(i);
      if(!p) continue;
      if(j.length()>1) j += ",";
      j += "\"" + jsonEsc(String(p->name)) + "\"";
    }
  }
  j += "]";
  ajaxserver.sendHeader("Cache-Control", "no-store");
  ajaxserver.send(200, "application/json", j);
}

// GET /api/srcpreview?proto=&topic=&format=&decimals=&numeric=&mul=&add=&thousands=&decsep=&replaceMap= - formatted last
// value of a configured "More sources" row. Matches a running source by proto+topic, takes its raw input
// (TrxNet bytes re-decoded per format, or the raw MQTT payload), then re-runs applyFormat() with the
// *edited* params from the query so the setup page reflects changes before they are saved. Returns "—"
// when nothing arrived.
void handleApiSrcPreview(){
  String topic = ajaxserver.arg("topic");
  int prot = ajaxserver.arg("proto").toInt();
  int fmt  = ajaxserver.arg("format").toInt();
  uint8_t dec = ajaxserver.hasArg("decimals") ? (uint8_t)constrain(ajaxserver.arg("decimals").toInt(), 0, 2) : 2;
  Source fmtCfg;                                    // formatting params straight from the (unsaved) form
  fmtCfg.decimals   = dec;
  fmtCfg.numeric    = ajaxserver.arg("numeric") == "1";
  fmtCfg.mul        = ajaxserver.hasArg("mul") ? ajaxserver.arg("mul").toFloat() : 1.0f;
  fmtCfg.add        = ajaxserver.hasArg("add") ? ajaxserver.arg("add").toFloat() : 0.0f;
  fmtCfg.thousands  = ajaxserver.arg("thousands");
  fmtCfg.decsep     = ajaxserver.arg("decsep");
  fmtCfg.replaceMap = ajaxserver.arg("replaceMap");
  String out = "—";
  for(int i=0;i<sourceCount;i++){
    Source& o = sources[i];
    if((int)o.proto != prot || o.topic != topic) continue;
    if(!o.haveValue) break;
    String raw = (prot==1) ? decodeTrx((uint8_t)fmt, o.rawBuf, o.rawLen, dec) : o.rawText;
    out = applyFormat(fmtCfg, raw);
    break;
  }
  ajaxserver.sendHeader("Cache-Control", "no-store");
  ajaxserver.send(200, "application/json", "{\"value\":\"" + jsonEsc(out) + "\"}");
}

// GET /api/wallcfg - WebSocket broker URI + root topic for the MQTT wall page (data/wall.html).
// The wall subscribes to "#" so you can watch every live topic and copy a path into a "More sources"
// row. WS port is fixed (WALL_WS_PORT); the broker must expose a WebSocket listener there.
void handleApiWallCfg(){
  String uri = "ws://" + String(mqttBroker[0]) + "." + String(mqttBroker[1]) + "."
             + String(mqttBroker[2]) + "." + String(mqttBroker[3]) + ":" + String(WALL_WS_PORT) + "/";
  String j = "{\"uri\":\"" + uri + "\",\"topic\":\"#\"}";
  ajaxserver.sendHeader("Cache-Control", "no-store");
  ajaxserver.send(200, "application/json", j);
}

void handleFactoryReset(){
  prefs.begin(NVS_NS, false);
  prefs.clear();
  prefs.end();
  ajaxserver.send(200, "text/plain", "erased - rebooting to AP mode");
  delay(1500);
  ESP.restart();
}

