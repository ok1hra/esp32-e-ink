# esp32 e-ink (firmware and 3D print box)

## Quick start
1. **Flash the firmware over USB** — open the [firmware installer](https://ok1hra.github.io/esp32-e-ink/) in Chrome or Edge, connect the board with a USB-C cable and click *Install*.
2. **Connect to its Wi-Fi** — after flashing the board starts an access point `esp32-e-ink-AP` (password `remoteqth`). Join it and the setup page opens by itself (captive portal). If it doesn't, open `http://192.168.4.1` (or `http://esp32eink.local`).
3. **Set your Wi-Fi and save** — the only required field is your **Wi-Fi SSID + password** (optionally a Device ID). Press **Save**; the board reboots and joins your network.

---

- Firmware for hardware from [LaskaKit - ESPink-42 ESP32 e-Paper](https://www.laskakit.cz/laskakit-espink-42-esp32-e-paper-pcb-antenna/) | [GitHub](https://github.com/LaskaKit/ESPink-42)
- Used as Wifi e-ink display for the for the following use:
  - **IP rotator** electronic show azimuth also after rotator turned off [Wiki page](https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v) | [copy on GitHub](https://github.com/ok1hra/IP-rotator/blob/main/Assembly-manual.md) | Main repository [Parameterizable 3D print Antenna rotator in OpenScad](https://github.com/ok1hra/Parameterizable-3D-print-Antenna-rotator-in-OpenScad)
  - **3D print WX station** [GitHub](https://github.com/ok1hra/3D-print-WX-station)
  - **Bash script trasfering WX data from aprs.fi** to MQTT [GitHub](https://github.com/ok1hra/esp32-e-ink/aprsfi2mqtt)
  - **Direct read WX data from aprs.fi** see [setup.cfg](https://github.com/ok1hra/esp32-e-ink/blob/main/setup.cfg)
    
  <img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/wx-station-display.png" height="220">
- Configured via setup.cfg file on microSD card
- Powered from USB-C connector
- Customized 3D printed box in OpenScad, without screw. If possible, the supports can be folded out or hung on a peg.

# Hardware reference (LaskaKit ESPink-42)

This firmware targets the **v2.x board (ESP32-WROOM-32E)**. Pin mapping and the battery
voltage divider differ on the newer v3.x (ESP32-S3) board — the values below are for v2.x.

| Function | GPIO (v2.x) | Note |
| --- | --- | --- |
| Display CS / DC / RST / BUSY | 5 / 17 / 16 / 4 | GDEW042T2 (UC8176), 400×300 |
| Display SPI MOSI / SCK | 23 / 18 | |
| **POWER** (e-paper supply transistor) | **2** | drive HIGH to power the panel; hold during deep sleep |
| I²C SDA / SCL | 21 / 22 | |
| **BAT** (battery sense) | **34** | ADC1_CH6, safe to read with Wi-Fi on |

### Battery voltage measurement

The battery is read through an integrated resistor divider (1 MΩ + 1.3 MΩ) on **GPIO34**:

```c
#define BAT 34
#define DIVIDER_RATIO 1.7693877551          // 1 MOhm + 1.3 MOhm divider
float vbat = analogReadMilliVolts(BAT) * DIVIDER_RATIO / 1000.0;  // volts
```

`analogReadMilliVolts()` already applies the chip's factory eFuse ADC calibration, so no
manual `esp_adc_cal` setup is needed. Reference: LaskaKit
[`SW/Simple/ADC_test`](https://github.com/LaskaKit/ESPink-42/tree/main/SW/Simple/ADC_test)
(use the `ESPink42_V2` pin set). On v2.3+ boards the divider was replaced by a MAX17048
fuel-gauge IC — not applicable to v2.2.

# Low power (battery) mode

Optional setup-menu toggle to extend battery life. When enabled the board spends most of
its time in **deep sleep (~10 µA)** and only wakes on a fixed interval to refresh the panel
— the e-ink image is retained with no power between wakes.

- **Wake cycle:** timer wake → fast Wi-Fi reconnect (cached BSSID + channel in RTC memory,
  no scan) → pull the latest reading → refresh only if the value changed → sleep again.
  No data is "missed": MQTT delivers the retained message on reconnect, and TrxNet replies
  to the board's join probe with a fresh snapshot.
- **Config access:** a power-on / RESET (cold boot) keeps the web UI alive for ~60–120 s
  (IP shown on the display) before the sleep cycle starts, so settings stay reachable.
- **Battery protection:** the cell voltage (GPIO34, above) is checked each wake. A low
  reading shows a "recharge" marker; a critical reading draws a final "recharge" screen and
  parks the board in long sleep until it is recharged and reset.
- **Settings:** *Low power mode* checkbox + *Wake interval* (minutes, default 15) in the
  web setup. Best suited to the WX station (weather changes slowly); on the rotator the
  azimuth would only update once per interval.

# Compile and upload
1.  **Install [Arduino IDE](https://www.arduino.cc/en/software)** rev 1.8.19
1.  **Install support [for ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)**
1.  **Install** these **libraries** in the versions listed
	* GxEPD2 rev 1.5.2
	* Adafruit_GFX_Library rev 1.11.3
	* Adafruit_BusIO rev 1.14.1
	* Wire rev 2.0.0
	* SPI rev 2.0.0
	* WiFi rev 2.0.0
	* AsyncTCP rev 1.1.1
	* ESPAsyncWebServer rev 1.2.3
	* FS rev 2.0.0
	* AsyncElegantOTA rev 2.2.7
	* Update rev 2.0.0
	* PubSubClient rev 2.8
1. **Select board** 'ESP32 Dev Module'
1. **Connect** the rotator with a **USB-C** cable and select the corresponding port in the arduino IDE
1. Now you can **compile and upload** code using USB

## Release a new firmware version
The web installer is regenerated by [`tools/gh-pages.sh`](tools/gh-pages.sh):
1. Increase `REV` in [esp32-e-ink.ino](esp32-e-ink.ino).
2. Arduino IDE *Sketch/Export compiled Binary* (board *ESP32 Dev Module*, Partition Scheme *No OTA (2MB APP/2MB SPIFFS)*) → produces `esp32-e-ink.ino.esp32.bin`.
3. Build only: `./tools/gh-pages.sh` — or build **and** publish: `./tools/gh-pages.sh --publish` (wipes the `gh-pages` branch, so only the latest firmware ever stays online).
4. Commit with the Release number and push.

To rebuild only the web UI (no new firmware), edit `data/*` then run `./tools/build_spiffs_image.sh` and flash `build/spiffs.bin` at `0x210000`.

## Picture

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot6.png" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/light-mode.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot7.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot10.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot8.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot12.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot11.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot13.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot4.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot5.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot9.jpg" height="200">
