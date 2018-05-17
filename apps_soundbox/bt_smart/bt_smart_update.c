#include "sdk_cfg.h"
#include "common/app_cfg.h"
#include "file_operate/file_op.h"
#include "fat/fs_io.h"
#include "fat/tff.h"
#include "bluetooth/ble_update_api.h"
#include "rtos/os_api.h"
#include "dev_manage/drv_manage.h"
#include "dev_manage/dev_ctl.h"

#define BLE_UPDATE_DEBUG_EN
#ifdef BLE_UPDATE_DEBUG_EN
#define ble_update_printf	printf
#define ble_update_puts		puts
#define ble_update_put_buf	printf_buf
#define ble_update_putchar	putchar
#else
#define ble_update_printf(...)
#define ble_update_puts(...)
#define ble_update_put_buf(...)
#define ble_update_putchar(...)
#endif

//#define AT_BLE_UPDATE_CODE
#if BLE_APP_UPDATE_SUPPORT_EN

extern void ble_update_receive_handler_register(void (*handler)(u8 *buffer,u16 buffer_size));
extern u32 ble_update_send(u8 *data,u16 len);


static FILE_OPERATE *f_op = NULL;
static s16 fw_update_dev_open(void)
{	
	s16 ret;

	f_op = file_opr_api_create((u32)sd0);		

	if(NULL == f_op)
	{
		ble_update_puts("create file opr err\n");
		return -1;	
	}

	ret = fs_open(f_op->cur_lgdev_info->lg_hdl->fs_hdl,
				&(f_op->cur_lgdev_info->lg_hdl->file_hdl),
				"/jl_690x.bfu",
				NULL,
				FA_CREATE_NEW|FA_WRITE
			);

	if(ret == FR_EXIST)
	{
		fs_delete(f_op->cur_lgdev_info->lg_hdl->file_hdl);		

		ret = fs_open(f_op->cur_lgdev_info->lg_hdl->fs_hdl,
				&(f_op->cur_lgdev_info->lg_hdl->file_hdl),
				"/jl_690x.bfu",
				NULL,
				FA_CREATE_NEW|FA_WRITE
				);

		if(ret != FR_OK && ret != FR_EXIST)
		{
			ble_update_printf("fs create file err = %x\n",ret);		
			file_opr_api_close(f_op);
			f_op = NULL;
			return -1;
		}
	}

	return 0;

}

void fw_update_file_delete(void)
{
	u32 ret;
	f_op = file_opr_api_create((u32)sd0);		

	if(NULL == f_op)
	{
		ble_update_puts("create file opr err\n");
		return;	
	}

	ret = fs_open(f_op->cur_lgdev_info->lg_hdl->fs_hdl,
				&(f_op->cur_lgdev_info->lg_hdl->file_hdl),
				"/jl_690x.bfu",
				NULL,
				FA_OPEN_EXISTING
			);

	if(ret != FR_OK && ret != FR_EXIST)
	{
		ble_update_printf("fs create file err = %x\n",ret);		
		file_opr_api_close(f_op);
		f_op = NULL;
	}

	ret = fs_delete(f_op->cur_lgdev_info->lg_hdl->file_hdl);
	{
		ble_update_printf("delete ret:%x\n",ret);		
	}

	fs_close(f_op->cur_lgdev_info->lg_hdl->file_hdl);
	file_opr_api_close(f_op);
}

static u16 fw_update_dev_read(u8 *buf,u32 len)
{
	if(NULL == f_op)
	{
		return 0;		
	}

	u16 read_bytes;
	read_bytes = fs_read(f_op->cur_lgdev_info->lg_hdl->file_hdl,buf,len);	

	return read_bytes;
}
/* #define DEBUG_PORT	JL_PORTC */
/* #define DEBUG_IO	BIT(6) */
static u16 fw_update_dev_write(u8 *buf,u32 len)
{
	if(NULL == f_op)	
		return (u16)-1;

	/* ble_update_putchar('-'); */
	/* DEBUG_PORT->DIR &= ~DEBUG_IO; */
	/* DEBUG_PORT->OUT |= DEBUG_IO; */

	u32 bw = fs_write(f_op->cur_lgdev_info->lg_hdl->file_hdl,buf,len);

	/* DEBUG_PORT->OUT	&= ~DEBUG_IO; */
	if(len != bw)
	{
		ble_update_printf("bw:%x\n",bw);
		ble_update_puts("fs write err\n");			
		return (u16)-1;
	}

	return 0;
}

static void fw_update_dev_seek(u8 seek_type,u32 offset)
{
	if(NULL == f_op)
	{		
		return;		
	}

	fs_seek(f_op->cur_lgdev_info->lg_hdl->file_hdl,seek_type,offset);
}

static void fw_update_dev_close(void)
{
	if(NULL == f_op)
	{		
		return;		
	}

	/* fs_close(f_op->cur_lgdev_info->lg_hdl->file_hdl); */
	file_opr_api_close(f_op);
	f_op = NULL;
}

static void fw_update_dev_file_close(void)
{
	if(NULL == f_op)
	{		
		return;		
	}

	fs_close(f_op->cur_lgdev_info->lg_hdl->file_hdl);
}

static const BLE_UPDATE_DEV_OP ble_update_file_op = 
{
	.dev_open = fw_update_dev_open,		
	.dev_read = fw_update_dev_read,
	.dev_seek = fw_update_dev_seek,
	.dev_write = fw_update_dev_write,
	.dev_file_close = fw_update_dev_file_close,
	.dev_close = fw_update_dev_close
};

static OS_SEM ble_update_sem;
static void ble_update_process_resume(void)
{
	//putchar('S');
	os_sem_post(&ble_update_sem);		
}

static void ble_update_process_suspend(void)
{
	os_sem_pend(&ble_update_sem,0);
	//putchar('P');
	if (os_task_del_req_name(OS_TASK_SELF) == OS_TASK_DEL_REQ)
	{
		ble_update_puts("fw_download_task:DEL_TASK\n");
		os_task_del_res_name(OS_TASK_SELF);
	}
}

static u8 ble_update_ok = 0;
extern void ble_update_send_conn_param_update(void);
extern void ble_server_disconnect(void);


u8 ble_update_get_status(void)
{
	return ble_update_ok;		
}

static void ble_fw_download_task(void *param) 
{
	int msg[3];

	ble_update_file_ops_register((BLE_UPDATE_DEV_OP *)(&ble_update_file_op));
	ble_update_send_handler_register(ble_update_send);
	ble_update_receive_handler_register(ble_update_receive);
	/* ble_update_send_conn_param_update(); */

	u32 ret;
	while(1)
	{
		ret = ble_update_cmd_process();
		
		ble_server_disconnect();			
		if(!ret)
		{
			ble_update_ok = 1;
			os_taskq_post_event("MainTask",3,MSG_UPDATA,DEV_UPDATA, (u32)sd0);
		}
		else
		{
			/* fw_update_file_delete(); */
		}

		while(1)
		{
			CLR_WDT();
			os_time_dly(100);
			/* ble_update_process_suspend(); */
		}
	}
}

#define FW_DOWNLOAD_TASK_NAME	"fw_download_task_name"

bool ble_fw_download_task_create(void)
{	
    u32 err;
	printf("\n--func=%s, line = %d\n", __FUNCTION__, __LINE__);
    err = os_task_create_ext(ble_fw_download_task,
                         NULL,
                         TaskBleUpatePrio,
                         10
#if OS_TIME_SLICE_EN > 0
                         , 1
#endif
                         ,FW_DOWNLOAD_TASK_NAME,
						 2*1024);

    if (err) {
        ble_update_printf("ble_fw_download_task task err:%x\n",err);
        return false;
    }

	os_sem_create(&ble_update_sem,0);	
	ble_update_process_ctl_register(ble_update_process_resume,ble_update_process_suspend);
	
	return true;
}

void ble_fw_download_task_del(void)
{
	u32 err;
	

	if (os_task_del_req(FW_DOWNLOAD_TASK_NAME) != OS_TASK_NOT_EXIST)
    {
        //os_taskq_post_event(FW_DOWNLOAD_TASK_NAME, 1, SYS_EVENT_DEL_TASK);
		ble_update_process_resume();

        do
        {
            OSTimeDly(1);
        }
        while (os_task_del_req(FW_DOWNLOAD_TASK_NAME) != OS_TASK_NOT_EXIST);
        ble_update_puts("fw_download_task:close task succ\n");
    }
}

#endif

