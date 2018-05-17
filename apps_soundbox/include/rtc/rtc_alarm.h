#ifndef __RTC_ALARM_H__
#define __RTC_ALARM_H__
#include "typedef.h"

u8 rtc_alarm_init(void);
void rtc_alarm_set_next_alarm(u8 flag);
tbool rtc_alarm_set_nearest_alarm(void);
void rtc_alarm_set_alarm_test(void);

#endif//  __RTC_ALARM_H__
