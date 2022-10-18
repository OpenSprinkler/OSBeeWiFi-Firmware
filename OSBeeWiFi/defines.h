/* OSBeeWiFi Firmware
 *
 * OSBeeWiFi macro defines and hardware pin assignments
 * December 2016 @ bee.opensprinkler.com
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

#ifndef _DEFINES_H
#define _DEFINES_H

/** Firmware version, hardware version, and maximal values */
#define OSB_FWV    102    // Firmware version: 102 means 1.0.2

/** GPIO pins */
#define PIN_VER_DETECT 10	// Version detection pin
#define PIN_BUTTON     0
#define PIN_BSTPWR    15

// Version 2 pin defines
#define V2_PIN_BSTEN   16 // Boost converter enable
#define V2_PIN_COM     2  // COM
#define V2_PIN_ZS0     12 // Zone switch 0
#define V2_PIN_ZS1     13 // Zone switch 1
#define V2_PIN_ZS2     14 // Zone switch 2

// Version 3 pin defines
#define V3_PIN_BSTNEN  16 // Boost converter enable (Active-Low)
#define V3_PIN_SR_LAT  12 // v3 uses 74HC595 shift register, this is latch pin

// Default device name
#define DEFAULT_NAME    "My OSBee WiFi"
// Default device key
#define DEFAULT_DKEY    "opendoor"
// Default location
#define DEFAULT_LOCA    "Boston,MA"
// Config file name
#define CONFIG_FNAME    "/config.dat"
// Log file name
#define LOG_FNAME       "/log.dat"
// Program file name
#define PROG_FNAME      "/prog.dat"

#define OSB_SOT_LATCH    0x00
#define OSB_SOT_NONLATCH 0x01

#define OSB_MOD_AP       0xA9
#define OSB_MOD_STA      0x2A

#define OSB_STATE_INITIAL        0
#define OSB_STATE_CONNECTING     1
#define OSB_STATE_CONNECTED      2
#define OSB_STATE_TRY_CONNECT    3
#define OSB_STATE_WAIT_RESTART   4
#define OSB_STATE_WIFIRESET      8
#define OSB_STATE_RESET          9

#define MAX_NUMBER_ZONES         3

// Blynk pin defines
#define BLYNK_RESET V0
#define BLYNK_S1    V1  // zone status
#define BLYNK_S2    V2
#define BLYNK_S3    V3
#define BLYNK_TEST_ZONE   V4
#define BLYNK_TEST_DUR    V5
#define BLYNK_RUN_TEST    V6
#define BLYNK_PROG        V7
#define BLYNK_RUN_PROG    V8
#define BLYNK_LCD         V9

#define MAX_LOG_RECORDS           200
#define MANUAL_PROGRAM_INDEX      'M' // 77
#define QUICK_PROGRAM_INDEX       'Q' // 81
#define TESTZONE_PROGRAM_INDEX    'T' // 84

#define DEFAULT_NTP1    "time.google.com"
#define DEFAULT_NTP2    "time.cloudflare.com"
#define DEFAULT_NTP3    "time.windows.com"
#define MIN_EPOCH_TIME  1577836800UL

#define CLD_NONE    0
#define CLD_BLYNK   1
#define CLD_OTC     2

typedef enum {
  OPTION_FWV = 0, // firmware version
  OPTION_TMZ,     // time zone
  OPTION_SOT,     // solenoid type
  OPTION_HTP,     // http port
  OPTION_MOD,     // mode
  OPTION_SSID,    // wifi ssid
  OPTION_PASS,    // wifi password
  OPTION_CLD,     // cloud option
  OPTION_AUTH,    // authentication token
  OPTION_CDMN,    // cloud server domain name
  OPTION_CPRT,    // cloud server port number
  OPTION_DKEY,    // device key
  OPTION_NAME,    // device name
  OPTION_ZON0,    // zone 1 name
  OPTION_ZON1,    // zone 2 name
  OPTION_ZON2,    // zone 3 name
  OPTION_BSVO,    // boosted voltage for opening
  OPTION_BSVC,    // boosted voltage for closing
  OPTION_DIM,     // LCD dimming when idling
  NUM_OPTIONS     // number of options
} OSB_OPTION_enum;

typedef enum {
  DISP_MODE_IP,
  DISP_MODE_MAC,
  DISP_MODE_SIGNAL,
  DISP_MODE_FWV,
  NUM_DISP_MODES
} OSB_DISP_enum;

#define BUTTON_WIFIRESET_TIMEOUT   3000 // if button is pressed for at least 3 seconds, WiFi reset
#define BUTTON_FACRESET_TIMEOUT    5000  // if button is pressed for at least 5 seconds, factory reset
#define LCD_DIMMING_TIMEOUT 15 // dim LCD after 15 seconds of no button click

#define LED_FAST_BLINK 100
#define LED_SLOW_BLINK 500

#define TIME_SYNC_TIMEOUT  3600

/** Serial debug functions */
//#define SERIAL_DEBUG
#if defined(SERIAL_DEBUG)

  #define DEBUG_BEGIN(x)   { Serial.begin(x); }
  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)

#else

  #define DEBUG_BEGIN(x)   {}
  #define DEBUG_PRINT(x)   {}
  #define DEBUG_PRINTLN(x) {}

#endif

typedef unsigned char byte;
typedef unsigned long ulong;

#endif  // _DEFINES_H
