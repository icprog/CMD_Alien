#ifndef __BT_SMART_H__
#define __BT_SMART_H__
#include "typedef.h"
#include "bt_smart_led.h"
#include "bt_smart_sco.h"
#include "smart_speex_api.h"

typedef enum __AI_MODE
{
	AI_MODE_SPEECH_INPUT = 0x0,
	AI_MODE_WECHAT,
	AI_MODE_CH_2_EN,
	AI_MODE_EN_2_CH,
		
}AI_MODE;

void bt_smart_enter_bt_task(void);
void bt_smart_exit_bt_task(void);
void bt_smart_ble_disconnect_deal(void);
void bt_smart_ble_connect_deal(void);
void bt_smart_init(void);
u32  bt_smart_rcsp_command_recieve(u8 *OperationData,u8 *data,u16 len);
void bt_smart_msg_deal(void *priv, int *msg);
u8 bt_smart_get_ai_status(void);

u32 bt_smart_user_data_send(u8*buf, u16 len);
void bt_smart_user_data_recieve(u8 *buf, u16 len);

#endif
