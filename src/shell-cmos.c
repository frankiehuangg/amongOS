#include "lib-header/shell-cmos.h"

struct CMOSTimeStamp current_data;

void read_rtc(void)
{
    time_t t = time(NULL);
    struct tm* ptr = localtime(&t);

    current_data.second = ptr->tm_sec;
    current_data.minute = ptr->tm_min;
    current_data.hour   = ptr->tm_hour;
    current_data.day    = ptr->tm_mday;
    current_data.month  = ptr->tm_mon;
    current_data.year   = ptr->tm_year + 1900;
}