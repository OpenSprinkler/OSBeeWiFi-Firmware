/* OSBeeWiFi Firmware
 *
 * Main loop wrapper for Arduino
 * June 2016 @ bee.opensprinkler.com
 *
 * This file is part of the OSBeeWiFi library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SSD1306.h>
#include <BlynkSimpleEsp8266.h>
#include <i2crtc.h>

void setup() {
  Serial.begin(115200);
  RTC.detect();
  tmElements_t tm;
  tm.Second = 1;
  tm.Minute = 2;
  tm.Hour =  3;
  tm.Day = 4;
  tm.Wday = 5;
  tm.Month = 6;
  tm.Year = 16;
  //RTC.write(tm);
}

void loop() {
  unsigned long t = RTC.get();
  tmElements_t tm;
  RTC.read(tm);
  Serial.print(tm.Year);
  Serial.print("-");
  Serial.print(tm.Month);
  Serial.print("-");
  Serial.print(tm.Day);
  Serial.print(" ");
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.print(tm.Minute);
  Serial.print(":");
  Serial.println(tm.Second);

  delay(2000);
}
