# esp32 e-ink (firmware and 3D print box)
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

## Upload binary via web interface
1.  The board **must contain one of the above firmware versions** (see compilation and upload)
2.  **Open url 'http://[YOUR IP]/update'**
3.  **Download** last release **.bin** file from [GitHub](https://github.com/ok1hra/esp32-e-ink/releases)
4.  **Upload .bin file** via web form, with the **Firmware** option selected  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-61.png" width="350">

## Picture

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot6.png" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/light-mode.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot7.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot10.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot8.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot12.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot11.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot13.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot4.jpg" height="200"><img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot5.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/esp32-e-ink/main/img/rot9.jpg" height="200">
