/*
 *********************************************************************************************************
 *                                            br16
 *                                            btstack
 *                                             CODE
 *
 *                          (c) Copyright 2016-2016, ZHUHAI JIELI
 *                                           All Rights Reserved
 *
 * File : *
 * By   : jamin.li
 * DATE : 2016-04-12 10:17AM    build this file
 *********************************************************************************************************
 */
#include "typedef.h"
//#include "error.h"
#include "sdk_cfg.h"
#include "common/msg.h"
#include "common/app_cfg.h"
/* #include "bluetooth/le_profile_def.h" */
#include <stdint.h>
#include "bluetooth/le_server_module.h"
#include "rtos/os_api.h"

#include "rtos/task_manage.h"
#include "rcsp/rcsp_interface.h"
#include "rcsp/rcsp_head.h"
#include "bt_smart.h"
#include "bluetooth/avctp_user.h"
#include "play_sel/play_sel.h"
#include "ui/led/led_eye.h"
#include "bluetooth/ble_api.h"

#if(BLE_BREDR_MODE&BT_BLE_EN)

extern char *keymsg_task_name;

uint16_t ble_conn_handle;//设备连接handle
static u8 ble_mutex_flag;// ble与spp 互斥标记,0:表示ble可连接，1：表示ble不可连接
static u8 ble_server_close_flag = 0;

static u32 app_data_send(u8 *data,u16 len);
static void app_data_recieve(u8 *data, u16 len);

extern u32  ble_smart_send(u8 * data,u16 len) ;
static u16 ble_update_send_handle = 0;
const uint8_t profile_data[] =
{
    // 0x0001 PRIMARY_SERVICE-GAP_SERVICE
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18, 
    // 0x0002 CHARACTERISTIC-GAP_DEVICE_NAME-READ | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x02, 0x03, 0x00, 0x00, 0x2a, 
    // 0x0003 VALUE-GAP_DEVICE_NAME-READ | DYNAMIC-'BLE_GAP_NAME'
    0x14, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, 0x2a, 0x42, 0x4c, 0x45, 0x5f, 0x47, 0x41, 0x50, 0x5f, 0x4e, 0x41, 0x4d, 0x45, 

    // 0x0004 PRIMARY_SERVICE-GATT_SERVICE
    0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x01, 0x18, 
    // 0x0005 CHARACTERISTIC-GATT_SERVICE_CHANGED-READ
    0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x02, 0x06, 0x00, 0x05, 0x2a, 
    // 0x0006 VALUE-GATT_SERVICE_CHANGED-READ-''
    0x08, 0x00, 0x02, 0x00, 0x06, 0x00, 0x05, 0x2a, 

    // 0x0007 PRIMARY_SERVICE-ae00
    0x0a, 0x00, 0x02, 0x00, 0x07, 0x00, 0x00, 0x28, 0x00, 0xae, 
    // 0x0008 CHARACTERISTIC-ae01-WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x08, 0x00, 0x03, 0x28, 0x04, 0x09, 0x00, 0x01, 0xae, 
    // 0x0009 VALUE-ae01-WRITE_WITHOUT_RESPONSE | DYNAMIC-''
    0x08, 0x00, 0x04, 0x01, 0x09, 0x00, 0x01, 0xae, 
    // 0x000a CHARACTERISTIC-ae02-NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x03, 0x28, 0x10, 0x0b, 0x00, 0x02, 0xae, 
    // 0x000b VALUE-ae02-NOTIFY-''
    0x08, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x02, 0xae, 
    // 0x000c CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x0c, 0x00, 0x02, 0x29, 0x00, 0x00, 
    // 0x000d CHARACTERISTIC-ae03-WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x0d, 0x00, 0x03, 0x28, 0x04, 0x0e, 0x00, 0x03, 0xae, 
    // 0x000e VALUE-ae03-WRITE_WITHOUT_RESPONSE | DYNAMIC-''
    0x08, 0x00, 0x04, 0x01, 0x0e, 0x00, 0x03, 0xae, 
    // 0x000f CHARACTERISTIC-ae04-NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x0f, 0x00, 0x03, 0x28, 0x10, 0x10, 0x00, 0x04, 0xae, 
    // 0x0010 VALUE-ae04-NOTIFY-''
    0x08, 0x00, 0x10, 0x00, 0x10, 0x00, 0x04, 0xae, 
    // 0x0011 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x11, 0x00, 0x02, 0x29, 0x00, 0x00, 
    // 0x0012 CHARACTERISTIC-ae05-WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x12, 0x00, 0x03, 0x28, 0x04, 0x13, 0x00, 0x05, 0xae, 
    // 0x0013 VALUE-ae05-WRITE_WITHOUT_RESPONSE | DYNAMIC-''
    0x08, 0x00, 0x04, 0x01, 0x13, 0x00, 0x05, 0xae, 
    // 0x0014 CHARACTERISTIC-ae06-NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x14, 0x00, 0x03, 0x28, 0x10, 0x15, 0x00, 0x06, 0xae, 
    // 0x0015 VALUE-ae06-NOTIFY-''
    0x08, 0x00, 0x10, 0x00, 0x15, 0x00, 0x06, 0xae, 
    // 0x0016 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x16, 0x00, 0x02, 0x29, 0x00, 0x00, 
    // 0x0017 CHARACTERISTIC-ae07-WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x17, 0x00, 0x03, 0x28, 0x04, 0x18, 0x00, 0x07, 0xae, 
    // 0x0018 VALUE-ae07-WRITE_WITHOUT_RESPONSE | DYNAMIC-''
    0x08, 0x00, 0x04, 0x01, 0x18, 0x00, 0x07, 0xae, 
    // 0x0019 CHARACTERISTIC-ae08-NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x19, 0x00, 0x03, 0x28, 0x10, 0x1a, 0x00, 0x08, 0xae, 
    // 0x001a VALUE-ae08-NOTIFY-''
    0x08, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x08, 0xae, 
    // 0x001b CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x1b, 0x00, 0x02, 0x29, 0x00, 0x00, 

#if BLE_APP_UPDATE_SUPPORT_EN
    // 0x001c PRIMARY_SERVICE-00001530-1212-EFDE-1523-785FEABCD123
    0x18, 0x00, 0x02, 0x00, 0x1c, 0x00, 0x00, 0x28, 0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00, 
    // 0x001d CHARACTERISTIC-00001531-1212-EFDE-1523-785FEABCD123-NOTIFY | WRITE | DYNAMIC
    0x1b, 0x00, 0x02, 0x00, 0x1d, 0x00, 0x03, 0x28, 0x18, 0x1e, 0x00, 0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef, 0x12, 0x12, 0x31, 0x15, 0x00, 0x00, 
    // 0x001e VALUE-00001531-1212-EFDE-1523-785FEABCD123-NOTIFY | WRITE | DYNAMIC-''
    0x16, 0x00, 0x18, 0x03, 0x1e, 0x00, 0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef, 0x12, 0x12, 0x31, 0x15, 0x00, 0x00, 
    // 0x001f CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x1f, 0x00, 0x02, 0x29, 0x00, 0x00, 
    // 0x0020 CHARACTERISTIC-00001532-1212-EFDE-1523-785FEABCD123-WRITE_WITHOUT_RESPONSE| DYNAMIC
    0x1b, 0x00, 0x02, 0x00, 0x20, 0x00, 0x03, 0x28, 0x04, 0x21, 0x00, 0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef, 0x12, 0x12, 0x32, 0x15, 0x00, 0x00, 
    // 0x0021 VALUE-00001532-1212-EFDE-1523-785FEABCD123-WRITE_WITHOUT_RESPONSE| DYNAMIC-''
    0x16, 0x00, 0x04, 0x03, 0x21, 0x00, 0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef, 0x12, 0x12, 0x32, 0x15, 0x00, 0x00, 
#endif
    // END
    0x00, 0x00, 
}; // total size 162 bytes 


//
// list mapping between characteristics and handles
//
#define ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_GATT_SERVICE_CHANGED_01_VALUE_HANDLE 0x0006
#define ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE 0x0009
#define ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE 0x000b
#define ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE 0x000c
#define ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE 0x000e
#define ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE 0x0010
#define ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE 0x0011
#define ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE 0x0013
#define ATT_CHARACTERISTIC_ae06_01_VALUE_HANDLE 0x0015
#define ATT_CHARACTERISTIC_ae06_01_CLIENT_CONFIGURATION_HANDLE 0x0016
#define ATT_CHARACTERISTIC_ae07_01_VALUE_HANDLE 0x0018
#define ATT_CHARACTERISTIC_ae08_01_VALUE_HANDLE 0x001a
#define ATT_CHARACTERISTIC_ae08_01_CLIENT_CONFIGURATION_HANDLE 0x001b
#define ATT_CHARACTERISTIC_00001531_1212_EFDE_1523_785FEABCD123_01_VALUE_HANDLE 0x001e
#define ATT_CHARACTERISTIC_00001531_1212_EFDE_1523_785FEABCD123_01_CLIENT_CONFIGURATION_HANDLE 0x001f
#define ATT_CHARACTERISTIC_00001532_1212_EFDE_1523_785FEABCD123_01_VALUE_HANDLE 0x0021

// DYNAMIC, read handle list 
const uint16_t gatt_read_callback_handle_list[] =
{
    ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE, 
    // END 
    0xffff
}; //0xffff is end

// DYNAMIC, write handle list 
const uint16_t gatt_write_callback_handle_list[] =
{
    ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE, 
    ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE, 
    ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE, 
    ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE, 
    ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, 
    ATT_CHARACTERISTIC_ae06_01_CLIENT_CONFIGURATION_HANDLE, 
    ATT_CHARACTERISTIC_ae07_01_VALUE_HANDLE, 
    ATT_CHARACTERISTIC_ae08_01_CLIENT_CONFIGURATION_HANDLE, 
    // END 
    0xffff
};//0xffff is end



static const u8 profile_adv_ind_data[31] =
{
    0x02, 0x01, 0x06,
    0x04, 0x0d, 0x00, 0x05, 0x10,
    0x03, 0x03, 0x0d, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,

};
//31bytes
//len,type, Manufacturer Specific data
#define GAP_NAME_ADV_SIZE   0x12
static u8 profile_scan_rsp_data[31] =
{
	0x09, 0xff, 'C', 'M', 'D', 'A', 'I', '0', '0',0x00,
	GAP_NAME_ADV_SIZE, 0x09, 'A','I','M', 'a', 't', 'e',
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 
};

//用户可配对的，这是样机跟客户开发的app配对的秘钥
/* const u8 link_key_data[16]={0x06,0x77,0x5f,0x87,0x91,0x8d,0xd4,0x23,0x00,0x5d,0xf1,0xd8,0xcf,0x0c,0x14,0x2b}; */
const u8 link_key_data[16]={0x06,0x77,0x5f,0x87,0x91,0x8d,0xd4,0x23,0x00,0x5d,0xf1,0xd8,0xab,0xcd,0xef,0x11};
extern u8 get_total_connect_dev(void);
extern void bt_prompt_play_by_name(void * file_name, void * let);
void app_server_le_msg_callback(uint16_t msg, uint8_t *buffer, uint16_t buffer_size)
{
    /* printf("\n-%s, msg= 0x%x\n",__FUNCTION__,msg); */
    switch(msg)
    {
    case BTSTACK_LE_MSG_CONNECT_COMPLETE:
        ble_conn_handle = buffer[0]+ (buffer[1]<<8);
        printf("conn_handle= 0x%04x\n",ble_conn_handle);

#if SUPPORT_APP_RCSP_EN
        rcsp_event_com_start(RCSP_APP_TYPE_BLE);
        rcsp_register_comsend(app_data_send);
#endif // SUPPORT_APP_RCSP_EN
		bt_smart_ble_connect_deal();
		
		if(get_total_connect_dev()!=0)
		{
			os_taskq_post(keymsg_task_name, 1, MSG_SMART_CONNECT);
		}

		//extern void fw_update_file_delete(void);
		//fw_update_file_delete();
		break;

    case BTSTACK_LE_MSG_DISCONNECT_COMPLETE:
        printf("disconn_handle= 0x%04x\n",buffer[0]+ (buffer[1]<<8));

        if(ble_conn_handle != 0)
        {
            ble_conn_handle = 0;
#if BLE_APP_UPDATE_SUPPORT_EN
			ble_update_send_handle = 0;

#endif

#if SUPPORT_APP_RCSP_EN
            rcsp_event_com_stop();
#endif // SUPPORT_APP_RCSP_EN

			bt_smart_ble_disconnect_deal();
			
			if(ble_server_close_flag)
			{
				printf("dis adv......\n");
				server_advertise_disable();
				break;
			}

			if(ble_mutex_flag == 0)
            {
                server_advertise_enable();
            }
        }


		if(get_total_connect_dev()!=0)
		{
			/* bt_smart_led_flick(100, 0); */
			/* EyeEffectCtl(EFFECT_NO_CONNECT); */

			os_taskq_post(keymsg_task_name, 1, MSG_SMART_DISCONNECT);
			/* eye_led_api(EFFECT_NO_CONNECT, 0 ,1); */
			/* bt_prompt_play_by_name(BPF_DISCONNECT_MP3,NULL); */
		}

        break;

	case BTSTACK_LE_MSG_CONNECTIONS_CHANGED:

		break;

	case BTSTACK_LE_MSG_INIT_COMPLETE:
        puts("init server_adv\n");
		server_advertise_enable();
		break;

    default:
        break;
    }

    /* puts("exit_le_msg_callback\n"); */
}


#if BLE_APP_UPDATE_SUPPORT_EN
extern void bt_exit_low_power_mode(void);
void ble_send_update_cmd()
{  
    puts("-------ble_update_run\n");
	//call lib func
    bt_exit_low_power_mode();
    os_taskq_post_msg(MAIN_TASK_NAME,2,MSG_UPDATA,BLE_UPDATE);
}  

extern u32 ble_update_save_att_server_info(const u8 *profile_data,u16 profile_len,u16 write_handle,u16 read_handle);
static void ble_update_save_att_server_info_handle(void)
{
	printf("profile_data size:%d\n",sizeof(profile_data));
	u8 ble_update_write_handle = ATT_CHARACTERISTIC_00001532_1212_EFDE_1523_785FEABCD123_01_VALUE_HANDLE; 
	u8 ble_update_notify_handle = ATT_CHARACTERISTIC_00001531_1212_EFDE_1523_785FEABCD123_01_VALUE_HANDLE;	
	ble_update_save_att_server_info(profile_data,sizeof(profile_data),ble_update_write_handle,ble_update_notify_handle);	
}


static void (*ble_update_receive_handler)(u8 *buffer,u16 buffer_size) = NULL;
void ble_update_receive_handler_register(void (*handler)(u8 *buffer,u16 buffer_size))
{
	ble_update_receive_handler = handler;	
}
#endif
// ATT Client Write Callback for Dynamic Data
// @param attribute_handle to be read
// @param buffer
// @param buffer_size
// @return 0
uint16_t app_server_write_callback(uint16_t attribute_handle, uint8_t * buffer, uint16_t buffer_size)
{

    u16 handle = attribute_handle;
    u16 copy_len;

#if 0
    if(buffer_size > 0)
    {
        printf("\n write_cbk: handle= 0x%04x",handle);
        put_buf(buffer,buffer_size);
    }
#endif

    switch (handle)
    {
    case ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE:
        /* printf("\n--write, %d\n",buffer_size); */
        /* put_buf(buffer,buffer_size); */
        break;

    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:

        /* printf("\n--write, %d\n",buffer_size); */
        /* put_buf(buffer,buffer_size); */
		app_data_recieve(buffer,buffer_size);

        break;
	case ATT_CHARACTERISTIC_ae07_01_VALUE_HANDLE:
		if(get_smart_pass() == TRUE)
		{
			bt_smart_user_data_recieve(buffer, buffer_size);
		}
		break;
#if BLE_APP_UPDATE_SUPPORT_EN
	case ATT_CHARACTERISTIC_00001531_1212_EFDE_1523_785FEABCD123_01_CLIENT_CONFIGURATION_HANDLE:
		if(buffer[0] != 0)
		{
			puts("update char notify enable!\n");
 			//ble_send_update_cmd();
			ble_update_send_handle = handle - 1;
#if FOR_ZHIBIN_TEST
			extern bool ble_fw_download_task_create(void);
			ble_fw_download_task_create();
#endif
		}
		else
		{
			ble_update_send_handle = 0;		
		}
		break;

		case ATT_CHARACTERISTIC_00001532_1212_EFDE_1523_785FEABCD123_01_VALUE_HANDLE:
		if(ble_update_receive_handler)
		{
			//puts("upd_rx:\n");
			ble_update_receive_handler(buffer,buffer_size);
		}
		break;
#endif
    default:
        break;
    }

	return 0;
}

//ble send data
static u32 app_data_send(u8 *data,u16 len)
{
    int ret_val;

	ret_val = ble_smart_send(data,len);

    if(ret_val !=  0)
    {
        puts("\n app_ntfy_fail!!!\n");
        return 4;//disconn
    }
    return 0;
}

u32 ble_update_send(u8 *data,u16 len)
{
	int ret_val;
	if(ble_update_send_handle == 0)
	{
	    return 4;// is disconn
	}

	/* puts("send data \n"); */

	/* put_buf(data,len); */
    ret_val = server_notify_indicate_send(ble_update_send_handle,data,len);

	return ret_val;
}

extern u8 get_ble_test_key_flag(void);
void ble_server_send_test_key_num(u8 key_num)
{
	if(get_ble_test_key_flag())
	{
		if(!ble_conn_handle)
			return;		

		ble_user_send_test_key_num(ble_conn_handle,key_num);
	}
}

//ble recieve data
static void app_data_recieve(u8 *data, u16 len)
{
#if SUPPORT_APP_RCSP_EN
   rcsp_comdata_recieve(data,len);
#elif BLE_FINGER_SPINNER_EN
   blefs_comdata_parse(data,len);
#endif // SUPPORT_APP_RCSP_EN

} 

extern void server_select_scan_rsp_data(u8 data_type);
extern void server_set_scan_rsp_data(u8 *data);
extern void server_set_advertise_data(const u8 *data);
extern void app_advertisements_set_params(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
                                   uint8_t direct_address_typ, u8 *direct_address, uint8_t channel_map, uint8_t filter_policy);
extern void server_connection_parameter_update(int enable);
extern void app_connect_set_params(uint16_t interval_min,uint16_t interval_max,uint16_t supervision_timeout);
extern void s_att_server_register_conn_update_complete(void (*handle)(uint16_t min,uint16_t max,uint16_t timeout));

/*
 * @brief Set Advertisement Paramters
 * @param adv_int_min
 * @param adv_int_max
 * @param adv_type
 * @param direct_address_type
 * @param direct_address
 * @param channel_map
 * @param filter_policy
 *
 * @note own_address_type is used from gap_random_address_set_mode
 */
void app_set_adv_parm(void)
{
// setup advertisements
	uint16_t adv_int_min = 0x0080;
	uint16_t adv_int_max = 0x00a0;
	uint8_t adv_type = 0;
	u8 null_addr[6];
	memset(null_addr, 0, 6);

	app_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
}

//连接参数请求，只能修改3个参数
void app_set_connect_param(void)
{
	//l2cap update connection parameters 
 	//Conn_Interval unit:1.25ms   range:0x0006-0x0c80(7.5ms-4s)
	//Supervision_timeout: unit:10ms range:0x000A-0x0c80(100ms-32s)
	//注:不同手机,不同系统对连接参数有不同的要求，选择连接参数需考虑兼容性
	//ios conn parameters request:
	//- Interval_Min >= 20ms
	//- Interval_Min + 20ms <= Interval_Max 
	//- Interval_Max*(Slave Latency+1) <= 2s
	//- connsupervisionTimeout <= 6s
	//- Slave Latency <= 4
	//- Interval Max*(Slave Latency+1)*3 < connsupervisiontimeout, 这里Slave Latency = 0;
	uint16_t conn_interval_min = 24;//1.25ms *此值
	uint16_t conn_interval_max = 24 + 16;//1.25ms *此值
	uint16_t conn_supervision_timeout = 550;

	app_connect_set_params(conn_interval_min,conn_interval_max,conn_supervision_timeout);
}

extern int server_advertise_set_interval(u16 conn_handle,u16 interval_min,u16 interval_max,u16 timeout);
void ble_update_send_conn_param_update(void)
{
	u16 conn_interval_min = 0x08;
	u16 conn_interval_max = 0x10;

	u16 supervision_timeout = conn_interval_max * 10;

	server_advertise_set_interval(ble_conn_handle,conn_interval_min,conn_interval_max,supervision_timeout);	
}

void app_server_conn_update_callback(uint16_t min,uint16_t max,uint16_t timeout)
{

	printf("app_min = %d\n",min);
	printf("app_max = %d\n",max);
	printf("timeout= %d\n",timeout);

}

void app_server_init(void)
{
    printf("\n%s\n",__FUNCTION__);
    server_register_profile_data(profile_data);
    server_set_advertise_data(profile_adv_ind_data);
	server_register_app_callbak(app_server_le_msg_callback,NULL,app_server_write_callback);
	server_select_scan_rsp_data(1);     //scan_rsp类型选择，0：默认，1：会自动填写ble名字，其他自由填写，2：全部内容自由填写
	server_set_scan_rsp_data(profile_scan_rsp_data);//注册scan_rsp包内容
	app_set_adv_parm();//注册广播参数
	server_connection_parameter_update(1);//连接参数使能，0：不修改连接参数，1：修改连接参数
	app_set_connect_param();//注册连接参数
	s_att_server_register_conn_update_complete(app_server_conn_update_callback);//注册连接参数请求回调函数
	ble_smart_link_key_reg(link_key_data);//用户配置样机配对link_key
	ble_mutex_flag = 0;

	ble_smart_init();
#if BLE_APP_UPDATE_SUPPORT_EN
	extern void ble_update_save_att_server_info_handle_register(void (*handle)(void));
	ble_update_save_att_server_info_handle_register(ble_update_save_att_server_info_handle);	
#endif
}

/*
spp 和 ble 互斥连接
1、当spp 连接后，ble变为不可连接
2、当ble连上后，若有spp连接上，则断开ble；ble变为不可连接
 */
void ble_enter_mutex(void)
{
	P_FUNCTION


    if(ble_mutex_flag == 1)
	{
		return;
	}

	puts("in enter mutex\n");
    ble_mutex_flag = 1;
    if(ble_conn_handle != 0)
    {
        ble_hci_disconnect(ble_conn_handle);
        rcsp_event_com_stop();
    }
    else
    {
        server_advertise_disable();
    }
    puts("exit_enter_mu\n");
}

void ble_exit_mutex(void)
{
    P_FUNCTION

	if(ble_mutex_flag == 0)
	{
		return;
	}
   
	puts("in_exit mutex\n");
	ble_mutex_flag = 0;
   	server_advertise_enable();
   	puts("exit_exit_mu\n");
}

void ble_server_disconnect(void)
{
    P_FUNCTION
    if(ble_conn_handle != 0)
    {
        printf("server discon handle= 0x%x\n ",ble_conn_handle); 
		ble_hci_disconnect(ble_conn_handle);
    }
    puts("exit_discnt\n");
}


tbool ble_check_connect_complete(void)
{
	if(ble_conn_handle != 0)		
		return true;
	else
		return false;
}

void ble_server_close(void)
{
	if(ble_conn_handle)
	{
		ble_server_close_flag =  1;
		ble_hci_disconnect(ble_conn_handle);
	}
	else
	{
		server_advertise_disable();
	}
}

void ble_server_set_connection_interval(u16 in_min, u16 in_max, u16 timeout)
{
	if(ble_conn_handle)
	{
		gap_set_ble_connection_interval(ble_conn_handle, in_min, in_max, timeout);	
	}
}

#endif //#if (BLE_BREDR_MODE&BT_BLE_EN)
