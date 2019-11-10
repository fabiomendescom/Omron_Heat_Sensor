
/*
 * This sketch must work in conjunction with ESP8266_heat_sensor.ino sketch. The Arduino portion below will connect to Omron  D6T-1A-01 and get the 
 * one single temperature. Then it will pass it by Serial port to ESP8266 as a 2 byte short integer. ESP8266 takes value and sends to MQTT message
 */

#include <OmronD6T.h>  //https://github.com/akita11/OmronD6T/blob/master/examples/test/test.ino
#include <SoftwareSerial.h>

OmronD6T sensor;
SoftwareSerial sw(2, 3); // RX, TX

int temp;
String tempstr;
char nullvalue = '\0';
int DELAYINSECS = 30;

void setup() {
  Serial.begin(115200);
  //initialize software serialization. This is better because if you use the serial built in, you will have to keep unplugging it
  //everytime you compile since it interferes with the compilation process
  sw.begin(9600);
}

void loop() {
  sensor.scanTemp();

  tempstr = sensor.temp[0][0];
  temp = tempstr.toInt();
      
  Serial.println(temp);
  //Sends a null value first to mark the start point. This means after this character, a two byte short will be sent.
  sw.print(nullvalue);
  //Convert value to a two byte short integer and sends over
  sw.write(temp / 256);
  sw.write(temp % 256);

  //Do this e
  delay(DELAYINSECS * 1000);
}
