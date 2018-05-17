#include "bluetooth/avctp_user.h"
#include "aec/aec_api.h"
#include "common/app_cfg.h"
#include "common/common.h"
#include "dac/dac_api.h"
#include "vm/vm_api.h"
#include "common/msg.h"
#include "sdk_cfg.h"
#include "clock.h"
#include "ui/led/led.h"
#include "key_drv/key_drv_ad.h"
#include "dac/dac.h"
#include "fcc_test.h"
#include "dev_manage/dev_ctl.h"
#include "dac/dac.h"
#include "dac/eq.h"
#include "rtc/rtc_api.h"
#include "play_sel/play_sel.h"
#include "rcsp/rcsp_interface.h"
#include "uart_param.h"
#include "rf_test_uart.h"
#include "sydf/syd_file.h"
#include "ui/led/led_eye.h"

#if (SMART_SOUNDBOX_EN)
#include "bt_smart.h"
#endif//SMART_SOUNDBOX_EN

extern volatile u8 low_power_flag;
extern u8 fast_test;

extern void bt_updata_auth_code_deal();
static u8 tws_channel_select=0;
static void in_low_pwr_port_deal(u8 mode) sec(.poweroff_text);
static void in_low_pwr_port_deal(u8 mode)
{
	u32 porta_value = 0xffff;
	u32 portb_value = 0xffff;
	u32 portc_value = 0xffff;
	u32 portd_value = 0xfff0;

#if (LEDB_PORT&LED_POARA)
	porta_value &= ~LED_B_BIT; 

#elif (LEDB_PORT&LED_POARB)
	portb_value &= ~LED_B_BIT;

#elif (LEDB_PORT&LED_POARC)
	portc_value &= ~LED_B_BIT;

#elif (LEDB_PORT&LED_POARD)
	portd_value &= ~LED_B_BIT;
#endif


#if   (LEDR_PORT&LED_POARA)
	porta_value &= ~LED_R_BIT;

#elif (LEDR_PORT&LED_POARB)
	portb_value &= ~LED_R_BIT;

#elif (LEDR_PORT&LED_POARC)
	portc_value &= ~LED_R_BIT;

#elif (LEDR_PORT&LED_POARD)
	portd_value &= ~LED_R_BIT;

#endif

	if (mode == 2) { //enter sleep mode
		JL_PORTA->DIR |= porta_value;   
		JL_PORTA->PU &= ~porta_value;   
		JL_PORTA->PD &= ~porta_value;
		JL_PORTA->DIE &= ~porta_value;  
	} else {	 

		JL_PORTA->DIR |=  porta_value;  
		JL_PORTA->PU &= ~porta_value;   
		JL_PORTA->PD &= ~porta_value;
		JL_PORTA->DIE &= ~porta_value;  
	}
	JL_PORTB->DIR |=  portb_value;  
	JL_PORTB->PU &= ~portb_value;   
	JL_PORTB->PD &= ~portb_value;   
	JL_PORTB->DIE &= ~portb_value;  


	JL_PORTC->DIR |=  portc_value;  
	JL_PORTC->PU &= ~portc_value;
	JL_PORTC->PD &= ~portc_value;   
	JL_PORTC->DIE &= ~portc_value;  


	JL_PORTD->DIR |= portd_value;   
	JL_PORTD->PU  &= ~portd_value;  
	JL_PORTD->PD  &= ~portd_value;  
	JL_PORTD->DIE &= ~portd_value; 

	/////////////////usb///////////////////
	USB_DP_DIR(1); 
	USB_DM_DIR(1); 
	USB_DP_PD(0);
	USB_DM_PD(0);
	USB_DM_PU(0);
	USB_DP_PU(0);
	JL_USB->CON0 = 0;
}	 

static u32 sniff_time_cnt =0;
/*�������û��ӿڣ�Э��ջ�ص�����*/
extern void io_key_init(void);
extern void ad_key0_init_io();
extern u8 is_pwrtimer_en();
extern void set_jiffies_10ms(u32 timer_ms);
static void out_low_pwr_deal(u32 time_ms)
{
	/* printf("%d\n",time_ms); */

#if KEY_IO_EN
    io_key_init();
#endif

#if KEY_AD_EN
	ad_key0_init_io();
#endif


	if(is_pwrtimer_en())
	{
		return;
	}

	sniff_time_cnt += time_ms;
	/* printf("%d\n",sniff_time_cnt); */
	if(sniff_time_cnt>=3000)
	{
		sniff_time_cnt =0;
	}else{
	}
#ifndef POWER_TIMER_HW_ENABLE
    set_jiffies_10ms(time_ms);
#endif

   // printf("sniff_time_cnt:%d  time_ms:%d\n",sniff_time_cnt,time_ms);
}

static void bt_updata_run()
{
    extern void bt_update_close_echo();
    bt_update_close_echo();
	puts("-------bt_updata_run\n");
	os_taskq_post_msg("MainTask",3,MSG_UPDATA,BT_UPDATA, NULL);
}

static void low_pwr_deal(u8 mode,u32 timer_ms) sec(.poweroff_text);
static void low_pwr_deal(u8 mode,u32 timer_ms)
{
	if(mode)
    {
		in_low_pwr_port_deal(mode);
	}
	else
	{
		out_low_pwr_deal(timer_ms);
	}
}

///---sdp service record profile- �û�ѡ��֧��Э��--///
#define USER_SUPPORT_PROFILE_SPP    1
#define USER_SUPPORT_PROFILE_HFP    1
#define USER_SUPPORT_PROFILE_A2DP   1
#define USER_SUPPORT_PROFILE_AVCTP  1
#define USER_SUPPORT_PROFILE_HID    0
#define USER_SUPPORT_PROFILE_PBAP   0
static void bt_profile_select_init(void)
{
    u8 support_profile;
    support_profile = SPP_CH | HFP_CH | A2DP_CH | AVCTP_CH | HID_CH | AVDTP_CH | PBAP_CH;
#if (USER_SUPPORT_PROFILE_HFP==0)
    support_profile &= ~HFP_CH;
#endif
#if (USER_SUPPORT_PROFILE_AVCTP==0)
    support_profile &= ~AVCTP_CH;
#endif
#if (USER_SUPPORT_PROFILE_A2DP==0)
    support_profile &= ~A2DP_CH;
    support_profile &= ~AVCTP_CH;
    support_profile &= ~AVDTP_CH;
#endif
#if (USER_SUPPORT_PROFILE_SPP==0)
    support_profile &= ~SPP_CH;
#endif
#if (USER_SUPPORT_PROFILE_HID== 0)
    support_profile &= ~HID_CH;
#endif

#if (USER_SUPPORT_PROFILE_PBAP== 0)

	    support_profile &= ~PBAP_CH;

#endif

    bt_cfg_default_init(support_profile);/*�ⲿ����֧��ʲôЭ��*/

#if BT_HID_INDEPENDENT_MODE
    set_hid_independent_info();//Ҫ�� bt_cfg_default_init()���ú�
#endif

}
#if BT_TWS 
void set_tws_channel_type(u8 type)
{
	tws_channel_select = type;
}
u8 get_tws_channel_type()
{
	return tws_channel_select;
}
#endif

void bt_save_osc_r_info(u8 r_val)
{
	u8 r_info;
    r_info = r_val;
    vm_write_api(VM_BT_OSC_INT_R,&r_info);
	printf("\n------osc_r_info=0x%x\n",r_info);
}

void bt_save_osc_l_info(u8 l_val)
{
	u8 l_info;
    l_info = l_val;
    vm_write_api(VM_BT_OSC_INT_L,&l_info);
	printf("\n------osc_l_info=0x%x\n",l_info);
}

extern u8 get_osc_internal_r(void);
extern u8 get_osc_internal_l(void);
void set_osc_2_vm(void)
{
	u8 osc_r_val = 0xff;
	u8 osc_l_val = 0xff;

	osc_r_val = get_osc_internal_r();
	osc_l_val = get_osc_internal_l();

	if((osc_r_val <= 0x1f)&&(osc_l_val <= 0x1f))
	{
 		bt_save_osc_r_info(osc_r_val);
 		bt_save_osc_l_info(osc_l_val);
	}
	printf("\n\nwrite_osc_info = 0x%x,0x%x\n",osc_r_val,osc_l_val);
}

//��������������������ӻ�����Ϣ��vm
static void bt_save_stereo_info(u8 stereo_type)
{
	u8 stereo_info;
    stereo_info = stereo_type;
    vm_write_api(VM_BT_STEREO_INFO,&stereo_info );
	printf("------stereo_info=0x%x\n",stereo_info );
}

///���������ӳɹ�֮��ͬ��ϵͳ�������ӻ�
static void stereo_sys_vol_sync()
{
     stereo_host_cmd_handle(MSG_VOL_STEREO,dac_ctl.sys_vol_l);
#if EQ_EN
     os_taskq_post_msg("btmsg", 1, MSG_BT_SYNC_STEREO_EQ);
#endif
}

///���������
static void stereo_deal_cmd(u8 msg,u8 value)
{
    if(msg < MSG_MAIN_MAX)
    {
         os_taskq_post_msg("MainTask", 2, msg,value);
    }
    else if(keymsg_task_name)
    {
        os_taskq_post_msg(keymsg_task_name, 2, msg,value);
    }
}

#define FW_VERSION_STRING	"AI_SOUNDBOX_E_RABBIT_V1.0.0"
static u8 fw_local_version_str[0x30];
extern lg_dev_list * lg_dev_find(u32 drv);
extern tbool syd_drive_open(void **p_fs_hdl,void *p_hdev,u32 drive_base);
extern tbool syd_drive_close(void **p_fs_hdl);
extern bool syd_get_file_byindex(void *p_fs_hdl, void **p_f_hdl, u32 file_number, char *lfn);
extern bool syd_file_close(void *p_fs_hdl, void **p_f_hdl);
extern u16 syd_read(void *p_f_hdl ,u8 _xdata *buff , u16 len);
extern bool syd_get_file_bypath(void *p_fs_hdl,void **p_f_hdl,u8 *path,char*lfn);
static void fw_version_file_init(void)
{
    lg_dev_list * pnode;
	void *fs_hdl = NULL;
	void *file_hdl = NULL;

    pnode = lg_dev_find('A');

    if (!syd_drive_open(&fs_hdl, pnode->p_cur_dev->lg_hdl->phydev_item, 0))
    {
        puts("syd_drive_open err!\n");
        return;
    }

    if(!syd_get_file_bypath(fs_hdl,&file_hdl,(u8 *)"fw_ver.str" ,0))
    {
        puts("syd_get_file_byindex err!\n");
        //
        syd_file_close(fs_hdl,&file_hdl);
        syd_drive_close(&fs_hdl);
        return;
    }

    u8 *buff;
	u16 file_len = ((SDFILE *)file_hdl)->length;
    buff = malloc(file_len);
    syd_read(file_hdl,buff,file_len);//

	memset(fw_local_version_str,0x00,sizeof(fw_local_version_str));
	memcpy(fw_local_version_str,buff,file_len);

	printf("version:%s\n",fw_local_version_str);
	printf("ver_len:%x file_len:%x\n",strlen((char *)fw_local_version_str),file_len);
	syd_file_close(fs_hdl,&file_hdl);//
	syd_drive_close(&fs_hdl);  //
	free(buff);
}

u8 *user_get_version_str(u16 *len)
{
#if 1
	*len = strlen((char *)fw_local_version_str);
	return fw_local_version_str;
#else

	*len = sizeof(FW_VERSION_STRING);
	return (u8 *)FW_VERSION_STRING; 
#endif
}

u8 ignore_pause_eye_display = 0;
extern u8 get_smart_pass();
void a2dp_decode_play_start_callback(void)
{
	/* printf("a2dp play >>>\n");	 */
	ignore_pause_eye_display = 0;

	/* if(get_smart_pass()) */
	/* { */
		/* eye_led_api(EFFECT_PLAY, 0, 0); */
	/* } */
	/* pa_2_D(); */
}
void a2dp_decode_play_stop_callback(void)
{
	/* printf("a2dp stop >>>\n");	 */
	/* if(get_smart_pass()) */
	/* { */
		if(ignore_pause_eye_display == 1)
		{
			ignore_pause_eye_display = 0;
		}
		else
		{
			if(get_curr_channel_state() != 0)
			{
				eye_led_api(EFFECT_PAUSE, 0, 0);
			}
		}
	/* } */
	/* pa_mute(); */
}


#if(BT_MODE == NORMAL_MODE)
	static const char host_name[]="AC69-Classic";
#else
	static const char host_name[]="AC69-RAM";
#endif
static const char hid_name[]="AC69_HID";
extern void __bt_set_background_conn_type_flag(u8 flag);

static void bt_function_select_init()
{
    /*������������ѡ������*/
#if(BT_MODE==NORMAL_MODE)
    u8 debug_addr[6] = {0x88, 0x67, 0x66, 0x89, 0x33, 0x11};
	__set_use_default_value(0);
#else
    u8 debug_addr[6] = {0x11, 0x22, 0x33, 0x33, 0x22, 0x11};
	__set_use_default_value(1);
#endif
    __set_host_name(host_name,sizeof(host_name));
    __set_hid_name(hid_name,sizeof(hid_name));
    __set_pin_code("0000");
    __set_bt_mac_addr(debug_addr);
	__set_host_name_idx(0);

    bt_profile_select_init();

    __bt_set_sniff(SNIFF_MODE_CONF,SNIFF_CNT_TIME);/*���ý���sniff�ǽ���poweroff����powerdown�����ÿ��ж���s����sniff*/

    __bt_set_update_battery_time(10000); /*���õ�����ʾ���͸��µ�����ʱ�䣬Ϊ0��ʾ�رյ�����ʾ���ܣ���λ���룬u32, ����С��5000ms*/
    __bt_set_a2dp_auto_play_flag(0); /*�߼���Ƶ��������Զ����Ÿ���ʹ��, 0��ʹ�ܣ�1ʹ��*/
    __set_simple_pair_flag(1);       /*�ṩ�ӿ��ⲿ������Է�ʽ,1ʹ�ܼ�����ԡ�0ʹ��pin code, ��ʹ�������ļ���ֵ*/
#if BT_TWS 
	__set_stereo_scan_key(BT_TWS_SCAN_ENBLE); 

    if(get_tws_channel_type()==0)
	{
	    __set_stereo_role_disconnect_phone(0); 
	}
	else
	{
	    __set_stereo_role_disconnect_phone(1); 
        __set_stereo_device_channel(get_tws_channel_type());
	}
	__set_sbc_cap_bitpool(38); 
    __set_user_ctrl_conn_num(2);     /*�û�����֧�����ӵĸ�����1 �� 2*/
    __set_auto_conn_device_num(2);   /*�ýӿ����������ϵ������Ҫ���������豸�ĸ�����0��ʾ�ϵ粻����������20��Ч��ֱ������һ��*/
#if BT_HID_INDEPENDENT_MODE
    if(__get_hid_independent_flag())
	{
    	__set_stereo_device_indicate("HD");/*���ö���������ʶ,support�����ַ���ʶ��inquiryʱ����,��������Ӧ�ı�ʶ����������*/
	}
	else
#endif		
	{
    	__set_stereo_device_indicate("MT");/*���ö���������ʶ,support�����ַ���ʶ��inquiryʱ����,��������Ӧ�ı�ʶ����������*/
	}
    __set_searche_timeout_value(12000); /*��������������ʱ�䡣ms��λ������������*/
    __set_connect_stereo_timeout_value(0); /*���û������䳬ʱʱ�䣬0Ϊ����ʱ��ֱ�������϶���Ϊֹ*/
    __set_start_connet_stereo(1);          /*���ÿ����Ƿ��������*/
    __set_soundtrack_compound(1);          /*����soundtrack,����������������ʹ��*/
    __set_indicate_name_cmp_en(0);      /*������ͬ�������ֲ�������ʹ��*/
#else
	__set_sbc_cap_bitpool(53); 
	__set_user_ctrl_conn_num(1);     /*�û�����֧�����ӵĸ�����1 �� 2*/
    __set_auto_conn_device_num(1);   /*�ýӿ����������ϵ������Ҫ���������豸�ĸ�����0��ʾ�ϵ粻����������20��Ч��ֱ������һ��*/
#endif
    __set_page_timeout_value(5000); /*��������ʱ�䳤������,��ʹ�øú���ע��ʹ�ã�ms��λ,u16*/
#if BT_HID_INDEPENDENT_MODE
    __set_hid_auto_disconn_en(0);    /*�Ƿ��Զ��Ͽ��ֻ���HID����,1��Ͽ���0�Ǳ���HID����*/
#else
    __set_hid_auto_disconn_en(1);    /*�Ƿ��Զ��Ͽ��ֻ���HID����,1��Ͽ���0�Ǳ���HID����*/
#endif
    __set_test_masetr_enable(1);     /*�ӿ������ǲ��Ժ�ģʽ��������SDK*/
    __set_super_timeout_value(8000); /*����ʱ��ʱ�������á�ms��λ����������Ч*/
    __set_connect_scan_timeout_value(0);/*//���ÿ��������ɷ��ֿ����ӵ�ʱ�䣬Ϊ��ʡ�磬����һ���Զ��رտ����ӵ�ʱ�䡣ms��λ��Ϊ 0 ʱ��ʹ���Զ��ر�*/
    //__set_connect_scan_with_key(0);     /*������һ̨֮�����ӵڶ�̨�Ƿ�ͨ����������connect_scan*/
	__bt_set_music_back_play_flag(1);
    __set_music_break_in_flag(1);  /* ��Ƶ���Ͽ��أ�0�������ϣ�1������*/
    __set_hfp_switch(1);             /*ͨ�����Ͽ��أ�0�������ϣ�1������*/
#if BT_TWS 
	__bt_set_background_conn_type_flag(0);
#else
	__bt_set_background_conn_type_flag(1);
#endif
     #if (SNIFF_MODE_CONF&SNIFF_EN)
	     __set_device_role(S_DEVICE_ROLE_SLAVE_TO_MASTER | M_DEVICE_ROLE_MASTER);  
	 #else
	     __set_device_role(S_DEVICE_ROLE_SLAVE | M_DEVICE_ROLE_MASTER);  
     #endif
	__change_hci_class_type(BD_CLASS_WEARABLE_HEADSET);/*�ı������豸���ͣ����Ըı����ӳɹ���ʾͼ��*/
	fw_version_file_init();
}

 /*
    ������һЩ��Ĺ��Ӻ���ע�ᣬһ�����ڸı������
    Ϊ�˼��ݲ�ͬ�ͻ��Բ�ͬ�������������й��Ӻ�����ע�Ὣʹ��Ĭ�ϲ�����
    �������һЩ�ص������ӿڣ��û��������������ã���
    �ص���������Э��ջ��ȡ��Ҫֵ���߼�ʱ�����Ľӿ�
 */
/*�����ȼ��仯*/
#define POWER_FLOOR_LEVEL   32
static int bt_get_battery_value()
{   //ֻ֧��1-9ֱ�ӵ�ֵ��9���ȼ�
    u16 battery_level;
    if(get_battery_level() <= POWER_FLOOR_LEVEL)
        battery_level = 1;
    else
        battery_level = get_battery_level() - POWER_FLOOR_LEVEL;
    return battery_level;
}

/*�绰ʱ�ı�������״ֵ̬��ע���ǵ绰���������ӿ�ʹ�ú���з���*/
static void call_vol_change(int vol)
{
	//iphone(0-15level)
    printf("vol %d",vol);
}
/*�ýӿڶ�ȡ����ʱ��Ӹûص��������أ�Ĭ�������Ͼ���һ�λص���
Ҫʹ��Զ���豸���ֵĿ���copy����*/
static void bt_read_remote_name(u8 *name)
{
    puts("\n remote name : ");
    puts((char *)name);
    puts("\n");
}

/*����һ����־������Ҫ������Ӳ�����*/
static void bt_is_auto_conn(u8 auto_conn_flag)
{   //���ֻ����ɹ����Ǳ����ɹ�
    if(auto_conn_flag)
    {
        //�����ɹ�
    }else{
        //�����ɹ�
    }
}

extern void pbap_register_packet_handler(void (*handler)(u8 type , const u8 *name, const u8 *number ,const int *date));
void phonebook_packet_handler(u8 type , const u8 *name, const u8 *number ,const int *date)
{
	static u16 number_cnt = 0;
	printf("NO.%d:",number_cnt );
	number_cnt++;
	printf("type:%d ", type);
	if(name)
		printf(" NAME:%s  ", name);
	if(number)
		printf("number:%s  ",number);
	if(date)
		printf("date:%s ",date);
	putchar('\n');
}


extern int spp_mutex_init(void);
extern int spp_mutex_del(void);
static void spp_data_deal(u8 packet_type, u16 channel, u8 *packet, u16 size)
{
    //��Ϊ�޸���user_send_cmd_prepare�ӿڷ��ʹ������ݻ�peng�ŷ���ŷ��ء�
    //���Ǹûص�ָ��������Э��ջѭ�����õģ�������user_send_cmd_prepare�ӿڷ�����
    //Ҫ�����ÿ���ֱ��int spp_data_try_send(const u8* buf, u16 len)�ӿ�
    switch(packet_type)
    {
        case 1:
             //���ӳɹ�
             puts("spp connect\n");
    		 spp_mutex_init();
             break;
        case 2:
             //���ӶϿ�
             puts("spp disconnect\n");
			 spp_mutex_del();
             break;
        case 7://DATA
            ///puts("SP ");
		if(packet[0]=='9')
		{
		    //os_taskq_post_msg(keymsg_task_name, 1,MSG_BT_SPP_UPDATA);
		}
#if EQ_EN
            if(packet[0]=='E'&&packet[1]=='Q')
             {
                    update_ef_info(packet+4,size-4,packet[2]);
                    break;
             }
#endif // EQ_EN
             break;
    }
}

#if SUPPORT_APP_RCSP_EN
extern void ble_enter_mutex(void);
extern void ble_exit_mutex(void);
static void spp_rcsp_data_deal(u8 packet_type, u16 channel, u8 *packet, u16 size)
{
    //��Ϊ�޸���user_send_cmd_prepare�ӿڷ��ʹ������ݻ�peng�ŷ���ŷ��ء�
    //���Ǹûص�ָ��������Э��ջѭ�����õģ�������user_send_cmd_prepare�ӿڷ�����
    //Ҫ�����ÿ���ֱ��int spp_data_try_send(const u8* buf, u16 len)�ӿ�
    switch(packet_type)
    {
        case 1:
             //���ӳɹ�
             puts("spp connect\n");
    		 spp_mutex_init();
#if (BLE_BREDR_MODE&BT_BLE_EN)
 			 ble_enter_mutex();
#endif
 			 rcsp_event_com_start(RCSP_APP_TYPE_SPP);
             rcsp_register_comsend(bt_spp_send_data);
             break;
        
		case 2:
             //���ӶϿ�
             puts("spp disconnect\n");
             rcsp_event_com_stop();
#if (BLE_BREDR_MODE&BT_BLE_EN)
             ble_exit_mutex();
#endif
			 spp_mutex_del();
             break;
        
		case 7://DATA
            ///puts("SP ");
            rcsp_comdata_recieve(packet,size);
             break;
    }
}
#endif

/*set_device_volume,get_dac_vol�Ǹ߼���Ƶͬ���ֻ�������صģ�ע����绰ʱ�������
�û���ʹ��*/
static void set_device_volume(int volume )
{
	u8 dac_max_vol = get_max_sys_vol(1);
    dac_ctl.sys_vol_l  = volume * dac_max_vol / 0x7F;
    dac_ctl.sys_vol_r = dac_ctl.sys_vol_l  ;
    if(is_dac_mute()) {
   		dac_mute(0,1);
    }
#if BT_TWS 
	if(!is_check_stereo_slave())
	{
		stereo_host_cmd_handle(MSG_VOL_STEREO,dac_ctl.sys_vol_r);
			
	}
#endif
    set_sys_vol(dac_ctl.sys_vol_l, dac_ctl.sys_vol_r, FADE_ON);
	printf("vol_sync,dac = %d\n",dac_ctl.sys_vol_l);
}

static int get_dac_vol()
{
	int dac_max_vol = get_max_sys_vol(1);
	int phone_vol = dac_ctl.sys_vol_l * 0x7F / dac_max_vol;
	printf("vol_sync,phone = %d\n",phone_vol);
    /*The value 0x0 corresponds to 0%. The value 0x7F corresponds to 100%.*/
    return phone_vol;
}

//phone_talking_end,save phone_vol and recover dac_vol
static void phone_sync_sys_dac_vol(u8 vol,u8 phone_vol)
{
   dac_ctl.sys_vol_l = vol;
   dac_ctl.sys_vol_r = vol;
   dac_ctl.phone_vol = phone_vol;
   vm_write_api(VM_PHONE_VOL,&dac_ctl.phone_vol);
   printf("phone_vol:%d ,sys_vol:%d\n",dac_ctl.phone_vol,vol);
}

//phone_talking_start,set phone_vol with vm_phone_vol
#define PHONE_VOL_REMEMBER
static u8 phone_sync_phone_vol(u8 phone_vol_max)
{
	u8 phone_vol = 0;
    //vm_read_api(VM_PHONE_VOL,&phone_vol);
  	printf("phone_vol:%d,max_vol:%d\n",dac_ctl.phone_vol,phone_vol_max);
	if(dac_ctl.phone_vol < 10)
		dac_ctl.phone_vol = phone_vol_max;

#ifndef	PHONE_VOL_REMEMBER
	dac_ctl.phone_vol = phone_vol_max;
#endif

#if ((SMART_SOUNDBOX_EN == 1) && (BT_SMART_SPEECH_SOURCE == SCO_INPUT))
	bt_mart_sco_active();		
#endif

   	dac_ctl.sys_vol_l = dac_ctl.phone_vol;
   	dac_ctl.sys_vol_r = dac_ctl.phone_vol;
	return dac_ctl.phone_vol;
}

extern u8 get_key_invalid_flag(void);

/*����Ƿ���Խ���͹���*/
/*1 : ���Խ�lowpower   0: �����Խ�lowpower*/
static u32 check_enter_lowpower(void)
{

#if  !BT_LOW_POWER_MODE
	return 0;
#endif

	if(fast_test)
	{
		return 0;	
	}

	//�Ǿ���ģʽ����lowpowerģʽ
    if(!is_auto_mute())
	{
		return 0;
	}

	if(play_sel_busy()) {
		//puts("play_sel_busy\n");
		return 0;
	}

	//playing key_voice,don't enter lowpower
	if(dac_ctl.sniff_check) {
		return 0;
	}

	//mute dac module
	if(!is_dac_mute()) {
		puts("---off----\n");
		os_taskq_post(MAIN_TASK_NAME,1,MSG_DAC_OFF);
		return 0;
	}

#if SYS_LVD_EN
	if(low_power_flag)
		return 0;
#endif

	return 1;
}

/*Ĭ��û��ע�ᣬ�ڶ�ȡ�������ļ������á��û���Ҫ�Ե�ַ�����⴦��Ŀ��Բ������ӽ����޸�*/
static void bt_addr_deal(u8 *addr)
{
    /*��ַָ�룬��ַֻ��6��byte��ע��Խ����ʻ�������, ����*/
    puts("bt_addr_deal\n");
    addr[0] = 0x1A;
    addr[1] = 0x2A;
    addr[2] = 0x3A;
//    addr[3] = 0x1A;
//    addr[4] = 0x2A;
//    addr[5] = 0x3A;
}

static u8 bt_aec_priority()
{
	return (u8)TaskBtAecPrio;
}


//�ú�����Ҫ����Ϊ��?�Щ���ֵ���ܻᱻ�û����޸�Ӱ�쵽������Ҫͨ���ӿڻ�ȡ�?
///*�ر�ע�ⷵ��ֵ�ķ�Χ*///
#define BT_TRIM_ALWAYS 1
#define BT_TRIM_ONCE   0
static u8 bt_get_macro_value(BT_STACK_MACRO type)
{
    //be careful :   MAXRO return value in the function must between 1-254;
    switch(type)
    {
        case BT_MACRO_AEC_SD_PRIO:
            return TaskBtAecSDPrio;
        case BT_MACRO_SBC_PRIO:
            return TaskSbcPrio;
        case BT_MACRO_BTLMP_PRIO:
            return TaskBtLmpPrio;
        case BT_MACRO_BTSTACK_PRIO:
            return TaskBtStackPrio;
		case BT_MACRO_BTESCO_PRIO:
			return TaskBtEscoPrio;
        case BT_MACRO_RESOURCE_PRIO:
            return TaskResoursrPrio;
        case BT_MACRO_MAX_SYS_VOL_L:
            return MAX_SYS_VOL_L;
        case BT_MACRO_MAX_SYS_VOL_R:
            return MAX_SYS_VOL_R;
        case BT_MACRO_SYSCFG_REMOTE_DB:
            return SYSCFG_REMOTE_DB;
        case BT_MACRO_SYSCFG_REMOTE_DB_HID:
            return SYSCFG_REMOTE_DB_HID;
        case BT_MACRO_MSG_LAST_WORKMOD:
            return MSG_LAST_WORKMOD;
        case BT_MACRO_SYS_EVENT_TASK_RUN_REQ:
            return SYS_EVENT_TASK_RUN_REQ;
        case BT_MACRO_MSG_BT_STACK_STATUS_CHANGE:
            return MSG_BT_STACK_STATUS_CHANGE;
        case BT_TRIM_MODE:
#if(BT_MODE == NORMAL_MODE)
            return BT_TRIM_ONCE;//BT_TRIM_ALWAYS
#else 
			return BT_TRIM_ALWAYS;
#endif
		case BT_ESCO_FILTER_LEVEL:
			return 0;
		case BT_MACRO_A2DP_AAC_EN:
			return 0;
       case BT_MACRO_STEREO_SYSCFG_REMOTE_DB:
            return SYSCFG_REMOTE_DB_STEREO;

       case BT_MACRO_STEREO_SYSCFG_REMOTE_DB_HID:
           return SYSCFG_REMOTE_DB_STEREO_HID;

        default:
            printf("check the lib,0x%x\n",type);
            return 0xff;
    }
}

static void low_pwr_dac_on_control()
{
	os_taskq_post(MAIN_TASK_NAME,1,MSG_DAC_ON);
}

static void low_pwr_dac_off_control()
{
	os_taskq_post(MAIN_TASK_NAME,1,MSG_DAC_OFF);
}

u32 pwr_timer_in(void)
{
	/* putchar('I'); */
	return 1;
}

u8 led_flag = 0;
u32 pwr_timer_out(void)
{
	/* putchar('O'); */
#if KEY_IO_EN
    io_key_init();
#endif
	return 1;
}


void bt_fast_test_handle()
{
	puts("bt_fast_test_handle \n");
	os_taskq_post(BTMSG_TASK_NAME,1,MSG_BT_FAST_TEST);
}

void bt_addr_handle(u8 * bredr_addr,u8 * ble_addr)
{
	puts("bredr addr : ");
	put_buf(bredr_addr,6);
	puts("ble addr : ");
	put_buf(ble_addr,6);
}

void get_phone_time(char *data , u16 len)
{
	put_buf((u8 *)data , len);		
	puts(data);		
}
extern void hook_get_phone_time_register(void (*handle)(char* data, u16 length));
extern u8 get_esco_st(u8 sw);
/*
�������õ���һϵ�п����ò������ã�ͨ��handle����ʽ��
�����û��Ͳ��ÿ��ǵ������ú����ľ���λ�á���Ҫ�ı����ע�ᣬ����Ҫ������Ĭ�ϲ���
*/

static void ble_name_custom_handle(char *name)
{
	printf("ble name:%s\n",name);		
	//read name form vm,memcpy(name,vm_name,...);
	//ble name support 16 bytes
}

static void bredr_name_custom_handle(char *name)
{
	printf("bredr name:%s\n",name);		
	//read name form vm ,memcpy(name,vm_name,...);
	//bredr name support 32 bytes
}

extern void read_vm_ble_name_handle_register(void (*handle)(char *name));
extern void read_vm_bt_name_handle_register(void (*handle)(char *name));
extern void set_sys_pd_ldo_level(u8 level);
extern void set_sys_pd_dvdd_level(u8 level);
extern void bt_music_info_handle_register(void (*handler)(u8 type, u32 time,u8 *info, u16 len));
void user_get_bt_music_info(u8 type , u32 time ,u8 *info ,  u16 len )
{
	//profile define type: 1-title 2-artist name 3-album names 4-track number 5-total number of tracks 6-genre  7-playing time
	//JL define 0x10-total time , 0x11 current play position
	u8  min , sec;
	//printf("type %d\n", type );
	if((info!=NULL) && (len!=0))	
	{
		puts((const char *)info);
		putchar('\n');
	}
	if(time != 0)
	{
		min = time / 1000 / 60;
		sec = time / 1000 - (min * 60);
		printf(" time %d %d\n ", min , sec );
	}
}
void common_handle_register(void)
{
	//���ý���powerdown��ʱ��vddio��rtcvdd�������ٷ�
	//0 : 3.53v  1 : 3.34v  2 : 3.18v  3 : 3.04v 
	//4 : 2.87v  5 : 2.73v  6 : 2.62v  7 : 2.52v
	set_sys_pd_ldo_level(7);
	set_sys_pd_dvdd_level(POWER_DOWN_DVDD_LEV);
    bt_low_pwr_handle_register(low_pwr_deal,low_pwr_dac_on_control,low_pwr_dac_off_control);
    bt_set_low_power(LOWPOWER_OSC_TYPE,LOWPOWER_OSC_HZ,BT_LOW_POWER_MODE,1,SYS_Hz/1000000L,check_enter_lowpower,NULL); /*�����͹�������*/
}

static void bt_osc_cap_save_hanlde(void)
{
	os_taskq_post(BTMSG_TASK_NAME,1,MSG_OSC_INTER_CAP_WRITE);			
}

static void bredr_handle_register()
{
    register_edr_init_handle();
	aec_get_macro_handle_register( bt_aec_priority);    /*must set*/
    set_bt_info_callback(bt_function_select_init);/*�൱��Ĭ�����ã�Ҫ��Э��ջ��ʼ��ǰ����*/
    get_battery_value_register(bt_get_battery_value);   /*������ʾ��ȡ�����Ľӿ�*/
    call_vol_change_handle_register(call_vol_change); /*ͨ�������е����������з����Ļص�����*/
    read_remote_name_handle_register(bt_read_remote_name);  /*���ӳɹ����ȡ�ֻ���?ִӸú������?*/
    discon_complete_handle_register(bt_discon_complete_handle);/*�Ͽ����������ϻ���õĺ��������ͻ�������Ϣ*/

#if SUPPORT_APP_RCSP_EN
	spp_data_deal_handle_register(spp_rcsp_data_deal);    /*�򵥴���*/
#else
	spp_data_deal_handle_register(spp_data_deal);    /*�򵥴���*/
#endif
	hook_get_phone_time_register(get_phone_time);
	hook_phone_number_register(hook_hfp_incoming_phone_number);   /*��ȡ���͵��绰���뷵��*/
    phone_sync_sys_vol_handle_register(phone_sync_sys_dac_vol,phone_sync_phone_vol );  /*ͨ��ʱͬ��ϵͳ����*/
    //music_vol_change_handle_register(set_device_volume, get_dac_vol );  /*ͬ���ֻ���������*/
    //bt_addr_spec_deal_handle_register(bt_addr_deal);   /*���Ը��Ի����õ�ַ*/
	bt_auto_conn_handle_register(bt_is_auto_conn);
	bt_osc_write_register(bt_osc_cap_save_hanlde);
    bt_updata_run_handle_register(bt_updata_run);/*������������*/
    bt_updata_auth_code_handle_register(bt_updata_auth_code_deal);/*������֤��ɹ�����*/
	bt_chip_io_type_setting(RTCVDD_TYPE , BTAVDD_TYPE);
	bt_set_noconnect_lowpower_fun(1,pwr_timer_in,pwr_timer_out);/*����û�����ӵ�ʱ���Ƿ����͹���ģʽ*/
#ifndef POWER_TIMER_HW_ENABLE
    bt_set_noconnect_lowpower_led_fun(1);
#endif
	esco_handle_register(get_esco_st);
#if BT_TWS 	
	bt_stereo_register(bt_save_stereo_info,stereo_sys_vol_sync,stereo_deal_cmd,NULL);  /*����ӿ�*/
#else
	bt_stereo_register(NULL,NULL,NULL,NULL);  /*����ӿ�*/
#endif
	bt_fast_test_handle_register(bt_fast_test_handle);
	/* bt_addr_handle_register(bt_addr_handle); */
	read_vm_bt_name_handle_register(bredr_name_custom_handle);
	/*pbap_register_packet_handler(phonebook_packet_handler);*/


	a2dp_play_callback_reg(a2dp_decode_play_start_callback, a2dp_decode_play_stop_callback);
}

//callback fun for testbox fre offset adjust handle
extern void set_osc_int(u8 r,u8 l);
static void ble_osc_cap_change_handler(u8 osc_l,u8 osc_r,u8 adjust_end_flag)
{
	printf("osc_l:%x osc_r:%x adjust_flag:%x\n",osc_l,osc_r,adjust_end_flag);
	set_osc_int(osc_r,osc_l);
	if(adjust_end_flag)
	{
		os_taskq_post(BTMSG_TASK_NAME,1,MSG_OSC_INTER_CAP_WRITE);			
	}
}

static void ble_testbox_connected_handle(u8 flag)
{
	printf("testbox connected!!!:%x\n",flag);		
	//For user to do ...  
}

extern void ble_osc_cap_change_handle_register(void (*handler)(u8,u8,u8));
extern void bt_addr_spec_deal_handle_register(void (*handle)(u8 *addr));
static void ble_handle_register()
{
    /* ble_init_config(); */
	//bt_addr_spec_deal_handle_register(bt_addr_deal);   /*���Ը��Ի����õ�ַ*/
	read_vm_ble_name_handle_register(ble_name_custom_handle);
    register_ble_init_handle();
	//if enable testbox auto adjust frequency offset,this callback must be registered
	ble_osc_cap_change_handle_register(ble_osc_cap_change_handler);
	ble_testbox_connect_handle_register(ble_testbox_connected_handle);
}

static void stereo_handle_register()
{
  
	register_stereo_init_handle();
}

void reset_osc_int_cfg()
{
	u8 osc_r = 0xff;
	u8 osc_l = 0xff;

    if (VM_READ_NO_INDEX == vm_read_api(VM_BT_OSC_INT_R,&osc_r))
	{
		puts("\nvm err1\n\n");
		return;
	}

    if (VM_READ_NO_INDEX == vm_read_api(VM_BT_OSC_INT_L,&osc_l))
	{
		puts("\nvm err2\n\n");
		return;
	}

	printf("\n\nosc %d,%d\n",osc_r,osc_l);

	if((osc_r <= 0x1f)&&(osc_l <= 0x1f))
	{
 		set_osc_int(osc_r,osc_l);
	}
}


/*�������û��ӿڣ�Э��ջ�ص�����*/
extern void bt_osc_internal_cfg(u8 sel_l,u8 sel_r);
extern void app_server_init(void);
extern u8 ble_rf_uart_rx_isr(u8 rx_data);
extern void ble_gatt_client_init(void);
#if BT_TWS 
static u8 rx_mem[10*1024] sec(.bt_classic_data);
static u8 tx_mem[5*1024] sec(.bt_classic_data);
#else
static u8 rx_mem[5*1024] sec(.bt_classic_data);
static u8 tx_mem[3*1024] sec(.bt_classic_data);
#endif
void bt_app_cfg()
{
	u8 (*fcc_uart_isr)(u8);
	u8 (*ble_rf_uart_isr)(u8);
    u8 poweroff = 0;
    poweroff = bt_power_is_poweroff_post();
    cfg_test_box_and_work_mode(NON_TEST,BT_MODE);
	cfg_ble_work_mode(BT_MODE);
#if (BT_MODE != NORMAL_MODE)
	fcc_uart_isr = NULL;
	ble_rf_uart_isr = NULL;
	rf_test_uart_para_init(UART1_HARDWARE_NAME,9600,UART_USBP_USBM);
	#if (BLE_BREDR_MODE&BT_BREDR_EN)
	fcc_uart_isr = fcc_uart_isr_callback;
	#endif
	#if (BLE_BREDR_MODE&BT_BLE_EN)
	ble_rf_uart_isr = ble_rf_uart_rx_isr;
	#endif
	register_rf_test_uart_callback(fcc_uart_isr,ble_rf_uart_isr);
#endif
	cfg_bt_pll_para(OSC_Hz,SYS_Hz,BT_ANALOG_CFG,BT_XOSC);
 	cfg_bredr_mem(rx_mem,sizeof(rx_mem),tx_mem,sizeof(tx_mem));
	/*internal capacitor range:0x00~0x1F*/

    bt_get_macro_handle_register(bt_get_macro_value); /*must set*/

    __set_ble_bredr_mode(BLE_BREDR_MODE);    /*bt enble BT_BLE_EN|BT_BREDR_EN */

#if BT_TWS 
    set_tws_channel_type(BT_TWS_CHANNEL_SELECT);
    __set_stereo_mode(1);
    stereo_handle_register();
#else
    __set_stereo_mode(0);
#endif // BT_STEREO

	set_bt_config_check_callback(bt_info_init);   //    bt_info_init();[>��ȡflash�������ļ�<]
#if (BT_MODE != NORMAL_MODE)
	fcc_test_handle_register(rf_test_uart_init_callback,test_fcc_default_info);/*fcc���Ժ���ע��*/
	ble_rf_test_info_register(BLE_BREDR_MODE,test_ble_rf_default_info); 
#endif
#if (BLE_BREDR_MODE&BT_BREDR_EN)
    bredr_handle_register();
#endif   //SUPPORT BREDR
#if (BLE_BREDR_MODE&BT_BLE_EN)
    ble_handle_register();
#if BLE_GATT_ROLE_CFG
    extern void app_client_init(void);
    app_client_init();
#else
    app_server_init();
#endif
#endif   //SUPPORT BLE
	
	common_handle_register();
	reset_osc_int_cfg();
}

