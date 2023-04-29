#ifndef _FAT32_CMOS_H
#define _FAT32_CMOS_H

#include "stdtype.h"
#include <time.h>

/* Time constants */
#define CURRENT_YEAR    2023
#define UNIX_START_YEAR 1970
#define UTC             7

/**
 * CMOS Time Stamp data structure
 *
 * @param second        Last recorded second
 * @param minute        Last recorded minute
 * @param hour          Last recorded hour
 * @param day           Last recorded day
 * @param month         Last recorded month
 * @param year          Last recorded year 
 */
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

// CMOS time stamp to store last read data
extern struct CMOSTimeStamp current_data;

/**
 * Check whether there is an update in progress, used to disable concurrent access
 * 
 * @return  uint8_t boolean whether there is an access
 */
uint8_t get_update_in_progress_flag(void);

/**
 * Read the register value provided by the parameter
 *
 * @param reg       Register number
 * @return uint8_t  Register value
 */
uint8_t get_RTC_register(uint8_t reg);


/**
 * Read current timestamp with CMOS registers
 */
void read_rtc(void);

#endif