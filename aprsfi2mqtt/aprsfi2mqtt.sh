#!/bin/bash
#
#  Script for transfer WX data from aprs.fi to MQTT for e-ink display https://github.com/ok1hra/esp32-e-ink
#
#  Four run steps
#	1. fill in the values on line 16 to 20 in this script
#  2. run './aprsfi2mqtt.sh setup' for generate setup.cfg
#  3. copy file setup.cfg to micro Sd card and plug to e-ink
#  4. run 'chmod 777 aprsfi2mqtt.sh'
#  5. add line to your /etc/crontab file, for run every 15 min
#     '15 *   * * *	{user} 	/{PATH}/aprsfi2mqtt.sh 2>&1 >/tmp/aprsfi2mqtt.log'
#     replace the contents of the brackets {} with specific values
#
#############################################################################

APRS_FI_CALLSIGN="OK1HRA-8"           # callsign of the weather station that we want to transfer to e-ink
APRS_FI_APIKEY="API-KEY"              # API key get from https://aprs.fi/account/
MQTT_BROKER_IP="54.38.157.134"        # 54.38.157.134 default remoteqth.com
E_INK_WIFI_SSID="SSID"                # for generate setup.cfg only
E_INK_WIFI_PASSWORD="PASSWORD"        # for generate setup.cfg only

#############################################################################

if [ $1 == "setup" ]; then
	echo "[mainHWdeviceSelect=1]" > setup.cfg
	echo "[SSID=$E_INK_WIFI_SSID]" >> setup.cfg
	echo "[PSWD=$E_INK_WIFI_PASSWORD]" >> setup.cfg
	echo "[eInkRotation=1]" >> setup.cfg
	echo "[WX_TOPIC=APRS-$APRS_FI_CALLSIGN]" >> setup.cfg
	echo "[mqttBroker0=$(echo $MQTT_BROKER_IP | cut -d'.' -f1)]" >> setup.cfg
	echo "[mqttBroker1=$(echo $MQTT_BROKER_IP | cut -d'.' -f2)]" >> setup.cfg
	echo "[mqttBroker2=$(echo $MQTT_BROKER_IP | cut -d'.' -f3)]" >> setup.cfg
	echo "[mqttBroker3=$(echo $MQTT_BROKER_IP | cut -d'.' -f4)]" >> setup.cfg
	echo "[MQTT_PORT=1883]" >> setup.cfg
	echo "[OfflineTimeout=20]" >> setup.cfg
	echo "*** setup.cfg file has been generated ***"
	exit 0
elif [ -z $1 ]; then
	JSON=$(wget -O - --ca-certificate=./aprs-fi-chain.pem "https://api.aprs.fi/api/get?name=${APRS_FI_CALLSIGN}&what=wx&apikey=${APRS_FI_APIKEY}&format=json")
	#JSON='{"command":"get","result":"ok","found":1,"what":"wx","entries":[{"name":"OK1HRA-8","time":"1693781931","temp":"12.2","pressure":"1017.1","humidity":"81","wind_direction":"222","wind_speed":"0.0","wind_gust":"9.0","rain_mn":"5.6"}]}'
	if [ $? -eq 0 ]; then
		TEMP=$(echo $JSON | sed -E 's/.*"temp":"?([^,"]*)"?.*/\1/')
		PRESSURE=$(echo $JSON | sed -E 's/.*"pressure":"?([^,"]*)"?.*/\1/')
		HUMIDITY=$(echo $JSON | sed -E 's/.*"humidity":"?([^,"]*)"?.*/\1/')
		WINDDIR=$(echo $JSON | sed -E 's/.*"wind_direction":"?([^,"]*)"?.*/\1/')
		WIND=$(echo $JSON | sed -E 's/.*"wind_gust":"?([^,"]*)"?.*/\1/')
		RAIN=$(echo $JSON | sed -E 's/.*"rain_mn":"?([^,"]*)"?.*/\1/')
		DEW=$(echo "scale=2; $TEMP-((100.0-$HUMIDITY)/5)" | bc)

		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/Pressure-hPa-BMP280 -m "$PRESSURE"
		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/HumidityRel-Percent-HTU21D -m "$HUMIDITY"
		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/WindDir-azimuth -m "$WINDDIR"
		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/WindSpeedMaxPeriod-mps -m "$WIND"
		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/RainToday-mm -m "$RAIN"
		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/DewPoint-Celsius-HTU21D -m "$DEW"
		mosquitto_pub -h $MQTT_BROKER_IP -t APRS-$APRS_FI_CALLSIGN/WX/Temperature-Celsius -m "$TEMP"
		#echo "Value is send ($PRESSURE hpa, $HUMIDITY %, $WINDDIR °, $WIND m/s, $RAIN mm, $DEW °C, $TEMP °C)"
	fi
	exit 0
else
	echo "*** invalid argument ***"
fi
exit 0
