#include "smart_speex_api.h"
#include "speex_encode.h"
#include "common/app_cfg.h"
#include "common/msg.h"
#include "rcsp/rcsp_interface.h"
#include "bluetooth/ble_api.h"
#include "bluetooth/avctp_user.h"
#include "ai_toy.h"
#include "sys_detect.h"
#include "dac/dac_api.h"
#include "bt_smart.h"


#define	SPEEX_ENCODE_TASK_NAME						"SPEEX_ENCODE_TASK" 
#define	SPEEX_ENCODE_WRITE_TASK_NAME				"SPEEX_ENCODE_WRITE_TASK" 

#define SPEEX_ENCODE_SAMPLERATE						(16000L)//目前暂时支持16K采样率

#define SPEEX_TIMEOUT_SECOND						(500L)

#define SPEEX_VAD_SPEAK_START_THRESHOLD				(10L)
#define SPEEX_VAD_NO_SPEAK_START_THRESHOLD			(80L)//(51L)
#define SPEEX_VAD_SPEAK_TIMEOUT_N_SECOND			(10 * SPEEX_TIMEOUT_SECOND)						
#define SPEEX_VAD_NO_SPEAK_TIMEOUT_N_SECOND			(10 * SPEEX_TIMEOUT_SECOND)						
#define SPEEX_VAD_MIN_N_SECOND						100//((1 * SPEEX_TIMEOUT_SECOND) / 2)						




#define  SMART_SPEEX_DEBUG_ENABLE
#ifdef SMART_SPEEX_DEBUG_ENABLE
#define smart_speex_printf	printf
#else
#define smart_speex_printf(...)
#endif//SMART_SPEEX_DEBUG_ENABLE

static SPEEX_ENCODE *speex_encode_hdl = NULL;
static u16 speex_encode_write_cbk(void *hdl, u8 *buf, u16 len);

static u8 smart_speex_auto_mute_status = 0;


extern void bt_prompt_play_by_name(void * file_name, void * let);
extern tbool mutex_resource_release(char *resource);
extern tbool mutex_resource_apply(char *resource,int prio ,void (*apply_response)(), void (*release_request)());

static const SPEEX_FILE_IO speex_encode_file = 
{
	.seek = NULL,		
	.read = NULL,		
	.write = speex_encode_write_cbk,		
};



static u16 speex_encode_write_cbk(void *hdl, u8 *buf, u16 len)
{
	 /* printf_buf(buf, len); */
	 u32 ret = 	ble_speex_send(buf, len);
	 if(ret)
	 {
		smart_speex_printf("Fail "); 	
		return 0;
	 }
	 return len;
}


//断句超时回调处理
static void speex_encode_timeout_callback(void *priv, u8 timeout)
{
	/* smart_speex_printf("speex encode timeout !!!\n");		 */
	char *father_name = priv;
	int msg = NO_MSG;
	if(timeout == 2)
	{
		msg = MSG_SPEEX_SEND_CONTINUE_END;
	}
	else
	{
		msg = MSG_SPEEX_SEND_STOP;
	}

	os_taskq_post(father_name, 1, msg);
}



static tbool smart_speex_encode_start(void *father_name)
{
	tbool ret;

	if(smart_speex_auto_mute_status)
	{
		digit_auto_mute_set(1,-1,-1,-1);//还原自动mute
	}

	if(speex_encode_hdl)
	{
		smart_speex_printf("smart speex encode aready\n");
		return false;		
	}

	SPEEX_ENCODE *obj = speex_encode_creat();
	if(obj == NULL)
	{
		smart_speex_printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);;		
		return false;
	}

	__speex_encode_set_vad_reg(obj, 
			SPEEX_VAD_SPEAK_START_THRESHOLD, //说话开始阀值, 值越大越难检测到说话
			SPEEX_VAD_NO_SPEAK_START_THRESHOLD,//断句阀值, 值越大，越不容易被断句
			SPEEX_VAD_SPEAK_TIMEOUT_N_SECOND, //一直说话
			SPEEX_VAD_NO_SPEAK_TIMEOUT_N_SECOND, //一直没说话
			SPEEX_VAD_MIN_N_SECOND, //防止说话太短设置
			(void *)speex_encode_timeout_callback,
			father_name
			);

	__speex_encode_set_father_name(obj, father_name);//注意这里填写控制线程的名称
	__speex_encode_set_samplerate(obj, SPEEX_ENCODE_SAMPLERATE);//16K
	__speex_encode_set_file_io(obj, (SPEEX_FILE_IO *)&speex_encode_file, NULL);

	ret = speex_encode_process(obj, SPEEX_ENCODE_TASK_NAME, TaskEncRunPrio);
	if(ret == true)
	{
		ret = speex_encode_write_process(obj, SPEEX_ENCODE_WRITE_TASK_NAME, TaskEncWFPrio);
		if(ret == true)
		{
		}
		else
		{
			smart_speex_printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);;		
			speex_encode_destroy(&obj);	
			return false;
		}
	}
	else
	{
		smart_speex_printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);;		
		speex_encode_destroy(&obj);	
		return false;
	}

	speex_encode_hdl = obj;


	smart_speex_printf("speex_encode init ok\n");

	return speex_encode_start(speex_encode_hdl);		
}

static void smart_speex_encode_stop(void)
{
	speex_encode_stop(speex_encode_hdl);
	speex_encode_destroy(&speex_encode_hdl);	
}


static tbool bt_smart_speex_encode_start(void)
{

	smart_speex_printf("speex start+++++++\n");
	return smart_speex_encode_start("btmsg");
}

static void bt_smart_speex_encode_stop(void)
{
	smart_speex_printf("speex stop------\n");
	smart_speex_encode_stop();
}



/*----------------------------------------------------------------------------*/
/**@brief  bt smart 语音发送开始
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
tbool bt_smart_speex_data_send(u8 speech_source, u8 op)
{
	u8 send_buf[2];
	send_buf[0] = speech_source;//语音输入源
	send_buf[1] = op;//语音用途， 例如：互动、中译英、音译中等

	u8 call_status = get_call_status();
	if(call_status == BT_CALL_ACTIVE 
			|| call_status ==  BT_CALL_OUTGOING
			|| call_status ==  BT_CALL_INCOMING
			|| call_status ==  BT_CALL_ALERT
	  )
	{
		///挂断处理
		smart_speex_printf("is phone talking !!\n");
		return false;		
	}
	
	smart_speex_auto_mute_status = get_digit_auto_mute_status();
	if(smart_speex_auto_mute_status)
	{
		digit_auto_mute_set(0,-1,-1,-1);//close auto mute
		dac_mute(0, 1);
	}

	bt_smart_speex_data_send_cancel();//告知app取消上次的语音输入
	if((op == AI_MODE_CH_2_EN) || (op == AI_MODE_EN_2_CH))
	{
		bt_prompt_play_by_name(AI_TOY_NOTICE_SPEECH_TRANC, NULL);
	}
	else
	{
		bt_prompt_play_by_name(AI_TOY_NOTICE_SPEECH_INPUT, NULL);
	}
	if(mutex_resource_apply("speex", 3, (void *)bt_smart_speex_encode_start, (void *)bt_smart_speex_encode_stop))
	{
		rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_START, send_buf, 2);
	}
	return true;
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 语音发送停止
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_speex_data_send_stop(void)
{
	if(mutex_resource_release("speex"))
	{
		rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_STOP, NULL, 0);//告知app停止语音输入
	}
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 语音发送取消
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_speex_data_send_cancel(void)
{
	if(mutex_resource_release("speex"))
	{
		rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_CANCEL, NULL, 0);
	}
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 退出连续互动
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_speex_data_send_continue_end(void)
{
	if(mutex_resource_release("speex"))
	{
		rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_CONTINUE_END, NULL, 0);
	}
}


