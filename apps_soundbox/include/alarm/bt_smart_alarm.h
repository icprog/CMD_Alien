#ifndef __BT_SMART_ALARM_H__
#define __BT_SMART_ALARM_H__
#include "typedef.h"
#include "sdk_cfg.h"
#include "rtc/rtc_api.h"

void bt_smart_alarm_debug_cur_alarm(void);
void bt_smart_alarm_clear_all_outdate_alarm(RTC_TIME *alarm);
tbool bt_smart_alarm_poweron_check(void);
void bt_smart_alarm_set_next_alarm(u8 flag);
void bt_smart_alarm_read(RTC_TIME *alarm);
tbool bt_smart_alarm_init(void);
u32 bt_smart_alarm_rcsp_op_deal(u8 *OperationData, u8 *data, u16 len);

#endif//  __BT_SMART_ALARM_H__
