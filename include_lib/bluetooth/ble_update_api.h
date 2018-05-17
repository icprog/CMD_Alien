#ifndef _BLE_UPDATE_API_H_
#define _BLE_UPDATE_API_H_

#include "typedef.h"

typedef struct _BLE_UPDATE_DEV_OP
{
	s16 (*dev_open)(void);
	u16 (*dev_read)(u8 *buf,u32 len);	
	void (*dev_seek)(u8 seek_type,u32 offset);
	u16 (*dev_write)(u8 *buf,u32 len);
	void (*dev_file_close)(void);
	void (*dev_close)(void);
}BLE_UPDATE_DEV_OP;

//lib api
extern void ble_update_send_handler_register(u32 (*handler)(u8 *buffer,u16 buffer_size));
extern void ble_update_receive(u8 *data,u16 len);
extern void ble_update_file_ops_register(BLE_UPDATE_DEV_OP *ops);
extern void ble_update_process_resume_register(void (*handler)(void));
extern u32 ble_update_cmd_process(void);
extern void ble_update_process_ctl_register(void (*resume_handler)(void),void (*suspend_handler)(void));

enum
{
	BLE_UPDATE_PROCESS_SUCC = 0,
	BLE_UPDATE_PROCESS_FAIL,	
};

#endif


