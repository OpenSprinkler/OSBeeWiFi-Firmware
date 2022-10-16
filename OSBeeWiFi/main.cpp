/* OSBeeWiFi Firmware
 *
 * Main loop
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

#if defined(SERIAL_DEBUG)
  #define BLYNK_DEBUG
  #define BLYNK_PRINT Serial
#endif

#include <BlynkSimpleEsp8266.h>
#include <DNSServer.h>
#include <OpenThingsFramework.h>
#include <Request.h>
#include <Response.h>
#include <WiFiUdp.h>
#include "TimeLib.h"
#include "OSBeeWiFi.h"
#include "espconnect.h"
#include "program.h"

OSBeeWiFi osb;
ProgramData pd;
OTF::OpenThingsFramework *otf = NULL;
ESP8266WebServer *updateServer = NULL;
DNSServer *dns = NULL;
SSD1306& disp = OSBeeWiFi::display;
byte idle_countdown = 0;

static uint16_t led_blink_ms = LED_FAST_BLINK;

WidgetLED blynk_led1(BLYNK_S1);
WidgetLED blynk_led2(BLYNK_S2);
WidgetLED blynk_led3(BLYNK_S3);
WidgetLCD blynk_lcd(BLYNK_LCD);

WidgetLED *blynk_leds[] = {&blynk_led1, &blynk_led2, &blynk_led3};

static String scanned_ssids;
static byte disp_mode = DISP_MODE_IP;
static byte curr_mode;
static ulong& curr_utc_time = OSBeeWiFi::curr_utc_time;
static Ticker restart_ticker;

void reset_zones();
void start_manual_program(byte, uint16_t);
void start_testzone_program(byte, uint16_t);
void start_quick_program(uint16_t[]);
void start_program(byte);

String two_digits(uint8_t x) {
  return String(x/10) + (x%10);
}

String toHMS(ulong t) {
  return two_digits(t/3600)+":"+two_digits((t/60)%60)+":"+two_digits(t%60);
}

void otf_send_html_P(OTF::Response &res, const __FlashStringHelper *content) {
	res.writeStatus(200, F("OK"));
	res.writeHeader(F("Content-Type"), F("text/html"));
	res.writeHeader(F("Access-Control-Allow-Origin"), F("*")); // from esp8266 2.4 this has to be sent explicitly
	res.writeHeader(F("Content-Length"), strlen_P((char *) content));
	res.writeHeader(F("Connection"), F("close"));
	res.writeBodyChunk((char *) "%s", content);
	DEBUG_PRINT(strlen_P((char *) content));
	DEBUG_PRINTLN(F(" bytes sent."));
}

void otf_send_json(OTF::Response &res, String json) {
	res.writeStatus(200, F("OK"));
	res.writeHeader(F("Content-Type"), F("application/json"));
	res.writeHeader(F("Access-Control-Allow-Origin"), F("*")); // from esp8266 2.4 this has to be sent explicitly
	res.writeHeader(F("Content-Length"), json.length());
	res.writeHeader(F("Connection"), F("close"));
	res.writeBodyChunk((char *) "%s",json.c_str());
}

void otf_send_result(OTF::Response &res, byte code, const char *item = NULL) {
	String json = F("{\"result\":");
	json += code;
	if (!item) item = "";
	json += F(",\"item\":\"");
	json += item;
	json += F("\"");
	json += F("}");
	otf_send_json(res, json);
}

void updateserver_send_result(byte code, const char* item = NULL) {
	String json = F("{\"result\":");
	json += code;
	if (!item) item = "";
	json += F(",\"item\":\"");
	json += item;
	json += F("\"");
	json += F("}");
	updateServer->sendHeader("Access-Control-Allow-Origin", "*"); // from esp8266 2.4 this has to be sent explicitly
	updateServer->send(200, "application/json", json);
}

/*void server_send_html(String html) {
  server->send(200, "text/html", html);
}

void server_send_result(byte code, const char* item = NULL) {
  String html = F("{\"result\":");
  html += code;
  html += F(",\"item\":\"");
  if(item) html += item;
  html += "\"";
  html += "}";
  server_send_html(html);
}
*/
bool get_value_by_key(const OTF::Request &req, const char* key, long& val) {
  char *p = req.getQueryParameter(key);
  if(p!=NULL) {
    val = String(p).toInt();
    return true;
  } else {
    return false;
  }
}

bool get_value_by_key(const OTF::Request &req, const char* key, String& val) {
  char *p = req.getQueryParameter(key);
  if(p!=NULL) {
    val = String(p);
    return true;
  } else {
    return false;
  }
}

void append_key_value(String& json, const char* key, const ulong value) {
  json += "\"";
  json += key;
  json += "\":";
  json += value;
  json += ",";
}

void append_key_value(String& json, const char* key, const int16_t value) {
  json += "\"";
  json += key;
  json += "\":";
  json += value;
  json += ",";
}

void append_key_value(String& json, const char* key, const String& value) {
  json += "\"";
  json += key;
  json += "\":\"";
  json += value;
  json += "\",";
}

char dec2hexchar(byte dec) {
  if(dec<10) return '0'+dec;
  else return 'A'+(dec-10);
}

String get_mac() {
  static String hex;
  if(!hex.length()) {
    byte mac[6];
    WiFi.macAddress(mac);

    for(byte i=0;i<6;i++) {
      hex += dec2hexchar((mac[i]>>4)&0x0F);
      hex += dec2hexchar(mac[i]&0x0F);
    }
  }
  return hex;
}

String get_ap_ssid() {
  static String ap_ssid;
  if(!ap_ssid.length()) {
    byte mac[6];
    WiFi.macAddress(mac);
    ap_ssid += "OSB_";
    for(byte i=3;i<6;i++) {
      ap_ssid += dec2hexchar((mac[i]>>4)&0x0F);
      ap_ssid += dec2hexchar(mac[i]&0x0F);
    }
  }
  return ap_ssid;
}

String get_ip(IPAddress _ip) {
  String ip = "";
  ip = _ip[0];
  ip += ".";
  ip += _ip[1];
  ip += ".";
  ip += _ip[2];
  ip += ".";
  ip += _ip[3];
  return ip;
}

void restart_in(uint32_t ms) {
  if(osb.state != OSB_STATE_WAIT_RESTART) {
    osb.state = OSB_STATE_WAIT_RESTART;
    DEBUG_PRINTLN(F("Prepare to restart..."));
    restart_ticker.once_ms(ms, osb.restart);
  }
}

bool verify_dkey(const OTF::Request &req) {
  if(curr_mode == OSB_MOD_AP) return false; // invalid request
  if(req.isCloudRequest()){ // no need for dkey if this is coming from cloud connection
    return true;
  }
  const char *dkey = req.getQueryParameter("dkey");
  if(dkey != NULL && strcmp(dkey, osb.options[OPTION_DKEY].sval.c_str()) == 0)
    return true;
  return false;
}

int16_t get_pid(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return -2;
  long v;
  if(get_value_by_key(req, "pid", v)) {
    return v;
  } else {
    otf_send_result(res, HTML_DATA_MISSING, "pid");
    return -2;
  }
}

void on_home(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) {
    otf_send_html_P(res, (const __FlashStringHelper *) connect_html);
  } else {
    otf_send_html_P(res, (const __FlashStringHelper *) index_html);
  }
}

void on_sta_view_options(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  otf_send_html_P(res, (const __FlashStringHelper *) settings_html);
}

void on_sta_view_manual(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  otf_send_html_P(res, (const __FlashStringHelper *) manual_html);
}

void on_sta_view_logs(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  otf_send_html_P(res, (const __FlashStringHelper *) log_html);
}

void on_sta_view_program(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  otf_send_html_P(res, (const __FlashStringHelper *) program_html);
}

void on_sta_view_preview(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  otf_send_html_P(res, (const __FlashStringHelper *) preview_html);  
}

void on_ap_scan(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_STA) return;
  otf_send_json(res, scanned_ssids);
}

void on_ap_change_config(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_STA) return;
  char *ssid = req.getQueryParameter("ssid");
  if(ssid!=NULL&&strlen(ssid)!=0) {
    osb.options[OPTION_SSID].sval = ssid;
    osb.options[OPTION_PASS].sval = req.getQueryParameter("pass");
    osb.options_save();
    otf_send_result(res, HTML_SUCCESS, nullptr);
    osb.state = OSB_STATE_TRY_CONNECT;
  } else {
    otf_send_result(res, HTML_DATA_MISSING, "ssid");
  }
}

void on_ap_try_connect(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_STA) return;
  String json;
  json += "{";
  ulong ip = (WiFi.status()==WL_CONNECTED)?(uint32_t)WiFi.localIP():0;
  append_key_value(json, "ip", ip);
  json.remove(json.length()-1);  
  json += "}";
  otf_send_json(res, json);
  if(WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
    DEBUG_PRINTLN(F("IP received by client, restart."));
    restart_in(1000);
  }
}

String get_zone_names_json() {
  String str=F("\"zons\":[");
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    str+="\"";
    str+=osb.options[OPTION_ZON0+i].sval;
    str+="\",";
  }
  str.remove(str.length()-1);
  str+="]";
  return str;
}

void on_sta_controller(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  String json;
  json += "{";
  append_key_value(json, "fwv", (int16_t)osb.options[OPTION_FWV].ival);
  append_key_value(json, "sot", (int16_t)osb.options[OPTION_SOT].ival);
  append_key_value(json, "utct", curr_utc_time);
  append_key_value(json, "pid", (int16_t)pd.curr_prog_index);
  append_key_value(json, "tid", (int16_t)pd.curr_task_index);
  append_key_value(json, "np",  (int16_t)pd.nprogs);
  append_key_value(json, "nt",  (int16_t)pd.scheduled_ntasks);
  append_key_value(json, "mnp", (int16_t)MAX_NUM_PROGRAMS);
  append_key_value(json, "prem",  pd.curr_prog_remaining_time);
  append_key_value(json, "trem",  pd.curr_task_remaining_time);
  append_key_value(json, "zbits", (int16_t)osb.curr_zbits);
  append_key_value(json, "name",  osb.options[OPTION_NAME].sval);
  append_key_value(json, "mac", get_mac());
  append_key_value(json, "cid", (ulong)ESP.getChipId());
  append_key_value(json, "rssi", (int16_t)WiFi.RSSI());
  int16_t cld = osb.options[OPTION_CLD].ival;
  append_key_value(json, "cld", cld);
  if(cld>CLD_NONE) {
    if(cld==CLD_BLYNK) append_key_value(json, "clds", (int16_t)(Blynk.connected()?1:0));
    else if(cld==CLD_OTC) append_key_value(json, "clds", (int16_t)(otf->getCloudStatus()));
    else append_key_value(json, "clds", (int16_t)0);
  }
  json += get_zone_names_json();
  json += "}";
  otf_send_json(res, json);
}

void on_sta_logs(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  String json = "{";
  append_key_value(json, "name", osb.options[OPTION_NAME].sval);
  
  json += F("\"logs\":[");
  if(!osb.read_log_start()) {
    json += F("]}");
    otf_send_json(res, json);
    return;
  }
  LogStruct l;
  bool remove_comma = false;
  for(uint16_t i=0;i<MAX_LOG_RECORDS;i++) {
    if(!osb.read_log_next(l)) break;
    if(!l.tstamp) continue;
    json += "[";
    json += l.tstamp;
    json += ",";
    json += l.dur;
    json += ",";
    json += l.event;
    json += ",";
    json += l.zid;
    json += ",";
    json += l.pid;
    json += ",";
    json += l.tid;
    json += "],";
    remove_comma = true;
  }
  osb.read_log_end();
  if(remove_comma) json.remove(json.length()-1); // remove the extra ,
  json += F("],");
  json += get_zone_names_json();
  json += "}";
  otf_send_json(res, json);
}

void on_sta_change_controller(const OTF::Request &req, OTF::Response &res) {
	if(!verify_dkey(req)) {
		otf_send_result(res, HTML_UNAUTHORIZED, nullptr);
		return;
	}
  
  if(req.getQueryParameter("reset")!=NULL) {
    otf_send_result(res, HTML_SUCCESS, nullptr);
    reset_zones();
  }
  if(req.getQueryParameter("reboot")!=NULL) {
    otf_send_result(res, HTML_SUCCESS, nullptr);
    restart_in(1000);
  }
}

// convert absolute remainder (reference time 1970 01-01) to relative remainder (reference time today)
// absolute remainder is stored in eeprom, relative remainder is presented to web
void drem_to_relative(byte days[2]) {
  byte rem_abs=days[0];
  byte inv=days[1];
  days[0] = (byte)((rem_abs + inv - (osb.curr_loc_time()/SECS_PER_DAY) % inv) % inv);
}

// relative remainder -> absolute remainder
void drem_to_absolute(byte days[2]) {
  byte rem_rel=days[0];
  byte inv=days[1];
  days[0] = (byte)(((osb.curr_loc_time()/SECS_PER_DAY) + rem_rel) % inv);
}

long parse_listdata(const String& s, uint16_t& pos) {
  uint16_t p;
  char tmp[13];
  tmp[0]=0;
  // copy to tmp until a non-number is encountered
  for(p=pos;p<pos+12;p++) {
    char c=s.charAt(p);
    if (c=='-' || c=='+' || (c>='0'&&c<='9'))
      tmp[p-pos] = c;
    else
      break;
  }
  tmp[p-pos]=0;
  pos = p+1;
  return atol(tmp);
}

// from here
void on_sta_change_program(const OTF::Request &req, OTF::Response &res) {
  if(!verify_dkey(req))  return;
  int16_t pid = get_pid(req, res);
  if(!(pid>=-1 && pid<pd.nprogs)) {
    otf_send_result(res, HTML_DATA_OUTOFBOUND, "pid");
    return;
  }
  
  ProgramStruct prog;
  long v;
  if(get_value_by_key(req, "config", v)) {
    // parse config bytes
    prog.enabled = v&0x01;
    prog.daytype = (v>>1)&0x01;
    prog.restr   = (v>>2)&0x03;
    prog.sttype  = (v>>4)&0x03;
    prog.days[0] = (v>>8)&0xFF;
    prog.days[1] = (v>>16)&0xFF;    
    if(prog.daytype == DAY_TYPE_INTERVAL && prog.days[1] > 1) {
      drem_to_absolute(prog.days);
    }
  } else {
    otf_send_result(res, HTML_DATA_MISSING, "config");
    return;
  }
  
  String sv;
  if(get_value_by_key(req, "sts", sv)) {
    // parse start times
    uint16_t pos=1;
    byte i;
    for(i=0;i<MAX_NUM_STARTTIMES;i++) {
      prog.starttimes[i] = parse_listdata(sv, pos);
    }
    if(prog.starttimes[0] < 0) {
      otf_send_result(res, HTML_DATA_OUTOFBOUND, "sts[0]");
      return;
    }
  } else {
    otf_send_result(res, HTML_DATA_MISSING, "sts");
    return;
  }
  
  if(get_value_by_key(req, "nt", v)) {
    if(!(v>0 && v<MAX_NUM_TASKS)) {
      otf_send_result(res, HTML_DATA_OUTOFBOUND, "nt");
      return;
    }
    prog.ntasks = v;
  } else {
    otf_send_result(res, HTML_DATA_MISSING, "nt");
    return;
  }
  
  if(get_value_by_key(req, "pt", sv)) {
    byte i=0;
    uint16_t pos=1;
    for(i=0;i<prog.ntasks;i++) {
      ulong e = parse_listdata(sv, pos);
      prog.tasks[i].zbits = e&0xFF;
      prog.tasks[i].dur = e>>8;
    }
  } else {
    otf_send_result(res, HTML_DATA_MISSING, "pt");
    return;
  }
  
  if(!get_value_by_key(req, "name", sv)) {
    sv = "Program ";
    sv += (pid==-1) ? (pd.nprogs+1) : (pid+1);
  }
  strncpy(prog.name, sv.c_str(), PROGRAM_NAME_SIZE);
  prog.name[PROGRAM_NAME_SIZE-1]=0;
  
  if(pid==-1) pd.add(&prog);
  else pd.modify(pid, &prog);
  
  otf_send_result(res, HTML_SUCCESS, nullptr);
}

void on_sta_delete_program(const OTF::Request &req, OTF::Response &res) {
  if(!verify_dkey(req)) {
    otf_send_result(res, HTML_UNAUTHORIZED, nullptr);
    return;
  }
  int16_t pid = get_pid(req, res);
  if(!(pid>=-1 && pid<pd.nprogs)) { otf_send_result(res, HTML_DATA_OUTOFBOUND, "pid"); return; }
    
  if(pid==-1) pd.eraseall();
  else pd.del(pid);
  otf_send_result(res, HTML_SUCCESS, nullptr);
}

void on_sta_run_program(const OTF::Request &req, OTF::Response &res) {
  if(!verify_dkey(req)) {
    otf_send_result(res, HTML_UNAUTHORIZED, nullptr);
    return;
  }
  int16_t pid = get_pid(req, res);
  long v=0;
  switch(pid) {
  case MANUAL_PROGRAM_INDEX:
    {
      if(get_value_by_key(req, "zbits", v)) {
        byte zbits = v;
        if(get_value_by_key(req, "dur", v)) start_manual_program(zbits, (uint16_t)v);
        else { otf_send_result(res, HTML_DATA_MISSING, "dur"); return; }
      } else { otf_send_result(res, HTML_DATA_MISSING, "zbits"); return; }
    }
    break;
  
  case QUICK_PROGRAM_INDEX:
    {
      String sv;
      if(get_value_by_key(req, "durs", sv)) {
        uint16_t pos=1;
        uint16_t durs[MAX_NUMBER_ZONES];
        bool valid=false;
        for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
          durs[i] = (uint16_t)parse_listdata(sv, pos);
          if(durs[i]) valid=true;
        }
        if(!valid) {otf_send_result(res, HTML_DATA_OUTOFBOUND, "durs"); return; }
        else start_quick_program(durs);
      } else { otf_send_result(res, HTML_DATA_MISSING, "durs"); return; }
    }
    break;
  
  case TESTZONE_PROGRAM_INDEX:
    {
      if(get_value_by_key(req, "zid", v)) {
        byte zid=v;
        if(get_value_by_key(req, "dur", v))  start_testzone_program(zid, (uint16_t)v);
        else { otf_send_result(res, HTML_DATA_MISSING, "dur"); return; }
      } else { otf_send_result(res, HTML_DATA_MISSING, "zid"); return; }     
    }
    break;
  
  default:
    {
      if(!(pid>=0 && pid<pd.nprogs)) { otf_send_result(res, HTML_DATA_OUTOFBOUND, "pid"); return; }
      else start_program(pid);
    }
  }
  otf_send_result(res, HTML_UNAUTHORIZED, nullptr);
}

void on_sta_change_options(const OTF::Request &req, OTF::Response &res) {
  if(!verify_dkey(req)) {
    otf_send_result(res, HTML_UNAUTHORIZED, nullptr);
    return;
  }
  String sval;
  byte i;
  OptionStruct *o = osb.options;
  
  // FIRST ROUND: check option validity
  // do not save option values yet
  for(i=0;i<NUM_OPTIONS;i++,o++) {
    const char *key = o->name.c_str();
    // these options cannot be modified here
    if(i==OPTION_FWV || i==OPTION_MOD  || i==OPTION_SSID ||
      i==OPTION_PASS || i==OPTION_DKEY)
      continue;
    
    if(o->max) {  // integer options
      char *sval = req.getQueryParameter(key);
      if(sval!=NULL) {
        long ival = String(sval).toInt();
        if(ival>o->max) {
          otf_send_result(res, HTML_DATA_OUTOFBOUND, key);
          return;
        }
      }
    }
  }
  
  
  // Check device key change
  const char* _nkey = "nkey";
  const char* _ckey = "ckey";

  char* nkey = req.getQueryParameter(_nkey);
  char* ckey = req.getQueryParameter(_ckey);

  if(nkey!=NULL) {
    if(ckey!=NULL) {
      if(strcmp(nkey,ckey)!=0) {
        otf_send_result(res, HTML_MISMATCH, _ckey);
        return;
      }
    } else {
      otf_send_result(res, HTML_DATA_MISSING, _ckey);
      return;
    }
  }
  
  // SECOND ROUND: change option values
  o = osb.options;
  for(i=0;i<NUM_OPTIONS;i++,o++) {
    const char *key = o->name.c_str();
    // these options cannot be modified here
    if(i==OPTION_FWV || i==OPTION_MOD  || i==OPTION_SSID ||
      i==OPTION_PASS || i==OPTION_DKEY)
      continue;
    
    char *sval = req.getQueryParameter(key);
    if(sval!=NULL) {
      if(o->max) {  // integer options
        long ival = String(sval).toInt();
        o->ival = ival;
      } else {
        o->sval = String(sval);
      }
    }
  }

  if(nkey!=NULL) {
    osb.options[OPTION_DKEY].sval = nkey;
  }

  osb.options_save();
  otf_send_result(res, HTML_SUCCESS, nullptr);
}

void on_sta_options(const OTF::Request &req, OTF::Response &res) {
  if(curr_mode == OSB_MOD_AP) return;
  String json = "{";
  OptionStruct *o = osb.options;
  for(byte i=0;i<NUM_OPTIONS;i++,o++) {
    if(!o->max) {
      if(i==OPTION_NAME || i==OPTION_AUTH || i==OPTION_CDMN) {  // only output selected string options
        append_key_value(json, o->name.c_str(), o->sval);
      }
    } else {  // if this is a int option
      if(osb.version==2 && (i==OPTION_BSVO || i==OPTION_BSVC)) continue; // only version 3 and above has bsvo and bsvc option
      if(osb.version==3 && (i==OPTION_BSVC && osb.options[OPTION_SOT].ival == OSB_SOT_NONLATCH)) continue; // if valve type is nonlatch then don't send bsvc option
      append_key_value(json, o->name.c_str(), (ulong)o->ival);
    }
  }
  // output zone names
  json += get_zone_names_json();
  json += "}";
  otf_send_json(res, json);
}

void on_sta_program(const OTF::Request &req, OTF::Response &res) {
  String json = "{";
  append_key_value(json, "tmz",  (int16_t)osb.options[OPTION_TMZ].ival);
  json += F("\"progs\":[");
  ulong v;
  ProgramStruct prog;
  bool remove_comma = false;
  for(byte pid=0;pid<pd.nprogs;pid++) {
    json += "{";
    pd.read(pid, &prog);

    v = *(byte*)(&prog);  // extract the first byte
    if(prog.daytype == DAY_TYPE_INTERVAL) {
      drem_to_relative(prog.days);
    }
    v |= ((ulong)prog.days[0]<<8);
    v |= ((ulong)prog.days[1]<<16);
    append_key_value(json, "config", (ulong)v);

    json += F("\"sts\":[");
    byte i;
    for(i=0;i<MAX_NUM_STARTTIMES;i++) {
      json += prog.starttimes[i];
      json += ",";
    }
    json.remove(json.length()-1);
    json += "],";
    
    append_key_value(json, "nt", (int16_t)prog.ntasks);
    
    json += F("\"pt\":[");
    for(i=0;i<prog.ntasks;i++) {
      v = prog.tasks[i].zbits;
      v |= ((long)prog.tasks[i].dur<<8);
      json += v;
      json += ",";
    }
    json.remove(json.length()-1);
    json += "],";
    
    append_key_value(json, "name", prog.name);
    json.remove(json.length()-1);  
    json += F("},");
    remove_comma = true;
  }
  if(remove_comma) json.remove(json.length()-1);  
  json += "]}";
  otf_send_json(res, json);
}

void on_ap_update(const OTF::Request &req, OTF::Response &res) {
  otf_send_html_P(res, (const __FlashStringHelper *) ap_update_html);
}

void on_sta_update(const OTF::Request &req, OTF::Response &res) {
  if(req.isCloudRequest()) otf_send_result(res, HTML_NOT_PERMITTED, "fw update");
  else otf_send_html_P(res, (const __FlashStringHelper *) update_html);
}

void on_sta_upload_fin() {
  // Verify the device key.
  if(!(updateServer->hasArg("dkey") && (updateServer->arg("dkey") == osb.options[OPTION_DKEY].sval))) {
    updateserver_send_result(HTML_UNAUTHORIZED);
    Update.end(false); // Update.reset(); FAB
    return;
  }

  // finish update and check error
  if(!Update.end(true) || Update.hasError()) {
    updateserver_send_result(HTML_UPLOAD_FAILED);
    return;
  }
  
  updateserver_send_result(HTML_SUCCESS);
  restart_in(1000);
}

void on_ap_upload_fin() { on_sta_upload_fin(); }

void on_sta_upload() {
  HTTPUpload& upload = updateServer->upload();
  if(upload.status == UPLOAD_FILE_START){
    WiFiUDP::stopAll();
    if(Blynk.connected()) Blynk.disconnect();
    DEBUG_PRINT(F("prepare to upload: "));
    DEBUG_PRINTLN(upload.filename);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000;
    if(!Update.begin(maxSketchSpace)) {
      DEBUG_PRINTLN(F("not enough space"));
    }
    
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    static int count = 0;
    count++;
    if(count%10==0) DEBUG_PRINT(".");
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      DEBUG_PRINTLN(F("size mismatch"));
    }
      
  } else if(upload.status == UPLOAD_FILE_END) {
    
    DEBUG_PRINTLN(F("upload completed"));
   
  } else if(upload.status == UPLOAD_FILE_ABORTED){
    Update.end();
    DEBUG_PRINTLN(F("upload aborted"));
  }
  delay(0);
}

void on_ap_upload() {
	HTTPUpload& upload = updateServer->upload();
	if(upload.status == UPLOAD_FILE_START){
		DEBUG_PRINTLN(F("prepare to upload: "));
		DEBUG_PRINTLN(upload.filename);
		uint32_t maxSketchSpace = (ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000;
		if(!Update.begin(maxSketchSpace)) {
			DEBUG_PRINTLN(F("not enough space"));
		}
	} else if(upload.status == UPLOAD_FILE_WRITE) {
    static int count = 0;
    count++;
    if(count%10==0) DEBUG_PRINT(".");
		if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
			DEBUG_PRINTLN(F("size mismatch"));
		}
	} else if(upload.status == UPLOAD_FILE_END) {
		DEBUG_PRINTLN(F("upload completed"));
	} else if(upload.status == UPLOAD_FILE_ABORTED){
		Update.end();
		DEBUG_PRINTLN(F("upload aborted"));
	}
	delay(0);
}


void on_sta_delete_log(const OTF::Request &req, OTF::Response &res) {
  if(!verify_dkey(req)) {
    otf_send_result(res, HTML_UNAUTHORIZED, nullptr);
    return;
  }
  osb.log_reset();
  otf_send_result(res, HTML_SUCCESS, nullptr);
}

const char* weekday_name(ulong t) {
  t /= 86400L;
  t = (t+3) % 7;  // Jan 1, 1970 is a Thursday
  static const char* weekday_names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  return weekday_names[t];
}

void reset_zones() {
  DEBUG_PRINTLN("reset zones");
  osb.clear_zbits();
  osb.apply_zbits();
  pd.reset_runtime();
}

void start_testzone_program(byte zid, uint16_t dur) {
  if(zid>=MAX_NUMBER_ZONES) return;
  pd.reset_runtime();
  TaskStruct *e = &pd.manual_tasks[0];
  e->zbits = (1<<zid);
  e->dur=dur;
  pd.scheduled_stop_times[0]=curr_utc_time+dur;
  pd.curr_prog_index = TESTZONE_PROGRAM_INDEX;
  pd.scheduled_ntasks=1;
  osb.program_busy=1;
}

void start_manual_program(byte zbits, uint16_t dur) {
  pd.reset_runtime();
  TaskStruct *e = &pd.manual_tasks[0];
  e->zbits = zbits;
  e->dur=dur;
  pd.scheduled_stop_times[0]=curr_utc_time+dur;
  pd.curr_prog_index = MANUAL_PROGRAM_INDEX;
  pd.scheduled_ntasks=1;
  osb.program_busy=1;
}

void schedule_run_program() {
  byte tid;
  ulong start_time = curr_utc_time;
  for(tid=0;tid<pd.ntasks;tid++) {
    pd.scheduled_stop_times[tid]=start_time+pd.scheduled_stop_times[tid];
    start_time=pd.scheduled_stop_times[tid];
  }
  pd.scheduled_ntasks = pd.ntasks;
  osb.program_busy=1;
}

void start_quick_program(uint16_t durs[]) {
  pd.reset_runtime();
  TaskStruct *e = pd.manual_tasks;
  byte nt = 0;
  ulong start_time = curr_utc_time;
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    if(durs[i]) {
      e[i].zbits = (1<<i);
      e[i].dur = durs[i];
      pd.scheduled_stop_times[i]=start_time+e[i].dur;
      start_time=pd.scheduled_stop_times[i];
      nt++;
    }
  }
  if(nt>0) {
    pd.curr_prog_index = QUICK_PROGRAM_INDEX;
    pd.scheduled_ntasks=nt;
    osb.program_busy=1;
  } else {
    pd.reset_runtime();
  }
}

void start_program(byte pid) {
  ProgramStruct prog;
  byte tid;
  uint16_t dur;
  if(pid>=pd.nprogs) return;
  pd.reset_runtime();
  pd.read(pid, &prog);
  if(!prog.ntasks) return;
  for(tid=0;tid<prog.ntasks;tid++) {
    dur=prog.tasks[tid].dur;
    pd.scheduled_stop_times[tid]=dur;
  }
  pd.curr_prog_index=pid;
  schedule_run_program();
}

void do_setup()
{
  DEBUG_BEGIN(115200);
  if(otf) {
  	delete otf;
  	otf = NULL;
  }
  if(updateServer) {
    delete updateServer;
    updateServer = NULL;
  }
  WiFi.persistent(false); // turn off persistent, fixing flash crashing issue  
  osb.begin();
  osb.options_setup();
  // close all zones at the beginning.
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    osb.close(i);
  }  
  pd.init();
  //curr_cloud_access_en = osb.get_cloud_access_en();
  curr_mode = osb.get_mode();
	if(!otf) {
		const String otfDeviceKey = osb.options[OPTION_AUTH].sval;

		if((osb.options[OPTION_CLD].ival==CLD_OTC) && (otfDeviceKey.length() >= 32)) {
			// Initialize with remote connection if a device key was specified.
			otf = new OTF::OpenThingsFramework(osb.options[OPTION_HTP].ival, osb.options[OPTION_CDMN].sval, osb.options[OPTION_CPRT].ival, otfDeviceKey, false);
			DEBUG_PRINTLN(F("Started OTF with remote connection"));
		} else {
			// Initialize just the local server if no device key was specified.
			otf = new OTF::OpenThingsFramework(osb.options[OPTION_HTP].ival);
			DEBUG_PRINTLN(F("Started OTF with just local connection"));
		}
    if(curr_mode==OSB_MOD_AP) dns = new DNSServer();
		DEBUG_PRINT(F("server started @ "));
		DEBUG_PRINTLN(osb.options[OPTION_HTP].ival);
	}  
  if(!updateServer) {
    updateServer = new ESP8266WebServer(8080);
    DEBUG_PRINT(F("update server started @ "));
  }
  led_blink_ms = LED_FAST_BLINK;
  idle_countdown = LCD_DIMMING_TIMEOUT;
}

void process_button() {
  // process button
  static ulong button_down_time = 0;
  if(osb.get_button() == LOW) {
    idle_countdown = LCD_DIMMING_TIMEOUT; // if button is clicked, turn on lcd brightness right away
    osb.lcd_set_brightness(100);
    if(!button_down_time) {
      button_down_time = millis();
    } else {
      ulong curr = millis();
      if(curr > button_down_time + BUTTON_FACRESET_TIMEOUT) {
        static bool once = false;
        // signal reset
        if(!once) {
		      disp.clear();
		      osb.boldFont(true);
		      disp.drawString(0, 0, "Factory Reset...");
		      disp.display();
		      led_blink_ms = 0;
		      osb.set_led(HIGH);
		      once = true;
		      osb.state = OSB_STATE_WAIT_RESTART;
		    }
      } else if (curr > button_down_time + BUTTON_WIFIRESET_TIMEOUT) {
        static bool once = false;
        if(!once) {
		      disp.clear();
		      osb.boldFont(true);
		      disp.drawString(0, 0, "Reset WiFi...");
		      disp.display();
		      led_blink_ms = 0;
		      osb.set_led(HIGH);
		      once = true;
		      osb.state = OSB_STATE_WAIT_RESTART;
        }
      }
    }
  } else {
    if (button_down_time > 0) {
      ulong curr = millis();
      if(curr > button_down_time + BUTTON_FACRESET_TIMEOUT) {
        osb.state = OSB_STATE_RESET;
      } else if(curr>button_down_time + BUTTON_WIFIRESET_TIMEOUT) {
        osb.state = OSB_STATE_WIFIRESET;
      }else if(curr > button_down_time + 50) {
        disp_mode = (disp_mode+1) % NUM_DISP_MODES;
      }
      button_down_time = 0;
    }
  } 
  // process led
  static ulong led_toggle_timeout = 0;
  if(led_blink_ms) {
    if(millis() > led_toggle_timeout) {
      // toggle led
      osb.toggle_led();
      led_toggle_timeout = millis() + led_blink_ms;
    }
  }
}

void check_status() {
  static ulong checkstatus_timeout = 0;
  if(curr_utc_time > checkstatus_timeout) {
    if(osb.options[OPTION_CLD].ival==CLD_BLYNK && Blynk.connected()) {
      byte i, zbits;
      for(i=0;i<MAX_NUMBER_ZONES;i++) {
        zbits = osb.curr_zbits;
        if((zbits>>i)&1) blynk_leds[i]->on();
        else blynk_leds[i]->off();
      }
      if(osb.program_busy) {
        String str = F("Prog ");
        if(pd.curr_prog_index==TESTZONE_PROGRAM_INDEX) {
          str += F("T");
        } else if(pd.curr_prog_index==QUICK_PROGRAM_INDEX) {
          str += F("Q");
        } else if(pd.curr_prog_index==MANUAL_PROGRAM_INDEX) {
          str += F("M");
        } else {
          str += (pd.curr_prog_index+1);
        }
        str += F(": ");        
        str += toHMS(pd.curr_prog_remaining_time);
        blynk_lcd.print(0, 0, str);
        
        str = F("Task ");
        str += (pd.curr_task_index+1);
        str += F(": ");
        str += toHMS(pd.curr_task_remaining_time);
        blynk_lcd.print(0, 1, str);
      } else {
        blynk_lcd.print(0, 0, "[Idle]          ");
        blynk_lcd.print(0, 1, get_ip(WiFi.localIP())+F("      "));
      }
    }
    if(osb.program_busy)  checkstatus_timeout = curr_utc_time + 2;  // when program is running, update more frequently
    else checkstatus_timeout = curr_utc_time + 5;
  }
}


static const byte stnx[]={10,45,80};
static const byte stny[]={23,23,23,23};
#define DELTAX 6
#define DELTAY 8
void process_display() {
  if(osb.state == OSB_STATE_RESET || osb.state == OSB_STATE_WAIT_RESTART || osb.state == OSB_STATE_WIFIRESET)  return;
  // handle idle brightness
  if(!idle_countdown) osb.lcd_set_brightness(osb.options[OPTION_DIM].ival);
  // display current time
  disp.clear();
  if(curr_utc_time < MIN_EPOCH_TIME)
    disp.drawString(0, 0, "NTP syncing...");
  else {
    ulong t = osb.curr_loc_time();      
    String stime = toHMS(t%86400L);
    disp.drawString(0, 0, stime);
    stime = " ";
    stime += weekday_name(t);
    stime += "  "+two_digits(month(t))+"-"+two_digits(day(t));
    disp.drawString(54, 0, stime);
  }
  
  // display zones
  if(!osb.curr_zbits) {
    disp.drawString(stnx[0],stny[0],"[all zones closed]");
  } else {
    byte i;
    for(i=0;i<MAX_NUMBER_ZONES;i++) {
      if((osb.curr_zbits>>i)&1) {
        String sn="Z";
        sn+=(i+1);
        disp.drawString(stnx[i], stny[i], sn);
        disp.drawCircle(stnx[i]+DELTAX, stny[i]+DELTAY,10+(curr_utc_time%3)*2);
      }
    }
  }  

  // display status bar
  String str;
  switch(disp_mode) {
    case DISP_MODE_IP:
      if(osb.state == OSB_STATE_CONNECTED)
        str = "IP: "+get_ip(WiFi.localIP());
      else
        str = "IP: connecting...";
      break;
    case DISP_MODE_MAC:
      str = "MAC: "+get_mac();
      break;
    case DISP_MODE_SIGNAL:
      str = "WiFi: ";
      str+=((int16_t)WiFi.RSSI());
      str+=" dBm";
      break;
    case DISP_MODE_FWV:
      {
      byte fwv = osb.options[OPTION_FWV].ival;
      str = "Firmware ver. ";
      str+=(fwv/100);
      str+=".";
      str+=((fwv/10)%10);
      str+=".";
      str+=(fwv%10);
      }
      break;
  }
  disp.drawString(0, 48, str);
  disp.display();
}

void time_keeping() {
  //static bool configured = false;
  static ulong prev_millis = 0;
  static ulong time_keeping_timeout;

	uint16_t port = osb.options[OPTION_HTP].ival;
	port = (port==8000) ? 8888:8000; // use a different port than http port

	if(!curr_utc_time || (curr_utc_time > time_keeping_timeout)) {
		ulong startt = millis();
		UDP *udp = new WiFiUDP();
		#define NTP_NTRIES 10
		#define NTP_PACKET_SIZE 48
		#define NTP_PORT 123
		#define N_PUBLIC_SERVERS 5
		
		static const char* public_ntp_servers[] = {
			"time.google.com",
			"time.nist.gov",
			"time.windows.com",
			"time.cloudflare.com",
			"pool.ntp.org" };
		static uint8_t sidx = 0;
			
		static byte packetBuffer[NTP_PACKET_SIZE];
		byte tries=0;
		ulong gt = 0;
		bool success = false;
		while(!success && tries<NTP_NTRIES) {
			// sendNtpPacket
			udp->begin(port);

			memset(packetBuffer, 0, NTP_PACKET_SIZE);
			packetBuffer[0] = 0b11100011;		// LI, Version, Mode
			packetBuffer[1] = 0;		 // Stratum, or type of clock
			packetBuffer[2] = 6;		 // Polling Interval
			packetBuffer[3] = 0xEC;  // Peer Clock Precision
			// 8 bytes of zero for Root Delay & Root Dispersion
			packetBuffer[12]	= 49;
			packetBuffer[13]	= 0x4E;
			packetBuffer[14]	= 49;
			packetBuffer[15]	= 52;
			
			// use one of the public NTP servers if ntp ip is unset
			
			DEBUG_PRINT(F("ntp: "));
			int ret;
			DEBUG_PRINT(public_ntp_servers[sidx]);
			ret = udp->beginPacket(public_ntp_servers[sidx], NTP_PORT);
			if(ret!=1) {
				DEBUG_PRINT(F(" not available (ret: "));
				DEBUG_PRINT(ret);
				DEBUG_PRINTLN(")");
				udp->stop();
				tries++;
				sidx=(sidx+1)%N_PUBLIC_SERVERS;
				continue;
			} else {
				DEBUG_PRINTLN(F(" connected"));
			}
			udp->write(packetBuffer, NTP_PACKET_SIZE);
			udp->endPacket();
			// end of sendNtpPacket
			
			// process response
			ulong timeout = millis()+2000;
			while(millis() < timeout) {
				if(udp->parsePacket()) {
					udp->read(packetBuffer, NTP_PACKET_SIZE);
					ulong highWord = word(packetBuffer[40], packetBuffer[41]);
					ulong lowWord = word(packetBuffer[42], packetBuffer[43]);
					ulong secsSince1900 = highWord << 16 | lowWord;
					ulong seventyYears = 2208988800UL;
					ulong gt = secsSince1900 - seventyYears;
					// check validity: has to be larger than 1/1/2020 12:00:00
					if(gt>1577836800UL) {
						curr_utc_time = gt;
						time_keeping_timeout = curr_utc_time + TIME_SYNC_TIMEOUT;
						prev_millis = millis();
						success=true;
						DEBUG_PRINT(F("took "));
						DEBUG_PRINTLN((millis()-startt));
						break;
					}
				}
			}
			tries++;
			udp->stop();
			sidx=(sidx+1)%N_PUBLIC_SERVERS;
		} 
		if(tries==NTP_NTRIES) {
			DEBUG_PRINTLN(F("NTP failed!!"));
			time_keeping_timeout = curr_utc_time + 60;
		}
		udp->stop();
		delete udp;
	}
	
/*  static ulong next_timekeeping = 0;
  static ulong next_readtime = 0;
  static ulong next_readtime_timeout = 0;

  if(!configured) {
    configTime(0, 0, DEFAULT_NTP1, DEFAULT_NTP2, DEFAULT_NTP3);
    configured = true;
    next_readtime = curr_utc_time + 2; // attempt to call time() in 2 seconds
    next_readtime_timeout = next_readtime + 30; // bound attempts to 30 seconds after initial call
    DEBUG_PRINTLN(time(nullptr));
  }

  if(!curr_utc_time || (curr_utc_time > time_keeping_timeout)) {
  	byte tick = 0;
  	ulong gt = 0;
  	ulong timeout = millis()+30000;
  	do {
  		gt = time(nullptr);
  		delay(2000);
  	} while(gt<1577836800UL && millis()<timeout);
    if(gt<1577836800UL) {
      // if we didn't get response, re-try after 2 seconds
      DEBUG_PRINTLN(F("ntp invalid! re-try in 60 seconds"));
      time_keeping_timeout = curr_utc_time + 60;
    } else {
    	DEBUG_PRINT(F("took "));
    	DEBUG_PRINTLN(30000-(timeout-millis()));
      curr_utc_time = gt;
      DEBUG_PRINT(F("Updated time from NTP: "));
      DEBUG_PRINTLN(curr_utc_time);
      // if we got a response, re-try after TIME_SYNC_TIMEOUT seconds
      time_keeping_timeout = curr_utc_time + TIME_SYNC_TIMEOUT;
      prev_millis = millis();
    }
  }
*/
  while(millis() - prev_millis >= 1000) {
    curr_utc_time ++;
    prev_millis += 1000;
  }
}
void do_loop() {
  static ulong connecting_timeout;
  
  switch(osb.state) {
  case OSB_STATE_INITIAL:
    if(curr_mode == OSB_MOD_AP) {
      scanned_ssids = scan_network();
      String ap_ssid = get_ap_ssid();
      start_network_ap(ap_ssid.c_str(), NULL);
      delay(500);
      dns->setErrorReplyCode(DNSReplyCode::NoError);
      dns->start(53, "*", WiFi.softAPIP());
      otf->on("/",   on_home);    
      otf->on("/js", on_ap_scan);
      otf->on("/cc", on_ap_change_config);
      otf->on("/jt", on_ap_try_connect);
      otf->on("/update", on_ap_update, OTF::HTTP_GET);
      updateServer->on("/update", HTTP_POST, on_ap_upload_fin, on_ap_upload);
      //otf->onMissingPage(on_home);
      updateServer->begin();
      osb.state = OSB_STATE_CONNECTED;
      DEBUG_PRINTLN(WiFi.softAPIP());
      disp.clear();
      osb.boldFont(true);
      disp.drawString(0, 0, "AP Mode");
      osb.boldFont(false);
      disp.drawString(0, 16, "SSID: "+ap_ssid);
      disp.drawString(0, 32, "APIP: "+get_ip(WiFi.softAPIP()));
      disp.display();
      connecting_timeout =  0;
    } else {
      led_blink_ms = LED_SLOW_BLINK;
      //osb.set_led(LOW);
      WiFi.mode(WIFI_STA);
      start_network_sta(osb.options[OPTION_SSID].sval.c_str(), osb.options[OPTION_PASS].sval.c_str());
      osb.state = OSB_STATE_CONNECTING;
      connecting_timeout = millis() + 60000;
    }
    break;
  case OSB_STATE_TRY_CONNECT:
    led_blink_ms = LED_SLOW_BLINK;  
    start_network_sta_with_ap(osb.options[OPTION_SSID].sval.c_str(), osb.options[OPTION_PASS].sval.c_str());
    osb.state = OSB_STATE_CONNECTED;
    break;
    
  case OSB_STATE_CONNECTING:
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT(F("Wireless connected, IP: "));
      DEBUG_PRINTLN(WiFi.localIP());
      time_keeping();
      otf->on("/", on_home);
      otf->on("/index", on_home);
      otf->on("/settings", on_sta_view_options);
      otf->on("/log", on_sta_view_logs);
      otf->on("/manual", on_sta_view_manual);
      otf->on("/program", on_sta_view_program);
      otf->on("/preview", on_sta_view_preview);
			otf->on("/update", on_sta_update, OTF::HTTP_GET);
			updateServer->on("/update", HTTP_POST, on_sta_upload_fin, on_sta_upload);
      otf->on("/jc", on_sta_controller);
      otf->on("/jo", on_sta_options);
      otf->on("/jl", on_sta_logs);
      otf->on("/jp", on_sta_program);
      otf->on("/cc", on_sta_change_controller);
      otf->on("/co", on_sta_change_options);
      otf->on("/cp", on_sta_change_program);
      otf->on("/dp", on_sta_delete_program);
      otf->on("/rp", on_sta_run_program);
      otf->on("/dl", on_sta_delete_log);
      updateServer->begin();

      String host = get_ap_ssid();
      if(MDNS.begin(host.c_str(), WiFi.localIP())) {
        MDNS.addService("http", "tcp", osb.options[OPTION_HTP].ival);
      }
      if(osb.options[OPTION_CLD].ival==CLD_BLYNK && osb.options[OPTION_AUTH].sval.length()>=32) {
        if(osb.options[OPTION_CDMN].sval.length()==0) {
          Blynk.config(osb.options[OPTION_AUTH].sval.c_str()); // use default server
        } else {
          Blynk.config(osb.options[OPTION_AUTH].sval.c_str(), // use specified server
                       osb.options[OPTION_CDMN].sval.c_str(),
                       (uint16_t)osb.options[OPTION_CPRT].ival);
        }
        if(Blynk.connect()) {DEBUG_PRINTLN(F("Blynk connected"));}
        else {DEBUG_PRINTLN(F("Blynk failed!"));}
      }
      led_blink_ms = 0;
      osb.set_led(LOW);
      osb.state = OSB_STATE_CONNECTED;
      connecting_timeout = 0;
    } else {
      if(millis() > connecting_timeout) {
        osb.state = OSB_STATE_INITIAL;
        DEBUG_PRINTLN(F("timeout"));
        osb.restart();
      }
    }
    break;
  
  case OSB_STATE_CONNECTED:
    if(curr_mode == OSB_MOD_AP) {
      if(dns) dns->processNextRequest();
      if(otf) otf->loop();
      if(updateServer) updateServer->handleClient();
      connecting_timeout = 0;
      if(osb.options[OPTION_MOD].ival == OSB_MOD_STA) break; // already in STA mode, waiting to reboot
      if(WiFi.status()==WL_CONNECTED && WiFi.localIP()) {
        DEBUG_PRINTLN(F("STA connected, updating option file"));
				osb.options[OPTION_MOD].ival = OSB_MOD_STA;
				osb.options_save();
				restart_in(10000);
			}
    } else {
      if(WiFi.status() == WL_CONNECTED) {
        MDNS.update();
        time_keeping();
        check_status();
        if(otf) otf->loop();
        if(updateServer) updateServer->handleClient();
        if(osb.options[OPTION_CLD].ival==CLD_BLYNK)
          Blynk.run();
        connecting_timeout = 0;
      } else {
        if(!connecting_timeout) {
          DEBUG_PRINTLN(F("State is CONNECTED but WiFi is disconnected, start timeout counter."));
        	connecting_timeout = millis()+60000;
        } else {
        	if(millis()>connecting_timeout) {
        		osb.state = OSB_STATE_INITIAL;
        		DEBUG_PRINTLN(F("timeout reached, reboot"));
        		osb.restart();
        	}
        }
      }
    }
    break;
  
  case OSB_STATE_WAIT_RESTART:
    if(dns) dns->processNextRequest();
    if(otf) otf->loop();
    if(updateServer) updateServer->handleClient();
    break;
    
  case OSB_STATE_WIFIRESET:
    DEBUG_PRINTLN(F("WiFi reset"));
    osb.reset_to_ap();
    disp.drawString(0, 16, "done.");
    disp.drawString(0, 32, "Rebooting...");    
    disp.display();
    restart_in(1000);
    
  case OSB_STATE_RESET:
    DEBUG_PRINTLN(F("Fac reset"));
    osb.options_reset();
    osb.log_reset();
    osb.progs_reset();
    disp.drawString(0, 16, "done.");
    disp.drawString(0, 32, "Rebooting...");    
    disp.display();
    restart_in(1000);
    break;
  
  }

  process_button();
  
  static ulong last_time = 0;
  static ulong last_minute = 0;
  if(last_time != curr_utc_time) {
    last_time = curr_utc_time;
    if(idle_countdown) idle_countdown--;
    process_display();
    
    // ==== Schedule program data ===
    ulong curr_minute = curr_utc_time / 60;
    byte pid;
    ProgramStruct prog;
    if(curr_minute != last_minute) {
      last_minute = curr_minute;
      if(!osb.program_busy) {
        for(pid=0;pid<pd.nprogs;pid++) {
          pd.read(pid, &prog, true);
          if(prog.check_match(osb.curr_loc_time())) {
            //pd.read(pid, &prog);
            start_program(pid);
            break;
          }
        }
      }
    }
    // ==== Run program tasks ====
    // Check there is any program running currently
    // If do, do zone book keeping
    if(osb.program_busy) {
      // check stop time
      if(pd.curr_task_index==-1 ||
         curr_utc_time >= pd.scheduled_stop_times[pd.curr_task_index]) {
        // move on to the next task
        pd.curr_task_index++;
        if(pd.curr_task_index >= pd.scheduled_ntasks) {
          // the program is now over
          DEBUG_PRINTLN("program finished");
          reset_zones();
          osb.program_busy = 0;
        } else {
          TaskStruct e;
          pd.load_curr_task(&e);
          osb.next_zbits = e.zbits;
          pd.curr_prog_remaining_time = pd.scheduled_stop_times[pd.scheduled_ntasks-1]-curr_utc_time;      
          pd.curr_task_remaining_time = pd.scheduled_stop_times[pd.curr_task_index]-curr_utc_time;
        }
      } else {
        pd.curr_prog_remaining_time = pd.scheduled_stop_times[pd.scheduled_ntasks-1]-curr_utc_time;
        pd.curr_task_remaining_time = pd.scheduled_stop_times[pd.curr_task_index]-curr_utc_time;
      }
    }
    osb.apply_zbits();
  }
}

// Handle Blynk Requests
static byte  blynk_test_zone = 255;
static ulong blynk_test_dur = 60;
static byte  blynk_prog = 0;
BLYNK_WRITE(BLYNK_TEST_ZONE) {
  blynk_test_zone = param.asInt()-1;
  if(blynk_test_zone>=MAX_NUMBER_ZONES) blynk_test_zone=0;
}
BLYNK_WRITE(BLYNK_TEST_DUR){
  blynk_test_dur = 60L * param.asInt();
  if(!blynk_test_dur) blynk_test_dur = 60;
}
BLYNK_WRITE(BLYNK_RUN_TEST) {
  if(param.asInt()) {
    start_testzone_program(blynk_test_zone, blynk_test_dur);
  }
}
BLYNK_WRITE(BLYNK_RESET) {
  if(param.asInt()) {
    reset_zones();
  }
}
BLYNK_WRITE(BLYNK_PROG) {
  blynk_prog = param.asInt()-1;
  if(blynk_prog>=MAX_NUM_PROGRAMS)
    blynk_prog = 0;
}
BLYNK_WRITE(BLYNK_RUN_PROG) {
  if(param.asInt()) {
    start_program(blynk_prog);
  }
}
