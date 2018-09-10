#include "common/app_cfg.h"
#include "rtos/os_api.h"
#include "rtos/os_cfg.h"
#include "common/error.h"
#include "common/msg.h"
#include "rtos/task_manage.h"
#include "sys_detect.h"
#include "dac/dac_api.h"
#include "play_sel/play_sel.h"
#include "key_drv/key.h"
#include "idle_key.h"
#include "idle.h"
#include "ai_toy.h"
#include "dac/dac.h"
#include "dac/dac_api.h"
#include "bt_smart.h"
#include "sys_detect.h"
#include "ui/led/led_eye.h"
#include "key_drv/key.h"
#include "key_drv/key_voice.h"
#include "timer_datetime.h"
#include "update.h"
#include "key_drv/key.h"


u8 first_power_on_flag = 1;

extern u8 bt_smart_update_flag;

extern void debug_loop_err_clear(void);
extern void ble_server_close(void);
extern void bt_edr_close(void);
extern bool ble_fw_download_task_create(void);
extern tbool ble_check_connect_complete(void);
extern u8 ble_update_get_status(void);
extern tbool bt_smart_connect_status(void);
extern void fw_update_file_delete(void);


static void idle_powerdown(void)
{
	printf("idle powerdown !!!\n");
	eye_led_api(EFFECTCLOSEALLLED, 0, 0);	
	ear_light_set_status(0);
	bt_smart_led_off();
	pa_mute();

	while(1)
	{
		clear_wdt();
		os_time_dly(100);		
	}
}


static tbool power_on_notice_play_callback(void *priv, int msg[])
{
	return false;
}


static tbool update_notice_play_callback(void *priv, int msg[])
{

	if(!first_power_on_flag)
	{
		switch(msg[0])		
		{
			case SYS_EVENT_DEV_ONLINE:
				printf("--idle SYS_EVENT_DEV_ONLINE\r");

				return true;

			case SYS_EVENT_DEV_OFFLINE:
				printf("--idle SYS_EVENT_DEV_OFFLINE\r");
				return true;

			default:
				break;		
		}

	}
	return false;
}

static void power_on_deal(void)
{
	int msg[3];
	NOTICE_PLAYER_ERR n_err;
	int poweron_flag = update_poweron_check();

	if(poweron_flag == 0)
	{
		eye_led_api(EFFECT_POWER_ON, 0, 0);	
		bt_smart_led_on();
		n_err = notice_player_play_by_path(IDLE_TASK_NAME, 
				(char *)AI_TOY_NOTICE_POWER_ON, 
				power_on_notice_play_callback,
				NULL);
		//开机播放完提示音切换到蓝牙模式
		os_taskq_post_msg(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, BTSTACK_TASK_NAME);

		while(1)
		{
			memset(msg,0x00,sizeof(msg));
			os_taskq_pend(0, ARRAY_SIZE(msg), msg);
			clear_wdt();
			switch(msg[0])
			{
				case SYS_EVENT_DEL_TASK:
					if (os_task_del_req_name(OS_TASK_SELF) == OS_TASK_DEL_REQ)
					{
						os_task_del_res_name(OS_TASK_SELF);
					}
					break;

				default:
					break;
			}
		}
	}
	else if(poweron_flag == 1)
	{
		printf("update ok\n");
		eye_led_api(EFFECTSMILE, 0, 0);	
		os_time_dly(200);
		n_err = notice_player_play_by_path(IDLE_TASK_NAME, 
				(char *)AI_TOY_NOTICE_UPDATE_OK, 
				update_notice_play_callback,
				NULL);
	}
	else
	{
		printf("update err\n");
		eye_led_api(EFFECTCRY, 0, 0);	
		os_time_dly(200);
		n_err = notice_player_play_by_path(IDLE_TASK_NAME, 
				(char *)AI_TOY_NOTICE_UPDATE_FAIL, 
				update_notice_play_callback,
				NULL);
	}

	pa_mute();
	while(1)
	{
		clear_wdt();
		os_time_dly(100);		
	}
	/* soft_power_ctl(PWR_OFF); */
}



static void idle_task(void *p)
{
	NOTICE_PLAYER_ERR n_err;
	dac_channel_on(MUSIC_CHANNEL, FADE_ON);
	set_sys_vol(dac_ctl.sys_vol_l,dac_ctl.sys_vol_r,FADE_ON);

	if(g_powerdown_flag)
	{
		/* eye_led_api(EFFECT_POWER_OFF, 0 ,0); */
		n_err = notice_player_play_by_path(IDLE_TASK_NAME, 
				(char *)AI_TOY_NOTICE_LOW_POWER_OFF, 
				power_on_notice_play_callback,
				NULL);
		if(first_power_on_flag)
		{
				
		}
		else
		{
			ble_server_close();
			bt_edr_close();
		}

		idle_powerdown();
	}
	else
	{
		if(first_power_on_flag)
		{
			power_on_deal();
		}
		else
		{
			if(bt_smart_update_flag)
			{
				bt_edr_close();
				os_time_dly(50);
				pa_mute();
				ble_fw_download_task_create();
				while(1)
				{
					clear_wdt();
					if((ble_check_connect_complete() == false) && (ble_update_get_status() != 1))	
					{
						NOTICE_PLAYER_ERR n_err;
						printf("ble update disconnect err !!\n");

						fw_update_file_delete();

						eye_led_api(EFFECTCRY, 0, 0);	
						n_err = notice_player_play_by_path(IDLE_TASK_NAME, 
								(char *)AI_TOY_NOTICE_UPDATE_FAIL, 
								update_notice_play_callback,
								NULL);

						ble_server_close();
						while(1)
						{
							clear_wdt();
							os_time_dly(100);		
						}
					}

					eye_led_api(EFFECT_UPDATE_ING, 0 ,0);
					os_time_dly(100);		
				}
			}
		}
	}
}


/*----------------------------------------------------------------------------*/
/**@brief  IDLE 任务创建
   @param  priv：任务间参数传递指针
   @return 无
   @note   static void idle_task_init(void *priv)
*/
/*----------------------------------------------------------------------------*/
static void idle_task_init(void *priv)
{
    u32 err;
	err = os_task_create(idle_task,
			(void*)0,
			TaskIdlePrio,
			10
#if OS_TIME_SLICE_EN > 0
			,1
#endif
			,IDLE_TASK_NAME);

    if(OS_NO_ERR == err)
    {
        key_msg_register(IDLE_TASK_NAME, idle_ad_table, idle_io_table, idle_ir_table, NULL);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief  IDLE 任务删除退出
   @param  无
   @return 无
   @note   static void idle_task_exit(void)
*/
/*----------------------------------------------------------------------------*/
static void idle_task_exit(void)
{
	if (os_task_del_req(IDLE_TASK_NAME) != OS_TASK_NOT_EXIST)
	{
		os_taskq_post_event(IDLE_TASK_NAME, 1, SYS_EVENT_DEL_TASK);
		do{
			OSTimeDly(1);
		} while(os_task_del_req(IDLE_TASK_NAME) != OS_TASK_NOT_EXIST);
		idle_puts("del_idle_task: succ\n");
	}
}


/*----------------------------------------------------------------------------*/
/**@brief  IDLE 任务信息
   @note   const struct task_info idle_task_info
*/
/*----------------------------------------------------------------------------*/
TASK_REGISTER(idle_task_info) = {
	.name = IDLE_TASK_NAME,
	.init = idle_task_init,
	.exit = idle_task_exit,
};
