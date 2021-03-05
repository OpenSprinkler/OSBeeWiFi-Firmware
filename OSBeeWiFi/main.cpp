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
#include "TimeLib.h"
#include "OSBeeWiFi.h"
#include "espconnect.h"
#include "program.h"

OSBeeWiFi osb;
ProgramData pd;
ESP8266WebServer *server = NULL;
SSD1306& disp = OSBeeWiFi::display;

static uint16_t led_blink_ms = LED_FAST_BLINK;

WidgetLED blynk_led1(BLYNK_S1);
WidgetLED blynk_led2(BLYNK_S2);
WidgetLED blynk_led3(BLYNK_S3);
WidgetLCD blynk_lcd(BLYNK_LCD);

WidgetLED *blynk_leds[] = {&blynk_led1, &blynk_led2, &blynk_led3};

static String scanned_ssids;
static bool curr_cloud_access_en = false;
static ulong restart_timeout = 0;
static byte disp_mode = DISP_MODE_IP;
static byte curr_mode;
static ulong& curr_utc_time = OSBeeWiFi::curr_utc_time;

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

void server_send_html(String html) {
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

bool get_value_by_key(const char* key, long& val) {
  if(server->hasArg(key)) {
    val = server->arg(key).toInt();   
    return true;
  } else {
    return false;
  }
}

bool get_value_by_key(const char* key, String& val) {
  if(server->hasArg(key)) {
    val = server->arg(key);   
    return true;
  } else {
    return false;
  }
}

void append_key_value(String& html, const char* key, const ulong value) {
  html += "\"";
  html += key;
  html += "\":";
  html += value;
  html += ",";
}

void append_key_value(String& html, const char* key, const int16_t value) {
  html += "\"";
  html += key;
  html += "\":";
  html += value;
  html += ",";
}

void append_key_value(String& html, const char* key, const String& value) {
  html += "\"";
  html += key;
  html += "\":\"";
  html += value;
  html += "\",";
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

bool verify_dkey() {
  if(curr_mode == OSB_MOD_AP) {
    server_send_result(HTML_WRONG_MODE);
    return false;
  }
  if(server->hasArg("dkey") && (server->arg("dkey") == osb.options[OPTION_DKEY].sval))
    return true;
  server_send_result(HTML_UNAUTHORIZED);
  return false;
}

int16_t get_pid() {
  if(curr_mode == OSB_MOD_AP) return -2;
  long v;
  if(get_value_by_key("pid", v)) {
    return v;
  } else {
    server_send_result(HTML_DATA_MISSING, "pid");
    return -2;
  }
}

void on_home()
{
  if(curr_mode == OSB_MOD_AP) {
    server_send_html(FPSTR(connect_html));
  } else {
    server_send_html(FPSTR(index_html));
  }
}

void on_sta_view_options() {
  if(curr_mode == OSB_MOD_AP) return;
  server_send_html(FPSTR(settings_html));
}

void on_sta_view_manual() {
  if(curr_mode == OSB_MOD_AP) return;
  server_send_html(FPSTR(manual_html));
}

void on_sta_view_logs() {
  if(curr_mode == OSB_MOD_AP) return;
  server_send_html(FPSTR(log_html));
}

void on_sta_view_program() {
  if(curr_mode == OSB_MOD_AP) return;
  server_send_html(FPSTR(program_html));
}

void on_sta_view_preview() {
  if(curr_mode == OSB_MOD_AP) return;
  server_send_html(FPSTR(preview_html));
}

void on_ap_scan() {
  if(curr_mode == OSB_MOD_STA) return;
  server_send_html(scanned_ssids);
}

void on_ap_change_config() {
  if(curr_mode == OSB_MOD_STA) return;
  if(server->hasArg("ssid")) {
    osb.options[OPTION_SSID].sval = server->arg("ssid");
    osb.options[OPTION_PASS].sval = server->arg("pass");
    osb.options[OPTION_AUTH].sval = server->arg("auth");
    if(osb.options[OPTION_SSID].sval.length() == 0) {
      server_send_result(HTML_DATA_MISSING, "ssid");
      return;
    }
    osb.options_save();
    server_send_result(HTML_SUCCESS);
    osb.state = OSB_STATE_TRY_CONNECT;
  }
}

void on_ap_try_connect() {
  if(curr_mode == OSB_MOD_STA) return;
  String html;
  html += "{";
  ulong ip = (WiFi.status()==WL_CONNECTED)?(uint32_t)WiFi.localIP():0;
  append_key_value(html, "ip", ip);
  html.remove(html.length()-1);  
  html += "}";
  server_send_html(html);
  if(WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
    server->handleClient();
    osb.options[OPTION_MOD].ival = OSB_MOD_STA;
    osb.options_save();  
    restart_timeout = millis() + 2000;
    osb.state = OSB_STATE_RESTART;
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

void on_sta_controller() {
  if(curr_mode == OSB_MOD_AP) return;
  String html;
  html += "{";
  append_key_value(html, "fwv", (int16_t)osb.options[OPTION_FWV].ival);
  append_key_value(html, "sot", (int16_t)osb.options[OPTION_SOT].ival);
  append_key_value(html, "utct", curr_utc_time);
  append_key_value(html, "pid", (int16_t)pd.curr_prog_index);
  append_key_value(html, "tid", (int16_t)pd.curr_task_index);
  append_key_value(html, "np",  (int16_t)pd.nprogs);
  append_key_value(html, "nt",  (int16_t)pd.scheduled_ntasks);
  append_key_value(html, "mnp", (int16_t)MAX_NUM_PROGRAMS);
  append_key_value(html, "prem",  pd.curr_prog_remaining_time);
  append_key_value(html, "trem",  pd.curr_task_remaining_time);
  append_key_value(html, "zbits", (int16_t)osb.curr_zbits);
  append_key_value(html, "name",  osb.options[OPTION_NAME].sval);
  append_key_value(html, "mac", get_mac());
  append_key_value(html, "cid", (ulong)ESP.getChipId());
  append_key_value(html, "rssi", (int16_t)WiFi.RSSI());
  html += get_zone_names_json();
  html += "}";
  server_send_html(html);
}

void on_sta_logs() {
  if(curr_mode == OSB_MOD_AP) return;
  String html = "{";
  append_key_value(html, "name", osb.options[OPTION_NAME].sval);
  
  html += F("\"logs\":[");
  if(!osb.read_log_start()) {
    html += F("]}");
    server_send_html(html);
    return;
  }
  LogStruct l;
  bool remove_comma = false;
  for(uint16_t i=0;i<MAX_LOG_RECORDS;i++) {
    if(!osb.read_log_next(l)) break;
    if(!l.tstamp) continue;
    html += "[";
    html += l.tstamp;
    html += ",";
    html += l.dur;
    html += ",";
    html += l.event;
    html += ",";
    html += l.zid;
    html += ",";
    html += l.pid;
    html += ",";
    html += l.tid;
    html += "],";
    remove_comma = true;
  }
  osb.read_log_end();
  if(remove_comma) html.remove(html.length()-1); // remove the extra ,
  html += F("],");
  html += get_zone_names_json();
  html += "}";
  server_send_html(html);
}

void on_sta_change_controller() {
  if(!verify_dkey())  return;
  
  if(server->hasArg("reset")) {
    server_send_result(HTML_SUCCESS);
    reset_zones();
  }
  if(server->hasArg("reboot")) {
    server_send_result(HTML_SUCCESS);
    restart_timeout = millis() + 1000;
    osb.state = OSB_STATE_RESTART;
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

void on_sta_change_program() {
  if(!verify_dkey())  return;
  int16_t pid = get_pid();
  if(!(pid>=-1 && pid<pd.nprogs)) {
    server_send_result(HTML_DATA_OUTOFBOUND, "pid");
    return;
  }
  
  ProgramStruct prog;
  long v;
  if(get_value_by_key("config", v)) {
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
    server_send_result(HTML_DATA_MISSING, "config");
    return;
  }
  
  String sv;
  if(get_value_by_key("sts", sv)) {
    // parse start times
    uint16_t pos=1;
    byte i;
    for(i=0;i<MAX_NUM_STARTTIMES;i++) {
      prog.starttimes[i] = parse_listdata(sv, pos);
    }
    if(prog.starttimes[0] < 0) {
      server_send_result(HTML_DATA_OUTOFBOUND, "sts[0]");
      return;
    }
  } else {
    server_send_result(HTML_DATA_MISSING, "sts");
    return;
  }
  
  if(get_value_by_key("nt", v)) {
    if(!(v>0 && v<MAX_NUM_TASKS)) {
      server_send_result(HTML_DATA_OUTOFBOUND, "nt");
      return;
    }
    prog.ntasks = v;
  } else {
    server_send_result(HTML_DATA_MISSING, "nt");
    return;
  }
  
  if(get_value_by_key("pt", sv)) {
    byte i=0;
    uint16_t pos=1;
    for(i=0;i<prog.ntasks;i++) {
      ulong e = parse_listdata(sv, pos);
      prog.tasks[i].zbits = e&0xFF;
      prog.tasks[i].dur = e>>8;
    }
  } else {
    server_send_result(HTML_DATA_MISSING, "pt");
    return;
  }
  
  if(!get_value_by_key("name", sv)) {
    sv = "Program ";
    sv += (pid==-1) ? (pd.nprogs+1) : (pid+1);
  }
  strncpy(prog.name, sv.c_str(), PROGRAM_NAME_SIZE);
  prog.name[PROGRAM_NAME_SIZE-1]=0;
  
  if(pid==-1) pd.add(&prog);
  else pd.modify(pid, &prog);
  
  server_send_result(HTML_SUCCESS);
}

void on_sta_delete_program() {
  if(!verify_dkey())  return;
  int16_t pid = get_pid();
  if(!(pid>=-1 && pid<pd.nprogs)) { server_send_result(HTML_DATA_OUTOFBOUND, "pid"); return; }
    
  if(pid==-1) pd.eraseall();
  else pd.del(pid);
  server_send_result(HTML_SUCCESS);
}

void on_sta_run_program() {
  if(!verify_dkey())  return;
  int16_t pid = get_pid();
  long v=0;
  switch(pid) {
  case MANUAL_PROGRAM_INDEX:
    {
      if(get_value_by_key("zbits", v)) {
        byte zbits = v;
        if(get_value_by_key("dur", v)) start_manual_program(zbits, (uint16_t)v);
        else { server_send_result(HTML_DATA_MISSING, "dur"); return; }
      } else { server_send_result(HTML_DATA_MISSING, "zbits"); return; }
    }
    break;
  
  case QUICK_PROGRAM_INDEX:
    {
      String sv;
      if(get_value_by_key("durs", sv)) {
        uint16_t pos=1;
        uint16_t durs[MAX_NUMBER_ZONES];
        bool valid=false;
        for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
          durs[i] = (uint16_t)parse_listdata(sv, pos);
          if(durs[i]) valid=true;
        }
        if(!valid) {server_send_result(HTML_DATA_OUTOFBOUND, "durs"); return; }
        else start_quick_program(durs);
      } else { server_send_result(HTML_DATA_MISSING, "durs"); return; }
    }
    break;
  
  case TESTZONE_PROGRAM_INDEX:
    {
      if(get_value_by_key("zid", v)) {
        byte zid=v;
        if(get_value_by_key("dur", v))  start_testzone_program(zid, (uint16_t)v);
        else { server_send_result(HTML_DATA_MISSING, "dur"); return; }
      } else { server_send_result(HTML_DATA_MISSING, "zid"); return; }     
    }
    break;
  
  default:
    {
      if(!(pid>=0 && pid<pd.nprogs)) { server_send_result(HTML_DATA_OUTOFBOUND, "pid"); return; }
      else start_program(pid);
    }
  }
  server_send_result(HTML_SUCCESS);
}

void on_sta_change_options() {
  if(!verify_dkey())  return;
  long ival = 0;
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
      if(get_value_by_key(key, ival)) {
        if(ival>o->max) {
          server_send_result(HTML_DATA_OUTOFBOUND, key);
          return;
        }
      }
    }
  }
  
  
  // Check device key change
  String nkey, ckey;
  const char* _nkey = "nkey";
  const char* _ckey = "ckey";
  
  if(get_value_by_key(_nkey, nkey)) {
    if(get_value_by_key(_ckey, ckey)) {
      if(!nkey.equals(ckey)) {
        server_send_result(HTML_MISMATCH, _ckey);
        return;
      }
    } else {
      server_send_result(HTML_DATA_MISSING, _ckey);
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
    
    if(o->max) {  // integer options
      if(get_value_by_key(key, ival)) {
        o->ival = ival;
      }
    } else {
      if(get_value_by_key(key, sval)) {
        o->sval = sval;
      }
    }
  }

  if(get_value_by_key(_nkey, nkey)) {
      osb.options[OPTION_DKEY].sval = nkey;
  }

  osb.options_save();
  server_send_result(HTML_SUCCESS);
}

void on_sta_options() {
  if(curr_mode == OSB_MOD_AP) return;
  String html = "{";
  OptionStruct *o = osb.options;
  for(byte i=0;i<NUM_OPTIONS;i++,o++) {
    if(!o->max) {
      if(i==OPTION_NAME || i==OPTION_AUTH) {  // only output selected string options
        append_key_value(html, o->name.c_str(), o->sval);
      }
    } else {  // if this is a int option
    	if(osb.version==2 && (i==OPTION_BSVO || i==OPTION_BSVC)) continue; // only version 3 and above has bsvo and bsvc option
    	if(osb.version==3 && (i==OPTION_BSVC && osb.options[OPTION_SOT].ival == OSB_SOT_NONLATCH)) continue; // if valve type is nonlatch then don't send bsvc option
      append_key_value(html, o->name.c_str(), (ulong)o->ival);
    }
  }
  // output zone names
  html += get_zone_names_json();
  html += "}";
  server_send_html(html);
}

void on_sta_program() {
  String html = "{";
  append_key_value(html, "tmz",  (int16_t)osb.options[OPTION_TMZ].ival);
  html += F("\"progs\":[");
  ulong v;
  ProgramStruct prog;
  bool remove_comma = false;
  for(byte pid=0;pid<pd.nprogs;pid++) {
    html += "{";
    pd.read(pid, &prog);

    v = *(byte*)(&prog);  // extract the first byte
    if(prog.daytype == DAY_TYPE_INTERVAL) {
      drem_to_relative(prog.days);
    }
    v |= ((ulong)prog.days[0]<<8);
    v |= ((ulong)prog.days[1]<<16);
    append_key_value(html, "config", (ulong)v);

    html += F("\"sts\":[");
    byte i;
    for(i=0;i<MAX_NUM_STARTTIMES;i++) {
      html += prog.starttimes[i];
      html += ",";
    }
    html.remove(html.length()-1);
    html += "],";
    
    append_key_value(html, "nt", (int16_t)prog.ntasks);
    
    html += F("\"pt\":[");
    for(i=0;i<prog.ntasks;i++) {
      v = prog.tasks[i].zbits;
      v |= ((long)prog.tasks[i].dur<<8);
      html += v;
      html += ",";
    }
    html.remove(html.length()-1);
    html += "],";
    
    append_key_value(html, "name", prog.name);
    html.remove(html.length()-1);  
    html += F("},");
    remove_comma = true;
  }
  if(remove_comma) html.remove(html.length()-1);  
  html += "]}";
  server_send_html(html);
}


void on_sta_update() {
  String html = FPSTR(update_html);
  server_send_html(html);
}

void on_sta_upload_fin() {
  if(!verify_dkey()) {
    Update.end(false);
    return;
  }

  // finish update and check error
  if(!Update.end(true) || Update.hasError()) {
    server_send_result(HTML_UPLOAD_FAILED);
    return;
  }
  
  server_send_result(HTML_SUCCESS);
  restart_timeout = millis();
  osb.state = OSB_STATE_RESTART;
}

void on_sta_upload() {
  HTTPUpload& upload = server->upload();
  if(upload.status == UPLOAD_FILE_START){
    WiFiUDP::stopAll();
    DEBUG_PRINT(F("prepare to upload: "));
    DEBUG_PRINTLN(upload.filename);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000;
    if(!Update.begin(maxSketchSpace)) {
      DEBUG_PRINTLN(F("not enough space"));
    }
    
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    DEBUG_PRINT(".");
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

void on_sta_delete_log() {
  if(!verify_dkey())  return;
  osb.log_reset();
  server_send_result(HTML_SUCCESS);
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
  if(server) {
    delete server;
    server = NULL;
  }
  WiFi.persistent(false); // turn off persistent, fixing flash crashing issue  
  osb.begin();
  osb.options_setup();
  // close all zones at the beginning.
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    osb.close(i);
  }  
  pd.init();
  curr_cloud_access_en = osb.get_cloud_access_en();
  curr_mode = osb.get_mode();
  if(!server) {
    server = new ESP8266WebServer(osb.options[OPTION_HTP].ival);
    DEBUG_PRINT(F("server started @ "));
    DEBUG_PRINTLN(osb.options[OPTION_HTP].ival);
  }
  led_blink_ms = LED_FAST_BLINK;  
}

void process_button() {
  // process button
  static ulong button_down_time = 0;
  if(osb.get_button() == LOW) {
    if(!button_down_time) {
      button_down_time = millis();
    } else {
      if(millis() > button_down_time + BUTTON_RESET_TIMEOUT) {
        // signal reset
        disp.clear();
        osb.boldFont(true);
        disp.drawString(0, 0, "Resetting...");
        disp.display();
        led_blink_ms = 0;
        osb.set_led(HIGH);
      }
    }
  } else {
    if (button_down_time > 0) {
      ulong curr = millis();
      if(curr > button_down_time + BUTTON_RESET_TIMEOUT) {
        osb.state = OSB_STATE_RESET;
      } else if(curr > button_down_time + 50) {
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
    if(curr_cloud_access_en && Blynk.connected()) {
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
  if(osb.state == OSB_STATE_RESET || osb.state == OSB_STATE_RESTART)  return;
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

extern "C" time_t time(time_t*);

void time_keeping() {
  static bool configured = false;
  static ulong prev_millis = 0;
  static ulong time_keeping_timeout = 0;

  if(!configured) {
    DEBUG_PRINTLN(F("config time server"));
    configTime(0, 0, "0.pool.ntp.org", "time.google.com", "1.pool.ntp.org");
    if (osb.has_rtc) {
      curr_utc_time = RTC.get();
    }
    configured = true;
  }

  if(curr_utc_time<MIN_EPOCH_TIME || curr_utc_time > time_keeping_timeout) {
    if (osb.has_rtc) curr_utc_time = RTC.get();  // get time from RTC
    if (osb.state == OSB_STATE_CONNECTED) {
    	byte tick = 0;
    	ulong gt = 0;
    	do {
    		gt = time(NULL);
    		tick++;
    		delay(2000);
    	} while(gt<MIN_EPOCH_TIME && tick<10);
      if(gt>MIN_EPOCH_TIME) {
        if (osb.has_rtc) RTC.set(gt);
        curr_utc_time = gt;
        DEBUG_PRINT(F("network time: "));
        DEBUG_PRINTLN(gt);
      } else {
      	DEBUG_PRINTLN(F("NTP failed."));
      }
    }
    time_keeping_timeout = curr_utc_time + TIME_SYNC_TIMEOUT;
    prev_millis = millis();
  }
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
      server->on("/",   on_home);    
      server->on("/js", on_ap_scan);
      server->on("/cc", on_ap_change_config);
      server->on("/jt", on_ap_try_connect);
      server->begin();
      osb.state = OSB_STATE_CONNECTED;
      DEBUG_PRINTLN(WiFi.softAPIP());
      disp.clear();
      osb.boldFont(true);
      disp.drawString(0, 0, "AP Mode");
      osb.boldFont(false);
      disp.drawString(0, 16, "SSID: "+ap_ssid);
      disp.drawString(0, 32, "APIP: "+get_ip(WiFi.softAPIP()));
      disp.display();
    } else {
      led_blink_ms = 0;
      osb.set_led(LOW);
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
      server->on("/", on_home);
      server->on("/index.html", on_home);
      server->on("/settings.html", on_sta_view_options);
      server->on("/log.html", on_sta_view_logs);
      server->on("/manual.html", on_sta_view_manual);
      server->on("/program.html", on_sta_view_program);
      server->on("/preview.html", on_sta_view_preview);
      server->on("/update.html", HTTP_GET, on_sta_update);
      server->on("/update", HTTP_POST, on_sta_upload_fin, on_sta_upload);
      server->on("/jc", on_sta_controller);
      server->on("/jo", on_sta_options);
      server->on("/jl", on_sta_logs);
      server->on("/jp", on_sta_program);
      server->on("/cc", on_sta_change_controller);
      server->on("/co", on_sta_change_options);
      server->on("/cp", on_sta_change_program);
      server->on("/dp", on_sta_delete_program);
      server->on("/rp", on_sta_run_program);
      server->on("/dl", on_sta_delete_log);
      server->begin();

      if(curr_cloud_access_en) {
        Blynk.config(osb.options[OPTION_AUTH].sval.c_str());
        Blynk.connect();
      }
      osb.state = OSB_STATE_CONNECTED;      
      led_blink_ms = 0;
      osb.set_led(LOW);
          
      DEBUG_PRINTLN(WiFi.localIP());
    } else {
      if(millis() > connecting_timeout) {
        osb.state = OSB_STATE_INITIAL;
        DEBUG_PRINTLN(F("timeout"));
      }
    }
    break;
  
  case OSB_STATE_CONNECTED:
    if(curr_mode == OSB_MOD_AP)
      server->handleClient();
    else {
      if(WiFi.status() == WL_CONNECTED) {
        server->handleClient();
        if(curr_cloud_access_en)
          Blynk.run();
      } else {
        osb.state = OSB_STATE_INITIAL;
      }
    }
    break;
    
  case OSB_STATE_RESTART:
    server->handleClient();
    if(millis() > restart_timeout) {
      osb.state = OSB_STATE_INITIAL;
      osb.restart();
    }
    break;
    
  case OSB_STATE_RESET:
    server->handleClient();
    DEBUG_PRINTLN(F("reset"));
    osb.options_reset();
    osb.log_reset();
    restart_timeout = millis();
    osb.state = OSB_STATE_RESTART;
    disp.drawString(0, 16, "done.");
    disp.drawString(0, 32, "Rebooting...");    
    disp.display();
    break;
  
  }
  
  if(curr_mode == OSB_MOD_STA) {
    time_keeping();
    check_status();
  }
  process_button();
  
  static ulong last_time = 0;
  static ulong last_minute = 0;
  if(last_time != curr_utc_time) {
    last_time = curr_utc_time;
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
