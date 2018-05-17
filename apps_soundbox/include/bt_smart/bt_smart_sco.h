#ifndef __BT_SMART_SCO_H__
#define __BT_SMART_SCO_H__
#include "typedef.h"

tbool bt_smart_sco_start(u8 speech_source, u8 op);
void  bt_smart_sco_stop(void);
void  bt_smart_sco_cancel(void);
void  bt_smart_sco_continue_end(void);
void bt_mart_sco_active(void);

#endif//__BT_SMART_SCO_H__
