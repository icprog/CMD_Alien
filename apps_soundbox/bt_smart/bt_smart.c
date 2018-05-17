#include "bt_smart.h"
#include "bt_smart_led.h"
#include "bluetooth/ble_api.h"
#include "common/msg.h"
#include "timer.h"
#include "sdk_cfg.h"
#include "bluetooth/avctp_user.h"
#include "rcsp/rcsp_interface.h"
#include "play_sel/play_sel.h"
#include "sdk_cfg.h"
#include "rtos/task_manage.h"
#include "play_sel/play_sel.h"
#include "sys_detect.h"
#include "rtc_alarm.h"
#include "ai_toy.h"
#include "ui/led/led_eye.h"


#if (BT_SMART_SPEECH_SOURCE == SCO_INPUT)

#define bt_smart_speech_start					bt_smart_sco_start
#define bt_smart_speech_stop					bt_smart_sco_stop
#define bt_smart_speech_cancel					bt_smart_sco_cancel
#define bt_smart_speech_continue_end			bt_smart_sco_continue_end

#elif(BT_SMART_SPEECH_SOURCE == SPEEX_INPUT)

#define bt_smart_speech_start					bt_smart_speex_data_send
#define bt_smart_speech_stop					bt_smart_speex_data_send_stop
#define bt_smart_speech_cancel					bt_smart_speex_data_send_cancel
#define bt_smart_speech_continue_end			bt_smart_speex_data_send_continue_end

#else

#define bt_smart_speech_start(...)
#define bt_smart_speech_stop(...)
#define bt_smart_speech_cancel(...)
#define bt_smart_speech_continue_end(...)

#endif

#define  BT_SMART_DEBUG_ENABLE
#ifdef BT_SMART_DEBUG_ENABLE
#define bt_smart_printf	printf
#else
#define bt_smart_printf(...)
#endif//BT_SMART_DEBUG_ENABLE

#define BT_SMART_WAIT_ANSWER_TIMEOUT_N_SECOND		(15*500L)

u8 bt_smart_update_flag = 0;


typedef enum __AI_STATUS
{
	AI_STATUS_UNACTIVE = 0x0,
	AI_STATUS_ACTIVE,
}AI_STATUS;

typedef enum __AI_MODE
{
	AI_MODE_SPEECH_INPUT = 0x0,
	AI_MODE_WECHAT,
	AI_MODE_CH_2_EN,
	AI_MODE_EN_2_CH,
		
}AI_MODE;

typedef struct __AI_CON
{
	u8 			status;		
	u32 		msg;
}AI_CON;


static AI_CON ai_ctl = 
{
	.status	 = AI_STATUS_UNACTIVE, 	
	.msg = NO_MSG,
};


u8 microphone_enable = 0;
static u8 g_ai_mode = 0;
static u8 bt_smart_notice_plan = 0;
static u8 bt_smart_connect_dev_type = 0;
/* static u8 bt_smart_speech_flag = 0; */


extern u8 ignore_pause_eye_display;

extern char *keymsg_task_name;
extern u32 AI_toy_public_cmd_parse(u8 *operation,u8 *data,u16 len);
/* extern void AI_toy_update_ai_mode(u8 status); */
extern void bt_prompt_play_by_name(void * file_name, void * let);
extern void aux_dac_channel_on(void);
extern void aux_dac_channel_off(void);
extern void AI_toy_music_notice_play(MUSIC_OP_API *mapi, char *path);
extern void AI_toy_active_ai_ok(void);
extern void AI_toy_update_disconnect_ble(void);

extern void ble_server_set_connection_interval(u16 in_min, u16 in_max, u16 timeout);

#define ANDRIOD_INTERVAL_MIN		(16L)
#define ANDRIOD_INTERVAL_MAX		(16L)
#define ANDRIOD_IIMEOUT				(550L)

/*----------------------------------------------------------------------------*/
/**@brief  bt smart 蓝牙断开处理，主要是清除一些状态
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_ble_disconnect_deal(void)
{
	//清除一些状态
	bt_smart_led_off();
	bt_smart_notice_plan = 0;
	bt_smart_connect_dev_type = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  bt smart 蓝牙连上，主要是初始化一些状态
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_ble_connect_deal(void)
{
		
}

/*----------------------------------------------------------------------------*/
/**@brief  bt smart 蓝牙连接状态判断
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
tbool bt_smart_connect_status(void)
{
	if((get_curr_channel_state() != 0) && (get_smart_pass() == true))
	{
		return true;	
	}
	else
	{
		return false;	
	}
}

extern u8 *rcsp_send_tmp_buf;
extern u8 *user_get_version_str(u16 *len);
u32 fw_report_firmware_version(void *bfu_version,u16 len)
{
	u32 err = RCSP_OP_ERR_NONE;
    u32 offset = 0;

    u8 *data_pt;
	u8 *data_boday; 

    data_pt = (u8 *)rcsp_send_tmp_buf;
	if(data_pt == NULL)
	{
		return RCSP_OP_ERR_MALLOC;	
	}

	data_boday = data_pt + rcsp_get_data_packet_body_offset();

	u8 *p_version;
	u16 version_len;

	p_version = user_get_version_str(&version_len);

	printf_buf(p_version, version_len);
	offset = offset + rcsp_fill_packet_frame_func(data_boday + offset, "VERN", FRAME_TYPE_ANSI, p_version, version_len);

    rcsp_fill_packet_head_func((void*)data_pt, 0xffff, offset,1);

	err = rcsp_data_send_func((void*)data_pt, rcsp_get_data_packet_body_offset() + offset, rcsp_check_csw_ackflag_func(CSW_REQUEST_ACK_CBW,1));
	rcsp_set_ack_csw_sw_func(0);

	return err;
}
extern 
bool ble_fw_download_task_create(void);
extern void ble_send_update_cmd();
/*----------------------------------------------------------------------------*/
/**@brief  bt smart 接收app发送命令处理函数
   @param  msg
   @return 无
   @note   这里收到命令之后，不要在这里直接处理（改变led状态这些除外），
   必须通过发送消息到对应的控制线程处理, 例如下面的消息CBW_CMD_NOTICE_CHECK_CONNECT,
   通过发送消息到btmsg处理，btmsg会在bt_smart_msg_deal中处理这个消息
*/
/*----------------------------------------------------------------------------*/
u32 bt_smart_rcsp_command_recieve(u8 *OperationData,u8 *data,u16 len)
{
	int msg = NO_MSG;
	u32 err = RCSP_OP_ERR_NONE;
	/* printf("smart_cmd:%x\n",OperationData[0]); */
	switch (OperationData[0])
	{
#if BLE_APP_UPDATE_SUPPORT_EN	
		case CBW_CMD_EXCHANGE_VERSION:
			printf("CBW_CMD_EXCHANGE_VERSION \n");
			err = fw_report_firmware_version(data,len);
			break;
		case CBW_CMD_START_FW_UPDATE:
			bt_smart_update_flag = 1;
			os_taskq_post(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, IDLE_TASK_NAME);
			//err = fw_report_start_fw_update();
			//ble_send_update_cmd();
			/* ble_fw_download_task_create(); */
			break;
#endif
		case CBW_CMD_SPEEX_SEND_STOP:
			bt_smart_printf("CBW_CMD_SPEEX_SEND_STOP\n");
			if(compare_task_name(BTSTACK_TASK_NAME))
			{
				os_taskq_post("btmsg", 1, MSG_SPEEX_SEND_STOP);
			}
			break;

		case CBW_CMD_SPEEX_SEND_CONTINUE:
			bt_smart_printf("CBW_CMD_SPEEX_SEND_CONTINUE\n");

			if(compare_task_name(BTSTACK_TASK_NAME) == 0)
			{
				break;
			}
			switch(g_ai_mode)
			{
				case AI_MODE_CH_2_EN:
					msg = MSG_CH_2_EN;
					break;
				case AI_MODE_EN_2_CH:
					msg = MSG_EN_2_CH;
					break;
				case AI_MODE_SPEECH_INPUT:
					msg = MSG_SPEECH_INPUT;
					break;
				default:
					bt_smart_printf("ai mode err !!!\n");
					break;
			}
			if(msg!=NO_MSG)
			{
				os_taskq_post(keymsg_task_name, 1, msg);
			}
			break;

		case CBW_CMD_SPEEX_WAIT_OK:
			///app通知语音识别完成
			/* if(smart_speex_encode_status() == false) */
			{
				bt_smart_printf("AI wait ok~~~~~~~~~~~~~~~~~\n");
				if(OperationData[1] == 0)
				{
					//识别识别成功	
					/* EyeEffectCtl(EFFECT_SPEECH_RESULT_OK); */
					eye_led_api(EFFECT_SPEECH_RESULT_OK, 0 ,0);
				}
				else
				{
					//识别失败	
					/* EyeEffectCtl(EFFECT_SPEECH_RESULT_FAIL); */
					eye_led_api(EFFECT_SPEECH_RESULT_FAIL, 0 ,0);
				}
				/* bt_smart_led_off(); */
			}
			break;

		case CBW_CMD_NOTICE_PLAN_STATUS:
			///app通知开始播放日程提醒
			bt_smart_printf("CBW_CMD_NOTICE_PLAN_STATUS = %d", OperationData[1]);
			bt_smart_notice_plan = OperationData[1];
			if(bt_smart_notice_plan != 0)
			{
				///当前正在语音输入, 发消息告知控制想成取消语音输入
				if(compare_task_name(BTSTACK_TASK_NAME))
				{
					os_taskq_post("btmsg", 1, MSG_SMART_NOTICE_PLAN);
				}
				else
				{
						
				}
			}
			break;

		case CBW_CMD_NOTICE_CHECK_CONNECT:
			///app通知连接不匹配, 这里会发送消息到控制线程断开当前连接
			bt_smart_printf("CBW_CMD_NOTICE_CHECK_CONNEC\n");
			os_taskq_post(keymsg_task_name, 1, MSG_SMART_CHECK_DISCONNECT);
			break;

		case CBW_CMD_AI_TOY_CMD:
			bt_smart_printf("CBW_CMD_AI_TOY_CMD ###################\n");
			err = AI_toy_public_cmd_parse(&OperationData[1], data, len);
			break;
		case CBW_CMD_CONN_DEV_TYPE:
			if(OperationData[1] == 1)
			{
				printf("connect dev is ios \n");		
				bt_smart_connect_dev_type = 1;
			}
			else if(OperationData[1] == 2)
			{
				printf("connect dev is Andriod \n");		
				bt_smart_connect_dev_type = 2;
				ble_server_set_connection_interval(ANDRIOD_INTERVAL_MIN, ANDRIOD_INTERVAL_MAX, ANDRIOD_IIMEOUT);
			}
			break;

		default:
			err = RCSP_OP_ERR_NO_CMD;
			break;
	}
	return err;
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 蓝牙模式消息处理
   @param  msg
   @return 无
   @note   bt smart相关处理建议都在这里处理
*/
/*----------------------------------------------------------------------------*/

void bt_smart_msg_deal(void *priv, int *msg)
{
	switch(msg[0])
	{
		/* case MSG_WECHAT: *///功能还没有实现
		case MSG_CH_2_EN:
		case MSG_EN_2_CH:
		case MSG_SPEECH_INPUT:
			if(bt_smart_connect_status() == false)
			{
				eye_led_api(EFFECT_NO_CONNECT, 0 ,0);
				if(compare_task_name(MUSIC_TASK_NAME))
				{
					AI_toy_music_notice_play(priv, AI_TOY_NOTICE_NO_NET);

				}
				else if(compare_task_name(BTSTACK_TASK_NAME))
				{
					bt_prompt_play_by_name(AI_TOY_NOTICE_NO_NET,NULL);
				}

				break;
			}
			if(compare_task_name(BTSTACK_TASK_NAME) == 0)
			{
				ai_ctl.status = AI_STATUS_ACTIVE;
				ai_ctl.msg = msg[0];
				os_taskq_post(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, BTSTACK_TASK_NAME);
			}
			else
			{
				ai_ctl.status = AI_STATUS_ACTIVE;
				switch(msg[0])
				{
					case MSG_WECHAT:
						bt_smart_printf("line = %d,_______\n", __LINE__);
						g_ai_mode = AI_MODE_WECHAT;
						break;
					case MSG_CH_2_EN:
						bt_smart_printf("line = %d,_______\n", __LINE__);
						g_ai_mode = AI_MODE_CH_2_EN;
						break;
					case MSG_EN_2_CH:
						bt_smart_printf("line = %d,_______\n", __LINE__);
						g_ai_mode = AI_MODE_EN_2_CH;
						break;
					case MSG_SPEECH_INPUT:
						bt_smart_printf("line = %d,_______\n", __LINE__);
						g_ai_mode = AI_MODE_SPEECH_INPUT;
						break;
				}

				ignore_pause_eye_display = 1;
				/* EyeEffectCtl(EFFECT_SPEECH_INPUT); */
				eye_led_api(EFFECT_SPEECH_INPUT, 0 ,0);
				if(bt_smart_speech_start(BT_SMART_SPEECH_SOURCE, g_ai_mode) == true)
				{
#if (BT_SMART_SPEECH_SOURCE == SPEEX_INPUT)
					bt_smart_led_on();			
#elif (BT_SMART_SPEECH_SOURCE == SCO_INPUT)
					///如果走sco则在通话激活之后才点亮灯
#endif//(BT_SMART_SPEECH_SOURCE == SPEEX_INPUT)
				}
				os_taskq_msg_clean(msg[0], 1);///清除由于录音启动导致堵塞的所有该消息
			}
			break;

		case MSG_MICROPHONE:
			/* if(microphone_enable) */
			if(msg[1] == 0)
			{
				microphone_enable = 0;
				msg[0] = MSG_ECHO_STOP;
				bt_smart_printf(" microphone_disable  !!!\n");
				if(compare_task_name(MUSIC_TASK_NAME))
				{
					AI_toy_music_notice_play(priv, AI_TOY_NOTICE_EXIT_MIC);
				}
				else if(compare_task_name(BTSTACK_TASK_NAME))
				{
					bt_prompt_play_by_name(AI_TOY_NOTICE_EXIT_MIC,NULL);
				}
			}
			else
			{
				bt_smart_printf("microphone_enable  !!!\n");
				microphone_enable = 1;
				msg[0] = MSG_ECHO_START;
				if(compare_task_name(MUSIC_TASK_NAME))
				{
					AI_toy_music_notice_play(priv, AI_TOY_NOTICE_ENTER_MIC);

				}
				else if(compare_task_name(BTSTACK_TASK_NAME))
				{
					bt_prompt_play_by_name(AI_TOY_NOTICE_ENTER_MIC,NULL);
				}
			}
			break;
		

		case MSG_AI_ACTIVE:
			if(compare_task_name(BTSTACK_TASK_NAME) == 0)
			{
				ai_ctl.status = AI_STATUS_ACTIVE;
				ai_ctl.msg = msg[0];
				os_taskq_post(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, BTSTACK_TASK_NAME);
			}
			else
			{
				bt_smart_printf("app active ai ok!!!\n");
				ai_ctl.status = AI_STATUS_ACTIVE;
				AI_toy_active_ai_ok();
			}
			break;

		case MSG_SPEEX_SEND_START:
			///按键触发语音传输开始
			if(compare_task_name(BTSTACK_TASK_NAME))
			{
				if(bt_smart_connect_status() == false)	
				{
					///播放提示音提醒， 蓝牙连接不正确
					bt_prompt_play_by_name(BPF_CONNECT_ERR, NULL);
					break;
				}

				if(bt_smart_notice_plan)
				{
					bt_smart_printf("notice planning!!!\n");
					break;		
				}

				bt_smart_printf("MSG_SPEEX_SEND_START\n");
				if(bt_smart_speech_start(BT_SMART_SPEECH_SOURCE, 0) == true)
				{
#if (BT_SMART_SPEECH_SOURCE == SPEEX_INPUT)
					bt_smart_led_on();			
#elif (BT_SMART_SPEECH_SOURCE == SCO_INPUT)
					///如果走sco则在通话激活之后才点亮灯
#endif//(BT_SMART_SPEECH_SOURCE == SPEEX_INPUT)
				}
			}

			os_taskq_msg_clean(msg[0], 1);///清除由于录音启动导致堵塞的所有该消息

			break;

		case MSG_SPEEX_SEND_STOP:
			///断句通知录音结束
			bt_smart_printf("MSG_SPEEX_SEND_STOP\n");
			bt_smart_speech_stop();
			eye_led_set_back_api(EFFECT_PAUSE);
			eye_led_api(EFFECT_SEARCH_ING, 10, 0);	
			pa_umute();
			/* bt_smart_led_flick(150, BT_SMART_WAIT_ANSWER_TIMEOUT_N_SECOND); */
			break;

		case MSG_SPEEX_SEND_CONTINUE_END:
			///告知app连续对话结束， 可能是因为没有检测到语音输入超时产生
			bt_smart_speech_continue_end();
			eye_led_api(EFFECT_PAUSE, 0, 0);	
			/* bt_smart_led_off(); */
			break;

		case MSG_SPEEX_ENCODE_ERR:
			///语音传输失败处理
			bt_smart_printf("MSG_SPEEX_ENCODE_ERR\n");
			bt_smart_speech_cancel();
			/* bt_smart_led_off(); */
			eye_led_api(EFFECT_PAUSE, 0, 0);	
			break;

		case MSG_SMART_NOTICE_PLAN:
			///app通知开始播放日程提醒, 取消当前的语音输入
			if(bt_smart_notice_plan)
			{
				bt_smart_speech_cancel();
				bt_smart_led_off();
			}
			break;

		case MSG_SMART_CHECK_DISCONNECT:
			///app通知连接不匹配，断开当前连接
			user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI,0,NULL);
			rcsp_smart_command_send(CBW_CMD_NOTICE_CHECK_CONNECT, NULL, 0);
			break;

		case MSG_AI_BT_DISCONNECT_CTRL:
			printf("MSG_AI_BT_DISCONNECT_CTRL \n");
			if(compare_task_name(BTSTACK_TASK_NAME) == 0)
			{
				ai_ctl.status = AI_STATUS_ACTIVE;
				ai_ctl.msg = msg[0];
				os_taskq_post(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, BTSTACK_TASK_NAME);
			}
			else
			{
				ai_ctl.status = AI_STATUS_ACTIVE;

				if((BT_STATUS_CONNECTING == get_bt_connect_status())   ||
						(BT_STATUS_TAKEING_PHONE == get_bt_connect_status())||
						(BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()))/*Á¬½Ó×´Ì¬*/
				{
					puts("bt_disconnect 2.1\n");/*ÊÖ¶¯¶Ï¿ªÁ¬½Ó*/
					user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI,0,NULL);
				}
				AI_toy_update_disconnect_ble();
				ignore_pause_eye_display = 1;
			}
			break;

		case MSG_LOW_POWER_VOICE:
			/* EyeEffectCtl(EFFECT_LOW_POWER_WARMING); */
			eye_led_api(EFFECT_LOW_POWER_WARMING, 15 ,0);
			if(compare_task_name(MUSIC_TASK_NAME))
			{
				AI_toy_music_notice_play(priv, AI_TOY_NOTICE_LOW_POWER);

			}
			else if(compare_task_name(BTSTACK_TASK_NAME))
			{
				bt_prompt_play_by_name(AI_TOY_NOTICE_LOW_POWER,NULL);
			}
			break;
//		case MSG_LOW_POWER:
//			/* EyeEffectCtl(EFFECT_POWER_OFF); */
//			eye_led_api(EFFECT_POWER_OFF, 0 ,0);
//			if(compare_task_name(MUSIC_TASK_NAME))
//			{
//				AI_toy_music_notice_play(priv, AI_TOY_NOTICE_LOW_POWER_OFF);
//
//			}
//			else if(compare_task_name(BTSTACK_TASK_NAME))
//			{
//				bt_prompt_play_by_name(AI_TOY_NOTICE_LOW_POWER_OFF,NULL);
//			}
//			while(play_sel_busy())
//			{
//				os_time_dly(3);		
//			}
//			soft_power_ctl(PWR_OFF);
//			break;
#if 0
		case MSG_IDLE_TIMEOUT_POWER_OFF:
			while(play_sel_busy())
			{
				os_time_dly(3);		
			}
			soft_power_ctl(PWR_OFF);
			break;
#endif
		case MSG_ALM_SETTING:
			bt_smart_printf("MSG_ALM_SETTING \n");
			/* rtc_alarm_set_nearest_alarm(); */
			/* rtc_alarm_set_alarm_test(); */
			break;

		case MSG_ALM_ON:
			break;


		case MSG_SMART_DISCONNECT:
			eye_led_api(EFFECT_NO_CONNECT, 0 ,0);
			if(compare_task_name(MUSIC_TASK_NAME))
			{
				AI_toy_music_notice_play(priv, BPF_DISCONNECT_MP3);
			}
			else if(compare_task_name(BTSTACK_TASK_NAME))
			{
				bt_prompt_play_by_name(BPF_DISCONNECT_MP3,NULL);
			}
			break;
	 
		case MSG_SMART_CONNECT:
			ignore_pause_eye_display = 1;
			eye_led_api(EFFECT_CONNECT_OK, 0, 0);
			if(compare_task_name(MUSIC_TASK_NAME))
			{
				AI_toy_music_notice_play(priv, BPF_CONNECT_MP3);
			}
			else if(compare_task_name(BTSTACK_TASK_NAME))
			{
				bt_prompt_play_by_name(BPF_CONNECT_MP3,NULL);
			}
			break;



		case MSG_CHANGE_NET_RES_VER_PRIMARY://
		case MSG_CHANGE_NET_RES_SUB_PRIMARY://
		case MSG_CHANGE_NET_RES_GRADE_PRIMARY://
		case MSG_CHANGE_NET_RES_VER_JUNIOR://
		case MSG_CHANGE_NET_RES_SUB_JUNIOR://
		case MSG_CHANGE_NET_RES_GRADE_JUNIOR://
		case MSG_CHANGE_NET_RES_CHILD://教材幼儿资源切换
			if(bt_smart_connect_status() == false)
			{
				eye_led_api(EFFECT_NO_CONNECT, 0 ,0);
				if(compare_task_name(MUSIC_TASK_NAME))
				{
					AI_toy_music_notice_play(priv, AI_TOY_NOTICE_NO_NET);

				}
				else if(compare_task_name(BTSTACK_TASK_NAME))
				{
					bt_prompt_play_by_name(AI_TOY_NOTICE_NO_NET,NULL);
				}

				break;
			}
			else
			{
				eye_led_api(EFFECT_PAUSE, 0 ,0);
				bt_smart_speech_cancel();
				switch(msg[0])
				{
					case MSG_CHANGE_NET_RES_VER_PRIMARY://教材版本切换
						ai_toy_change_net_resource(1, 1);
						break;
					case MSG_CHANGE_NET_RES_SUB_PRIMARY://教材学科切换
						ai_toy_change_net_resource(2, 1);
						break;
					case MSG_CHANGE_NET_RES_GRADE_PRIMARY://教材年级切换
						ai_toy_change_net_resource(3, 1);
						break;

					case MSG_CHANGE_NET_RES_VER_JUNIOR:
						ai_toy_change_net_resource(1, 2);
						break;
					case MSG_CHANGE_NET_RES_SUB_JUNIOR:
						ai_toy_change_net_resource(2, 2);
						break;
					case MSG_CHANGE_NET_RES_GRADE_JUNIOR:
						ai_toy_change_net_resource(3, 2);
						break;
					case MSG_CHANGE_NET_RES_CHILD://教材幼儿资源切换
						ai_toy_change_net_resource(4, 0);
						break;
					default:
						break;

				}

				os_taskq_msg_clean(msg[0], 1);///清除由于录音启动导致堵塞的所有该消息
			}

			break;


		default:
			break;
	}
}


u8 bt_smart_get_ai_status(void)
{
	return ai_ctl.status;
}

/*----------------------------------------------------------------------------*/
/**@brief  bt smart 初始化
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_init(void)
{
	bt_smart_led_init();
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 进入蓝牙模式处理
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_enter_bt_task(void)
{
	bt_smart_printf("func = %s, line = %d, status = %d\n", __FUNCTION__, __LINE__, ai_ctl.status);
	if(ai_ctl.status != AI_STATUS_UNACTIVE)	
	{
		bt_smart_printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
		if(ai_ctl.msg != NO_MSG)
		{
			bt_smart_printf("fun = %s, line = %d, msg = %x\n", __FUNCTION__, __LINE__, ai_ctl.msg);
			os_taskq_post(BTMSG_TASK_NAME, 1, ai_ctl.msg);
		}
	}
	else
	{
		/* bt_smart_led_blink(200, 2);	 */
	}
}

/*----------------------------------------------------------------------------*/
/**@brief  bt smart 退出蓝牙模式处理
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_exit_bt_task(void)
{
	bt_smart_speech_stop();
	bt_smart_led_off();
	ai_ctl.status = AI_STATUS_UNACTIVE;
	ai_ctl.msg = NO_MSG;
}


/*----------------------------------------------------------------------------*/
/**@brief  bt smart 自定义数据发送接口
   @param  buf:要求保证不是局部变量， len:发送数据长度
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
u32 bt_smart_user_data_send(u8*buf, u16 len)
{
	return ble_smart_user_data_send(buf, len);
}

/*----------------------------------------------------------------------------*/
/**@brief  bt smart 自定义数据接收， 对于较长的数据不会一次性返回， 需要开发使用者拼包处理
   @param  
   @return 
   @note  
*/
/*----------------------------------------------------------------------------*/
void bt_smart_user_data_recieve(u8 *buf, u16 len)
{
	bt_smart_printf("recive smart user data:\n");		
	printf_buf(buf, len);
}
///以下是自定义数据发送demo
//#if 0
//u8 user_data_send_buf[16] = {
//	0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xb,0xb,0xe,0xf	
//};
//void bt_smart_user_data_send_demo(void)
//{
//	u32 ret = bt_smart_user_data_send(user_data_send_buf, sizeof(user_data_send_buf));
//	if(ret)
//	{
//		bt_smart_printf("bt_smart_user_data_send_demo fail!!!\n");
//	}
//	else
//	{
//		bt_smart_printf("bt_smart_user_data_send_demo succ!!!\n");
//	}
//}
//#endif

