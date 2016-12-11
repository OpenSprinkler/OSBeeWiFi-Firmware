/* OSBeeWiFi Firmware
 *
 * Program data structures and functions header file
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

#ifndef _PROGRAM_H
#define _PROGRAM_H

#include <OSBeeWiFi.h>

#define MAX_NUM_PROGRAMS      6
#define MAX_NUM_STARTTIMES    5
#define PROGRAM_NAME_SIZE    32
#define MAX_NUM_TASKS        32

#define DAY_TYPE_WEEKLY   0
#define DAY_TYPE_INTERVAL 1

#define RESTR_NONE  0
#define RESTR_ODD   1
#define RESTR_EVEN  2

#define STARTTIME_TYPE_NONE   0
#define STARTTIME_TYPE_FIXED  1
#define STARTTIME_TYPE_REPEAT 2

struct TaskStruct{
  byte      zbits;
  uint16_t  dur;
};

// Program data structure
class ProgramStruct {
public:
  byte enabled:1; // HIGH means the program is enabled

  byte daytype:1;  

  byte restr:2; 

  byte sttype:2;

  // weather data
  byte use_weather:1;
  
  byte dummy:1;
    
  // weekly:   days[0][0..6] correspond to Monday till Sunday
  // interval: days[0] stores the interval (0 to 255), days[1] stores the starting day remainder (0 to 254)
  byte days[2];  
  
  int16_t starttimes[MAX_NUM_STARTTIMES];

  byte ntasks;
  TaskStruct tasks[MAX_NUM_TASKS];

  char name[PROGRAM_NAME_SIZE];
  
  byte check_match(time_t t);

protected:

  byte check_day_match(time_t t);
};

// program structure size
#define PROGRAMSTRUCT_SIZE         (sizeof(ProgramStruct))

class ProgramData {
public:  
  static unsigned long scheduled_stop_times[MAX_NUM_TASKS]; // scheduled start time for each entry
  static int8_t curr_prog_index;
  static int8_t curr_task_index;
  static ulong curr_prog_remaining_time;
  static ulong curr_task_remaining_time;
  static byte scheduled_ntasks;
  static byte nprogs;
  static byte ntasks;
  static TaskStruct manual_tasks[MAX_NUMBER_ZONES];
  
  static void init();
  static void reset_runtime();
  static void load_curr_task(TaskStruct *t);
  static void eraseall();
  static void read(byte pid, ProgramStruct *buf, bool header_only=false);
  static byte add(ProgramStruct *buf);
  static byte modify(byte pid, ProgramStruct *buf);
  static byte del(byte pid);
private:
  static void load_count();
  static void save_count();  
};

#endif  // _PROGRAM_H_
