#include "bt_smart_sco.h"
#include "bt_smart_led.h"
#include "rcsp/rcsp_interface.h"

static volatile u8 bt_smart_sco_active_flag = 0;
/*----------------------------------------------------------------------------*/
/**@brief  bt smart 语音发送开始
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
tbool bt_smart_sco_start(u8 speech_source, u8 op)
{
	u8 send_buf[2];
	send_buf[0] = speech_source;//语音输入源
	send_buf[1] = op;//语音用途， 例如：互动、中译英、音译中等
	
	bt_smart_sco_cancel();//告知app取消上次的语音输入
	rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_START, send_buf, 2);//告知app检测到语音输入按键按下
	bt_smart_sco_active_flag = 1;

	return true;
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 语音发送停止
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_sco_stop(void)
{
	bt_smart_sco_active_flag = 0;
	/* rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_STOP, NULL, 0);//告知app停止语音输入 */
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 语音发送取消
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_sco_cancel(void)
{
	bt_smart_sco_active_flag = 0;
	rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_CANCEL, NULL, 0);
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 退出连续互动
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_sco_continue_end(void)
{
	bt_smart_sco_active_flag = 0;
	rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_CONTINUE_END, NULL, 0);
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 电话链路激活处理
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_mart_sco_active(void)
{
	if(bt_smart_sco_active_flag == 1)	
	{
		bt_smart_sco_active_flag = 0;
		bt_smart_led_on();		
	}
}
