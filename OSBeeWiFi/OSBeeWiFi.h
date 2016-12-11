/* OSBeeWiFi Firmware
 *
 * OSBeeWiFi library header file
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

#ifndef _OSBEEWIFI_H
#define _OSBEEWIFI_H

#include <Arduino.h>
#include <FS.h>
#include <SSD1306.h>
#include <i2crtc.h>
#include "defines.h"

struct OptionStruct {
  String name;    // option name
  uint16_t ival;  // integer value
  uint16_t max;   // maximum value
  String sval;    // string value
};

struct LogStruct {
  ulong tstamp; // time stamp
  ulong dur;    // duration
  byte  event;  // event
  byte  zid;    // zone id
  byte  pid;    // program id
  byte  tid;    // task id
};

class OSBeeWiFi {
public:
  static OptionStruct options[];
  static byte state;
  static byte has_rtc;
  static byte curr_zbits; // current zone bits
  static byte next_zbits; // next zone bits
  static ulong curr_utc_time;
  static ulong curr_loc_time();
  static ulong open_tstamp[];
  static byte program_busy;
  static SSD1306 display;
  static void begin();
  static void open(byte zid);
  static void close(byte zid);
  static void apply_zbits();
  static void set_zone(byte zid, byte value);
  static void clear_zbits() { next_zbits = 0; }
  static void options_setup();
  static void options_load();
  static void options_save();
  static void options_reset();
  static void progs_reset();
  static void restart() { ESP.restart(); }
  static byte get_mode()   { return options[OPTION_MOD].ival; }
  static byte get_button() { return digitalRead(PIN_BUTTON); }
  static void toggle_led();
  static void set_led(byte status);
  static bool get_cloud_access_en();
  static int8_t find_option(String name);
  static void log_reset();
  static void write_log(const LogStruct& data);
  static bool read_log_start();
  static bool read_log_next(LogStruct& data);
  static bool read_log_end();
  static void boldFont(bool bold);
private:
  static byte st_pins[];
  static File log_file;
  static File prog_file;
  static void setallpins(byte value);
  static void button_handler();
  static void boost();
  static void flashScreen();
};

#endif  // _OSBEEWIFI_H_
