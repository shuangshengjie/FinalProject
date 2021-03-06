#include <SoftwareSerial.h>
#include <Wire.h>
#include <VL6180X.h>

VL6180X sensor;
SoftwareSerial mySerial(10,11);

void setup() 
{
  Serial.begin(115200);
  Wire.begin();
  mySerial.begin(115200);
  
  sensor.init();
  sensor.configureDefault();
  sensor.setTimeout(500);
}

void loop() 
{ 
  int range = sensor.readRangeSingleMillimeters();
  Serial.print(range);
  if(range > 200 || range < 40){
    mySerial.write('n');
  }
  mySerial.flush();
  Serial.println();
  delay(1000);
}
