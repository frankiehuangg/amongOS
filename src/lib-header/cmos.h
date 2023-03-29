#ifndef _CMOS_H
#define _CMOS_H

#include "lib-header/stdtype.h"
#include "lib-header/portio.h"

#define CURRENT_YEAR    2023
#define UNIX_START_YEAR 1970
#define UTC             7

#define CMOS_PORT_CMD   0x70
#define CMOS_PORT_DATA  0x71

struct CMOSTimeStamp {
    // Time
    uint8_t second;
    uint8_t minute;
    uint8_t hour;

    // Date
    uint8_t day;
    uint8_t month;
    uint16_t year;
}__attribute__((packed));

extern struct CMOSTimeStamp current_data;

uint8_t get_update_in_progress_flag(void);

uint8_t get_RTC_register(uint8_t reg);

void read_rtc(void);


#endif