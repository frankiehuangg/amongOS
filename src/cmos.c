#include "lib-header/cmos.h"

struct CMOSTimeStamp current_data;

uint8_t get_update_in_progress_flag(void)
{
    out(CMOS_PORT_CMD, 0x0A);
    return in((CMOS_PORT_DATA) & 0x80);
}

uint8_t get_RTC_register(uint8_t reg)
{
    out(CMOS_PORT_CMD, reg);
    return in(CMOS_PORT_DATA);
}

void read_rtc(void)
{
    uint8_t registerB;

    struct CMOSTimeStamp last_data;

    while (get_update_in_progress_flag());

    current_data.second = get_RTC_register(0x00);
    current_data.minute = get_RTC_register(0x02);
    current_data.hour = get_RTC_register(0x04);
    current_data.day = get_RTC_register(0x07);
    current_data.month = get_RTC_register(0x08);
    current_data.year = get_RTC_register(0x09);

    do 
    {
        last_data.second = current_data.second;
        last_data.minute = current_data.minute;
        last_data.hour = current_data.hour;
        last_data.day = current_data.day;
        last_data.month = current_data.month;
        last_data.year = current_data.year;

        while (get_update_in_progress_flag());

        current_data.second = get_RTC_register(0x00);
        current_data.minute = get_RTC_register(0x02);
        current_data.hour = get_RTC_register(0x04);
        current_data.day = get_RTC_register(0x07);
        current_data.month = get_RTC_register(0x08);
        current_data.year = get_RTC_register(0x09);
    } while( 
        (last_data.second != current_data.second) ||
        (last_data.minute != current_data.minute) ||
        (last_data.hour != current_data.hour) || 
        (last_data.day != current_data.day) ||
        (last_data.month != current_data.month) ||
        (last_data.year != current_data.year)
    );

    registerB = get_RTC_register(0x0B);

    if (!(registerB & 0x04))
    {
        current_data.second = (current_data.second & 0x0F) + ((current_data.second / 16) * 10);
        current_data.minute = (current_data.minute & 0x0F) + ((current_data.minute / 16) * 10);
        current_data.hour = ((((current_data.hour & 0x0F) + (((current_data.hour & 0x70) / 16) * 10)) | (current_data.hour & 0x80)) + UTC) % 12;
        current_data.day = (current_data.day & 0x0F) + ((current_data.day / 16) * 10);
        current_data.month = (current_data.month & 0x0F) + ((current_data.month / 16) * 10);
        current_data.year = (current_data.year & 0x0F) + ((current_data.year / 16) * 10);
    }

    // Convert 12 hour clock to 24 hour clock
    // if (!(registerB & 0x02) && (current_data.hour & 0x80))
    //     current_data.hour = ((current_data.hour & 0x7F) + 12) % 24;

    // Calculate full year
    current_data.year += (CURRENT_YEAR / 100) * 100;
    if (current_data.year < CURRENT_YEAR) current_data.year += 100;
}