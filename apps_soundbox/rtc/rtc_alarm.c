#include "rtc_alarm.h"
#include "rtc/rtc_api.h"
#include "sdk_cfg.h"
#include "key_drv/key.h"
#include "sys_detect.h"
#include "vm/vm_api.h"
#include "rcsp/rcsp_interface.h"
#include "rcsp/rcsp_head.h"
#include "timer_datetime.h"
#if 0
#define ALARM_NUM_MAX		(5)


#define USE_RTC_MODULE_ALARM_EN		1

#if(USE_RTC_MODULE_ALARM_EN)
u8 rtc_module_alarm_status(void)
{
	return 0;	
}
#define set_alarm		rtc_module_write_alarm_datetime	
#define set_datetime	rtc_module_write_rtc_datetime	
#define read_alarm		rtc_module_read_alarm_datetime
#define read_datetime	rtc_module_read_rtc_datetime
#define alarm_switch	rtc_module_alarm_switch
#define alarm_status	rtc_module_alarm_status
#else
#define set_alarm		timer_datetime_set_alarm	
#define set_datetime	timer_datetime_set_datetime	
#define read_alarm		timer_datetime_get_cur_alarm
#define read_datetime	timer_datetime_get_cur_datetime
#define alarm_switch	timer_datetime_alarm_switch
#define alarm_status	timer_datetime_alarm_status
#endif//USE_RTC_MODULE_ALARM_EN

enum{
	RTC_ALARM_OP_SYNC_TIME = 0x0,
	RTC_ALARM_OP_GET_INFO,
	RTC_ALARM_OP_ADD,
	RTC_ALARM_OP_DEL,

};

#define ALARM_NAME_LEN			(24)
typedef struct __ALARM_ITEM
{
	u8 index;
	u8 hour;//高位表示闹钟是否开启
	u8 min;
	u8 repeat_mode;//单次、一、二、三...日
	u8 name[ALARM_NAME_LEN];
	u8 Reserved[8];
}ALARM_ITEM;

#define VM_ALARM_ITEM_LEN		(sizeof(ALARM_ITEM) + 4)

static int num_cmp(u32 num1, u32 num2)
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


static void rtc_alarm_isr(u8 flag)
{
    if(INTERRUPT_TYPE_ALARM_COME == flag)
    {
        puts("--ALM-ON-ISR--\n");
        rtc_module_alarm_switch(0);  //响闹后关闭闹钟
		if(keymsg_task_name)
		{
			os_taskq_post(keymsg_task_name, 1, MSG_ALM_ON);
		}
    }
    if(INTERRUPT_TYPE_PCNT_OVF==flag)
    {
        puts("--TYPE_PCNT_OVF--\n");
    }
    if(INTERRUPT_TYPE_LDO5V_DET==flag)
    {
        puts("--LDO5V DET--\n");
    }
}

static void rtc_alarm_reset(void)
{
	RTC_TIME time;
	RTC_TIME alarm;
	rtc_module_read_rtc_datetime(&time);
    rtc_module_read_alarm_datetime(&alarm);
    if (((time.dYear > 2100)
            || (time.dYear < 2000))
            || (time.bMonth >= 12)
            || (time.bHour >= 24)
            || (time.bMin >= 60)
            || (time.bSec >= 60)
            ||(rtc_module_get_power_up_flag()))
    {
        puts("\n--------RTC RESET--------\n");
        time.dYear = 2016;
        time.bMonth = 9;
        time.bDay = 26;
        time.bHour = 0;
        time.bMin = 0;
        time.bSec = 0;

        //puts("\n--------ALM RESET--------\n");
        memcpy(&alarm, &time,sizeof(RTC_TIME));

        ///update date
        rtc_module_write_rtc_datetime(&time);
        rtc_module_write_alarm_datetime(&alarm , 0);
    }

    rtc_module_reset_irtc();
}

u8 rtc_alarm_init(void)
{
	u8 is_alarm_comming;	
    if(rtc_module_get_alm_come_flag())
    {
        //update mode ,maybe powerup alarming
		printf("is alarm power up !!!!!\n");
		is_alarm_comming = 1;
    }
    else
    {
		is_alarm_comming = 0;
    }

    rtc_module_on(WAKE_UP_ENABLE, 0 , rtc_alarm_isr);
	rtc_alarm_reset();
	set_keep_osci_flag(1);

	printf("rtc alarm init ok\n");
	return is_alarm_comming;
}

static tbool rtc_alarm_sort(ALARM_ITEM alarm_item[], u8 counter)
{
	if(alarm_item == NULL || counter == 0)		
		return false;

	u8 i ,j;
	int ret = 0;
	ALARM_ITEM tmp_item;
	for (i=0; i<counter; i++)
	{
		for (j=0; j<(counter-i-1); j++)
		{
			ret = num_cmp(alarm_item[j].hour & 0x7f, alarm_item[j+1].hour & 0x7f);
			if(ret == 0)
			{
				ret = num_cmp(alarm_item[j].min, alarm_item[j+1].min);
			}

			if(ret > 0)
			{
				memcpy((u8 *)&tmp_item,(u8 *)&alarm_item[j+1],sizeof(ALARM_ITEM));
				memcpy((u8 *)&alarm_item[j+1],(u8 *)&alarm_item[j],sizeof(ALARM_ITEM));
				memcpy((u8 *)&alarm_item[j],(u8 *)&tmp_item,sizeof(ALARM_ITEM));
			}
		}
	}
	return true;
}




static u8 rtc_alarm_read_one(u8 vm_index, ALARM_ITEM *item)
{
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	u8 i;
	s32 read_len = 0;
	for(i=vm_index; i<ALARM_NUM_MAX; i++)
	{
		read_len = vm_read_api(VM_RTC_ALARM_0 + i, tmp);	

		if(read_len == VM_RTC_ALARM_LEN)
		{
			/* printf_buf(tmp, VM_ALARM_ITEM_LEN); */
			if(tmp[0] == 'A' && tmp[1] == 'L' && tmp[2] == 'M' && tmp[3] == 'I')
			{
				memcpy((u8 *)item, &tmp[4], sizeof(ALARM_ITEM));		
				break;
			}
		}
	}

	if(i < ALARM_NUM_MAX)
	{
		return i;	
	}

	return 0xff;	
}

static u8 rtc_alarm_find(u8 index)
{
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	ALARM_ITEM item;
	u8 i;
	s32 read_len = 0;
	for(i=0; i<ALARM_NUM_MAX; i++)
	{
		read_len = vm_read_api(VM_RTC_ALARM_0 + i, tmp);	

		if(read_len == VM_RTC_ALARM_LEN)
		{
			if(tmp[0] == 'A' && tmp[1] == 'L' && tmp[2] == 'M' && tmp[3] == 'I')
			{
				memcpy((u8 *)&item, &tmp[4], sizeof(ALARM_ITEM));		
				if(item.index == index)
				{
					break;	
				}
			}
		}
	}
	if(i < ALARM_NUM_MAX)
	{
		return i;	
	}

	return 0xff;	
}


static u8 rtc_alarm_find_empty(void)
{
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	ALARM_ITEM item;
	u8 i;
	s32 read_len = 0;
	for(i=0; i<ALARM_NUM_MAX; i++)
	{
		read_len = vm_read_api(VM_RTC_ALARM_0 + i, tmp);	
		if(read_len == VM_RTC_ALARM_LEN)
		{
			if(tmp[0] == 'A' && tmp[1] == 'L' && tmp[2] == 'M' && tmp[3] == 'I')
			{
				continue;
			}
			else
			{
				break;		
			}
		}
		else
		{
			break;		
		}
	}

	if(i < ALARM_NUM_MAX)
	{
		return i;	
	}

	return 0xff;	
}

static u8 rtc_alarm_count(void)
{
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	ALARM_ITEM item;
	u8 i;
	u8 j = 0;
	s32 read_len = 0;
	for(i=0; i<ALARM_NUM_MAX; i++)
	{
		read_len = vm_read_api(VM_RTC_ALARM_0 + i, tmp);	
		if(read_len == VM_RTC_ALARM_LEN)
		{
			/* printf("VM_RTC_ALARM_0 + i = %d, read_len = %d\n", VM_RTC_ALARM_0 + i, read_len); */
			/* printf_buf(tmp, VM_ALARM_ITEM_LEN); */
			if(tmp[0] == 'A' && tmp[1] == 'L' && tmp[2] == 'M' && tmp[3] == 'I')
			{
				/* printf("find one !!\n"); */
				j++;
			}
		}
	}

	printf("alarm count =  %d\n", j);
	return j;	
}




typedef struct
{
    u8  pk_flag[2];
    u8  pk_crc[2];
    u8  data_len[2];
    u8  data[1];
}JL_data_packet_t;

typedef struct
{
    u8 id_str[4];
    u8 len[4];  //high 8bit is code_type
    u8 value[1];
}JL_frame_t;
extern void rcsp_fill_packet_head(u8 *pkt_p,u16 pkt_flag,u16 data_len,u16 cal_crc_flag);
extern u32 rcsp_fill_packet_frame(JL_frame_t* frame_p,char *id_str, u8 code_type,u8 *value,u32 value_len);
extern u8 rcsp_check_csw_ackflag(u8 check_type,u8 send_data_flag);
extern u32 rcsp_data_send(u8 *buf, u16 len, u8 need_csw_replay_flag);
#define CSW_REQUEST_ACK_CBW         (1 << 0)

extern u8  rcsp_ack_csw_flag;
u32 fw_report_rtc_alarm_info(void)
{
    u32 err = RCSP_OP_ERR_NONE;
    /* u8 task_cnt, fw_app_id; */
    /* int app_id; */
	ALARM_ITEM item;
	u8 i;
	u8 vm_index = 0;
    u32 offset = 0;
    JL_data_packet_t *data_pt;
	u8 alarm_count = rtc_alarm_count();


    data_pt = malloc(256);

    offset = offset + rcsp_fill_packet_frame((void*)&data_pt->data[offset], "ALMN", FRAME_TYPE_ANSI, &alarm_count, 1);
	for(i=0; i<alarm_count; i++)
	{
		//find alarm
		vm_index = rtc_alarm_read_one(vm_index, &item);
		if(vm_index != 0xff)
		{
			vm_index ++;
			offset = offset + rcsp_fill_packet_frame((void*)&data_pt->data[offset], "ALMI", FRAME_TYPE_ANSI, (void*)&item, sizeof(ALARM_ITEM));
		}
	}

    rcsp_fill_packet_head((void*)data_pt, 0xffff, offset, 1);
	/* printf("alarm send data  \n"); */
	/* printf_buf((u8*)data_pt, sizeof(JL_data_packet_t) + offset -1); */
    err = rcsp_data_send((void*)data_pt,sizeof(JL_data_packet_t) + offset -1, rcsp_check_csw_ackflag(CSW_REQUEST_ACK_CBW,1));
    rcsp_ack_csw_flag = 0;
    free(data_pt);
 
	/* printf("alarm send data end\n"); */
	return err;
}

static tbool rtc_alarm_add(ALARM_ITEM *item)
{
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	if(item == NULL)
		return false;

	u8 vm_index = rtc_alarm_find(item->index);

	if(vm_index == 0xff)
	{
		printf("not found the same index !!\n");
		vm_index = rtc_alarm_find_empty();
		if(vm_index == 0xff)
		{
			printf("over limit !!!!\n");
			return false;
		}
	}

	tmp[0] = 'A';
	tmp[1] = 'L';
	tmp[2] = 'M';
	tmp[3] = 'I';
	memcpy(&tmp[4], (u8*)item, sizeof(ALARM_ITEM));
	/* printf_buf(tmp, VM_ALARM_ITEM_LEN); */
    vm_write_api(vm_index + VM_RTC_ALARM_0, tmp);

	vm_check_all(0);

	/* if(rtc_module_get_alarm_flag() == 0) */
//	{
//		//如果当前没有闹钟开启， 告知控制线程， 查找最近要响的闹钟，并设置
//		os_taskq_post(keymsg_task_name, 1, MSG_ALM_SETTING);
//	}

	printf("alarm add ok!!\n");
	return true;		
}

static tbool rtc_alarm_del(u8 index)
{
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	u8 vm_index = rtc_alarm_find(index);
	if(vm_index == 0xff)
	{
		printf("not found the index alarm to del!!!\n");
		return false;
	}

	memset(tmp, 0x0, VM_ALARM_ITEM_LEN);
    vm_write_api(vm_index + VM_RTC_ALARM_0, tmp);
	vm_check_all(0);
	return true;		
}


static u8 rtc_alarm_get_all_alarm_info(ALARM_ITEM item[ALARM_NUM_MAX])
{
	ALARM_ITEM item_tmp[ALARM_NUM_MAX];
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	s32 read_len = 0;
	u8 i = 0;
	u8 j = 0;
	u8 cnt = 0;
	memset((u8*)item, 0x0, sizeof(ALARM_ITEM)*ALARM_NUM_MAX);	
	memset((u8*)&item_tmp, 0x0, sizeof(ALARM_ITEM)*ALARM_NUM_MAX);	
	for(i=0; i<ALARM_NUM_MAX; i++)
	{
		read_len = vm_read_api(VM_RTC_ALARM_0 + i, tmp);	

		if(read_len == VM_RTC_ALARM_LEN)
		{
			if(tmp[0] == 'A' && tmp[1] == 'L' && tmp[2] == 'M' && tmp[3] == 'I')
			{
				printf("+++++++++\n");
				memcpy((u8 *)(item_tmp + j), &tmp[4], sizeof(ALARM_ITEM));		
				j++;
				/* break; */
			}
		}	
	}

	///找出所有使能的闹钟
	for(i=0; i<j; i++)
	{
		if((item_tmp[i].hour & BIT(7)) != 0)
		{
			cnt++;
				printf("------------------\n");
			memcpy((u8*)&item[i], (u8 *)&item_tmp[i], sizeof(ALARM_ITEM));		
		}
	}
	return cnt;		
}


tbool rtc_alarm_get_nearest_alarm(RTC_TIME *alarm_time)
{
	if(alarm_time == NULL)
		return false;
	RTC_TIME time;
	ALARM_ITEM alarm_item[ALARM_NUM_MAX];
	ALARM_ITEM alarm_item_tmp[ALARM_NUM_MAX];
	ALARM_ITEM alarm;
	u8 tmp[VM_ALARM_ITEM_LEN] = {0};
	u8 week_date = 0;
	u8 i = 0;
	u8 j = 0;
	u8 k = 0;
	u8 cnt = 0;
	u8 find = 0;
	u8 find_once_alarm = 0;
	u8 out_of_date = 0;
	s32 read_len = 0;
	int ret = 0;
	memset((u8*)&alarm_item, 0x0, sizeof(ALARM_ITEM)*ALARM_NUM_MAX);	
	memset((u8*)&alarm_item_tmp, 0x0, sizeof(ALARM_ITEM)*ALARM_NUM_MAX);	
	for(i=0; i<ALARM_NUM_MAX; i++)
	{
		read_len = vm_read_api(VM_RTC_ALARM_0 + i, tmp);	

		if(read_len == VM_RTC_ALARM_LEN)
		{
			if(tmp[0] == 'A' && tmp[1] == 'L' && tmp[2] == 'M' && tmp[3] == 'I')
			{
				memcpy((u8 *)(alarm_item_tmp + j), &tmp[4], sizeof(ALARM_ITEM));		
				j++;
				/* break; */
			}
		}	
	}

	///找出所有使能的闹钟
	for(i=0; i<j; i++)
	{
		if((alarm_item_tmp[i].hour & BIT(7)) != 0)
		{
			memcpy((u8*)&alarm_item[cnt], (u8 *)&alarm_item_tmp[i], sizeof(ALARM_ITEM));		
			cnt++;
		}
	}

	printf("alarm open, j = %d, cnt = %d\n", j, cnt);
	if(cnt == 0)
	{
		return false;		
	}

	if(rtc_alarm_sort(alarm_item, cnt) == false)
	{
		printf("rtc_alarm_sort err\n");
		return false;		
	}

	///计算出今天是星期几, 并且从改天开始
	read_datetime(&time);
	week_date = rtc_calculate_week_val(&time);
	if(week_date == 0)
	{
			week_date = 7;//公式算出的星期天是0, 这里算法用7表示星期天
	}
	for(i=0; i<cnt; i++)
	{
		if((alarm_item[i].repeat_mode &  BIT(0)) != 0)
		{
			ret = num_cmp(time.bHour, (alarm_item[i].hour & 0x7f));
			if(ret == 0)
			{
				ret = num_cmp(time.bMin, alarm_item[i].min);
			}

			if(ret >= 0)
			{
				//今天时间超过了闹钟时间
				u8 next_day = week_date + 1;
				if(next_day > 7)
				{
					next_day = 1;	
				}
				alarm_item[i].repeat_mode |= BIT(next_day);
			}
			else
			{
				alarm_item[i].repeat_mode |= BIT(week_date);
			}
		}
	}


	//从当天开始 ，查找接下来一个星期内最近的一个闹钟
	k = week_date;
	printf("today is week day %d\n", k);
	for(i=0; i<7; i++)
	{
		for(j = 0; j<cnt; j++)
		{
		/* printf_buf((u8*)&alarm_item[j], sizeof(ALARM_ITEM)); */
			if((alarm_item[j].repeat_mode & BIT(k)) != 0)
			{
				if(k == week_date)//如果是当天,判断此闹钟时间是否过时,单次闹钟不会进入此判断
				{
					printf("al hour = %d, min = %d, repeat_mode %d\n", (alarm_item[j].hour & 0x7f), alarm_item[j].min, alarm_item[j].repeat_mode);
					ret = num_cmp(time.bHour, (alarm_item[j].hour & 0x7f));
					if(ret == 0)
					{
						ret = num_cmp(time.bMin, alarm_item[j].min);
					}

					if(ret >= 0)
					{
						//过时了
						if(out_of_date == 0)
						{
							printf("alarm outfdate today\n");
							out_of_date = 1;
							memcpy((u8*)&alarm, (u8 *)&alarm_item[j], sizeof(ALARM_ITEM));// 如果之后没有找到闹钟，第一个过时的闹钟就是最近的闹钟		
						}
				/* printf("func =%s, line = %d\n", __FUNCTION__, __LINE__); */
						continue;
					}
				}
				/* printf("func =%s, line = %d\n", __FUNCTION__, __LINE__); */
				find = 1;
				memcpy((u8*)&alarm, (u8 *)&alarm_item[j], sizeof(ALARM_ITEM));
				break;
			}
		}

		if(find)
		{
			printf("find one alarm !!!\n");
			break;
		}

		k ++;
		if(k > 7)
		{
			k = 1;
		}
	}


	if((find == 0) && (out_of_date == 0))
	{
		printf("no found alarm\n");
		return false;
	}
	
///计算闹钟的时间
	alarm_time->bSec = 0;
	alarm_time->bMin = alarm.min;
	alarm_time->bHour = alarm.hour & 0x7f;
	alarm_time->bDay = time.bDay;
	if(find)
	{
		alarm_time->bDay += i;
	}
	else
	{
		alarm_time->bDay += 7;//如果是当天过时,下个星期的这一天就是闹钟，所以加7
	}
	printf("++++++++++ i = %d, day = %d\n", i, alarm_time->bDay);
	alarm_time->bMonth = time.bMonth; 
	alarm_time->dYear = time.dYear;
	if(alarm_time->bDay > month_for_day(time.bMonth, time.dYear))
	{
		alarm_time->bDay -= month_for_day(time.bMonth, time.dYear);
		alarm_time->bMonth++; 
		if(alarm_time->bMonth > 12)
		{
			alarm_time->bMonth = 1;
			alarm_time->dYear++;
		}
	}

	return true;
}
///主要是清除单次的闹钟
static void rtc_alarm_clear_all_outdate_alarm(void)
{
	RTC_TIME time;
	RTC_TIME alarm;
	ALARM_ITEM item[ALARM_NUM_MAX];
	u8 cnt;
	u8 i;
	int ret;
	/* rtc_module_read_rtc_datetime(&time); */

	read_datetime(&time);
	if(alarm_status() == 1)
	{
		read_alarm(&alarm);
	}
	else
	{
		printf("no alarm on !!!\n");
		return;	
	}

	cnt = rtc_alarm_get_all_alarm_info(item);
	if(cnt == 0)
		return;
	/* printf("fund = %s, line = %d, cnt = %d\n", __FUNCTION__, __LINE__, cnt); */
	for(i=0; i<cnt; i++)
	{
		/* printf_buf((u8*)&item[i], sizeof(ALARM_ITEM)); */
		if((item[i].repeat_mode & BIT(0)) != 0)
		{
	/* printf("fund = %s, line = %d\n", __FUNCTION__, __LINE__); */
			ret = num_cmp(alarm.bHour, (item[i].hour & 0x7f));
			if(ret == 0)
			{
				ret = num_cmp(alarm.bMin, item[i].min);
			}

			if(ret <= 0)
			{
	/* printf("fund = %s, line = %d\n", __FUNCTION__, __LINE__); */
				ret = num_cmp(time.bHour, (item[i].hour & 0x7f));
				if(ret == 0)
				{
					ret = num_cmp(time.bMin, item[i].min);
				}
				if(ret >= 0)
				{
	/* printf("fund = %s, line = %d\n", __FUNCTION__, __LINE__); */
					///找到处于闹钟响到闹钟停止或者被打断这段时间内容响的闹钟,并将这些闹钟设置为过期		
					printf("find one outdate alarm !!!!~~~~\n");
					item[i].hour &= ~BIT(7);
					rtc_alarm_add(&item[i]);
				}
			}
		}
	}
}

void rtc_alarm_set_next_alarm(u8 flag/*clear outdate alarm*/)
{
	RTC_TIME alarm;
	//clear所有单次改时间点的闹钟
	if(flag)
	{
		rtc_alarm_clear_all_outdate_alarm();
	}
	//找下一个最近的闹钟
	if(rtc_alarm_get_nearest_alarm(&alarm) == true)
	{
		printf("set alarm ok \n");

	printf("alarm val ===========,%d-%d-%d, %d:%d:%d\n",alarm.dYear,alarm.bMonth,alarm.bDay,\
			alarm.bHour,alarm.bMin,alarm.bSec);
		set_alarm(&alarm, 0);
		alarm_switch(1);
	}
	else
	{
		printf("set next alarm err!!\n");		
		alarm_switch(0);
	}
}

static void rtc_alarm_sync_time(u8 *OperationData, u8 *data, u16 len)
{
	RTC_TIME time;
	OperationData += 2;
	time.dYear = READ_BIG_U16(&OperationData[0]);
	time.bMonth = OperationData[2];
	time.bDay = OperationData[3];
	time.bHour = OperationData[4];
	time.bMin = OperationData[5];
	time.bSec = OperationData[6];

	/* printf("sync data len = %d!!!!!\n", len); */
	/* printf_buf(OperationData, len); */

	printf("sync time,%d-%d-%d, %d:%d:%d\n",time.dYear,time.bMonth,time.bDay,\
			time.bHour,time.bMin,time.bSec);


	/* rtc_module_write_rtc_datetime(&time); */
	set_datetime(&time);

}




tbool rtc_alarm_rcsp_alarm_add(u8 *OperationData, u8 *data, u16 len)
{
	ALARM_ITEM item;
	memset((u8*)&item, 0x0, sizeof(ALARM_ITEM));
	printf("add data len = %d, data =  \n");
	printf_buf(data, len);
	data+=14;
	item.index = data[0];
	item.hour = data[1];
	item.min = data[2];
	item.repeat_mode = data[3];
	memcpy(item.name, &data[4], ALARM_NAME_LEN);
	return rtc_alarm_add(&item);		
}

tbool rtc_alarm_rcsp_alarm_del(u8 *OperationData, u8 *data, u16 len)
{
	u8 index = OperationData[2];
	return rtc_alarm_del(index);		
}


u32 rtc_alarm_rcsp_op_deal(u8 *OperationData, u8 *data, u16 len)
{
    u32 err = RCSP_OP_ERR_NONE;
	switch(OperationData[1])
	{
		case RTC_ALARM_OP_SYNC_TIME:
			rtc_alarm_sync_time(OperationData, data, len);
			rtc_alarm_set_next_alarm(0);
			break;

		case RTC_ALARM_OP_GET_INFO:
			err = fw_report_rtc_alarm_info();
			break;

		case RTC_ALARM_OP_ADD:
			if(rtc_alarm_rcsp_alarm_add(OperationData, data, len))
			{
				rtc_alarm_set_next_alarm(0);					
				/* os_taskq_post(keymsg_task_name, 1, MSG_ALM_SETTING); */
			}
			else
			{
				err = CSW_ERR_OPT;	
			}
			break;

		case RTC_ALARM_OP_DEL:
			rtc_alarm_rcsp_alarm_del(OperationData, data, len);
			rtc_alarm_set_next_alarm(0);					
			break;

		default:
			break;
	}
	return err;
}
#endif

static void  (*rtc_alarm_on_callback)(void) = NULL;

static volatile u8 rtc_alarm_on_flag = 0;

u8 rtc_alarm_status(void)
{
	return rtc_alarm_on_flag;	
}

static void rtc_alarm_isr(u8 flag)
{

	printf("rtc_alarm_isr 00\n");
    if(INTERRUPT_TYPE_ALARM_COME == flag)
    {
		puts("--ALM-ON-ISR--\n");
		rtc_alarm_on_flag = 1;
		/* rtc_module_alarm_switch(0);  //响闹后关闭闹钟 */
		if(rtc_alarm_on_callback)
		{
			rtc_alarm_on_callback();
		}
    }
    if(INTERRUPT_TYPE_PCNT_OVF==flag)
    {
        puts("--TYPE_PCNT_OVF--\n");
    }
    if(INTERRUPT_TYPE_LDO5V_DET==flag)
    {
        puts("--LDO5V DET--\n");
    }
}

static void rtc_alarm_reset(void)
{
	RTC_TIME time;
	RTC_TIME alarm;
	rtc_module_read_rtc_datetime(&time);
    rtc_module_read_alarm_datetime(&alarm);
    if (((time.dYear > 2100)
            || (time.dYear < 2000))
            || (time.bMonth >= 12)
            || (time.bHour >= 24)
            || (time.bMin >= 60)
            || (time.bSec >= 60)
            ||(rtc_module_get_power_up_flag()))
    {
        puts("\n--------RTC RESET--------\n");
        time.dYear = 2016;
        time.bMonth = 9;
        time.bDay = 26;
        time.bHour = 0;
        time.bMin = 0;
        time.bSec = 0;

        //puts("\n--------ALM RESET--------\n");
        memcpy(&alarm, &time,sizeof(RTC_TIME));

        ///update date
        rtc_module_write_rtc_datetime(&time);
        rtc_module_write_alarm_datetime(&alarm , 0);
    }

    rtc_module_reset_irtc();
}

void rtc_alarm_set_alarm(RTC_TIME *alarm)
{
	rtc_alarm_on_flag = 0;	
	rtc_module_write_alarm_datetime(alarm, 0);
}


tbool rtc_alarm_init(void (*p)(void))
{
	tbool is_alarm_comming;	
    if(rtc_module_get_alm_come_flag())
    {
        //update mode ,maybe powerup alarming
		printf("is alarm power up !!!!!\n");
		is_alarm_comming = true;
    }
    else
    {
		is_alarm_comming = false;
	}
	
	rtc_alarm_on_callback = p;
	rtc_module_on(WAKE_UP_ENABLE, 0 , rtc_alarm_isr);
	rtc_alarm_reset();
	set_keep_osci_flag(1);

	printf("rtc alarm init ok \n");
	return is_alarm_comming;
}

