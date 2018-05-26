#ifndef __RTC_ALARM_H__
#define __RTC_ALARM_H__
#include "typedef.h"
#include "rtc/rtc_api.h"

tbool rtc_alarm_init(void (*p)(void));
void rtc_alarm_set_alarm(RTC_TIME *alarm);
void rtc_alarm_set_next_alarm(u8 flag);
u8 rtc_alarm_status(void);

#endif//  __RTC_ALARM_H__
