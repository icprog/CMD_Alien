#ifndef __TIMER_DATATIME_H__
#define __TIMER_DATATIME_H__
#include "typedef.h"
#include "rtc/rtc_api.h"

void timer_datetime_set_alarm(RTC_TIME *alarm);
void timer_datetime_disable_alarm(void);
void timer_datetime_sync(RTC_TIME *time);
tbool timer_datetime_get_cur(RTC_TIME *time);
tbool timer_datetime_get_cur_alarm(RTC_TIME *alarm);
u8 timer_datetime_check_alarm_status(void);
void timer_datetime_set_alarm_off(void);
void timer_datetime_init(void);

#endif//__TIMER_DATATIME_H__
