#include "timer_datetime.h"
#include "sdk_cfg.h"
#include "timer.h"
#include "key_drv/key.h"


typedef struct __TIMER_DATETIME
{
	RTC_TIME date;
	RTC_TIME alarm;
	volatile u8 sync_flag;
	volatile u8 alarm_enable;
	volatile u8 alarm_on;
}TIMER_DATETIME;

TIMER_DATETIME cur_timer_datatime = 
{
	.sync_flag = 0,
	.alarm_enable = 0,
	.alarm_on = 0,
};


typedef enum
{
    TIME_MEMBER_YEAR = 0x0,
    TIME_MEMBER_MONTH,
    TIME_MEMBER_DAY,
    TIME_MEMBER_HOUR,
    TIME_MEMBER_MIN,
    TIME_MEMBER_SEC,
    TIME_MEMBER_MAX,
}TIME_MEMBER_ENUM;

static int timer_datetime_num_cmp(u32 num1, u32 num2)
{
    if(num1 > num2)
    {
        return 1;
    }
    else if(num1 == num2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

static int timer_datetime_num_member_cmp(RTC_TIME time1, RTC_TIME time2, TIME_MEMBER_ENUM type)
{
    switch(type)
    {
        case TIME_MEMBER_YEAR:
            return timer_datetime_num_cmp(time1.dYear,  time2.dYear);
        case TIME_MEMBER_MONTH:
            return timer_datetime_num_cmp(time1.bMonth, time2.bMonth);
        case TIME_MEMBER_DAY:
            return timer_datetime_num_cmp(time1.bDay,   time2.bDay);
        case TIME_MEMBER_HOUR:
            return timer_datetime_num_cmp(time1.bHour,  time2.bHour);
        case TIME_MEMBER_MIN:
            return timer_datetime_num_cmp(time1.bMin,   time2.bMin);
        case TIME_MEMBER_SEC:
            return timer_datetime_num_cmp(time1.bSec,   time2.bSec);
		default:
			return -2;
    }
	return -2;
}

static int timer_datetime_cmp(RTC_TIME time1, RTC_TIME time2)
{
    u8 i;
    int ret = 0;
    for(i=0; i<TIME_MEMBER_MAX; i++)
    {
        ret = timer_datetime_num_member_cmp(time1, time2, i);
        if(ret != 0)
        {
            break;
        }
    }
    return ret;
}




void timer_datetime_sync(RTC_TIME *time)
{
	OS_ENTER_CRITICAL();	
	memcpy((u8 *)&(cur_timer_datatime.date), (u8*)time, sizeof(RTC_TIME));
	cur_timer_datatime.sync_flag = 1;
	OS_EXIT_CRITICAL();
}

void timer_datetime_set_alarm(RTC_TIME *alarm)
{
	OS_ENTER_CRITICAL();	
	cur_timer_datatime.alarm_enable = 1;
	memcpy((u8*)&cur_timer_datatime.alarm, (u8 *)alarm, sizeof(RTC_TIME));
	OS_EXIT_CRITICAL();
}

void timer_datetime_disable_alarm(void)
{
	OS_ENTER_CRITICAL();	
	cur_timer_datatime.alarm_enable = 0;
	OS_EXIT_CRITICAL();
}


static void timer_datetime_run(void *p)
{
	if(cur_timer_datatime.sync_flag == 0)	
		return ;
	/* printf("t:%4d-%2d-%2d, %2d:%2d:%2d\n",cur_timer_datatime.date.dYear,cur_timer_datatime.date.bMonth,cur_timer_datatime.date.bDay,\ */
			/* cur_timer_datatime.date.bHour,cur_timer_datatime.date.bMin,cur_timer_datatime.date.bSec); */
	cur_timer_datatime.date.bSec ++;
	if(cur_timer_datatime.date.bSec >= 60)
	{
		cur_timer_datatime.date.bSec -= 60;
		cur_timer_datatime.date.bMin++;
		if(cur_timer_datatime.date.bMin >= 60)
		{
			cur_timer_datatime.date.bMin -= 60;
			cur_timer_datatime.date.bHour++;
			if(cur_timer_datatime.date.bHour >= 24)
			{
				cur_timer_datatime.date.bHour -= 24;
				cur_timer_datatime.date.bDay ++;
				if(cur_timer_datatime.date.bDay > month_for_day(cur_timer_datatime.date.bMonth, cur_timer_datatime.date.dYear))
				{
					cur_timer_datatime.date.bDay = 1;
					cur_timer_datatime.date.bMonth ++; 
					if(cur_timer_datatime.date.bMonth > 12)
					{
						cur_timer_datatime.date.bMonth = 1;
						cur_timer_datatime.date.dYear ++;
					}
				}
			}
		}
	}


	if(cur_timer_datatime.alarm_enable == 1 && cur_timer_datatime.alarm_on == 0)	
	{

	/* printf("a:%4d-%2d-%2d, %2d:%2d:%2d\n",cur_timer_datatime.alarm.dYear,cur_timer_datatime.alarm.bMonth,cur_timer_datatime.alarm.bDay,\ */
			/* cur_timer_datatime.alarm.bHour,cur_timer_datatime.alarm.bMin,cur_timer_datatime.alarm.bSec); */

		if(timer_datetime_cmp(cur_timer_datatime.date, cur_timer_datatime.alarm) >= 0)
		{
			cur_timer_datatime.alarm_enable = 0;
			cur_timer_datatime.alarm_on = 1;
			/* printf("alarm on !!!!!!!!!!!!!!!!!!\n"); */
			os_taskq_post_msg(MAIN_TASK_NAME, 1, MSG_ALM_ON);
		}
	}
}




void timer_datetime_init(void)
{
	memset((u8*)&cur_timer_datatime, 0x0, sizeof(TIMER_DATETIME));

	s32 ret = timer_reg_isr_fun(timer0_hl,500,(void *)timer_datetime_run,NULL);
	if(ret != TIMER_NO_ERR)
	{
		printf("timer datetime err = %x\n",ret);
	}
}

tbool timer_datetime_get_cur(RTC_TIME *time)
{
	if(time == NULL)
		return false;

	if(cur_timer_datatime.sync_flag)	
	{
		memcpy((u8*)time, (u8*)&(cur_timer_datatime.date), sizeof(RTC_TIME));
		return true;
	}

	return false;
}

tbool timer_datetime_get_cur_alarm(RTC_TIME *alarm)
{
	if(alarm == NULL)
		return false;
	if(cur_timer_datatime.alarm_on)
	{
		memcpy((u8*)alarm, (u8*)&(cur_timer_datatime.alarm), sizeof(RTC_TIME));
		return true;
	}

	return false;
}


u8 timer_datetime_check_alarm_status(void)
{
	return cur_timer_datatime.alarm_on;
}


void timer_datetime_set_alarm_off(void)
{
	cur_timer_datatime.alarm_on = 0;
}
