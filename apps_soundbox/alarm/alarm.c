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
#include "alarm_key.h"
#include "alarm.h"
#include "bt_smart_alarm.h"
#include "ui/led/led_eye.h"


#define ALARM_TIMEOUT_SECOND	(60L)

u8 g_alarm_back = 0;

/*----------------------------------------------------------------------------*/
/**@brief  Idle 任务
   @param  p：任务间参数传递指针
   @return 无
   @note   static void alarm_task(void *p)
*/
/*----------------------------------------------------------------------------*/
static void alarm_task(void *p)
{
	int msg[3];

    tbool psel_enable;
	u32 alarm_timeout = ALARM_TIMEOUT_SECOND * 2;
	u8 alarm_off_flag = 0;
	RTC_TIME alarm;

	alarm_puts("\n************************alarm TASK********************\n");

	eye_led_api(EFFECT_ALARM_ON, 0, 0);	
	bt_smart_alarm_read(&alarm);
	os_taskq_post_msg(ALARM_TASK_NAME, 1, MSG_ALM_ON); //触发idle模块初始化

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
					if(psel_enable)
					{
						psel_enable = 0;
						play_sel_stop();
					}
					bt_smart_alarm_clear_all_outdate_alarm(&alarm);

					if(compare_last_task_name(BTSTACK_TASK_NAME))
					{
						extern tbool bt_smart_connect_status(void);
						if(bt_smart_connect_status() == false)
						{
							eye_led_api(EFFECT_NO_CONNECT, 0 ,0);
						}
						else
						{
							eye_led_api(EFFECT_CONNECT_OK, 0 ,0);
						}

					}


					os_task_del_res_name(OS_TASK_SELF);
				}
				break;

			case SYS_EVENT_PLAY_SEL_END: //提示音结束
				psel_enable = 0;
                play_sel_stop();
				break;

			case MSG_ALM_ON:
				printf("alarm on ... \n");
				alarm_off_flag = 0;
				alarm_timeout = ALARM_TIMEOUT_SECOND * 2;
				play_sel_stop_without_end_msg();
				bt_smart_alarm_clear_all_outdate_alarm(&alarm);
				bt_smart_alarm_set_next_alarm(1);
				psel_enable = tone_play_by_name_rpt(ALARM_TASK_NAME, 10, 0, 1, BPF_ALARM_MP3);
				break;

			case MSG_ALARM_OFF:
				if(alarm_off_flag)
				{
					break;	
				}
				alarm_off_flag = 1;
				g_alarm_back = 1;
				if(bt_smart_alarm_poweron_check() == true)
				{
					printf("back to soft poweroff\n");
					bt_smart_alarm_clear_all_outdate_alarm(&alarm);
					os_taskq_post_msg(MAIN_TASK_NAME, 1, MSG_POWER_OFF);
					/* printf("poweron alarm, run bt mode fisrt\n"); */
					/* os_taskq_post_msg(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, BTSTACK_TASK_NAME); */
				}
				else
				{
					printf("back to lask mode\n");
					os_taskq_post_msg(MAIN_TASK_NAME, 1, MSG_LAST_WORKMOD);
				}
				break;

			case MSG_HALF_SECOND:
				bt_smart_alarm_debug_cur_alarm();
				if(alarm_timeout)
				{
					alarm_timeout --;
				}
				else
				{
					os_taskq_post_msg(ALARM_TASK_NAME, 1, MSG_ALARM_OFF);
				}
				break;

			default:
				break;
		}
	}
}


/*----------------------------------------------------------------------------*/
/**@brief  IDLE 任务创建
   @param  priv：任务间参数传递指针
   @return 无
   @note   static void alarm_task_init(void *priv)
*/
/*----------------------------------------------------------------------------*/
static void alarm_task_init(void *priv)
{
    u32 err;
	err = os_task_create(alarm_task,
			(void*)0,
			TaskAlarmlePrio,
			10
#if OS_TIME_SLICE_EN > 0
			,1
#endif
			,ALARM_TASK_NAME);

    if(OS_NO_ERR == err)
    {
        key_msg_register(ALARM_TASK_NAME, alarm_ad_table, alarm_io_table, alarm_ir_table, NULL);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief  IDLE 任务删除退出
   @param  无
   @return 无
   @note   static void alarm_task_exit(void)
*/
/*----------------------------------------------------------------------------*/
static void alarm_task_exit(void)
{
	if (os_task_del_req(ALARM_TASK_NAME) != OS_TASK_NOT_EXIST)
	{
		os_taskq_post_event(ALARM_TASK_NAME, 1, SYS_EVENT_DEL_TASK);
		do{
			OSTimeDly(1);
		} while(os_task_del_req(ALARM_TASK_NAME) != OS_TASK_NOT_EXIST);
		alarm_puts("del_alarm_task: succ\n");
	}
}


/*----------------------------------------------------------------------------*/
/**@brief  IDLE 任务信息
   @note   const struct task_info idle_task_info
*/
/*----------------------------------------------------------------------------*/
TASK_REGISTER(alarm_task_info) = {
	.name = ALARM_TASK_NAME,
	.init = alarm_task_init,
	.exit = alarm_task_exit,
};
