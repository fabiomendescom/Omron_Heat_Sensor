/*
 * This sketch must work in conjunction with Arduino_heat_sensor.ino sketch. The Arduino portion below will connect to Omron  D6T-1A-01 and get the 
 * one single temperature. Then it will pass it by Serial port to ESP8266 as a 2 byte short integer. ESP8266 takes value and sends to MQTT message
 * NOTE: MAKE SURE YOU COMPILE THIS WITH A FLASH SIZE THAT INCLUDES SPIFFS. This will guarantee space on ESP8266 to save this config below.
 */
 
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266mDNS.h>   
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>

ESP8266WebServer server(80);

int incomingByte;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[40];

char chipid[40];

SoftwareSerial sw(13, 15, false, 256); // RX, TX

WiFiManager wifiManager;

long count = 0;

//flag for saving data
bool shouldSaveConfig = false;
int sensorValue;

void handleReset() {
  server.send(200, "text/plain", "WIFI settings reset.");   // Send HTTP status 200 (Ok) and send some text to the browser/client
  delay(3000);
  wifiManager.resetSettings();
  delay(3000);
  //reset and try again, or maybe put it to deep sleep
  ESP.reset();
  delay(5000);
}

void handleHardReset() {
  server.send(200, "text/plain", "WIFI settings HARD reset.");   // Send HTTP status 200 (Ok) and send some text to the browser/client
  delay(3000);
  SPIFFS.format();
  wifiManager.resetSettings();
  delay(3000);
  //reset and try again, or maybe put it to deep sleep
  ESP.reset();
  delay(5000);
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

WiFiClient espClient;
PubSubClient client(espClient);


void setup() {  
  // put your setup code here, to run once:
  Serial.begin(115200);
  sw.begin(9600);
  Serial.println();

  String ci = "ESP8266" + String(ESP.getChipId());
  ci.toCharArray(chipid,40);

  //clean FS, for testing. This clears the saved config file
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt_topic", mqtt_topic, 40);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  server.on("/reset", handleReset);
  server.on("/hardreset", handleHardReset);
  server.begin(); 

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  Serial.println("MQTT");
  Serial.println(mqtt_server);
  Serial.println("Topic");
  Serial.println(mqtt_topic);

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  String port = mqtt_port;
  client.setServer(mqtt_server, port.toInt());
  
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(chipid)) {
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000); 
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient(); 
  if (!client.connected()) {
      Serial.println("Reconnecting");
      if(client.connect(chipid)) {
         Serial.println("Reconnected");
      } else {
         Serial.println("Not reconnecting");
      }
  } else { 
     client.loop();
  }
  if (sw.available() > 0) {
    //make sure you receive a null char first
    if(sw.read()=='\0') {
        delay(100);
        if(sw.available() > 0) {
          byte b1 = sw.read();
          delay(100);
          if(sw.available() > 0) {
            byte b2 = sw.read();
            short val = b2  + b1 * 256;
            char buf[10];
            count++;
            val = (val * 9/5) + 32;
            Serial.print(count);
            Serial.print(": ");
            Serial.println(val);
            String valstring = String(val);
            valstring.toCharArray(buf,10);
            client.publish(mqtt_topic, buf);
          }
      }
    }    
  }
}
