#ifndef __E_RABBIT_H__
#define __E_RABBIT_H__
#include "typedef.h"
#include "sdk_cfg.h"
#include "dec/music_api.h"
#include "dev_manage/dev_ctl.h"

#define AI_TOY_NOTICE_CHILD			"child.***"
#define AI_TOY_NOTICE_STROY			"stroy.***"
#define AI_TOY_NOTICE_ENLISH		"english.***"
#define AI_TOY_NOTICE_CHINESE		"chinese.***"
#define AI_TOY_NOTICE_MUSIC			"music.***"
#define AI_TOY_NOTICE_INSTUM		"instum.***"
#define AI_TOY_NOTICE_SLEEP			"sleep.***"

#define AI_TOY_NOTICE_POWER_ON		"poweron.***"
#define AI_TOY_NOTICE_POWER_OFF		"poweroff.***"
#define AI_TOY_NOTICE_UPDATE_OK		"updateok.***"
#define AI_TOY_NOTICE_UPDATE_FAIL	"upfail.***"
#define AI_TOY_NOTICE_LOW_POWER_OFF	"lowpowoff.***"
#define AI_TOY_NOTICE_LOW_POWER		"lowpower.***"
#define AI_TOY_NOTICE_NOFILE		"nofile.***"
#define AI_TOY_NOTICE_NOFILE_1		"nofile1.***"
#define AI_TOY_NOTICE_PAUSE			"pause.***"
#define AI_TOY_NOTICE_PLAY			"play.***"
#define AI_TOY_NOTICE_NEXT			"next.***"
#define AI_TOY_NOTICE_PREV			"prev.***"
#define AI_TOY_NOTICE_DEV_OFFLINE	"tfoffline.***"
#define AI_TOY_NOTICE_DEV_ONLINE	"tfonline.***"
#define AI_TOY_NOTICE_NO_DEV_ONLINE	"inssd.***"
#define AI_TOY_NOTICE_ENTER_MIC		"micmode.***"
#define AI_TOY_NOTICE_EXIT_MIC		"micexit.***"
#define AI_TOY_NOTICE_SPEECH_INPUT	"do.***"
#define AI_TOY_NOTICE_SPEECH_TRANC	"di.***"
#define AI_TOY_NOTICE_P_CONNECT		"pcon.***"
#define AI_TOY_NOTICE_NO_NET		"nonet.***"
#define AI_TOY_NOTICE_MODE_TF		"tfmode.***"

enum
{
	AI_TOY_DIR_MODE_CHILDREN_SONG = 0x0,
	AI_TOY_DIR_MODE_STORY,
	AI_TOY_DIR_MODE_ENGLISH,
	AI_TOY_DIR_MODE_CHINESE,
	AI_TOY_DIR_MODE_MUSIC,
	AI_TOY_DIR_MODE_INSTRUMENT,
	AI_TOY_DIR_MODE_SLEEP_SONG,
	AI_TOY_DIR_MODE_MAX,
	AI_TOY_DIR_MODE_UNACTIVE,
};

void AI_toy_dev_online(DEV_HANDLE dev);
void AI_toy_dev_offline(DEV_HANDLE dev);
void AI_toy_update_ai_mode(u8 status);
void ai_toy_change_net_resource(u8 type, u8 period);

#endif// __E_RABBIT_H__
