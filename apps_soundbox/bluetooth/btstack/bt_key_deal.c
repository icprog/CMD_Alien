#include "bt_key_deal.h"
#include "rtos/os_api.h"
#include "common/msg.h"
#include "common/app_cfg.h"
#include "bluetooth/avctp_user.h"
#include "rtos/task_manage.h"
#include "play_sel/play_sel.h"
#include "dec/decoder_phy.h"
#include "ui/led/led.h"
#include "dac/dac_api.h"
#include "dac/dac.h"
#include "ui/ui_api.h"
#include "bt_key_msg.h"
#include "key_drv/key.h"
#include "bt_ui.h"
#include "dac/eq_api.h"
#include "sys_detect.h"
#include "dac/eq.h"
#include "encode/encode.h"
#include "echo/echo_api.h"
#include "echo/echo_deal.h"
#include "vm/vm.h"
#include "vm/vm_api.h"
#include "rcsp/rcsp_interface.h"
#include "bluetooth/nfc_api.h"
#include "key_drv/key_voice.h"
#include "ai_toy.h"
#include "ui/led/led_eye.h"

#if (SMART_SOUNDBOX_EN)
#include "bt_smart.h"
#include "bt_smart_alarm.h"
#endif//SMART_SOUNDBOX_EN
#include "bluetooth/ble_api.h"

#define BT_REC_Hz  120000000L

RECORD_OP_API * rec_bt_api = NULL;
REVERB_API_STRUCT * bt_reverb = NULL;
BT_DIS_VAR bt_ui_var;
u8 fast_test = 0;

struct user_ctrl{
    u8 is_phone_number_come;
    u8 play_phone_number_flag;
    u8 phone_prompt_tone_playing_flag;
    u8 phone_num_len;
    u8 income_phone_num[30];
    u8 user_cmd_param_buf[30];
    u8 bt_eq_mode;
    void* tone_by_file_name;
    u8 auto_connection_counter;
    u16 auto_shutdown_cnt;
	u8 esco_close_echo_flag;
	u8 wait_app_updata_flag;
	u8 discover_control_cnt;
	u8 auto_connection_stereo;
};

#ifdef HAVE_MALLOC
_PLAY_SEL_PARA *psel_p = NULL;
#else
_PLAY_SEL_PARA __psel_p;
#define psel_p (&__psel_p)
#endif

static struct user_ctrl __user_val sec_used(.ram1_data);
#define user_val (&__user_val)

extern tbool mutex_resource_release(char *resource);
extern tbool mutex_resource_apply(char *resource,int prio ,void (*apply_response)(), void (*release_request)());
extern void set_poweroff_wakeup_io();
extern void set_poweroff_wakeup_io_handle_register(void (*handle)(),void (*sleep_io_handle)(),void (*sleep_handle)());
extern u8 a2dp_get_status(void);
extern bool get_sco_connect_status();
extern void set_esco_sr(u16 sr);

extern char rec_file_name_last[];
extern char rec_let_last;
extern u8 ignore_pause_eye_display;
extern u8 g_alarm_back;

extern void debug_loop_err_clear(void);

OS_MUTEX tone_manage_semp ; //= OS_MUTEX_INIT();

void bt_update_close_echo()
{
    echo_exit_api((void**)&bt_reverb);
	exit_rec_api(&rec_bt_api);//exit rec when esco change
		
}
u8 get_esco_st(u8 sw)//esco call back fun, get esco status, 0 off, 1 on
{
#if BT_KTV_EN
	int msg_echo_start = MSG_ECHO_START;
	int msg_echo_stop = MSG_ECHO_STOP;

	exit_rec_api(&rec_bt_api);//exit rec when esco change
	ladc_pga_gain(2, 0);//2:mic_channel, 0:gain

	if(sw)
	{
		/* puts("***iphone ktv start\n"); */
	}
	else 
	{
		puts("***iphone ktv stop\n");
		echo_msg_deal_api((void**)&bt_reverb, &msg_echo_stop);
		os_taskq_post_event("btmsg", 1, MSG_ECHO_START);
		/* echo_msg_deal_api((void**)&bt_reverb, &msg_echo_start); */
	}
	return 1;

#else
	exit_rec_api(&rec_bt_api);//exit rec when esco change

	if(sw)//esco on
	{
		echo_exit_api((void **)&bt_reverb);//echo not support when esco on
	}
	return 0;
#endif//end of BT_KTV_EN
}

u8 get_bt_eq_mode(void)
{
    return user_val->bt_eq_mode;
}

void set_bt_eq_mode(int new_mode)
{
#if EQ_EN
	if(new_mode > 5)
	{
		new_mode = 0;
	}
	user_val->bt_eq_mode = new_mode;
	eq_mode_sw(new_mode);
#endif
}

void create_bt_msg_task(char *task_name)
{
    u32 err;
    err = os_task_create(TaskBtMsgStack,(void *)0,TaskBTMSGPrio,30,task_name);
    if(OS_NO_ERR != err)
    {
        puts("create bt msg fail\n");
        while(1);
    }
}

#if PLAY_SANZIJIN_EN
u8 play_sel_stop_flag = 0;
#endif
/*��ʾ����ʼ����Դ�����߳̽ӿڣ��û����ܵ���*/
static void tone_mutex_play()
{
#if(BT_MODE!=NORMAL_MODE)
	 return;
#endif

	exit_rec_api(&rec_bt_api);//exit rec when src change

    puts("tone_mutex_play\n");
    play_sel("resourse",psel_p->let,psel_p);
    if(psel_p->max_file_number > 3) //���ſ�ʼ
    {
        user_val->play_phone_number_flag = 1;
    }
#if PLAY_SANZIJIN_EN
	if(AI_TOY_NOTICE_P_CONNECT == user_val->tone_by_file_name){
		play_sel_stop_flag = 1;		//δ������ʾ����ʼ����
	}
	else{
		play_sel_stop_flag = 0;
	}
#endif
    free_fun((void **)&psel_p);
    os_sem_post(&tone_manage_semp);
}

/*��Դ�����߳̽ӿڣ��û����ܵ���*/
static void tone_mutex_stop()
{
#if(BT_MODE!=NORMAL_MODE)
	 return;
#endif
    play_sel_stop();
    /* dac_channel_on(BT_CHANNEL, FADE_ON); */
	puts("tone_mutex_stop\n");
    if( user_val->play_phone_number_flag == 1)
    {   //���籨����ʾ�������Ҫ��Ҫѭ������ʾ��
        user_val->play_phone_number_flag = 0;
        update_bt_current_status(NULL, BT_STATUS_PHONE_INCOME, 0);
    }
#if PLAY_SANZIJIN_EN
	if(play_sel_stop_flag){
		play_sel_stop_flag = 2;		//δ������ʾ���������
		os_taskq_post("btmsg", 1, MSG_BT_DISCONNET_VOICE);	//���Ͳ������־�����Ϣ
	}
	else{
		play_sel_stop_flag = 0;
	}
#endif
}
u8 wait_phone_number_play = 0;
/*��ȡ������绰���룬���ڱ��Ź���*/
void hook_hfp_incoming_phone_number(char* number, u16 length)
{

    if(user_val->is_phone_number_come == 0)
    {
        puts("phone_number\n");
        put_buf((u8 *)number,length);
        user_val->is_phone_number_come = 1;
	wait_phone_number_play = 0;
        user_val->phone_num_len = length;
        memcpy(user_val->income_phone_num,number,length);
    #if BT_PHONE_NUMBER 
		if(!is_bt_in_background_mode())
		{
		wait_phone_number_play = 1;
        	update_bt_current_status(NULL , BT_STATUS_PHONE_NUMBER,0);
		}
    #endif
    }
    //put_buf(user_val->income_phone_num,user_val->phone_num_len);
}


/*��װ��������״̬ѡ����ʾ��*/
void user_ctrl_prompt_tone_play(u8 status,void *ptr)
{
    u8* buf_addr = NULL;
    u32 need_buf;
#if(BT_MODE!=NORMAL_MODE)
	 return;
#endif
    if((bt_power_is_poweroff_post() && (status != BT_STATUS_FIRST_DISCONNECT))
			||is_bt_in_background_mode())  //background not play tone
    {
        return;
    }
    puts("play_tone\n");
    if(get_remote_test_flag()&&(status == BT_STATUS_FIRST_CONNECTED))   //���Ժв��Բ�������ʾ��
    {
        return;
    }

	if((going_to_pwr_off == 1) && (status != BT_STATUS_POWER_OFF))   //��ػ���ʱ��ֻ���ػ���ʾ��
	{
        puts("-----------powering off---------\n");
		return;
	}

    os_sem_pend(&tone_manage_semp,0);
    if(play_sel_busy())
    {
        play_sel_stop();
        puts("------------play_sel_stop----------\n");
    }

    need_buf = sizeof(_PLAY_SEL_PARA) + 30*4;//30 file_name*sizeof(int)

    buf_addr = malloc_fun(buf_addr, need_buf, 0);

    if(buf_addr == NULL)
    {
        return ;
    }

	memset(buf_addr, 0x00, need_buf);

    psel_p = (_PLAY_SEL_PARA *)buf_addr;//malloc_fun(psel_p ,sizeof(_PLAY_SEL_PARA),0);
    buf_addr+= sizeof(_PLAY_SEL_PARA);
    psel_p->file_name = (void *)(buf_addr);//malloc_fun(psel_p->file_name,30*4,0); //�绰����������Ҫ11�����ʺ�����ܺܶ�λ
    switch(status)
    {
        case BT_STATUS_POWER_ON:
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 1;
            //psel_p->file_name[0] = BPF_BT_MP3;

#if BT_HID_INDEPENDENT_MODE || USER_SUPPORT_PROFILE_HID
            if(__get_hid_independent_flag())
            {
                //HID������ʾ��
                psel_p->file_name[0] = BPF_INIT_HID_MP3;
            }else
#endif
            {
                psel_p->file_name[0] = BPF_BT_MP3;
            }

            break;

        case BT_STATUS_POWER_OFF:
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 1;
            psel_p->file_name[0] = BPF_POWER_OFF_MP3;
            break;

        case BT_STATUS_RESUME:
            /* psel_p->max_file_number = 2; */
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 1;
            psel_p->file_name[0] = BPF_BT_MP3;

            /* if(get_curr_channel_state() != 0) */
            /* { */
                /* psel_p->file_name[1] = BPF_CONNECT_MP3; */
            /* } */
            /* else */
            /* { */
                /* psel_p->file_name[1] = BPF_DISCONNECT_MP3; */
            /* } */
            break;
        case BT_STATUS_FIRST_CONNECTED:
        case BT_STATUS_SECOND_CONNECTED:
			bt_power_max(0xFF);
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 1;
#if BT_HID_INDEPENDENT_MODE || USER_SUPPORT_PROFILE_HID
            if(__get_hid_independent_flag())
            {   //HID������ʾ��
                psel_p->file_name[0] = BPF_CONN_HID_MP3;
            }else
#endif
            {
#if BT_TWS 
               if(BT_CURRENT_CONN_PHONE==get_bt_current_conn_type())
               {
				   puts("------BT_CURRENT_CONN_PHONE\n");
                   psel_p->file_name[0] = BPF_CONNECT_ONLY_MP3;
               }
               else if(BT_CURRENT_CONN_STEREO_MASTER_PHONE==get_bt_current_conn_type())
               {
				   puts("--------BT_CURRENT_CONN_STEREO_MASTER_PHONE\n");
                   psel_p->file_name[0] = BPF_CONNECT_LEFT_MP3;
               }
               else if((BT_CURRENT_CONN_STEREO_MASTER==get_bt_current_conn_type())||(BT_CURRENT_CONN_STEREO_PHONE_MASTER==get_bt_current_conn_type()))
               {
				   puts("--------BT_CURRENT_CONN_STEREO_MASTER\n");
                   psel_p->file_name[0] = BPF_CONNECT_LEFT_MP3;
               }
               else if(BT_CURRENT_CONN_STEREO_SALVE==get_bt_current_conn_type())
               {
				   puts("--------BT_CURRENT_CONN_STEREO_SALVE\n");
                   psel_p->file_name[0] = BPF_CONNECT_RIGHT_MP3;
		
               }
               else 
               {
				   puts("--------BPF_CONNECT_ONLY_MP3\n");
                   psel_p->file_name[0] = BPF_CONNECT_ONLY_MP3;
               }
#else
                psel_p->file_name[0] = BPF_CONNECT_MP3;
#endif
            }
            break;
        case BT_STATUS_FIRST_DISCONNECT:
        case BT_STATUS_SECOND_DISCONNECT:
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 1;
#if BT_HID_INDEPENDENT_MODE || USER_SUPPORT_PROFILE_HID
            if(__get_hid_independent_flag())
            {   //HID������ʾ��
                psel_p->file_name[0] = BPF_DISCON_HID_MP3;
            }else
#endif
            {
#if BT_TWS 
               if(BT_CURRENT_CONN_PHONE==get_bt_current_conn_type())
			   {
				   puts("----disconn BT_CURRENT_CONN_STEREO_MASTER\n");
                  psel_p->file_name[0] =BPF_DISCONNECT_MP3;
			   }
			   else
			   {
				   puts("-----disconn BPF_DISCONNECT_MP3\n");
                  psel_p->file_name[0] = BPF_DISCONNECT_MP3;
			   }
#else
                psel_p->file_name[0] = BPF_DISCONNECT_MP3;
#endif
            }

            break;
        case BT_STATUS_TONE_BY_FILE_NAME:
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 1;
            psel_p->file_name[0] = user_val->tone_by_file_name;
			if(ptr)
			{
				psel_p->let = 'B';//(u8)*(u8*)ptr;
			}

			printf("FILE_NAME = %s\n",psel_p->file_name[0]);
			printf("psel_p->let = %c\n",psel_p->let);

            break;

#if PLAY_SANZIJIN_EN
        case BT_STATUS_START_DISCONNECT:
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 0;
            psel_p->file_name[0] = BPF_START_MP3;
            break;	
#endif			
        case BT_STATUS_PHONE_INCOME:
            psel_p->max_file_number = 1;
            psel_p->delay = 10;
            psel_p->rpt_time = 0;
            psel_p->file_name[0] = BPF_RING_MP3;
            user_val->play_phone_number_flag = 0;
            user_val->phone_prompt_tone_playing_flag = 1;
            break;
#if BT_PHONE_NUMBER
        case BT_STATUS_PHONE_NUMBER:
            if(user_val->is_phone_number_come)   //�е绰����Ͳ���
            {
                int i;
                psel_p->max_file_number=0;

                psel_p->delay = 20;
                psel_p->rpt_time = 1;


                for(i = 0;i < user_val->phone_num_len; i++)
                {
                    if((user_val->income_phone_num[i] >= 0x30) && (user_val->income_phone_num[i] <= 0x39))
                    {
                        psel_p->file_name[psel_p->max_file_number] = tone_number_get_name(user_val->income_phone_num[i] - 0x30);
                        psel_p->max_file_number++;
                    }

                }
                user_val->phone_prompt_tone_playing_flag = 1;
                break;//breakҪ����if���棬������ֱ��free��
            }
#endif

		default:
			os_sem_post(&tone_manage_semp);
			//            free_fun((void **)&psel_p->file_name);
            free_fun((void **)&psel_p);
            return;
    }

    mutex_resource_apply("tone",5,tone_mutex_play,tone_mutex_stop);
    os_sem_pend(&tone_manage_semp,0);
    os_sem_post(&tone_manage_semp);
}

/*��װһ�㷽��ֱ��ʹ���ļ��Ų���*/
void bt_prompt_play_by_name(void * file_name, void * let)
{
#ifdef BT_PROMPT_EN         //������avctp_user.h
    puts("bt_prompt_play\n");
    user_val->tone_by_file_name = file_name;
#if PLAY_SANZIJIN_EN	
	if(AI_TOY_NOTICE_NOCONNECT == file_name){
		user_ctrl_prompt_tone_play(BT_STATUS_START_DISCONNECT, let);//��ʼ�������־�
	}
	else{
		user_ctrl_prompt_tone_play(BT_STATUS_TONE_BY_FILE_NAME, let);
	}
#else
	user_ctrl_prompt_tone_play(BT_STATUS_TONE_BY_FILE_NAME, let);
#endif
#endif

}
u8 get_stereo_salve(u8 get_type)
{
	u8 stereo_info;
	if(0 == get_type)//����ֱ�ӽ������״̬
	{
	   return BD_CONN_MODE;
	}
	else//��vm�л�ȡ�����ϴ����������������ӻ�
	{
        if (-VM_READ_NO_INDEX == vm_read_api(VM_BT_STEREO_INFO,&stereo_info))
        {
     	   return BD_CONN_MODE;
        }
       	printf("-----is_stereo_salve=0x%x\n",stereo_info );
        if(0xaa ==stereo_info)//�ӻ�
     	{
     		return BD_PAGE_MODE;
     	}
	}
	return BD_CONN_MODE;
}
void bt_stereo_poweroff_togerther()
{
#if BT_TWS 
#if BT_TWS_POWEROFF_TOGETHER 
   	if(is_check_stereo_slave())
   	{
       	stereo_slave_cmd_handle(MSG_POWER_OFF,0xff);
   	}
   	else
   	{
       	stereo_host_cmd_handle(MSG_POWER_OFF,0);
   	}
	os_time_dly(5);
#endif
#endif

}
void bt_tws_delete_addr()
{
#if BT_TWS 
    delete_stereo_addr();
   	if(is_check_stereo_slave())
   	{
       	stereo_slave_cmd_handle(MSG_BT_TWS_DELETE_ADDR,0);
   	}
   	else
   	{
       	stereo_host_cmd_handle(MSG_BT_TWS_DELETE_ADDR,0);
   	}
#endif

}

static OS_MUTEX spp_mutex;
int spp_mutex_init(void)
{
    return os_mutex_create(&spp_mutex);
}
static inline int spp_mutex_enter(void)
{
    os_mutex_pend(&spp_mutex,0);
    return 0;
}
static inline int spp_mutex_exit(void)
{
    return os_mutex_post(&spp_mutex);
}
int spp_mutex_del(void)
{
    return os_mutex_del(&spp_mutex,OS_DEL_ALWAYS);
}

/*���ʹ������ݵĽӿڣ��������ٷ���*/
u32 bt_spp_send_data(u8 *data,u16 len)
{
    u32 err;
    spp_mutex_enter();
    err = user_send_cmd_prepare(USER_CTRL_SPP_SEND_DATA,len,data);
    spp_mutex_exit();
    return err;
}

void bt_spp_disconnect(void)
{
    spp_mutex_enter();
    user_send_cmd_prepare(USER_CTRL_SPP_DISCONNECT,0,0);
    spp_mutex_exit();
}

/*���ؿɷ��ֿ����ӵĺ����ӿ�*/
static void bt_page_scan(u8 enble)
{
  if(enble)
  {
	  /* printf("====================================================================\n"); */
	  /* printf("====================================================================\n"); */
	  /* printf("====================================================================\n"); */
	  /* printf("====================================================================\n"); */
    if(!is_1t2_connection())
    {
        if(enble==3)
        {
			user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE,0,NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE_KEY,0,NULL);
        }
        else if(enble==2)
        {
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE_KEY,0,NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE_KEY,0,NULL);
        }
        else
        {
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE,0,NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE,0,NULL);
        }

    }
  }
  else
  {
	  /* printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); */
	  /* printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); */
	  /* printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); */
	  /* printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); */
     user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE,0,NULL);
     user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE,0,NULL);
  }
}

/*Э��ջ״̬�仯�û�����ӿ�*/
static void btstack_status_change_deal(void *ptr, u8 status)
{
	int msg_echo_start = MSG_ECHO_START;
        update_bt_prev_status(get_bt_newest_status());
		printf("status %d %2x\n", status, get_resume_flag()); 
		while(get_resume_flag())   //bt stack is require resume,wait
			os_time_dly(1);
        switch(status)
        {
            case BT_STATUS_RESUME:
#if (BT_SMART_ALARM_EN)
				if(g_alarm_back)
				{
					g_alarm_back = 0;	
					break;
				}
				else
#endif
				{
					if(bt_smart_get_ai_status() != 0)
					{
						break;		
					}

					user_ctrl_prompt_tone_play(BT_STATUS_RESUME,NULL);
					led_fre_set(5,0);
				}
               break;
            case BT_STATUS_RESUME_BTSTACK:
               //Э��ջҪ���
    			#if BT_PHONE_NUMBER 
					if(user_val->is_phone_number_come && wait_phone_number_play==0)
					{
        				update_bt_current_status(NULL , BT_STATUS_PHONE_NUMBER,0);
					}
    			#endif
			   puts("stack back to bluetooth\n");
               break;
            case BT_STATUS_POWER_ON:
			   printf("---- bt_power_on______\n");
               user_ctrl_prompt_tone_play(BT_STATUS_POWER_ON,NULL);
               led_fre_set(5,0);
               break;
			case BT_STATUS_INIT_OK:
			   eye_led_api(EFFECT_CONNECT, 0, 0);
#if NFC_EN
			   nfc_init(NFC_BREDR_OOB);
#endif

#if BT_TWS 
               bd_work_mode = get_stereo_salve(TWS_SLAVE_WAIT_CON);
#endif
                if(bd_work_mode == BD_PAGE_MODE)/*��ʼ����ɽ������ģʽ*/
                {
                    led_fre_set(7,1);
#if BT_TWS 
                    stereo_clear_current_search_index();
#endif
#if BT_TWS_SCAN_ENBLE 
                    bt_page_scan(3);
#else
                    bt_page_scan(1);
#endif
                }
                else/*��ʼ����ɽ������ģʽ*/
                {
					/* bt_smart_led_flick(100, 0); */
					/* EyeEffectCtl(EFFECT_CONNECT); */
					printf("BT_STATUS_INIT_OK\n");
					/* while(play_sel_busy()) {
						os_time_dly(1);
					}
					dac_off_control(); */
					bt_power_max(4);
        			user_send_cmd_prepare(USER_CTRL_START_CONNECTION,0,NULL);
					led_fre_set(7,1);
#if BT_KTV_EN
					/* set_esco_sr(8000); */
					set_esco_sr(16000L);
					echo_msg_deal_api((void**)&bt_reverb, &msg_echo_start);
#endif
                }
                break;
            case BT_STATUS_FIRST_CONNECTED:
				bt_power_max(0xFF);
                //if(is_auto_connect()){}  //�����жϡ��ǻ����ɹ�����1���ǻ������ӷ���0��
				printf("2.1  connect ok!!!!\n");
				if(get_smart_pass())
				{
					ignore_pause_eye_display = 1;
					eye_led_api(EFFECT_CONNECT_OK, 0, 0);
					/* eye_ef_timeout = 5; */
					user_ctrl_prompt_tone_play(BT_STATUS_FIRST_CONNECTED,NULL);
				}
                led_fre_set(100,0);
                break;
             case BT_STATUS_SECOND_CONNECTED:
				bt_power_max(0xFF);
                //if(is_auto_connect()){}  //�����жϡ��ǻ����ɹ�����1���ǻ������ӷ���0��
                user_ctrl_prompt_tone_play(BT_STATUS_SECOND_CONNECTED,NULL);
                led_fre_set(100,0);
                break;
            case BT_STATUS_FIRST_DISCONNECT:
				printf("first_disconn:%d\n",get_total_connect_dev());
                /* if(get_total_connect_dev()==0) */
				{
				 	led_fre_set(7,1);

					if(get_smart_pass())
					{
						printf("BT_STATUS_FIRST_DISCONNECT ~~~~~\n");
						os_taskq_post(keymsg_task_name, 1, MSG_SMART_DISCONNECT);
						/* bt_smart_led_flick(100, 0); */
						/* EyeEffectCtl(EFFECT_NO_CONNECT); */
						/* eye_led_api(EFFECT_NO_CONNECT, 0, 0); */
						/* user_ctrl_prompt_tone_play(BT_STATUS_FIRST_DISCONNECT,NULL); */
					}
				}
                break;
            case BT_STATUS_SECOND_DISCONNECT:
                //printf("second_disconn:%d\n",get_total_connect_dev());
                user_ctrl_prompt_tone_play(BT_STATUS_SECOND_DISCONNECT,NULL);
                if(get_total_connect_dev()==0)
				{
                    led_fre_set(7,1);
				}
                break;
            case BT_STATUS_PHONE_INCOME:
				printf("BT_STATUS_PHONE_INCOME\n");

#if ((SMART_SOUNDBOX_EN == 1) && (BT_SMART_SPEECH_SOURCE == SPEEX_INPUT))
				bt_smart_speex_data_send_cancel();
#endif//((SMART_SOUNDBOX_EN == 1) && (BT_SMART_SPEECH_SOURCE == SPEEX_INPUT))

                digit_auto_mute_set(0,-1,-1,-1); // ���Զ�mute
				dac_mute(0,1);
                user_send_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT,0,NULL);//�������ȡ�绰����
                os_time_dly(5);
				user_ctrl_prompt_tone_play(BT_STATUS_PHONE_INCOME,NULL);
				break;
			case BT_STATUS_PHONE_OUT:
				puts("phone_out\n");

				//dac_mute(0,1);
               break;

#if BT_PHONE_NUMBER
            case BT_STATUS_PHONE_NUMBER:
               user_ctrl_prompt_tone_play(BT_STATUS_PHONE_NUMBER,NULL);
               break;
#endif
            case BT_STATUS_PHONE_ACTIVE:
                user_val->play_phone_number_flag = 0;
                if(user_val->phone_prompt_tone_playing_flag)
                {
                    play_sel_stop();
                    user_val->phone_prompt_tone_playing_flag = 0;
                }
                break;
            case BT_STATUS_PHONE_HANGUP:
               digit_auto_mute_set(1,-1,-1,-1); // ���Զ�mute
               user_val->play_phone_number_flag = 0;
               user_val->is_phone_number_come = 0;
               if(user_val->phone_prompt_tone_playing_flag)
               {
                  play_sel_stop();
                   user_val->phone_prompt_tone_playing_flag = 0;
               }
                //��̨�绰�������绰�����Զ��˳�
                user_send_cmd_prepare(USER_CTRL_BTSTACK_SUSPEND,0,NULL);    //go back to background
               break;

            default:
               break;
        }
}

void stereo_led_deal()
{
#if BT_TWS 
   if(BT_CURRENT_CONN_PHONE==get_bt_current_conn_type())
   {

   }
   else if(BT_CURRENT_CONN_STEREO_MASTER==get_bt_current_conn_type())
   {

   }
   else if(BT_CURRENT_CONN_STEREO_SALVE==get_bt_current_conn_type())
   {

   }
   else if(BT_CURRENT_CONN_STEREO_MASTER_PHONE==get_bt_current_conn_type())
   {

   }
   else if(BT_CURRENT_CONN_STEREO_PHONE_MASTER==get_bt_current_conn_type())
   {

   }
   else 
   {
   }
#endif
	
}  

extern void set_osc_2_vm(void);
static void btstack_key_handler(void *ptr,int *msg)
{
	u8 test_box_vol_up=0x41;
	u8 test_box_vol_down=0x42;
	static u8 play_st_ctl = 0;
#if BT_TWS 
    if(is_check_stereo_slave())
    {
        switch(msg[0])
        {
        case MSG_VOL_DOWN:
        case MSG_VOL_UP:
        case MSG_BT_PP:
        case MSG_BT_NEXT_FILE:
        case MSG_BT_PREV_FILE:
        case MSG_BT_ANSWER_CALL:
        case MSG_BT_CALL_LAST_NO:
        case MSG_BT_CALL_REJECT:
        case MSG_BT_CALL_CONTROL:
            stereo_slave_cmd_handle(msg[0],0);
            return;
        default:
            break;
        }
    }
#endif

#if (SMART_SOUNDBOX_EN)
		switch(msg[0])
		{
			case MSG_BT_PP:
			case MSG_BT_NEXT_FILE:
			case MSG_BT_PREV_FILE:
			case MSG_BT_ANSWER_CALL:
			case MSG_BT_CALL_LAST_NO:
			case MSG_BT_CALL_REJECT:
			case MSG_BT_CALL_CONTROL:
				bt_smart_speex_data_send_cancel();
				break;
		}
#endif//SMART_SOUNDBOX_EN

	switch(msg[0]){
#if BT_TWS 
#if EQ_EN
 	 	case  MSG_BT_SYNC_STEREO_EQ:
			if(user_val->bt_eq_mode == 0)
			{
				return;
			}
			if(is_check_stereo_slave())
			{
            	stereo_slave_cmd_handle(MSG_BT_MUSIC_EQ,user_val->bt_eq_mode);
			}
			else
			{
           		stereo_host_cmd_handle(MSG_BT_MUSIC_EQ,user_val->bt_eq_mode);
			}
            os_taskq_post("btmsg", 2, MSG_BT_MUSIC_EQ,user_val->bt_eq_mode);
			break;
 	 	case  MSG_BT_STEREO_EQ:
    		if(is_check_stereo_slave())
			{
            	stereo_slave_cmd_handle(MSG_BT_MUSIC_EQ,0);
			}
			else
			{
            	stereo_host_cmd_handle(MSG_BT_MUSIC_EQ,0);
			}


            os_taskq_post("btmsg", 1, MSG_BT_MUSIC_EQ);
			break;
#endif

        case MSG_VOL_STEREO://������������������ʱͬ���ӻ�
			/* puts("bt MSG_VOL_STEREO\n"); */
            stereo_host_cmd_handle(msg[0],msg[1]);
	        break;
#endif
        case MSG_BT_PP:
            puts("MSG_BT_PP\n");
            if(msg[1] != 0)
           {
                play_st_ctl = (u8)msg[1];
                break;
            }

			printf("call_status:%d\n",get_call_status());
            if ((get_call_status() == BT_CALL_OUTGOING) ||
					(get_call_status() == BT_CALL_ALERT) ||
					(get_call_status() == BT_CALL_ACTIVE)
				){
				 user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP,0,NULL);
			}else if(get_call_status() == BT_CALL_INCOMING){
				 user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER,0,NULL);
			/* }else if(get_call_status() == BT_CALL_ACTIVE){ */
				/* user_send_cmd_prepare(USER_CTRL_SCO_LINK,0,NULL); */
			}else{
#if 1
				if(get_total_connect_dev()!=0)
				{
					u8 cancel_flag = 0;
					u8 delay_cnt = 2;
					while((play_sel_busy() != 0) || (delay_cnt != 0))
					{
						os_taskq_pend_no_clean(0, ARRAY_SIZE(msg), msg);
						if(msg[0] == MSG_BT_PP) 
						{
							os_taskq_msg_clean(msg[0], 1);
						}
						else if(msg[0] == MSG_HALF_SECOND)
						{
							if(delay_cnt)
							{
								delay_cnt --;
							}
							os_taskq_msg_clean(msg[0], 1);
						}
						else
						{
							cancel_flag = 1;
							break;	
						}
					}
					if(cancel_flag)
					{
						printf("pp notice cancel !!!!~~~~~~~~~~\n");
						break;	
					}
					if((BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()) && (BT_MUSIC_STATUS_STARTING == a2dp_get_status()) && (get_dac_energy_value() > 10))
					{
						printf("pause ============================!!\n");
						user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY,0,NULL);
						bt_prompt_play_by_name(AI_TOY_NOTICE_PAUSE,NULL);
						eye_led_api(EFFECT_PAUSE, 0, 0);
					}
					else
					{
						printf("play ++++++++++++++++++++++++++++++ %d!!\n", get_dac_energy_value());
						user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY,0,NULL);
						bt_prompt_play_by_name(AI_TOY_NOTICE_PLAY,NULL);
						eye_led_api(EFFECT_PLAY, 0, 0);
					}
				}
#endif

				/* user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY,0,NULL); */
			}
            break;

        case MSG_BT_NEXT_FILE:
            puts("MSG_BT_NEXT_FILE\n");
            if(get_call_status() == BT_CALL_ACTIVE)
            {
                user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP,0,NULL);
			}else{

				if(get_total_connect_dev()!=0)
				{
					ignore_pause_eye_display = 1;
					eye_led_api(EFFECT_NEXT, 15, 0);
					bt_prompt_play_by_name(AI_TOY_NOTICE_NEXT,NULL);

				}
				user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT,0,NULL);
            }

            break;

		case MSG_BT_PREV_FILE:
			puts("MSG_BT_PREV_FILE\n");
			if(get_call_status() == BT_CALL_ACTIVE)
			{
				user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN,0,NULL);
			}else{

				if(get_total_connect_dev()!=0)
				{
					ignore_pause_eye_display = 1;
					eye_led_api(EFFECT_PREV, 15, 0);
					bt_prompt_play_by_name(AI_TOY_NOTICE_PREV,NULL);
				}
				user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV,0,NULL);
			}
			break;
		case SYS_EVENT_DEV_ONLINE:
			bt_prompt_play_by_name(AI_TOY_NOTICE_DEV_ONLINE,NULL);
			break;

		case SYS_EVENT_DEV_OFFLINE:
			bt_prompt_play_by_name(AI_TOY_NOTICE_DEV_OFFLINE,NULL);
			break;

		case MSG_AI_TOY_VOL_UP:
			printf("v +\n");
			if(dac_ctl.sys_vol_l < MAX_SYS_VOL_L)
				dac_ctl.sys_vol_l++;
			user_send_cmd_prepare(USER_CTRL_TEST_KEY,1,&test_box_vol_up);//test_box test
			user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL,0,NULL);
			dac_ctl.sys_vol_r = dac_ctl.sys_vol_l;
			if(is_dac_mute()) {
				dac_mute(0,1);
				//led_fre_set(15,0);
			}
			set_sys_vol(dac_ctl.sys_vol_l, dac_ctl.sys_vol_r, FADE_OFF);
#if (SYS_DEFAULT_VOL==0)
			vm_cache_write(VM_SYS_VOL,&dac_ctl.sys_vol_l,2);
#endif
			if(dac_ctl.sys_vol_l == MAX_SYS_VOL_L)
			{
				send_key_voice(500);
				/* bt_prompt_play_by_name(AI_TOY_NOTICE_VOL,NULL); */
			}
			break;
		case MSG_AI_TOY_VOL_DOWN:
			printf("v -\n");
			if(dac_ctl.sys_vol_l)
				dac_ctl.sys_vol_l--;
            user_send_cmd_prepare(USER_CTRL_TEST_KEY,1,&test_box_vol_down);//test_box test
			user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL,0,NULL);
			dac_ctl.sys_vol_r = dac_ctl.sys_vol_l;
			if(is_dac_mute()) {
				dac_mute(0,1);
			}
			set_sys_vol(dac_ctl.sys_vol_l, dac_ctl.sys_vol_r, FADE_OFF);
#if (SYS_DEFAULT_VOL==0)
			vm_cache_write(VM_SYS_VOL,&dac_ctl.sys_vol_l,2);
#endif
			/* if(dac_ctl.sys_vol_l == 0) */
			/* { */
				/* bt_prompt_play_by_name(AI_TOY_NOTICE_VOL,NULL); */
			/* } */
			break;
#if EQ_EN
         case MSG_BT_MUSIC_EQ:
			set_bt_eq_mode(user_val->bt_eq_mode+1);
            printf("MSG_BT_EQ %d\n", user_val->bt_eq_mode);

            break;
#endif // EQ_EN

        case MSG_BT_PAGE_SCAN:
             puts("MSG_BT_PAGE_SCAN\n");
             bt_page_scan(2);
             break;
#if BT_TWS
        case MSG_BT_STEREO_SEARCH_DEVICE:
             puts("MSG_BT_STEREO_SEARCH_DEVICE\n");
			 if(fast_test)
			 {
                puts("fast_test MSG_BT_STEREO_SEARCH_DEVICE\n");
				break; 	
			 }
            if(BT_TWS_CHANNEL_SELECT&&BT_TWS_CHANNEL_SELECT==TWS_CHANNEL_RIGHT)
			{
                puts(" TWS_CHANNEL_RIGHT not search\n");
				break;	
			}
            user_send_cmd_prepare(USER_CTRL_SEARCH_DEVICE,0,NULL);
#if BT_TWS_SCAN_ENBLE 
            if(!is_check_stereo_slave())
			{
               bt_page_scan(2);
			}
#endif
            break;
		case MSG_BT_TWS_DELETE_ADDR:
		    puts("MSG_BT_TWS_DELETE_ADDR\n");
             delete_stereo_addr();
			break;
#endif
        case MSG_BT_ANSWER_CALL:
            puts("MSG_BT_ANSWER_CALL\n");
            if(get_call_status() == BT_CALL_INCOMING){
                user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER,0,NULL);
            }
            break;

        case MSG_BT_CALL_LAST_NO:
            puts("MSG_BT_CALL_LAST_NO\n");
            if(get_call_status()!=BT_CALL_HANGUP)
                break;

            if(get_last_call_type()== BT_CALL_OUTGOING)
            {
                user_send_cmd_prepare(USER_CTRL_HFP_CALL_LAST_NO,0,NULL);
            }else if(get_last_call_type() == BT_CALL_INCOMING){
                user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER,user_val->phone_num_len,
                                                            user_val->income_phone_num);
            }
            break;

        case MSG_BT_CALL_REJECT:
            puts("MSG_BT_CALL_REJECT\n");
            if(get_call_status() != BT_CALL_HANGUP)
            {
                user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP,0,NULL);
            }
            break;

        case MSG_BT_CALL_CONTROL:   //�������ٵ�ʱ�����
			printf("MSG_BT_CALL_CONTROL ~~~~~~~~~~~~~~~~~~~~~\n");
            if(get_call_status() != BT_CALL_HANGUP)
            {
                user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP,0,NULL);
            }
            else
            {
                if(get_last_call_type() == BT_CALL_OUTGOING)
                {
                    user_send_cmd_prepare(USER_CTRL_HFP_CALL_LAST_NO,0,NULL);
                }else if(get_last_call_type() == BT_CALL_INCOMING){
                    user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER,user_val->phone_num_len,
                                                                user_val->income_phone_num);
                }
            }
            break;

        case MSG_BT_CONNECT_CTL:
            puts("MSG_BT_CONNECT_CTL\n");
            if((BT_STATUS_CONNECTING == get_bt_connect_status())   ||
               (BT_STATUS_TAKEING_PHONE == get_bt_connect_status())||
               (BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()))/*����״̬*/
            {
                puts("bt_disconnect\n");/*�ֶ��Ͽ�����*/
                user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI,0,NULL);
            }
            else
            {
                /* puts("bt_connect\n");[>�ֶ�����<] */
                /* user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR_MANUALLY,0,NULL); */
            }
            break;

#if BT_TWS 
		case MSG_BT_CONNECT_STEREO_CTL:
            puts("MSG_BT_CONNECT_STEREO_CTL\n");
            if((BT_STATUS_CONNECTING == get_stereo_bt_connect_status())   ||
               (BT_STATUS_TAKEING_PHONE == get_stereo_bt_connect_status())||
               (BT_STATUS_PLAYING_MUSIC == get_stereo_bt_connect_status()))/*����״̬*/
            {
                puts("bt_disconnect\n");/*�ֶ�����Ͽ�����*/
                user_send_cmd_prepare(USER_CTRL_DISCONNECTIO_STEREO_HCI,0,NULL);
            }
            else
            {
                puts("bt_connect\n");/*�ֶ���������*/
                user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR_STEREO,0,NULL);
            }
			break;
#endif
        case MSG_BT_HID_CTRL:
            puts("MSG_BT_HID_CTRL\n");
            if(get_curr_channel_state() & HID_CH)
            {
                user_send_cmd_prepare(USER_CTRL_HID_DISCONNECT,0,NULL);
            }else{
                user_send_cmd_prepare(USER_CTRL_HID_CONN,0,NULL);
            }

            break;
        case MSG_BT_HID_TAKE_PIC:
            puts("MSG_BT_HID_TAKE_PIC\n");
            if(get_curr_channel_state() & HID_CH)
            {
                 user_send_cmd_prepare(USER_CTRL_HID_BOTH,0,NULL);
            }
            break;

		case MSG_PROMPT_PLAY:
			puts("MSG_PROMPT_PLAY\n");
			bt_prompt_play_by_name(BPF_TEST_MP3, NULL);
			break;

#if BT_REC_EN
		case MSG_REC_PLAY:
			if(play_sel_busy())
			{
				play_sel_stop();
			}
			else
			{
				exit_rec_api(&rec_bt_api);//exit rec when esco change
				bt_prompt_play_by_name(rec_file_name_last, (void *)rec_let_last);
			}
			break;
#endif

        case MSG_POWER_OFF:
            puts("bt_power_off\n");
            if((BT_STATUS_CONNECTING == get_bt_connect_status())   ||
               (BT_STATUS_TAKEING_PHONE == get_bt_connect_status())||
               (BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()))
            {
                puts("bt_disconnect\n");/*����״̬���ȶϿ������ٹػ�*/
                user_send_cmd_prepare(USER_CTRL_POWER_OFF,0,NULL);
            }

            /*�ػ���ʾ��*/
//            user_ctrl_prompt_tone_play(BT_STATUS_POWER_OFF,NULL);
//            do{
//                os_time_dly(1);
//            }while(play_sel_busy());
//            puts("power_off_tone_end\n");
            break;

#if LCD_SUPPORT_MENU
        case MSG_ENTER_MENULIST:
            if(get_bt_connect_status() == BT_STATUS_TAKEING_PHONE)
            {
                UI_menu_arg(MENU_LIST_DISPLAY,UI_MENU_LIST_ITEM|(1<<8));
            }
            else
            {
                UI_menu_arg(MENU_LIST_DISPLAY,UI_MENU_LIST_ITEM);
            }
            break;
#endif

		case MSG_BT_SPP_UPDATA:
			puts("MSG_BT_SPP_UPDATA\n");
            user_send_cmd_prepare(USER_CTRL_SPP_UPDATA_DATA,0,NULL);
            user_val->wait_app_updata_flag = 1;
			break;
        case MSG_HALF_SECOND:
            //puts(" BT_H ");
			debug_loop_err_clear();

#if (BT_SMART_ALARM_EN)
			bt_smart_alarm_debug_cur_alarm();
#endif//BT_SMART_ALARM_EN

			stereo_led_deal();
			if(fast_test)
			{
               user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE,0,NULL);
               user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE,0,NULL);
                	
            }
#if BT_TWS 
  #if (BT_TWS_SCAN_ENBLE==0)
            if(0 == get_total_connect_dev() ){
                if(user_val->discover_control_cnt == 1){
                    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE,0,NULL);
                }else if(user_val->discover_control_cnt==15){
                    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE,0,NULL);
                }
                user_val->discover_control_cnt++;
                if(user_val->discover_control_cnt>=20)
                    user_val->discover_control_cnt =1;
            }
    #endif
#endif

            if((BT_STATUS_CONNECTING == get_bt_connect_status())   ||
               (BT_STATUS_TAKEING_PHONE == get_bt_connect_status())||
               (BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()))/*����״̬*/
            {
                if(BT_MUSIC_STATUS_STARTING == a2dp_get_status())       /*����״̬*/
                {
                    //puts("BT_MUSIC\n");
					//led_fre_set(15,0);                                /*��������*/
					/* bt_smart_led_flick(500, 0); */
					if(get_total_connect_dev()!=0)
					{

					if((get_dac_energy_value() > 10) && (play_sel_busy() == 0))
					{
						eye_led_api(EFFECT_PLAY, 0, 0);
					}
						//if(eye_ef_timeout == 0)
						//{

						//	if(EyeEffect_get_cur() != EFFECT_SPEECH_INPUT)
						//	{
						//		extern u8 GetMusicEnergyLevel(void);
						//		if (GetMusicEnergyLevel() == 1)
						//		{
						//			/* EyeEffectCtl(EFFECTSOUNDCOLUMNSLOW); */
						//			eye_led_api(EFFECTSOUNDCOLUMNSLOW, 0);
						//		}
						//		else if (GetMusicEnergyLevel() == 2)
						//		{
						//			/* EyeEffectCtl(EFFECTSOUNDCOLUMNNORMAL); */
						//			eye_led_api(EFFECTSOUNDCOLUMNNORMAL, 0);
						//		}
						//		else
						//		{
						//			/* EyeEffectCtl(EFFECTSOUNDCOLUMNFAST); */
						//			eye_led_api(EFFECTSOUNDCOLUMNFAST, 0);
						//		}
						//	}
						//}
					}
                    play_st_ctl = 0;
					user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_GET_PLAY_TIME,0,NULL);
                }
                else if(BT_STATUS_TAKEING_PHONE == get_bt_connect_status())
                {
                    ///puts("bt_phone\n");
                }
                else
				{
					///puts("bt_other\n");
					play_st_ctl = 0;
					//puts("BT_CONN\n");
					//led_fre_set(0,0);                                 /*��ͣ����*/
				}
				user_val->auto_shutdown_cnt = AUTO_SHUT_DOWN_TIME;
            }else  if(BT_STATUS_WAITINT_CONN == get_bt_connect_status()&&user_val->auto_shutdown_cnt){
                //puts("BT_STATUS_WAITINT_CONN\n");
                user_val->auto_shutdown_cnt--;
				//printf("power cnt:%d\n",user_val->auto_shutdown_cnt);
                if(user_val->auto_shutdown_cnt==0)
                {
                    //��ػ�
                    puts("*****shut down*****\n");
					going_to_pwr_off = 1;
					os_taskq_post("MainTask", 2, MSG_POWER_OFF_KEY_MSG ,0x44);
                }
            }
       #if (SNIFF_MODE_CONF&SNIFF_EN)
            if(check_in_sniff_mode())
            {
                 printf("check_in_sniff_mode ok\n");
				 vm_check_all(0);
                 user_send_cmd_prepare(USER_CTRL_SNIFF_IN,0,NULL);
                 //user_send_cmd_prepare(USER_CTRL_SNIFF_EXIT,0,NULL);
	        }
       #endif

            if((bt_ui_var.ui_bt_connect_sta != get_bt_connect_status())||
               (bt_ui_var.ui_bt_a2dp_sta != a2dp_get_status()))
            {
                bt_ui_var.ui_bt_connect_sta = get_bt_connect_status();
                bt_ui_var.ui_bt_a2dp_sta = a2dp_get_status();
                UI_menu(MENU_REFRESH);
            }
			if(user_val->wait_app_updata_flag)
			{
				if(check_bt_app_updata_run())
				{
				     user_val->wait_app_updata_flag =0;		
				}
			}

			calling_del_page();

//           UI_menu(MENU_MAIN);
           UI_menu(MENU_HALF_SEC_REFRESH);
            break;

        case MSG_BT_STACK_STATUS_CHANGE:
            //����Ϣ�Ƚ����⣬�����ڰ�����������Э��ջ״̬�仯������
            puts("MSG_BT_STACK_STATUS_CHANGE\n");
            btstack_status_change_deal(NULL, msg[1]);
            break;
        case SYS_EVENT_DEL_TASK:
            puts("SYS_EVENT_DEL_TASK\n");
            //�Ͽ��͹ر�Э��ջ
            user_send_cmd_prepare(USER_CTRL_POWER_OFF,0,NULL);
            //ɾ��Э��ջ
            delete_stack_task();
            puts("del stack ok\n");
            play_sel_stop();
			if(dac_ctl.toggle == 0) {
				dac_on_control();
			}
            //ɾ����Ϣ�߳�
            if (os_task_del_req_name(OS_TASK_SELF) == OS_TASK_DEL_REQ)
            {
                puts("del_ket_masg");
				echo_exit_api((void**)&bt_reverb);
				os_task_del_res_name(OS_TASK_SELF); 	//ȷ�Ͽ�ɾ�����˺������ٷ���
            }
            break;
		case MSG_BT_FAST_TEST:
			puts("MSG_BT_FAST_TEST \n");
	        set_sys_vol(26,26,FADE_ON);
			digit_auto_mute_set(0, -1, -1, -1);
			microphone_open(60,1);
			fast_test = 1;
			break;
		case MSG_OSC_INTER_CAP_WRITE:
			puts("MSG_OSC_INTER_CAP_WRITE\n");
			set_osc_2_vm();
			break;
		default:
#if (SMART_SOUNDBOX_EN)
			bt_smart_msg_deal(NULL, msg);
#endif//(SMART_SOUNDBOX_EN)
			if(get_call_status() != BT_CALL_HANGUP){
				/* puts("phone call ignore echo msg\n"); */
			}
			else{
				/* puts("bt_ctl_echo\n"); */
				echo_msg_deal_api((void**)&bt_reverb, msg);
			}

			break;
	}
}



#if BT_REC_EN
void bt_rec_msg_deal(int *msg)
{
#if BT_KTV_EN
	/* printf("get_bt_connect_status() = 0x%x\n", get_bt_connect_status()); */
	if(msg[0] == MSG_REC_START)
	{
		if((BT_STATUS_TAKEING_PHONE == get_bt_connect_status())
		||(BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()))
		{
			if(!play_sel_busy())//play_sel is not BUSY
			{
				rec_msg_deal_api(&rec_bt_api, msg);
			}
		}
	}
	else
	{
		rec_msg_deal_api(&rec_bt_api, msg);
	}
#else
	if((BT_STATUS_TAKEING_PHONE != get_bt_connect_status())&&(msg[0] == MSG_REC_START))
	if(!get_sco_connect_status()&&(msg[0] == MSG_REC_START))
	{
		return;
	}
	rec_msg_deal_api(&rec_bt_api, msg);
#endif
}
#endif
/*��Ϣ����*/
void TaskBtMsgStack(void *p_arg)
{
    //p_arg = p_arg;
    key_msg_register("btmsg",bt_ad_table,bt_io_table,bt_ir_table,NULL);
    os_sem_create(&tone_manage_semp, 1);

    while(1)
    {
        int msg[6];
		u32 res;
		memset(msg,0x00,sizeof(msg));///need do it
        res = os_taskq_pend(0, ARRAY_SIZE(msg), msg);
        clear_wdt();
        if(res != OS_NO_ERR)
        {
            msg[0] = 0xff;
        }

#if SUPPORT_APP_RCSP_EN
		rcsp_bt_task_msg_deal_before(msg);
#endif
		//printf("bt_msg:%d",msg[0]);
        btstack_key_handler(NULL, msg);

#if BT_REC_EN
        bt_rec_msg_deal(msg);
#endif
#if SUPPORT_APP_RCSP_EN
		rcsp_bt_task_msg_deal_after(msg);
#endif
	}

}

/*��Ҫ�����ϵ������Щ����������ڲ����л�ģʽ״̬*/
/*�������û��ӿڣ�Э��ջ�ص�����*/
extern int is_bt_stack_cannot_exist(void);
int msg_mask_off_in_bt()
{
#if BT_BACKMODE
    return is_bt_stack_cannot_exist();
#else
    return false;
#endif
}
extern u8 first_power_on_flag;
void enter_btstack_task()
{
    puts("\n************************BLUETOOTH TASK********************\n");
	key_msg_register("btmsg",bt_ad_table,bt_io_table,bt_ir_table,NULL);
    /* spp_mutex_init(); */
    user_val->auto_shutdown_cnt = AUTO_SHUT_DOWN_TIME;
    dac_channel_on(BT_CHANNEL, FADE_ON);

	if(first_power_on_flag)
	{
		first_power_on_flag = 0;
	}
	else
	{
		user_ctrl_prompt_tone_play(BT_STATUS_POWER_ON,NULL);
	}
	printf("-------led-1\n");
    led_fre_set(7,1);

    bt_ui_var.bt_eq_mode = &(user_val->bt_eq_mode);
    bt_ui_var.ui_bt_connect_sta = get_bt_connect_status();
    bt_ui_var.ui_bt_a2dp_sta = a2dp_get_status();
    ui_open_bt(&bt_ui_var, sizeof(BT_DIS_VAR));

#if EQ_EN
    eq_enable();
    user_val->bt_eq_mode = get_eq_default_mode();
#endif // EQ_EN

    SET_UI_MAIN(MENU_BT_MAIN);
    UI_menu(MENU_BT_MAIN);



#if (SMART_SOUNDBOX_EN)
extern u8 microphone_enable;
	if(microphone_enable)
	{
		int tmp_msg = MSG_ECHO_START;
		printf("bt mode open echo !!!\n");
		/* os_taskq_post(BTMSG_TASK_NAME, 1, MSG_ECHO_START); */
		echo_msg_deal_api((void**)&bt_reverb, &tmp_msg);
	}

	bt_smart_enter_bt_task();
#endif//(SMART_SOUNDBOX_EN)
}

/*�������û�?ӿڣ�Э��ջ�ص����?*/
/*ͨ�������л��˳�����ʱ�ص��ӿ�*/
void exist_btstack_task()
{

#if (SMART_SOUNDBOX_EN)
	bt_smart_exit_bt_task();
#endif//(SMART_SOUNDBOX_EN)

    user_val->play_phone_number_flag = 0;
    user_val->is_phone_number_come = 0;
    if(user_val->phone_prompt_tone_playing_flag)
    {
		play_sel_stop();
		user_val->phone_prompt_tone_playing_flag = 0;
    }

	while(play_sel_busy())
	{
		os_time_dly(3);
	}

    puts("----exit bt task---\n");
    /* spp_mutex_del(); */
    dac_channel_off(BT_CHANNEL, FADE_ON);
    ui_close_bt();

#if EQ_EN
    eq_disable();
#endif // EQ_EN

    SET_UI_MAIN(MENU_WAIT);
    UI_menu(MENU_WAIT);

#if BT_BACKMODE
	background_suspend();
#else
#if NFC_EN
   nfc_close();
#endif
    no_background_suspend();
#endif

    echo_exit_api((void**)&bt_reverb);
	exit_rec_api(&rec_bt_api);//exit rec when esco change


}
extern void sys_time_auto_connection();
void sys_time_auto_connection_caback(u8* addr)
{
    if(user_val->auto_connection_counter&&get_bt_prev_status() != BT_STATUS_SUSPEND)
    {
        bt_page_scan(0);
        printf("auto_conn_cnt1:%d\n",user_val->auto_connection_counter);
        user_val->auto_connection_counter--;
        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR,6,addr);
    }
}

/*�������û��ӿڣ�Э��ջ�ص�����*/
void bt_discon_complete_handle(u8 *addr , int reason)
{
    printf("bt_discon_complete:0x%x\n",reason);
    if(reason==0||reason==0x40 )
    {   //���ӳɹ�
#if BT_TWS 
		if((user_val->auto_connection_stereo)&&reason==0x40)
		{
			puts("-----------not clean auto_connection_stereo1\n");
		}
		else
		{
           user_val->auto_connection_counter = 0;
           user_val->auto_connection_stereo=0;
		}
#else
        user_val->auto_connection_counter = 0;
#endif
        return ;
    }else if(reason==0xfc){
        //�³���û�м����ַ���޷��������
		puts("vm no bd_addr\n");
		bt_page_scan(1);
		while(play_sel_busy())
		{
			os_time_dly(3);
		}

		eye_led_api(EFFECT_CONNECT, 0 ,0);

		bt_prompt_play_by_name(AI_TOY_NOTICE_P_CONNECT,NULL);
        return ;
    }
#if BT_TWS 
	else if(reason==0x10||reason==0xf||reason==0x13||reason==0x14)
	{
		puts("conneciton accept timeout\n");
		bt_page_scan(1);
        return ;
	}
	else if(reason==0x09)
	{
		puts("bt stereo search device timeout\n");
        return ;
	}
#else
	else if((reason==0x10)||(reason == 0xf))
	{
		puts("conneciton accept timeout\n");
		bt_page_scan(1);
        return ;
	}
	else if(reason == 0x16)
	{
		if(compare_task_name(BTSTACK_TASK_NAME))
		{
			eye_led_api(EFFECT_NO_CONNECT, 0, 0);
		}
	}
#endif

#if BT_TWS 
    clear_a2dP_sink_addr(addr);
#endif

	if (reason == 0x16) {
		puts("Conn Terminated by Local Host\n");
#if BT_TWS 
          if(get_current_search_index() >= 1)
		  {    //����������һ���豸
               user_send_cmd_prepare(USER_CTRL_START_CONNECTION,0,NULL);
		  }else{
              bt_page_scan(1);
		  }
#else
        bt_page_scan(1);
#endif
		vm_check_all(0);
	} else if(reason == 0x08){
		puts("\nconntime timeout\n");
		if(!get_remote_test_flag()){

#if BT_TWS 
            if(get_bt_prev_status() != BT_STATUS_SUSPEND &&
                 (get_call_status() == BT_CALL_HANGUP)&&
				 (BD_CONN_MODE == get_stereo_salve(1)))
#else
            if(get_bt_prev_status() != BT_STATUS_SUSPEND &&
                 (get_call_status() == BT_CALL_HANGUP))
#endif
            {
                user_val->auto_connection_counter = 6;
                puts("\nsuper timeout\n");
#if BT_TWS 
                if(BT_STATUS_STEREO_WAITING_CONN== get_stereo_bt_connect_status())///�ֻ����ӵ��ߣ������ֻ�֮�������������
				{
                     puts("\nafter auto_connection_stereo\n");
                     after_auto_connection_stereo(1,0);
				     user_send_cmd_prepare(USER_DEL_PAGE_STEREO_HCI,0,NULL);
				     user_val->auto_connection_stereo = 1;
                     user_val->auto_connection_counter = 4;
				}
#endif
                user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR,6,addr);
            }
		}else{
		     user_val->auto_connection_counter = 0;
		     bt_page_scan(1);
		}
	}else if(reason == 0x04){
		if(! user_val->auto_connection_counter){
		    puts("page timeout---\n");
		    if(get_current_search_index() >= 1)
		    {    //����������һ���豸
                 user_send_cmd_prepare(USER_CTRL_START_CONNECTION,0,NULL);
		    }else{
#if BT_TWS 
				if(user_val->auto_connection_stereo)
				{
					user_val->auto_connection_stereo = 0;
					puts("-----------clean auto_connection_stereo2\n");
					if((BT_STATUS_WAITINT_CONN == get_stereo_bt_connect_status()))
					{
						puts("\nstart2 auto_connection_stereo\n");
						bt_page_scan(1);
						after_auto_connection_stereo(0,1);
						return;
					}
				}
				user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR_STEREO,0,NULL);
#endif
                bt_page_scan(1);
				bt_power_max(0xFF);

				while(play_sel_busy())
				{
					os_time_dly(3);
				}
				bt_prompt_play_by_name(AI_TOY_NOTICE_P_CONNECT,NULL);
				bt_smart_led_flick(100, 0);

				if(compare_task_name(BTSTACK_TASK_NAME))
				{
					eye_led_api(EFFECT_NO_CONNECT, 0, 0);
				}

		    }
		} else {
				 printf("------3----\n");
		    if(get_bt_prev_status() != BT_STATUS_SUSPEND){
                 printf("auto_conn_cnt:%d\n",user_val->auto_connection_counter);
                 user_val->auto_connection_counter--;
                 if(user_val->auto_connection_counter%2) {
                     bt_page_scan(1);
                     sys_time_auto_connection(addr);
                 }
                 else {
                     bt_page_scan(0);
                     user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR,6,addr);
                 }
		    }
		}
	} else if(reason == 0x0b){
		puts("Connection Exist\n");
		user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR,6,addr);
	}else if(reason == 0x06){
		//connect continue after link missing
		//user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR,6,addr);	
	}
}

void bt_edr_close(void)
{
	user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI,0,NULL);
	bt_page_scan(0);
}
