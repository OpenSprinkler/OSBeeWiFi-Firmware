#include <time.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SSD1306.h>
#include <BlynkSimpleEsp8266.h>
#include <i2crtc.h>
#include <TimeLib.h>
#include <OSBeeWiFi.h>
#include <espconnect.h>
#include "images.h"

const char* ssid = "freefly";
const char* pass = "lrtsucks";

SSD1306 display(0x3c, SDA, SCL);
byte st_pins[] = {PIN_ZS0, PIN_ZS1, PIN_ZS2};

void setallpins(byte value) {
  byte i;
  digitalWrite(PIN_COM, value);
  for(i=0;i<MAX_NUMBER_ZONES;i++) {
    digitalWrite(st_pins[i], value);
  }
  pinMode(PIN_COM, OUTPUT);
  for(i=0;i<MAX_NUMBER_ZONES;i++) {
    pinMode(st_pins[i], OUTPUT);
  }
}

void boost() {
  // turn on boost converter for 500ms
  digitalWrite(PIN_BST_PWR, HIGH);
  delay(500);
  digitalWrite(PIN_BST_PWR, LOW);  
}

void open(byte zid) {
  byte pin = st_pins[zid];
  boost();  // boost voltage
  setallpins(HIGH);       // set all switches to HIGH, including COM
  digitalWrite(pin, LOW); // set the specified switch to LOW
  digitalWrite(PIN_BST_EN, HIGH); // dump boosted voltage
  delay(100);                     // for 250ms
  digitalWrite(pin, HIGH);        // set the specified switch back to HIGH
  digitalWrite(PIN_BST_EN, LOW);  // disable boosted voltage
}

void close(byte zid) {
  byte pin = st_pins[zid];
  boost();  // boost voltage
  setallpins(LOW);        // set all switches to LOW, including COM
  digitalWrite(pin, HIGH);// set the specified switch to HIGH
  digitalWrite(PIN_BST_EN, HIGH); // dump boosted voltage
  delay(100);                     // for 250ms
  digitalWrite(PIN_COM, LOW);     // set the specified switch back to LOW
  digitalWrite(PIN_BST_EN, LOW);  // disable boosted voltage
  setallpins(HIGH);               // set all switches back to HIGH
}

void setup() {
  // init
  digitalWrite(PIN_BST_PWR, LOW);
  pinMode(PIN_BST_PWR, OUTPUT);
  digitalWrite(PIN_BST_EN, LOW);
  pinMode(PIN_BST_EN, OUTPUT);
  setallpins(HIGH);
  Serial.begin(115200);
  WiFi.persistent(false); // turn off persistent, fixing flash crashing issue

  // test display
  display.init();
  display.flipScreenVertically();
  display.drawXbm(34, 24, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display.display();

  // detect rtc
  delay(1000);
  Serial.println(RTC.detect()?"RTC detected":"NO RTC!!!");

  String tstr;
  tstr = (ulong)RTC.get();
  display.drawString(0, 8, tstr);
  display.display();

  // test WiFi
  Serial.println("Connecting:");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(WiFi.localIP());

  // test rtc
  if(RTC.get() < 1000000000L) {
    ulong gt = 0;
    configTime(0, 0, "pool.ntp.org", "time.nist.org", NULL);
    Serial.println("Network time:");
    while(true) {
      gt = time(NULL);
      if(!gt) {Serial.print("."); delay(5000);}
      else {Serial.println(gt); RTC.set(gt); break;}
    }
  }
  byte i;
  // test solenoid driver
  for(i=0;i<MAX_NUMBER_ZONES;i++) {
    open(i);
    delay(200);
    close(i);
  }

  for(i=0;i<MAX_NUMBER_ZONES;i++) {
    open(i);
  }  

  for(i=0;i<MAX_NUMBER_ZONES;i++) {
    close(i);
  }  
}

void loop() {

}
