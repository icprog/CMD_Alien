#ifndef __BT_SMART_LED_H__
#define __BT_SMART_LED_H__
#include "typedef.h"

void bt_smart_led_on(void);
void bt_smart_led_off(void);
void bt_smart_led_flick(u32 freq, u32 timeout);
void bt_smart_led_blink(u32 cnt, u32 freq);
void bt_smart_led_init(void);

#endif//__BT_SMART_LED_H__
