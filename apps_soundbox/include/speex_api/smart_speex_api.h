#ifndef __SMART_SPEEX_ENCODE_H__
#define __SMART_SPEEX_ENCODE_H__
#include "typedef.h"

tbool bt_smart_speex_data_send(u8 speech_source, u8 op);
void bt_smart_speex_data_send_stop(void);
void bt_smart_speex_data_send_cancel(void);
void bt_smart_speex_data_send_continue_end(void);

#endif//__SMART_SPEEX_ENCODE_H__
