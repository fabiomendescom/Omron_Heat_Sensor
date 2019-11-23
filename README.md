# Omron Heat Sensor
This is a combination of Arduino and ESP8266 code to get an Omron Heat Sensor working together
Details about the sensor: https://omronfs.omron.com/en_US/ecb/products/pdf/en_D6T_users_manual.pdf

## Hookup

| 3.3v-5v Step up | ESP8266      | Omron       |
| --------------- |---------------|-------------| 
| OUT+            |               | Red (VCC)   |
| OUT-            |               | Black (GND) |
| D1              |               | Yellow (SCL)|
| D2              |               | Blue (SDA)  |
| IN+             | 3v3           |             | 
| IN-             | GND           |             |

Note: Pull up 10K resistors on D1,D2 to VCC

## How to use run it
You need an ESP8266 in addition to an MQTT server

1. Install the library from https://github.com/tzapu/WiFiManager for ESP8266
3. Install these libraries for ESP8266 if they are not there: https://github.com/esp8266/Arduino, https://github.com/tzapu/WiFiManager and https://github.com/bblanchon/ArduinoJson
5. Compile the code into ESP8266. Make sure you compile the flash with option "SPIFFS" of at least 64k. This ensures space to save the config file
7. In your computer, access WIFI endpoint AutoConnectAP (you have to find the network in your WIFI. It won't display automatically). Setup access there.
