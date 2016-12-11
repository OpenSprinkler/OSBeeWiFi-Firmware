#include <Wire.h>
#include <SSD1306.h>

#define PCF8563_ADDR 0x21

byte pcf_value = 0xff;

SSD1306 display(0x3c, SDA, SCL);

void pcf_write() 
{
  Wire.beginTransmission(PCF8563_ADDR);
  Wire.write(pcf_value);
  Wire.endTransmission();
}

void pcf_write(byte i, byte v)
{
  if(v) pcf_value |= (1<<i);
  else pcf_value &= ~(1<<i);
  pcf_write();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pcf_write();

  display.init();
  display.flipScreenVertically();  
}

byte i = 0;
unsigned long t = 0;
void loop() {
  if(!t) {
    pcf_write(i, 0);
    t=1;
  } else if(millis() > t + 2000) {
    pcf_write(i, 1);
    i=(i+1)%8;
    pcf_write(i, 0);
    t=millis();
  }
  String text;
  Serial.println(analogRead(A0));
  text += analogRead(A0);
  display.clear();
  display.drawString(0, 0, text);
  display.display();
  delay(250);
}