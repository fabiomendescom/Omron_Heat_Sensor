# Omron Heat Sensor
This is a combination of Arduino and ESP8266 code to get an Omron Heat Sensor working together
Details about the sensor: https://omronfs.omron.com/en_US/ecb/products/pdf/en_D6T_users_manual.pdf

## Hookup

| Arduino        | ESP8266      | Omron       |
| ------------- |---------------|-------------| 
| 5v            |               | Red (VCC)   |
| GND           |               | Black (GND) |
| A5            |               | Yellow (SCL)|
| A4            |               | Blue (SDA)  |
| 3.3v          | VIN           |             | 
| GND           | GND           |             |
| 3             | D7            |             |

## How to use run it
You need an Arduino Uno and an ESP8266 in addition to an MQTT server

1. Install the library from https://github.com/tzapu/WiFiManager for ESP8266
2. Install the library from https://github.com/akita11/OmronD6T for Arduino UNO
3. Install these libraries for ESP8266 if they are not there: https://github.com/esp8266/Arduino, https://github.com/tzapu/WiFiManager and https://github.com/bblanchon/ArduinoJson
4. Compile the code into Arduino ESP8266
5. Compile the code into ESP8266. Make sure you compile the flash with option "SPIFFS" of at least 64k. This ensures space to save the config file
6. Power the Arduino Uno with USB
7. In your computer, access WIFI endpoint AutoConnectAP (you have to find the network in your WIFI. It won't display automatically)
