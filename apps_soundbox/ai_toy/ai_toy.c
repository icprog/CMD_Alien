#include "ai_toy.h"
#include "common/app_cfg.h"
#include "rtos/os_api.h"
#include "rtos/os_cfg.h"
#include "common/error.h"
#include "stdarg.h"
#include "music.h"
#include "common/msg.h"
#include "rtos/task_manage.h"
#include "music_key.h"
#include "dec/music_api.h"
#include "vm/vm_api.h"
#include "play_sel/play_sel.h"
#include "dac/dac_api.h"
#include "dev_manage/drv_manage.h"
#include "ui/ui_api.h"
#include "ui/led/led.h"
#include "key_drv/key.h"
#include "music_prompt.h"
#include "music_info.h"
#include "music_ui.h"
#include "fat/tff.h"
#include "fat/fs_io.h"
#include "common/common.h"
#include "lyrics_api.h"
#include "dac/eq_api.h"
#include "dac/dac_api.h"
#include "dac/dac.h"
#include "dac/eq.h"
#include "clock_interface.h"
#include "clock.h"
#include "dec/decoder_phy.h"
#include "vm/vm.h"
#include "rcsp/rcsp_interface.h"
#include "rcsp/rcsp_head.h"
#include "speed_pitch_api.h"
#include "bt_smart.h"
#include "dev_manage/dev_ctl.h"
#include "ui/led/led_eye.h"
#include "update.h"
#include "key_drv/key_voice.h"

/* #define AI_TOY_DEBUG_EN */
#ifdef AI_TOY_DEBUG_EN
#define AI_toy_printf	printf
#define AI_toy_printf_buf	printf_buf
#else
#define AI_toy_printf(...)
#define AI_toy_printf_buf(...)
#endif//AI_TOY_DEBUG_EN

u8	AI_toy_dir_mode = AI_TOY_DIR_MODE_UNACTIVE;
extern u8 g_alarm_back;

static const char *creat_dir_list[AI_TOY_DIR_MODE_MAX] = 
{
	"/children",		
	"/story",		
	"/english",		
	"/chinese",		
	"/music",		
	"/instrum",		
	"/sleep",		
};

static const char *dir_list[AI_TOY_DIR_MODE_MAX] = 
{
	"/children/",		
	"/story/",		
	"/english/",		
	"/chinese/",		
	"/music/",		
	"/instrum/",		
	"/sleep/",		
};


static const char *notice_dir_mode_path_list[AI_TOY_DIR_MODE_MAX] = 
{
	AI_TOY_NOTICE_CHILD,	
	AI_TOY_NOTICE_STROY,	
	AI_TOY_NOTICE_ENLISH,	
	AI_TOY_NOTICE_CHINESE,	
	AI_TOY_NOTICE_MUSIC,	
	AI_TOY_NOTICE_INSTUM,	
	AI_TOY_NOTICE_SLEEP,	
};



extern u32 file_operate_ini(void *parm,u32 *chg_flag);
extern u32 lg_dev_mount(void *parm,u8 first_let,u32 total_lgdev);
extern void lg_dev_close(void *parm);
extern void music_stop_decoder(MUSIC_OP_API *parm);
extern u32 music_play_api(MUSIC_OP_API *m_op_api,ENUM_DEV_SEL_MODE dev_sel,u32 dev_let,u32 file_sel,void *pra);
extern int music_decoder_init(MUSIC_OP_API **mapi_out);


extern void debug_loop_err_clear(void);

extern MUSIC_OP_API *music_operate_mapi;
extern void *music_reverb;

static u8 auto_next = 1; 
static _DECODE_STATUS AI_toy_pp_st = DECODER_STOP;
static volatile u8 ai_finish_op = 0;
static u32 g_cur_file_num = 0;


enum{
	CMD_PLAY_FILE_BY_CLUST = 0,
	CMD_UPDATE_DEV_STATUS = 1,
	CMD_AI_ACTIVE_CHANGE_MODE = 2,
	CMD_AI_FINISH = 3,
	CMD_AI_NET_RESOURCE_CHANGE = 4,
	CMD_BT_DISCONNECT = 5,

};

///ai 识别处理完成跳回音乐播放处理
enum{
	AI_FINISH_OP_REVERT = 0x1,
	AI_FINISH_OP_PREV,
	AI_FINISH_OP_NEXT,
	AI_FINISH_OP_PAUSE,
	AI_FINISH_OP_PLAY,
};

static void AI_toy_send_data(u8 *buf, u8 len)
{
	rcsp_smart_command_send(CBW_CMD_AI_TOY_CMD, buf, len);
}

void AI_toy_update_dev_status(u8 is_online)
{
	u8 send_buf[3];
	WRITE_BIG_U16(send_buf, CMD_UPDATE_DEV_STATUS);	
	send_buf[2] = is_online;
	AI_toy_send_data(send_buf, sizeof(send_buf));
}

void ai_toy_change_net_resource(u8 type, u8 period)
{
	u8 send_buf[4];
	WRITE_BIG_U16(send_buf, CMD_AI_NET_RESOURCE_CHANGE);	
	send_buf[2] = type;
	send_buf[3] = period;
	AI_toy_send_data(send_buf, sizeof(send_buf));
}

void AI_toy_update_ai_mode(u8 mode)
{
	rcsp_smart_command_send(CBW_CMD_SPEEX_SEND_START, &mode, 1);
}


void AI_toy_active_ai_ok(void)
{
	u8 send_buf[2];
	WRITE_BIG_U16(send_buf, CMD_AI_ACTIVE_CHANGE_MODE);	
	AI_toy_send_data(send_buf, sizeof(send_buf));
}

void AI_toy_update_disconnect_ble(void)
{
	u8 send_buf[3];
	WRITE_BIG_U16(send_buf, CMD_BT_DISCONNECT);	
	send_buf[2] = 0;
	AI_toy_send_data(send_buf, sizeof(send_buf));
}

#include "rtc/rtc_api.h"
static u8 port3_cur_status = 0;
void AI_toy_led_init(void)
{
	PORTR_PU(PORTR3,0);
	PORTR_PD(PORTR3,0);
	PORTR_DIR(PORTR3,0);
	PORTR_DIE(PORTR3,1);
	port3_cur_status = 0;
	PORTR_OUT(PORTR3,port3_cur_status);
}

void AI_toy_led_set_status(u8 status)
{
	port3_cur_status = status;
	PORTR_OUT(PORTR3,port3_cur_status);
}

void AI_toy_led_set_swap(u8 status)
{
	port3_cur_status = !port3_cur_status;
	PORTR_OUT(PORTR3,port3_cur_status);
}

void AI_toy_creat_dir(FILE_OPERATE *fop)
{
	u32 i;
	for(i=0; i<(sizeof(creat_dir_list)/sizeof(creat_dir_list[0])); i++)
	{
		AI_toy_printf("%s\n", creat_dir_list[i]);
		fs_mk_dir(fop->cur_lgdev_info->lg_hdl->fs_hdl, (char *)creat_dir_list[i], FA_CREATE_NEW);		
	}
	AI_toy_printf("creat dir ok!!\n");

}

void AI_toy_del_update_file(FILE_OPERATE *fop)
{
	if(fop)	
	{
		s16 ret = fs_open(fop->cur_lgdev_info->lg_hdl->fs_hdl,
							&(fop->cur_lgdev_info->lg_hdl->file_hdl),
							"/jl_690x.bfu",
							NULL,
							FA_OPEN_EXISTING
							);	
		if(ret == FR_EXIST || ret == FR_OK)
		{
			ret = fs_delete(fop->cur_lgdev_info->lg_hdl->file_hdl);		
			if(!ret)
			{
				AI_toy_printf("delete update file ok\n");		
			}
			else
			{
				AI_toy_printf("delete update file file\n");		
			}
		}
		else
		{
			AI_toy_printf("no update file delect\n");		
		}
	}
}

static FILE_OPERATE *AI_toy_fop = NULL;
void AI_toy_dev_online(DEV_HANDLE dev)
{
	u32 phy_dev_chg;
	u32 err;
	AI_toy_fop = calloc(1, sizeof(FILE_OPERATE));
	ASSERT(AI_toy_fop);
	AI_toy_fop->fop_init = calloc(1, sizeof(FILE_OPERATE_INIT));
	ASSERT(AI_toy_fop->fop_init);
	AI_toy_fop->fop_init->cur_dev_mode = DEV_SEL_SPEC;
	AI_toy_fop->fop_init->dev_let = (u32)dev;

	err = file_operate_ini(AI_toy_fop, &phy_dev_chg);
	if(err == FILE_OP_NO_ERR)
	{
		if(AI_toy_fop->cur_lgdev_info->fat_type == 0)
		{
			err = lg_dev_mount(AI_toy_fop->cur_lgdev_info->lg_hdl->phydev_item, AI_toy_fop->cur_lgdev_info->dev_let, 1);	
			if(err == FILE_OP_NO_ERR)
			{
				err = file_operate_ini(AI_toy_fop, &phy_dev_chg);
				if(err == FILE_OP_NO_ERR)
				{
					if(update_poweron_check() == 0)
					{
						AI_toy_creat_dir(AI_toy_fop);
						rcsp_set_music_bs_op_api(AI_toy_fop);
					}
					else
					{
						AI_toy_del_update_file(AI_toy_fop);
					}
				}
			}
		}
	}
	AI_toy_update_dev_status(1);
	AI_toy_printf(" bs fs_hdl = %x, func = %s, line = %d\n", AI_toy_fop->cur_lgdev_info->lg_hdl->fs_hdl, __FUNCTION__, __LINE__);
;
}

void AI_toy_dev_offline(DEV_HANDLE dev)
{
	if(AI_toy_fop)
	{
		rcsp_set_music_bs_op_api(NULL);
		lg_dev_close(AI_toy_fop->cur_lgdev_info);
		free(AI_toy_fop->fop_init);
		free(AI_toy_fop);
		AI_toy_fop = NULL;
		malloc_stats();
	}

	AI_toy_update_dev_status(0);
}



typedef struct __SPEC_FILE_INFO
{
	u32 clust;
	u8 	flag;		
	u16 dir_index;
}SPEC_FILE_INFO;

static SPEC_FILE_INFO spec_file = 
{
	.flag = 0,		
};

extern char *keymsg_task_name;
u32 AI_toy_public_cmd_parse(u8 *operation,u8 *data,u16 len)
{
	u32 err = RCSP_OP_ERR_NONE;
	u16 cmd = READ_BIG_U16(&operation[0]);
	AI_toy_printf("AI_toy cmd = %x\n", cmd);
	switch(cmd)		
	{
		case CMD_PLAY_FILE_BY_CLUST:
			if(AI_toy_fop != NULL)
			{
				u32 status=file_operate_ctl(FOP_DEV_STATUS,AI_toy_fop,0,0);
				if(status == 1)
				{
					if(compare_task_name(MUSIC_TASK_NAME))
					{
						os_taskq_post(MUSIC_TASK_NAME, 3, MSG_MUSIC_SPC_FILE, READ_BIG_U32(&operation[2]), READ_BIG_U16(&operation[6]));
					}
					else
					{
						spec_file.flag = 1;
						spec_file.clust = READ_BIG_U32(&operation[2]);
						spec_file.dir_index = READ_BIG_U16(&operation[6]);
						os_taskq_post(MAIN_TASK_NAME, 2, SYS_EVENT_TASK_RUN_REQ, MUSIC_TASK_NAME);
					}
				}
				else
				{
					return CSW_ERR_OPT;
				}
			}
			else
			{
				return CSW_ERR_OPT;
			}
			break;		

		case CMD_AI_ACTIVE_CHANGE_MODE:
			AI_toy_printf("app active ai !!!\n");
			os_taskq_post(keymsg_task_name, 1, MSG_AI_ACTIVE);
			break;

		case CMD_AI_FINISH:
			ai_finish_op = operation[2];
			if(ai_finish_op)
			{
				if(compare_task_name(BTSTACK_TASK_NAME))
				{
					os_taskq_post(MAIN_TASK_NAME, 1, MSG_LAST_WORKMOD);
				}
			}
			break;
		default:
			break;
	}
	return err;
}


static tbool AI_toy_notice_play_callback(void *priv, int msg[])
{
	u8 data[2];
	MUSIC_OP_API *mapi = priv;
	switch(msg[0])		
	{
		case SYS_EVENT_DEV_ONLINE:
			AI_toy_printf("--music SYS_EVENT_DEV_ONLINE\r");
			return true;

		case SYS_EVENT_DEV_OFFLINE:
			AI_toy_printf("--music SYS_EVENT_DEV_OFFLINE\r");
			return true;

		case SYS_EVENT_DEC_DEVICE_ERR:
			AI_toy_printf("--music SYS_EVENT_DEC_DEVICE_ERR\r");
			return true;
		default:
			break;		
	}
	return false;
}


static tbool AI_toy_tf_mode_notice_play_callback(void *priv, int msg[])
{
	u8 data[2];
	MUSIC_OP_API *mapi = priv;
	switch(msg[0])		
	{
		default:
			break;		
	}
	return false;
}


static u32 AI_toy_music_play(MUSIC_OP_API *m_op_api,ENUM_DEV_SEL_MODE dev_sel,u32 dev_let,u32 file_sel,void *pra)
{
	u32 err = music_play_api(m_op_api, dev_sel, dev_let, file_sel, pra);	

	if(err == SUCC_MUSIC_START_DEC)
	{
		/* EyeEffectCtl(EFFECTSOUNDCOLUMNSLOW); */
		eye_led_api(EFFECT_PLAY, 0 ,0);
		AI_toy_pp_st = DECODER_PLAY;
		/* dac_channel_on(MUSIC_CHANNEL, FADE_ON); */
	}

	return err;
}

static void AI_toy_music_stop(MUSIC_OP_API *m_op_api)
{
	music_stop_decoder(m_op_api);		
	AI_toy_pp_st = DECODER_STOP;
}

static _DECODE_STATUS AI_toy_music_pp(MUSIC_OP_API *mapi)
{
	NOTICE_PLAYER_ERR n_err;
	u32 err;
	if(AI_toy_pp_st == DECODER_PLAY)		
	{
		/* EyeEffectCtl(EFFECT_PAUSE); */
		eye_led_api(EFFECT_PAUSE, 0 ,0);
		mapi->dop_api->dec_api.save_brk = 1;
		music_stop_decoder(mapi);
		mapi->dop_api->dec_api.save_brk = 0;
		n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
				(char *)AI_TOY_NOTICE_PAUSE, 
				AI_toy_notice_play_callback,
				mapi);

		AI_toy_pp_st = DECODER_PAUSE;
		/* pa_mute(); */
		/* dac_channel_on(MUSIC_CHANNEL, FADE_ON); */
	}
	else if(AI_toy_pp_st == DECODER_PAUSE)
	{
		n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
				(char *)AI_TOY_NOTICE_PLAY, 
				AI_toy_notice_play_callback,
				mapi);

		if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
		{

		}
		else
		{
			err = music_play_api(mapi,DEV_SEL_SPEC,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
			if(err != SUCC_MUSIC_START_DEC)
			{
				AI_toy_printf("pp play fail\n");		
			}
			else
			{
				AI_toy_pp_st = DECODER_PLAY;
				/* EyeEffectCtl(EFFECTSOUNDCOLUMNSLOW); */
				eye_led_api(EFFECT_PLAY, 0 ,0);
			}
		}

		/* dac_channel_on(MUSIC_CHANNEL, FADE_ON); */
	}
	else
	{
		AI_toy_printf("status err in line = %d\n", __LINE__);		
	}
	return AI_toy_pp_st;
}


void AI_toy_music_notice_play(MUSIC_OP_API *mapi, char *path)
{
	NOTICE_PLAYER_ERR n_err;
	u8 revert = 0;
	if(AI_toy_pp_st == DECODER_PLAY)
	{
		mapi->dop_api->dec_api.save_brk = 1;
		music_stop_decoder(mapi);
		mapi->dop_api->dec_api.save_brk = 0;
		revert = 1;
	}
	

	n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
			path, 
			AI_toy_notice_play_callback,
			mapi);

	if(revert == 0)
		return ;

	if(NOTICE_PLAYER_TASK_DEL_REQ_ERR != n_err)
	{
		u32 err;
		err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
		if(err != SUCC_MUSIC_START_DEC)
		{
			AI_toy_printf("err fun = %s, line = %d\n", __FUNCTION__, __LINE__);		
		}
		else
		{
		}
		/* dac_channel_on(MUSIC_CHANNEL, FADE_ON); */
	}
}


static u32 AI_toy_music_play_by_sclust(MUSIC_OP_API *mapi,u32 sclust)
{
    u32 err;
    mapi->fop_api->fop_init->filesclust = sclust;
	mapi->fop_api->fop_init->cur_play_mode = REPEAT_FOLDER;
    /* mapi->fop_api->fop_init->dev_let = (u32)sd0; */
    err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,FOP_OPEN_FILE_BYSCLUCT,&(mapi->dop_api->file_num));
//    AI_toy_printf("mapi->dop_api->file_num = %d\n",mapi->dop_api->file_num);
    return err;
}

static u32  AI_toy_music_play_by_path(MUSIC_OP_API *mapi, char *path, u32 file_num)
{
    u32 err;
	if(mapi == NULL)
		return ERR_MUSIC_API_NULL;
//    u8 *path;
    /* const char path[]="/folder1/abc.mp3"; */
//    path="/音乐/";
	mapi->fop_api->fop_init->cur_play_mode = REPEAT_FOLDER;
	mapi->fop_api->fop_init->filepath = (u8*)path;
	mapi->dop_api->file_num = file_num;
//    AI_toy_printf("file path : %s  \n",path);
    err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,PLAY_FILE_BYPATH,&(mapi->dop_api->file_num));
    return err;
}

/*
 * 目录模式切换处理
 */

static u32 AI_toy_change_dir_mode(MUSIC_OP_API *mapi, u8 mode)
{
#if 0
	NOTICE_PLAYER_ERR n_err;
	AI_toy_music_stop(mapi);
	if(mode == 0)
	{
		AI_toy_dir_mode ++;
		if(AI_toy_dir_mode >= AI_TOY_DIR_MODE_MAX)
		{
			AI_toy_dir_mode = 0;
		}
		///play notice
		n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
				(char *)notice_dir_mode_path_list[AI_toy_dir_mode], 
				AI_toy_notice_play_callback,
				mapi);
		if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
		{
			return 0;		
		}
	}
	AI_toy_printf("%s----->\n", dir_list[AI_toy_dir_mode]);

	u32 err = AI_toy_music_play_by_path(mapi, (char *)dir_list[AI_toy_dir_mode], 1);
	if(err == SUCC_MUSIC_START_DEC)
	{
		AI_toy_printf("dir play ok!!\n");		
	}
	else
	{
		AI_toy_printf("dir play fail err = %d!!\n", err);		
		if((FILE_OP_ERR_NUM == err) || (FILE_OP_ERR_NO_FILE_ONEDEV == err))
		{
			AI_toy_printf("no file this folder\n");	
			err = 0;
			n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
					(char *)AI_TOY_NOTICE_NOFILE, 
					AI_toy_notice_play_callback,
					mapi);

			if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
			{
				return 0;		
			}
		}
	}

	return err;

#else
	u8 i;
	u8 find = 0;
	u32 err = 0;
	NOTICE_PLAYER_ERR n_err;
	AI_toy_music_stop(mapi);
	FILE_OPERATE *fop = NULL;
	u32 file_num = 1;

	fop = file_opr_api_create((u32)sd0);
	if(fop)
	{
		if(mode == 0)
		{
			AI_toy_dir_mode ++;
		}

		for(i=0; i<AI_TOY_DIR_MODE_MAX; i++)
		{

			if(AI_toy_dir_mode >= AI_TOY_DIR_MODE_MAX)
			{
				AI_toy_dir_mode = 0;
			}

			fop->fop_init->filepath = (u8 *)dir_list[AI_toy_dir_mode];
			err=file_operate_ctl(PLAY_FILE_BYPATH, fop, &file_num, NULL);
			if(err!=FILE_OP_NO_ERR)
			{
				AI_toy_dir_mode ++;
				AI_toy_printf("%s has no file !!!, find next !!\n", dir_list[AI_toy_dir_mode]);
				continue;		
			}
			AI_toy_printf("find play dir file !!~~~~~\n");
			find = 1;
			break;
			/* _FIL_HDL *p1 = mapi->fop_api->cur_lgdev_info->lg_hdl->file_hdl; */
			/* FIL *fp = p1->hdl; */
			/* cur_dev_fop->cur_lgdev_info->total_file_num = fp->fs_msg.file_total_indir; */
		}
	}
	file_opr_api_close(fop);

	if(find)
	{
		///play notice
		n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
				(char *)notice_dir_mode_path_list[AI_toy_dir_mode], 
				AI_toy_notice_play_callback,
				mapi);
		if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
		{
			return 0;		
		}

		err = AI_toy_music_play_by_path(mapi, (char *)dir_list[AI_toy_dir_mode], 1);
		if(err == SUCC_MUSIC_START_DEC)
		{
			AI_toy_printf("dir play ok!!\n");		
		}
		else
		{
			AI_toy_printf("dir play fail err = %d!!\n", err);		
			if((FILE_OP_ERR_NUM == err) || (FILE_OP_ERR_NO_FILE_ONEDEV == err))
			{
				AI_toy_printf("no file this folder\n");	
				err = 0;
			}
		}
	}
	else
	{
		AI_toy_printf("no file all folder\n");
		err = 0;
		n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
				(char *)AI_TOY_NOTICE_NOFILE, 
				AI_toy_notice_play_callback,
				mapi);

		if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
		{
			return 0;		
		}
	}


	return err;
#endif
}


static void AI_toy_before_decoder_callback(MUSIC_OP_API *m_op_api,u32 dev_let)
{
	AI_toy_printf("%s\n", __FUNCTION__);		
	vm_check_all(0);
}

static void AI_toy_after_decoder_callback(MUSIC_OP_API *m_op_api,u32 dev_let)
{
	if(0 == strcmp(m_op_api->dop_api->dec_api.phy_ops->dec_ops->name,"FLA"))//解码器FLAC
	{
		set_sys_freq(FLAC_SYS_Hz);
	}
	else if(0 == strcmp(m_op_api->dop_api->dec_api.phy_ops->dec_ops->name,"APE"))//解码器APE
	{
		set_sys_freq(APE_SYS_Hz);
	}
	else
	{
		set_sys_freq(SYS_Hz);
	}
}
static void AI_toy_music_play_err_deal(MUSIC_OP_API *mapi, u32 *err) 
{
	u32 totalerrfile=0;
	while(0 != (*err))
	{
		dec_phy_printf("--music err  : %02x\n",(*err));
		u32 status;
		switch((*err))
		{
			case FILE_OP_ERR_INIT:              ///<文件选择器初始化错误
			case FILE_OP_ERR_OP_HDL:            ///<文件选择器指针错误
			case FILE_OP_ERR_LGDEV_NULL:        ///<没有设备
			case FILE_OP_ERR_NO_FILE_ALLDEV:    ///<没有文件（所有设备）
				file_operate_ctl(FOP_CLOSE_LOGDEV,mapi->fop_api,0,0);
//				wait_exit=1;            ///<进入等待退出状态
//				wait_ok = 1;
//				if(OS_NO_ERR == os_taskq_post(MAIN_TASK_NAME, 1, MSG_CHANGE_WORKMODE))
//				{
//					wait_ok = 0;
//				}
				(*err) = 0;
				break;
			case FILE_OP_ERR_OPEN_BPFILE:
				dec_phy_printf("err  : %02x\r",(*err));
				mapi->dop_api->file_num = 1;
				(*err) = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_FIRST_FILE,&(mapi->dop_api->file_num));
				break;
			case FILE_OP_ERR_NUM:               ///<选择文件的序号出错
				mapi->dop_api->file_num = 1;
				(*err) = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_SPEC_FILE,&(mapi->dop_api->file_num));
				break;
			case FILE_OP_ERR_LGDEV_MOUNT:
			case FILE_OP_ERR_NO_FILE_ONEDEV:    ///<当前选择的设备没有文件
				mapi->dop_api->file_num = 1;
				(*err) = AI_toy_music_play(mapi,DEV_SEL_NEXT,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
				/* (*err) = AI_toy_music_play(mapi,DEV_SEL_NEXT,0,PLAY_SPEC_FILE,&(mapi->dop_api->file_num)); */
				break;
			case FILE_OP_ERR_LGDEV_NO_FIND:     ///<没找到指定的逻辑设备
				mapi->dop_api->file_num = 1;
				(*err) = AI_toy_music_play(mapi,DEV_SEL_NEXT,0,PLAY_SPEC_FILE,&(mapi->dop_api->file_num));
				break;

			case SUCC_MUSIC_START_DEC:
				(*err) = 0;
				auto_next = 1;
				dec_phy_printf("SUCC_MUSIC_START_DEC \r");
				break;
			case FILE_OP_ERR_OPEN_FILE:         ///<打开文件失败
			case ERR_MUSIC_START_DEC:
				AI_toy_printf("ERR_MUSIC_START_DEC\n");
				status=file_operate_ctl(FOP_DEV_STATUS,mapi->fop_api,0,0);
				if(status==FILE_OP_ERR_OP_HDL)
				{
					///<逻辑设备不再链表
					(*err)=FILE_OP_ERR_OP_HDL;
					break;
				}
				else if(!status)
				{
					///<逻辑设备掉线
					(*err)=FILE_OP_ERR_LGDEV_NO_FIND;
					break;
				}
				totalerrfile++;
				dec_phy_printf("---total err =%08x \n",totalerrfile);
				dec_phy_printf("---total_file_num =%08x \n",mapi->fop_api->cur_lgdev_info->total_file_num);
				if(totalerrfile>=mapi->fop_api->cur_lgdev_info->total_file_num)
				{
					///<当前设备中音乐文件全部不可以解码，做好标识
					file_operate_ctl(FOP_ALLFILE_ERR_LGDEV,mapi->fop_api,0,0);
					(*err)=FILE_OP_ERR_NO_FILE_ONEDEV;
					totalerrfile=0;
					break;
				}
				if(auto_next)
				{
					dec_phy_printf("err next \r");
					(*err) = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_NEXT_FILE,&(mapi->dop_api->file_num));
				}
				else
				{
					dec_phy_printf("err priv \r");
					(*err) = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_PREV_FILE,&(mapi->dop_api->file_num));
				}
				if(((*err)!=FILE_OP_ERR_OPEN_FILE)&&((*err)!=ERR_MUSIC_START_DEC))
				{
					///<不再是顺序找，清空错误数字
					totalerrfile=0;
				}
				break;
			case ERR_MUSIC_API_NULL:
				dec_phy_printf("err  : %02x\r",(*err));
				(*err) = 0;
				break;
			case FILE_OP_ERR_END_FILE:
				mapi->dop_api->file_num = 1;
				mapi->dop_api->dec_api.save_brk = 0;
				file_operate_ctl(FOP_CLEAR_BPINFO,mapi->fop_api,&mapi->dop_api->brk->inf,0);
				(*err) = AI_toy_music_play(mapi,DEV_SEL_NEXT,0,PLAY_FIRST_FILE,&(mapi->dop_api->file_num));
				break;
			case FILE_OP_ERR_PRE_FILE:
				mapi->dop_api->file_num = 1;
				(*err) = AI_toy_music_play(mapi,DEV_SEL_PREV,0,PLAY_LAST_FILE,&(mapi->dop_api->file_num));
				break;
			default:
				dec_phy_printf("err default : %02x\r",(*err));
				(*err) = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_NEXT_FILE,0);
				AI_toy_printf("err my \n");
				break;
		}
	}
}
/* AI_STATUS ai_status_flag = AI_STATUS_UNACTIVE; */
extern u8 microphone_enable;
extern void bt_smart_msg_deal(void *priv, int *msg);
void AI_toy_music_task(void *parg)
{
    MUSIC_OP_API *mapi=NULL;
    int msg[6];
    u32 err = 0;
	NOTICE_PLAYER_ERR n_err;
	lg_dev_info *tmplgdev;

    AI_toy_printf("\n*************************%s********************\n", __FUNCTION__);

	dac_channel_on(MUSIC_CHANNEL, FADE_ON);
	set_sys_vol(dac_ctl.sys_vol_l,dac_ctl.sys_vol_r,FADE_ON);

    music_reverb = NULL;
	auto_next = 1;
#if EQ_EN
    eq_enable();
    eq_mode = get_eq_default_mode();
#endif // EQ_EN

	music_decoder_init(&mapi);
	music_decoder_info(AI_toy_before_decoder_callback,  AI_toy_after_decoder_callback);
	music_operate_mapi = mapi;
#if SUPPORT_APP_RCSP_EN
	rcsp_music_task_init();
#endif


	bt_smart_led_blink(200, 2);	

	if(microphone_enable)
	{
		AI_toy_printf("tf mode open echo  111!!!\n");
		int tmp_msg = MSG_ECHO_START;
		echo_msg_deal_api(&music_reverb, &tmp_msg);
	}

	if((AI_toy_fop != NULL) && (file_operate_ctl(FOP_DEV_STATUS,AI_toy_fop,0,0) == 1))
	{
		AI_toy_printf("ai_finish_op %d\n", ai_finish_op);

		if(g_alarm_back)
		{
			g_alarm_back = 0;
			err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
		}
		else
		{
			if(ai_finish_op)
			{
				switch(ai_finish_op)
				{
					case AI_FINISH_OP_PLAY:
					case AI_FINISH_OP_REVERT:
						err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
						break;
					case AI_FINISH_OP_PREV:
						dac_mute(1, 0);
						err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
						music_stop_decoder(mapi);
						dac_mute(0, 1);
						err = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_PREV_FILE,&(mapi->dop_api->file_num));
						break;
					case AI_FINISH_OP_NEXT:
						dac_mute(1, 0);
						err = AI_toy_music_play(mapi,DEV_SEL_SPEC,0,PLAY_BREAK_POINT,&(mapi->dop_api->file_num));
						music_stop_decoder(mapi);
						dac_mute(0, 1);
						err = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_NEXT_FILE,&(mapi->dop_api->file_num));
						break;
					case AI_FINISH_OP_PAUSE:
						break;
					default:
						break;
				}

				ai_finish_op = 0;

			}
			else
			{
				if(spec_file.flag)
				{
					spec_file.flag = 0;
					os_taskq_post(MUSIC_TASK_NAME, 3, MSG_MUSIC_SPC_FILE, spec_file.clust, spec_file.dir_index);
				}
				else
				{
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							AI_TOY_NOTICE_MODE_TF, 
							AI_toy_tf_mode_notice_play_callback,
							mapi);

					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
					else
					{
						os_taskq_post(MUSIC_TASK_NAME, 1, MSG_MUSIC_DIR_MODE);
					}
				}
			}
		}
	}
	else
	{
		//提示设备不在线	
		AI_toy_printf("no dev !!!!\n");
		n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
				AI_TOY_NOTICE_MODE_TF, 
				AI_toy_tf_mode_notice_play_callback,
				mapi);
		if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
		{

		}
		else
		{
			n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
					AI_TOY_NOTICE_NO_DEV_ONLINE, 
					AI_toy_tf_mode_notice_play_callback,
					mapi);
		}

		AI_toy_dir_mode = AI_TOY_DIR_MODE_UNACTIVE;

	}



	while(1)
	{
		memset(msg,0x00,sizeof(msg));
		os_taskq_pend(0, ARRAY_SIZE(msg), msg);
		clear_wdt();

		switch(msg[0])	
		{
			case MSG_MUSIC_DIR_MODE:
			case MSG_MUSIC_NEXT_FILE:
			case MSG_MUSIC_PREV_FILE:
			case MSG_MUSIC_PP:
			case MSG_MUSIC_PLAY:
			case MSG_MUSIC_PAUSE:
			case MSG_MUSIC_PLAY_SN:
			case MSG_MUSIC_SPC_FILE:
				if((AI_toy_fop == NULL) || (file_operate_ctl(FOP_DEV_STATUS,AI_toy_fop,0,0) != 1))
				{
					msg[0] = NO_MSG;
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							AI_TOY_NOTICE_NO_DEV_ONLINE, 
							AI_toy_tf_mode_notice_play_callback,
							mapi);
					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
					else
					{

					}
				}
				break;
			default:
				break;
		}

#if SUPPORT_APP_RCSP_EN
		rcsp_music_task_msg_deal_before(msg);
#endif
		switch(msg[0])
		{
			case MSG_MUSIC_DIR_MODE:
				err = AI_toy_change_dir_mode(mapi, 0);
				break;

			case MSG_AI_TOY_VOL_UP:
				if(dac_ctl.sys_vol_l < MAX_SYS_VOL_L)
					dac_ctl.sys_vol_l++;
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
					/* AI_toy_music_notice_play(mapi, AI_TOY_NOTICE_VOL); */
				}
				break;
			case MSG_AI_TOY_VOL_DOWN:
				if(dac_ctl.sys_vol_l)
					dac_ctl.sys_vol_l--;
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
					/* AI_toy_music_notice_play(mapi, AI_TOY_NOTICE_VOL); */
				/* } */
				break;

			case SYS_EVENT_DEC_END:
				/* puts("----SYS_EVENT_DEC_FR_END---\n"); */
				dec_phy_printf("decoder stop\r\n");
				err = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_AUTO_NEXT,&(mapi->dop_api->file_num));
				break;

			case MSG_MUSIC_NEXT_FILE:

				if((AI_toy_fop != NULL) && (file_operate_ctl(FOP_DEV_STATUS,AI_toy_fop,0,0) == 1))
				{
					dec_phy_printf("KEY_EVENT_NEXT\r\n");
					auto_next = 1;

					music_stop_decoder(mapi);
					/* EyeEffectCtl(EFFECT_NEXT); */
					eye_led_api(EFFECT_NEXT, 15 ,0);
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							(char *)AI_TOY_NOTICE_NEXT, 
							AI_toy_notice_play_callback,
							mapi);

					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
					else
					{
						err = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_NEXT_FILE,&(mapi->dop_api->file_num));
					}

				}
				else
				{
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							(char *)AI_TOY_NOTICE_NOFILE_1, 
							AI_toy_notice_play_callback,
							mapi);

					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
				}
				break;
			case MSG_MUSIC_PREV_FILE:
				if((AI_toy_fop != NULL) && (file_operate_ctl(FOP_DEV_STATUS,AI_toy_fop,0,0) == 1))
				{
					dec_phy_printf("KEY_EVENT_PREV\r\n");
					auto_next = 0;

					music_stop_decoder(mapi);
					/* EyeEffectCtl(EFFECT_PREV); */
					eye_led_api(EFFECT_PREV, 15 ,0);
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							(char *)AI_TOY_NOTICE_PREV, 
							AI_toy_notice_play_callback,
							mapi);

					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
					else
					{
						err = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_PREV_FILE,&(mapi->dop_api->file_num));
					}

				}
				else
				{
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							(char *)AI_TOY_NOTICE_NOFILE_1, 
							AI_toy_notice_play_callback,
							mapi);

					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
				}
				break;
			case MSG_MUSIC_PP:
				if((AI_toy_fop != NULL) && (file_operate_ctl(FOP_DEV_STATUS,AI_toy_fop,0,0) == 1))
				{
					puts("MSG_MUSIC_PP\n");
					AI_toy_music_pp(mapi);
				}
				else
				{
					n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
							(char *)AI_TOY_NOTICE_NOFILE_1, 
							AI_toy_notice_play_callback,
							mapi);

					if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
					{

					}
				}
				break;
			case MSG_MUSIC_PLAY:
				if(AI_toy_pp_st == DECODER_PAUSE)
				{
					AI_toy_music_pp(mapi);
				}
				break;

			case MSG_MUSIC_PAUSE:
				if(AI_toy_pp_st == DECODER_PLAY)
				{
					AI_toy_music_pp(mapi);
				}
				break;

			case MSG_MUSIC_PLAY_SN:
				puts("MSG_MUSIC_PLAY_SN\n");
				if ((msg[1] <= mapi->fop_api->cur_lgdev_info->total_file_num)&&(msg[1] != 0))
				{
					mapi->dop_api->file_num = msg[1];
					err = AI_toy_music_play(mapi,DEV_SEL_CUR,0,PLAY_SPEC_FILE,&(mapi->dop_api->file_num));
				}
				break;

			case SYS_EVENT_DEV_ONLINE:
				AI_toy_printf("music SYS_EVENT_DEV_ONLINE\r");
				music_stop_decoder(mapi);
				n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
						(char *)AI_TOY_NOTICE_DEV_ONLINE, 
						AI_toy_notice_play_callback,
						mapi);

				if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
				{

				}
				else
				{
					AI_toy_dir_mode = 0;
					err = AI_toy_change_dir_mode(mapi, 1);
				}
				break;

			case SYS_EVENT_DEV_OFFLINE:
				music_stop_decoder(mapi);
				file_operate_ctl(FOP_CLOSE_LOGDEV,mapi->fop_api,0,0);
				n_err = notice_player_play_by_path(MUSIC_TASK_NAME, 
						(char *)AI_TOY_NOTICE_DEV_OFFLINE, 
						AI_toy_notice_play_callback,
						mapi);
				if(n_err == NOTICE_PLAYER_MSG_BREAK_ERR || n_err == NOTICE_PLAYER_TASK_DEL_REQ_ERR)
				{

				}
				else
				{
					AI_toy_printf("fun = %s, line = %d\n", __FUNCTION__, __LINE__);
				}
				break;

			case SYS_EVENT_DEC_DEVICE_ERR:
				break;

			case SYS_EVENT_DEL_TASK: 				//请求删除music任务
#if SUPPORT_APP_RCSP_EN
				rcsp_music_task_exit();
#endif
				mapi->dop_api->dec_api.save_brk = 1;
				AI_toy_music_stop(mapi);

				music_operate_mapi = NULL;
				set_sys_freq(SYS_Hz);
				dac_channel_off(MUSIC_CHANNEL, FADE_ON);


#if EQ_EN
				eq_disable();
#endif/*EQ_EN*/
				//                set_flac_sys_freq(0);//set_sys_clk
				if(mapi->dop_api->dec_api.dec_phy_name)
				{
					if (os_task_del_req(mapi->dop_api->dec_api.dec_phy_name) != OS_TASK_NOT_EXIST)
					{
						os_taskq_post_event(mapi->dop_api->dec_api.dec_phy_name, 1, SYS_EVENT_DEL_TASK);
						do
						{
							OSTimeDly(1);
						}
						while(os_task_del_req(mapi->dop_api->dec_api.dec_phy_name) != OS_TASK_NOT_EXIST);
						mapi->dop_api->dec_api.dec_phy_name = NULL;
						puts("del music dec phy succ\n");
					}
				}
				if(mapi->dop_api->dec_api.ef_enable&&mapi->dop_api->dec_api.ef_out_name)
				{
					if (os_task_del_req(mapi->dop_api->dec_api.ef_out_name) != OS_TASK_NOT_EXIST)
					{
						os_taskq_post_event(mapi->dop_api->dec_api.ef_out_name, 1, SYS_EVENT_DEL_TASK);
						do
						{
							OSTimeDly(1);
						}
						while(os_task_del_req(mapi->dop_api->dec_api.ef_out_name) != OS_TASK_NOT_EXIST);
						mapi->dop_api->dec_api.ef_out_name = NULL;
						puts("del music ef_out_name succ\n");
					}
				}

				///<关闭文件操作器，释放资源
				file_operate_ctl(FOP_CLOSE_LOGDEV,mapi->fop_api,0,0);
				///<释放文件操作器初始化信息

				echo_exit_api(&music_reverb);
				if(mapi->fop_api)
				{
					if(mapi->fop_api->fop_init)
						free_fun((void**)&mapi->fop_api->fop_init);
					free_fun((void**)&mapi->fop_api);
				}

				if(mapi->ps_api)
				{
					free_fun((void**)&mapi->ps_api);
				}

				free_fun((void**)&mapi->dop_api->brk);
				free_fun((void**)&mapi->dop_api->io);
				free_fun((void**)&mapi->dop_api);
				free_fun((void**)&mapi);

				extern tbool bt_smart_connect_status(void);
				if(bt_smart_connect_status() == false)
				{
					eye_led_api(EFFECT_NO_CONNECT, 0 ,0);
				}
				else
				{
					eye_led_api(EFFECT_CONNECT_OK, 0 ,0);
				}


				if (os_task_del_req_name(OS_TASK_SELF) == OS_TASK_DEL_REQ)
				{
					os_task_del_res_name(OS_TASK_SELF); 	//确认可删除，此函数不再返回
				}
				break;

			case MSG_MUSIC_SPC_FILE:
				auto_next = 1;
				AI_toy_dir_mode = msg[2];
				AI_toy_printf("MSG_MUSIC_SPC_FILE-----,clust = 0x%x, %d\n",msg[1], AI_toy_dir_mode);
				err=AI_toy_music_play_by_sclust(mapi, msg[1]);
				break;


			case MSG_HALF_SECOND:
				AI_toy_printf("-MH-\n");
				debug_loop_err_clear();
#if 0
				_EYEEFFECT cur_eye_ef = EyeEffect_get_cur();

				if(cur_eye_ef == EFFECTSOUNDCOLUMNSLOW 
						|| cur_eye_ef == EFFECTSOUNDCOLUMNNORMAL 
						|| cur_eye_ef == EFFECTSOUNDCOLUMNFAST 
				  )
				{
					if(AI_toy_pp_st == DECODER_PLAY)		
					{
						/* AI_toy_printf("***********************led_on_01****************** \n"); */
						//extern u8 GetMusicEnergyLevel(void);
						//if (GetMusicEnergyLevel() == 1)
						//{
						//	EyeEffectCtl(EFFECTSOUNDCOLUMNSLOW);
						//}
						//else if (GetMusicEnergyLevel() == 2)
						//{
						//	EyeEffectCtl(EFFECTSOUNDCOLUMNNORMAL);
						//}
						//else
						//{
						//	EyeEffectCtl(EFFECTSOUNDCOLUMNFAST);
						//}
						//bt_smart_led_on();
					}		
				}
				
				/* else */
				/* { */
					/* AI_toy_printf("***********************led_flick_01****************** \n"); */
					/* EyeEffectCtl(EFFECTTRIANGLE); */
					/* bt_smart_led_flick(500, 0); */
				/* } */
#endif
				break;

			default:
				AI_toy_printf("dev status =%08x \n",err);
				bt_smart_msg_deal(mapi, msg);
				echo_msg_deal_api(&music_reverb, msg);
				break;

		}

		AI_toy_music_play_err_deal(mapi, &err);

#if SUPPORT_APP_RCSP_EN
		rcsp_music_task_msg_deal_after(msg);
#endif
	}
}

static void AI_toy_music_task_init(void *priv)
{
    u32 err;

//      GET_SYS_MEM_STAT();

    err = os_task_create_ext(AI_toy_music_task, 0, TaskMusicPrio, 50, "MusicTask", MUSIC_TASK_STACK_SIZE);
    if(OS_NO_ERR == err)
    {
        key_msg_register("MusicTask",music_ad_table,music_io_table,music_ir_table,NULL);
        AI_toy_printf("open_music_task: succ\n");
    }

}

/*----------------------------------------------------------------------------*/
/**@brief  退出音乐线程
   @param  void
   @return void
   @note
*/
/*----------------------------------------------------------------------------*/
static void AI_toy_music_task_exit()
{
    if (os_task_del_req("MusicTask") != OS_TASK_NOT_EXIST)
    {
        os_taskq_post_event("MusicTask", 1, SYS_EVENT_DEL_TASK);
        do
        {
            OSTimeDly(1);
        }
        while(os_task_del_req("MusicTask") != OS_TASK_NOT_EXIST);
        AI_toy_printf("del_music_task: succ\n");
    }

//      GET_SYS_MEM_STAT();
}


TASK_REGISTER(AI_toy_music_task_info) =
{
    .name = "MusicTask",
    .init = AI_toy_music_task_init,
    .exit = AI_toy_music_task_exit,
};






