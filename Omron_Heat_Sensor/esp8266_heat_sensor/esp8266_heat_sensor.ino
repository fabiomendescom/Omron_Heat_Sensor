
/*
   This sketch must work in conjunction with ESP8266_heat_sensor.ino sketch. The Arduino portion below will connect to Omron  D6T-1A-01 and get the
   one single temperature. Then it will pass it by Serial port to ESP8266 as a 2 byte short integer. ESP8266 takes value and sends to MQTT message
*/

//https://github.com/akita11/OmronD6T/blob/master/examples/test/test.ino  (<-- Code borrowed from this)


#include <Wire.h>

#define I2C_ADDR_D6T 0x0a
#define CMD 0x4c

void setup() {
  Serial.begin(115200);
}

void loop() {
  uint8_t buf[4];
  float temp;
  int i=0;
  
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
  
  Serial.println(temp);


  delay(3000);

}
