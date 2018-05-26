#ifndef __TIMER_DATATIME_H__
#define __TIMER_DATATIME_H__
#include "typedef.h"
#include "rtc/rtc_api.h"

void timer_datetime_set_alarm(RTC_TIME *alarm);
void timer_datetime_alarm_switch(u8 onoff);
void timer_datetime_set_datetime(RTC_TIME *time);
tbool timer_datetime_get_cur_datetime(RTC_TIME *time);
void timer_datetime_get_cur_alarm(RTC_TIME *alarm);
u8 timer_datetime_alarm_status(void);
void timer_datetime_set_alarm_off(void);
tbool timer_datetime_init(void (*p)(void));

#endif//__TIMER_DATATIME_H__
