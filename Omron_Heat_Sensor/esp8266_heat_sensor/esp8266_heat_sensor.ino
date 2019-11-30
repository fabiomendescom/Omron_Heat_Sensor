
/*
   This sketch must work in conjunction with ESP8266_heat_sensor.ino sketch. The Arduino portion below will connect to Omron  D6T-1A-01 and get the
   one single temperature. Then it will pass it by Serial port to ESP8266 as a 2 byte short integer. ESP8266 takes value and sends to MQTT message
*/

//https://github.com/akita11/OmronD6T/blob/master/examples/test/test.ino  (<-- Code borrowed from this)

//For this sketch to work you must set MQTT_MAX_PACKET_SIZE in PubSubClient.h to 1000 !!!!!

#include <Wire.h>
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <ESP8266mDNS.h>   


/****************************************/
/******* DON'T CHANGE THIS **************/
/****************************************/
#define NUMTRIESMQTT  3 //number of tries to connect to MQTT

//define your default values here, if there are different values in config.json, they are overwritten.
char chipid[40];
char chipname[40];
char mqtt_server[40];
char mqtt_port[6] = "1883";
//char mqtt_topic[40];
//char mqtt_topic_keepalive[20];
//char mqtt_topic_command[20];
String str_mqtt_topic_keepalive;
String str_mqtt_topic_command;

//flag for saving data
bool shouldSaveConfig = false;
int connectedToMQTT = 0;
int timemillis;
int keepalivemillis;
int discoverymillis;
String chipstatus;
String logstring = "";

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);
WiFiManager wifiManager;

void log(String lg) {
  logstring += String("<br/>") + lg;
  if(logstring.length() > 1000) {
    logstring = logstring.substring(logstring.length()-1000);
  }
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void commandReset() {
  delay(3000);
  wifiManager.resetSettings();
  delay(3000);
  //reset and try again, or maybe put it to deep sleep
  ESP.reset();
  delay(5000); 
}

void handleReset() {
  server.send(200, "text/plain", "WIFI settings reset.");   // Send HTTP status 200 (Ok) and send some text to the browser/client
  commandReset();
}

void commandReboot() {
  delay(3000);
  //reset and try again, or maybe put it to deep sleep
  ESP.reset();
  delay(5000);  
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting.");   // Send HTTP status 200 (Ok) and send some text to the browser/client
  commandReboot();
}

void commandErase() {
  commandReset();
  SPIFFS.format();
}

void handleErase() {
  commandErase();
  server.send(200, "text/plain", "Data Erased");  
}

void handleKeepAlive() {
  server.send(200, "text/plain", String(millis()));   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleStatus() {
  String out;
  out = "<!DOCTYPE HTML>\r\n<html>";
  out += String("STATUS: ") + String(chipstatus) + String("<br/>") + String(logstring);
  out += "</html>\r\n\r\n";
  server.send(200, "text/html", out);
}

void handleInfo() {
  String s;
  s = "<!DOCTYPE HTML>\r\n<html>";
  s += "Chip Name: " + String(chipname) + "<br/>";
  s += "Chip ID: " + String(ESP.getChipId()) + "<br/>";
  s += "MQTT Server: " + String(mqtt_server) + "<br/>";
  s += "MQTT Topic Keep Alive: " + str_mqtt_topic_keepalive + "<br/>";
  s += "MQTT Topic Command: " + str_mqtt_topic_command + "<br/>";
  s += "</html>\r\n\r\n";

  server.send(200, "text/html", s);
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  log("Message arrived in topic: ");
  log(topic);

  String msg;
  Serial.print("Message:");
  log("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg += (char)payload[i];
    log(msg);
  }
 
  Serial.println();
  Serial.println("-----------------------");
  log("");
  log("-----------------------");

  String topicstr = String(topic);
  if(topicstr.equals(str_mqtt_topic_command)) {
    if(msg.equals("reset")) {
      commandReset();
    }
    if(msg.equals("reboot")) {
      commandReboot();
    }    
    if(msg.equals("erase")) {
      commandErase();
    }    
  }
 
}

void keepAlive() {
  if(millis() - keepalivemillis > 20000) {
      char charbuf[100];
      String valstring = String(millis());
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      valstring = ipStr + " - " + valstring;
      valstring.toCharArray(charbuf,100);
      char out[100];
      str_mqtt_topic_keepalive.toCharArray(out,100);
      client.publish(out, charbuf);
      keepalivemillis = millis();
  }
}

void checkMqtt() {
  if(connectedToMQTT==1) {
    int t = 0;
    while (!client.connected()) {
      ++t;
      if(t < NUMTRIESMQTT) {
          Serial.println("Connecting to MQTT...");
          log("Connecting to MQTT...");
 
          if (client.connect(chipid)) {
            Serial.println("connected");  
            log("connected");
            //you can subscribe to topics here if you want to.
            char out[100];
            str_mqtt_topic_command.toCharArray(out,100);
            client.subscribe(out);
            Serial.print("Subscribing to ");
            log("Subscribing to ");
            Serial.println(out);
            log(String(out));
          } else {
            Serial.print("failed with state ");
            log("Failed with state ");
            Serial.print(client.state());
            log(String(client.state()));
            //delay(2000);
          }
      } else {
        connectedToMQTT = 0;
      }
    } 
  }
}

void clientpublish(String topic, char* c) {
  char out[100];
  topic.toCharArray(out,100);
  Serial.println("Publishing");
  log("Publishing");
  chipstatus = String("Publishing");
  if(!client.publish(out, c)) {
    checkMqtt();
    if(!client.publish(out, c)) {
      Serial.println("Failed to publish");
      log("Failed to publish");
      chipstatus = String("Failed to publish");
    } else {
      Serial.print("Published: ");
      log("Published: ");
      Serial.println(c);
      log(String(c));
      chipstatus = String("Ready");      
    }
  } else {
    Serial.print("Published: ");
    log("Published: ");
    Serial.println(c);
    log(String(c));
    chipstatus = String("Ready");
  }
}

void prepareSetup() {
  chipstatus = String("Starting...");

  Serial.begin(115200);
  String ci = "ESP8266" + String(ESP.getChipId());
  ci.toCharArray(chipid,40);

  //clean FS, for testing. This clears the saved config file
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");
  log("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    log("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      log("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        log("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          log("parsed json");
          if(json.containsKey("chipname")) {
              strcpy(chipname, json["chipname"]);
          }
          if(json.containsKey("mqtt_server")) {
              strcpy(mqtt_server, json["mqtt_server"]);
          }
          if(json.containsKey("mqtt_port")) {
              strcpy(mqtt_port, json["mqtt_port"]);
          }
         // if(json.containsKey("mqtt_topic")) {
          //    strcpy(mqtt_topic, json["mqtt_topic"]);
         // }
         // if(json.containsKey("mqtt_topic_keepalive")) {
          //    strcpy(mqtt_topic_keepalive, json["mqtt_topic_keepalive"]);
          //}    
         // if(json.containsKey("mqtt_topic_command")) {
         //     strcpy(mqtt_topic_command, json["mqtt_topic_command"]);
         // }    
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
  WiFiManagerParameter custom_chipname("chipname", "chipname", chipname, 40);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  //WiFiManagerParameter custom_mqtt_topic("topic", "mqtt_topic", mqtt_topic, 40);
  //WiFiManagerParameter custom_mqtt_topic_keepalive("topickeepalive", "mqtt_topic_keepalive", mqtt_topic_keepalive, 20);
  //WiFiManagerParameter custom_mqtt_topic_command("topiccommand", "mqtt_topic_command", mqtt_topic_command, 20);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));pi
  
  //add all your parameters here
  wifiManager.addParameter(&custom_chipname);  
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  //wifiManager.addParameter(&custom_mqtt_topic);
  //wifiManager.addParameter(&custom_mqtt_topic_keepalive);
  //wifiManager.addParameter(&custom_mqtt_topic_command);

  //reset settings - for testing. Use this if you want to clear the WIFI setttings and reconfigure it.
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

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

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  log("Connected");

  //read updated parameters
  strcpy(chipname, custom_chipname.getValue());  
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  //strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  //strcpy(mqtt_topic_keepalive, custom_mqtt_topic_keepalive.getValue());
  //strcpy(mqtt_topic_command, custom_mqtt_topic_command.getValue());

  Serial.print("Chip Name: ");
  Serial.println(chipname);
  Serial.print("MQTT: ");
  Serial.println(mqtt_server);
  Serial.print("Topic: ");
  //Serial.println(mqtt_topic);
  //Serial.print("Topic Keep Alive: ");
  //Serial.println(mqtt_topic_keepalive);
  //Serial.print("Topic Command: ");
  //Serial.println(mqtt_topic_command);  

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["chipname"] = chipname;    
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    //json["mqtt_topic"] = mqtt_topic;
    //json["mqtt_topic_keepalive"] = mqtt_topic_keepalive;
    //json["mqtt_topic_command"] = mqtt_topic_command;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
      log("failed to open config file for writing");
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
  client.setCallback(callback);

  int currenttry = 0;
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    log("Connecting to MQTT...");
 
    if (client.connect(chipid)) {
      Serial.println("connected");  
      log("connected");
      //you can subscribe to topics here if you want to.
      char out[100];
      str_mqtt_topic_command.toCharArray(out,100);
      client.subscribe(out);
      connectedToMQTT = 1;
    } else {
      ++currenttry;
      Serial.print("failed with state ");
      log("failed with state");
      Serial.print(client.state());
      log(client.state());
      if(currenttry > NUMTRIESMQTT) {
        connectedToMQTT = 0;
        chipstatus = String("Can't connect to MQTT");
        break;
      }
    }
  }

  if (!MDNS.begin(chipname)) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
    log("Error setting up MDNS responder!");
  } else {
     MDNS.addService("http", "tcp", 80);
     log("MDNS setup successfully");
  }

 
  timemillis = millis();
  keepalivemillis = millis();
  discoverymillis = millis();

  str_mqtt_topic_keepalive = String("homeassistant/keepalive/") + String(chipname);
  str_mqtt_topic_command = String("homeassistant/command/") + String(chipname);


  server.on("/reset", handleReset);
  server.on("/reboot", handleReboot);
  server.on("/status", handleStatus);
  server.on("/keepalive", handleKeepAlive);
  server.on("/info", handleInfo);
  server.on("/erase", handleErase);

  //Add command and capabilities here
  server.on("/capabilities", handleCapabilities);

  server.begin(); 
}

String quotestring(String s) {
  String t;
  t = String("\"") + String(s) + String("\"");
  return t;
}

String namevaluepair(String n, String v) {
  String t;
  t = quotestring(n) + String(":") + quotestring(v);
  return t;
}

String namevaluepairnumber(String n, String v) {
  String t;
  t = quotestring(n) + String(":") + v;
  return t;
}

/****************************************************/
/******* BELOW THIS LINE ADD YOUR CODE **************/
/****************************************************/
#define DELAYINSECS 10 *1000
#define I2C_ADDR_D6T 0x0a
#define CMD 0x4c

String str_mqtt_topic;
String str_mqtt_topic2;
String str_selfdiscovery_topic;
String str_selfdiscovery_topic2;


//Capabilities have attributes and commands. Device has multiple capabilities
//Use this https://smartthings.developer.samsung.com/docs/api-ref/capabilities.html#Temperature-Measurement for lists of capabilities and commands

// You must set this function with your capabilities

void handleCapabilities() {
  String res;
  
  res += "{";
  res += "  \"capabilties\": [{";
  res += "    \"name\": \"temperatureMeasurement\",";
  res += "    \"attributes\": [{";
  res += "        \"name\": \"temperatureValue\",";
  res += "        \"value\": \"N/A\"";
  res += "      },";
  res += "      {";
  res += "        \"name\": \"TemperatureUnit\",";
  res += "        \"value\": \"F\"";
  res += "      }";
  res += "    ]";
  res += "  }]";
  res += "}  ";
  
  server.send(200, "application/json", res);
}

void selfDiscovery() {
     String d_state_topic2;
     String d_unique_id;
     String d_name;
     String d_state_topic;
     String d_unit_of_measurement;
     String d_force_update;
     String d_device_class;
     String payload;
     String payload2;
     char payloadbuff[1000];
     char payloadbuff2[1000];


    //every 10 seconds send self discovery message
    if(millis() - discoverymillis > 30000) {
        // Do this for the sensor
        str_selfdiscovery_topic = String("homeassistant/sensor/") + String(chipname) + String("/config"); 
       
        d_unique_id = String("ESP8266-") + String(ESP.getChipId());
        d_name = String(chipname);
        d_state_topic = str_mqtt_topic;
        d_unit_of_measurement = "F";
        d_force_update = "true";
        d_device_class = "temperature";

        payload = String("{") + \
                      namevaluepair("name",d_name) + \
                      String(",") + \
                      namevaluepair("unique_id",d_unique_id) + \
                      String(",") + \
                      namevaluepair("state_topic",d_state_topic) + \
                      String(",") + \
                      namevaluepair("unit_of_measurement",d_unit_of_measurement) + \
                      String(",") + \
                      namevaluepair("force_update",d_force_update) + \
                      String(",") + \
                      namevaluepair("device_class",d_device_class) + \
                      String(",") + \
                      namevaluepair("value_template","{{value_json.temperature}}") + \
                      String("}");

        payload.toCharArray(payloadbuff,1000);

     
        //This is also a binary sensor
        str_selfdiscovery_topic2 = String("homeassistant/binary_sensor/") + String(chipname) + String("/config"); 
       
        d_unique_id = String("ESP8266-") + String(ESP.getChipId());
        d_name = String(chipname);
        d_state_topic2 = str_mqtt_topic2;
        d_force_update = "true";
        d_device_class = "heat";

        payload2 = String("{") + \
                      namevaluepair("name",d_name) + \
                      String(",") + \
                      namevaluepair("unique_id",d_unique_id) + \
                      String(",") + \
                      namevaluepair("state_topic",d_state_topic2) + \
                      String(",") + \
                      namevaluepair("force_update",d_force_update) + \
                      String(",") + \
                      namevaluepair("device_class",d_device_class) + \
                      String(",") + \
                      namevaluepair("value_template","{{value_json.state}}") + \
                      String("}");

        payload2.toCharArray(payloadbuff2,1000);
     
     
  
      if(connectedToMQTT==1) {
          log("Publishing self discovery");
          Serial.println("Publishing self discovery");
          clientpublish(str_selfdiscovery_topic, payloadbuff);
          log(payload);
          Serial.println(payloadbuff);
          log("Done publishing self discovery");
          Serial.println("Done publishing self discovery");

          log("Publishing self discovery");
          Serial.println("Publishing self discovery");
          clientpublish(str_selfdiscovery_topic2, payloadbuff2);
          log(payload2);
          Serial.println(payloadbuff2);
          log("Done publishing self discovery");
          Serial.println("Done publishing self discovery");          
      }   
      discoverymillis = millis();
    }
}


void setup() {
     //Always calls this to setup all stuff
     prepareSetup();

     str_mqtt_topic = String("homeassistant/sensor/") + String(chipname) + "/state";
     log("Setup topic");
     log(str_mqtt_topic);

     str_mqtt_topic2 = String("homeassistant/binary_sensor/") + String(chipname) + "/state";
     log("Setup topic");
     log(str_mqtt_topic2);

}

void loop() {
  uint8_t buf[4];
  float temp;
  int i=0;
  char temperaturevalue[10];

  // Items below must be called in loop in all cases for all functionality
  server.handleClient();   
  MDNS.update();
  //checkMqtt();
  server.handleClient();   
  MDNS.update();
  client.loop();
  delay(10);   
  keepAlive();  //report keep alive every 20 seconds
  selfDiscovery();
  // END

  //execute if time is up
  if(millis() - timemillis > DELAYINSECS) {
      Wire.begin();
      Wire.beginTransmission(I2C_ADDR_D6T);
      Wire.write(CMD);
      Wire.endTransmission();
      Wire.requestFrom(I2C_ADDR_D6T, 4);
      while (Wire.available() && i < 4) {
        buf[i++] = Wire.read();
      }
      //READ the second value which is the one that has the 0 sensor for a 1 sensor Omrom
      // See datasheet here -> https://omronfs.omron.com/en_US/ecb/products/pdf/en_D6T_users_manual.pdf
      temp = (256*buf[3] + buf[2]) * 0.10;
      temp = (temp * 9/5) + 32;

      char charbuf[50];
      String valstring = String("{") + \
                         namevaluepairnumber("temperature", String(temp)) + \
                         String("}");
      valstring.toCharArray(temperaturevalue,50);
      Serial.println(temperaturevalue);
      log(String(temperaturevalue));

      if(connectedToMQTT==1) {
          clientpublish(str_mqtt_topic, temperaturevalue);
          if(temp > 95) {
            clientpublish(str_mqtt_topic2, "{\"state\":\"ON\"}");
          } else {
            clientpublish(str_mqtt_topic2, "{\"state\":\"OFF\"}");
          }
      }
      
      timemillis = millis();
  }    



}
