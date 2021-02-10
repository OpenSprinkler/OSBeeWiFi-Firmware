/* OSBeeWiFi Firmware
 *
 * OSBeeWiFi library
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

#include "OSBeeWiFi.h"
#include "font.h"
#include "images.h"
#include "program.h"

byte OSBeeWiFi::version = 2;	// default version is 2
byte OSBeeWiFi::state = OSB_STATE_INITIAL;
byte OSBeeWiFi::has_rtc = false;
byte OSBeeWiFi::curr_zbits = 0;
byte OSBeeWiFi::next_zbits = 0;
byte OSBeeWiFi::program_busy = 0;
File OSBeeWiFi::log_file;
File OSBeeWiFi::prog_file;
byte OSBeeWiFi::st_pins[] = {V2_PIN_ZS0, V2_PIN_ZS1, V2_PIN_ZS2};
ulong OSBeeWiFi::open_tstamp[]={0,0,0};
ulong OSBeeWiFi::curr_utc_time = 0;
SSD1306 OSBeeWiFi::display(0x3c, SDA, SCL);

const char* config_fname = CONFIG_FNAME;
const char* log_fname = LOG_FNAME;
const char* prog_fname = PROG_FNAME;

extern ProgramData pd;

/* Options name, default integer value, max value, default string value
 * Integer options don't have string value
 * String options don't have integer or max value
 */
OptionStruct OSBeeWiFi::options[] = {
  {"fwv", OSB_FWV,     255, ""},
  {"tmz", 48,          104, ""},
  {"sot", OSB_SOT_LATCH,   255, ""},
  {"htp", 80,        65535, ""},
  {"mod", OSB_MOD_AP,  255, ""},
  {"ssid", 0, 0, ""},  // string options have 0 max value
  {"pass", 0, 0, ""},
  {"auth", 0, 0, ""},
  {"dkey", 0, 0, DEFAULT_DKEY},
  {"name", 0, 0, DEFAULT_NAME},
  {"zon0", 0, 0, "Zone 1"},
  {"zon1", 0, 0, "Zone 2"},
  {"zon2", 0, 0, "Zone 3"},
  {"bsvo", 0, 21, ""},
  {"bsvc", 0, 21, ""},  
};

ulong OSBeeWiFi::curr_loc_time() {
  return curr_utc_time + (ulong)options[OPTION_TMZ].ival*900L - 43200L;
}

void OSBeeWiFi::begin() {

  digitalWrite(PIN_BSTPWR, LOW);	// turn off boost power pin
  pinMode(PIN_BSTPWR, OUTPUT);

	// detect version: on v3 PIN_VER_DETECT is externally pulled low
	pinMode(PIN_VER_DETECT, INPUT_PULLUP);
	if(digitalRead(PIN_VER_DETECT)==LOW) {
		version = 3;
		SPI.begin(); // must call SPI.begin first, because we are using pin 12 as latch next
	  digitalWrite(V3_PIN_SR_LAT, HIGH);	// latch pin is active low, so set it high for now
	  pinMode(V3_PIN_SR_LAT, OUTPUT);
	  set_sr_output(0);
	  digitalWrite(V3_PIN_BSTNEN, HIGH);
	  pinMode(V3_PIN_BSTNEN, OUTPUT);
	}
	else {
		version = 2;
		digitalWrite(V2_PIN_BSTEN, LOW);
		pinMode(V2_PIN_BSTEN, OUTPUT);
	  setallpins(HIGH);
	}
  
  state = OSB_STATE_INITIAL;
  
  if(!SPIFFS.begin()) {
    DEBUG_PRINTLN(F("failed to mount file system!"));
  }
  
  display.init();
  display.flipScreenVertically();
  
  flashScreen();
  
  if(RTC.detect()) {
    has_rtc = true;
    DEBUG_PRINTLN("RTC detected");
  }
  else {
    has_rtc = false;
    DEBUG_PRINTLN("RTC is NOT detected!");
  }

}

void OSBeeWiFi::boldFont(bool bold) {
  if(bold) display.setFont(Noto_Sans_Bold_12);
  else display.setFont(Noto_Sans_12);

}

void OSBeeWiFi::flashScreen() {
  boldFont(true);
  display.drawString(0, 0, "OpenSprinkler Bee");
  display.drawXbm(34, 24, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  String vstring = "v";
  vstring+=version;
  boldFont(false);
  display.drawString(0, 45, vstring);
  display.display();
}

void OSBeeWiFi::boost(bool direction) {
  // turn on boost converter for a maximum of 1000 ms
  if(version==2) {
  	// version 2 does not have mechanism to control boost voltage
		digitalWrite(PIN_BSTPWR, HIGH);
		delay(800);
		digitalWrite(PIN_BSTPWR, LOW);
	} else {
		// version 3 uses A0 to control boosted voltage
		digitalWrite(PIN_BSTPWR, HIGH);
		uint16_t volt=21; // maximum voltage by design is 21 volt
		if(direction==OPEN) {
			if(options[OPTION_BSVO].ival>=5) volt=options[OPTION_BSVO].ival; // if bsvo is defined
		} else {
			if(options[OPTION_BSVC].ival>=5) volt=options[OPTION_BSVC].ival; // if bsvc is defined		
		}
		uint16_t top = (uint16_t)(volt * 20.5f); // ADC voltage divider uses 1.6k/79.9k
		uint32_t start = millis();
		// delay for the smaller of 1000 ms and the time for A0 to reach top value
		while(millis()<start+1000 && analogRead(A0)<top) {
			delay(10);
		}
		digitalWrite(PIN_BSTPWR, LOW);
	}
}

/* Set shift register to a given value */
void OSBeeWiFi::set_sr_output(byte value) {
	if(version!=3) return;
  digitalWrite(V3_PIN_SR_LAT, LOW);
  SPI.transfer(value);  
  digitalWrite(V3_PIN_SR_LAT, HIGH);
}

/* Set all pins (including COM)
 * to a given value
 */
void OSBeeWiFi::setallpins(byte value) {
	if(version!=2) return;
  digitalWrite(V2_PIN_COM, value);
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    digitalWrite(st_pins[i], value);
  }
  static byte first=true;
  if(first) {
    first=false;
    pinMode(V2_PIN_COM, OUTPUT);
    for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
      pinMode(st_pins[i], OUTPUT);
    }
  }
}

void OSBeeWiFi::options_setup() {
  if(!SPIFFS.exists(config_fname)) { // if config file does not exist
    options_save(); // save default option values
    return;
  } 
  options_load();
  
  if(options[OPTION_FWV].ival != OSB_FWV)  {
    // if firmware version has changed
    // re-save options, thus preserving
    // shared options with previous firmwares
    options[OPTION_FWV].ival = OSB_FWV;
    options_save();
    return;
  }
}

void OSBeeWiFi::options_reset() {
  DEBUG_PRINT(F("reset to factory default..."));
  if(!SPIFFS.remove(config_fname)) {
    DEBUG_PRINT(F("failed to remove config file"));
    return;
  }
  DEBUG_PRINTLN(F("ok"));
}

void OSBeeWiFi::log_reset() {
  if(!SPIFFS.remove(log_fname)) {
    DEBUG_PRINTLN(F("failed to remove log file"));
    return;
  }
}

void OSBeeWiFi::progs_reset() {
  if(!SPIFFS.remove(prog_fname)) {
    DEBUG_PRINTLN(F("failed to remove program file"));
    return;
  }
  DEBUG_PRINTLN(F("ok"));    
}

int8_t OSBeeWiFi::find_option(String name) {
  for(byte i=0;i<NUM_OPTIONS;i++) {
    if(name == options[i].name) {
      return i;
    }
  }
  return -1;
}

void OSBeeWiFi::options_load() {
  File file = SPIFFS.open(config_fname, "r");
  DEBUG_PRINT(F("loading config file..."));
  if(!file) {
    DEBUG_PRINTLN(F("failed"));
    return;
  }
  while(file.available()) {
    String name = file.readStringUntil(':');
    String sval = file.readStringUntil('\n');
    sval.trim();
    int8_t idx = find_option(name);
    if(idx<0) continue;
    if(options[idx].max) {  // this is an integer option
      options[idx].ival = sval.toInt();
    } else {  // this is a string option
      options[idx].sval = sval;
    }
  }
  DEBUG_PRINTLN(F("ok"));
  file.close();
}

void OSBeeWiFi::options_save() {
  File file = SPIFFS.open(config_fname, "w");
  DEBUG_PRINT(F("saving config file..."));  
  if(!file) {
    DEBUG_PRINTLN(F("failed"));
    return;
  }
  OptionStruct *o = options;
  for(byte i=0;i<NUM_OPTIONS;i++,o++) {
    file.print(o->name + ":");
    if(o->max)
      file.println(o->ival);
    else
      file.println(o->sval);
  }
  DEBUG_PRINTLN(F("ok"));  
  file.close();
}

void OSBeeWiFi::toggle_led() {
  static byte status = 0;
  status = 1-status;
  set_led(!status);
}

void OSBeeWiFi::set_led(byte status) {
  display.setColor(status ? WHITE : BLACK);
  display.fillCircle(122, 58, 4);
  display.display();
  display.setColor(WHITE);
}

bool OSBeeWiFi::get_cloud_access_en() {
  if(options[OPTION_AUTH].sval.length()==32) {
    return true;
  }
  return false;
}

void OSBeeWiFi::write_log(const LogStruct& data) {
  File file;
  uint curr = 0;
  DEBUG_PRINT(F("saving log data..."));  
  if(!SPIFFS.exists(log_fname)) {  // create log file
    file = SPIFFS.open(log_fname, "w");
    if(!file) {
      DEBUG_PRINTLN(F("failed"));
      return;
    }
    // fill log file
    uint next = curr+1;
    file.write((const byte*)&next, sizeof(next));
    file.write((const byte*)&data, sizeof(LogStruct));
    LogStruct l;
    l.tstamp = 0;
    for(;next<MAX_LOG_RECORDS;next++) {
      file.write((const byte*)&l, sizeof(LogStruct));
    }
  } else {
    file = SPIFFS.open(log_fname, "r+");
    if(!file) {
      DEBUG_PRINTLN(F("failed"));
      return;
    }
    file.readBytes((char*)&curr, sizeof(curr));
    uint next = (curr+1) % MAX_LOG_RECORDS;
    file.seek(0, SeekSet);
    file.write((const byte*)&next, sizeof(next));

    file.seek(sizeof(curr)+curr*sizeof(LogStruct), SeekSet);
    file.write((const byte*)&data, sizeof(LogStruct));
  }
  DEBUG_PRINTLN(F("ok"));      
  file.close();
}

bool OSBeeWiFi::read_log_start() {
  if(log_file) log_file.close();
  log_file = SPIFFS.open(log_fname, "r");
  if(!log_file) return false;
  uint curr;
  if(log_file.readBytes((char*)&curr, sizeof(curr)) != sizeof(curr)) return false;
  if(curr>=MAX_LOG_RECORDS) return false;
  return true;
}

bool OSBeeWiFi::read_log_next(LogStruct& data) {
  if(!log_file) return false;
  if(log_file.readBytes((char*)&data, sizeof(LogStruct)) != sizeof(LogStruct)) return false;
  return true;  
}

bool OSBeeWiFi::read_log_end() {
  if(!log_file) return false;
  log_file.close();
  return true;
}

void OSBeeWiFi::apply_zbits() {
  static LogStruct l;
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    byte mask = (byte)1<<i;
    if(next_zbits & mask) {
      if(curr_zbits & mask)     continue; // bit is already set
      open(i); // open solenoid
      open_tstamp[i]=curr_utc_time;
      l.tstamp = curr_utc_time;
      l.dur = 0;
      l.event = 'o';
      l.zid = i;
      l.pid = pd.curr_prog_index;
      l.tid = pd.curr_task_index;
      write_log(l);
    } else {
      if(!(curr_zbits & mask))  continue; // bit is already reset
      close(i);  // close solenoid
      l.tstamp = curr_utc_time;
      l.dur = curr_utc_time-open_tstamp[i]+1;
      l.event = 'c';
      l.zid = i;
      l.pid = pd.curr_prog_index;
      l.tid = pd.curr_task_index;
      write_log(l);
      open_tstamp[i]=0;
    }
  }
  curr_zbits = next_zbits;  // update curr_zbits
}

void OSBeeWiFi::set_zone(byte zid, byte value) {
  if(zid>=MAX_NUMBER_ZONES)  return; // out of bound
  byte mask = (byte)1<<zid;
  if(value) next_zbits = next_zbits | mask;
  else      next_zbits = next_zbits & (~mask);
}

// open a zone by zone index
void OSBeeWiFi::open(byte zid) {
	if(version==2) open_v2(zid);
	if(version==3) open_v3(zid);
}

// close a zone
void OSBeeWiFi::close(byte zid) {
	if(version==2) close_v2(zid);
	if(version==3) close_v3(zid);
}

void OSBeeWiFi::open_v3(byte zid) {
	if(version!=3) return;
  set_sr_output(0);
  boost(OPEN);  // boost voltage
  set_sr_output(0b00000010 | (0b01<<((zid+1)*2)));
  digitalWrite(V3_PIN_BSTNEN, LOW);
  delay(100);
  digitalWrite(V3_PIN_BSTNEN, HIGH);
  set_sr_output(0);	
}

void OSBeeWiFi::close_v3(byte zid) {
	if(version!=3) return;
  set_sr_output(0);
  digitalWrite(V3_PIN_BSTNEN, LOW);
  boost(CLOSE);  // boost voltage
  set_sr_output(0b00000001 | (0b10<<((zid+1)*2)));
  delay(100);
  digitalWrite(V3_PIN_BSTNEN, HIGH);
  set_sr_output(0);	
}

void OSBeeWiFi::open_v2(byte zid) {
	if(version!=2) return;
  byte pin = st_pins[zid];
  if(options[OPTION_SOT].ival == OSB_SOT_LATCH) {
    // for latching solenoid
    boost(OPEN);  // boost voltage
    setallpins(HIGH);       // set all switches to HIGH, including COM
    digitalWrite(pin, LOW); // set the specified switch to LOW
    digitalWrite(V2_PIN_BSTEN, HIGH); // dump boosted voltage
    delay(100);                     // for 100ms
    digitalWrite(pin, HIGH);        // set the specified switch back to HIGH
    digitalWrite(V2_PIN_BSTEN, LOW);  // disable boosted voltage
  } else {
    DEBUG_PRINT("open_nl ");
    DEBUG_PRINTLN(zid);
    // for non-latching solenoid
    digitalWrite(V2_PIN_BSTEN, LOW);  // disable output of boosted voltage 
    boost(OPEN);                        // boost voltage
    digitalWrite(V2_PIN_COM, HIGH);    // set COM to HIGH
    digitalWrite(V2_PIN_BSTEN, HIGH); // dump boosted voltage    
    digitalWrite(pin, LOW);         // set specified switch to LOW
  }
}

void OSBeeWiFi::close_v2(byte zid) {
	if(version!=2) return;
  byte pin = st_pins[zid];
  if(options[OPTION_SOT].ival == OSB_SOT_LATCH) {  
    // for latching solenoid
    boost(CLOSE);  // boost voltage
    setallpins(LOW);        // set all switches to LOW, including COM
    digitalWrite(pin, HIGH);// set the specified switch to HIGH
    digitalWrite(V2_PIN_BSTEN, HIGH); // dump boosted voltage
    delay(100);                     // for 250ms
    digitalWrite(pin, LOW);     // set the specified switch back to LOW
    digitalWrite(V2_PIN_BSTEN, LOW);  // disable boosted voltage
    setallpins(HIGH);               // set all switches back to HIGH
  } else {
    DEBUG_PRINT("close_nl ");
    DEBUG_PRINTLN(zid);   
    // for non-latching solenoid
    digitalWrite(pin, HIGH);
  }
}

