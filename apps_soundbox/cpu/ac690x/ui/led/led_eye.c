#include "ui/led/led.h"
#include "timer.h"
#include "ui/led/led_eye.h"

_led_picture_ctl ledPictureCtl = 
{
	.curPictrue = 0,
	.timeCnt = 0,
	.led_picture = NULL,
};

const u8 pictureTest[][LED_COLUMN_NUM+1] = 
{
	{0x02,0x00,0x06,0x00,0x00,0x00,0x64},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x64},
	{0x00,0x00,0x00,0x00,0x00,0x07,0x64},
/*
	{0x00,0x00,0x00,0x00,0x00,0x00},
	{0x00,0x00,0x00,0x00,0x00,0x00},
*/
};
/*****************效果2，正方形变化**********************/
const u8 pictureEffect2[][LED_COLUMN_NUM+1] =
{
	{0x00,0x00,0x04,0x00,0x00,0x00,0x8},
	{0x00,0x00,0x04,0x00,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x1f,0x11,0x11,0x11,0x1f,0x00,0x8},
	{0x1f,0x11,0x11,0x11,0x1f,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
	{0x1f,0x11,0x11,0x11,0x1f,0x00,0x8},
	{0x1f,0x11,0x11,0x11,0x1f,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
	{0x1f,0x11,0x11,0x11,0x1f,0x00,0x8},
	{0x1f,0x11,0x11,0x11,0x1f,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x0e,0x0a,0x0e,0x00,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x8},
};
_led_picture led_pictureEffect2 = 
{
	.pictureNum = sizeof(pictureEffect2)/sizeof(pictureEffect2[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect2,
	.timeChangeNum_cb = NULL,
};   
/************正方形效果外调接口***********/
void EffectSquare(void)
{
	OS_ENTER_CRITICAL();	
	
	if (ledPictureCtl.led_picture == &led_pictureEffect2)
	{
		OS_EXIT_CRITICAL();
		return;
	}

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;
	
	led_pictureEffect2.pictureNum = sizeof(pictureEffect2)/sizeof(pictureEffect2[0]);
	led_pictureEffect2.changeTime = 40;
	led_pictureEffect2.picture = pictureEffect2;

	ledPictureCtl.led_picture = &led_pictureEffect2;
	OS_EXIT_CRITICAL();
}

#define  EFFECTPLAYSPEED  0
const u8 pictureEffect_play[][LED_COLUMN_NUM+1] =
{
	{0x03,0x1f,0x1F,0x0f,0x07,0x00,EFFECTPLAYSPEED},
	{0x01,0x0f,0x1F,0x03,0x01,0x00,EFFECTPLAYSPEED},
	{0x01,0x07,0x1F,0x1F,0x0f,0x00,EFFECTPLAYSPEED},
	{0x01,0x01,0x0f,0x1f,0x03,0x03,EFFECTPLAYSPEED},
	{0x00,0x1F,0x1f,0x03,0x07,0x03,EFFECTPLAYSPEED},
	{0x00,0x03,0x0F,0x03,0x07,0x00,EFFECTPLAYSPEED},
	{0x01,0x0f,0x07,0x0F,0x03,0x03,EFFECTPLAYSPEED},
	{0x01,0x03,0x07,0x1f,0x0f,0x01,EFFECTPLAYSPEED},
	{0x03,0x07,0x1f,0x07,0x03,0x03,EFFECTPLAYSPEED},
	{0x00,0x1f,0x07,0x0f,0x01,0x00,EFFECTPLAYSPEED},
	{0x07,0x1f,0x0f,0x1F,0x03,0x01,EFFECTPLAYSPEED},
	{0x07,0x0f,0x1F,0x07,0x03,0x03,EFFECTPLAYSPEED},
	{0x01,0x1F,0x0f,0x0F,0x07,0x03,EFFECTPLAYSPEED},
	{0x01,0x07,0x0f,0x1F,0x07,0x01,EFFECTPLAYSPEED},
};
extern u8 GetMusicEnergyLevel(void);
static const u8 play_speed_tab[4][2] = 
{
	{0x24, 0x1c}, 
	{0x20, 0x18},
	{0x18, 0x10},	
	{0x12, 0x0},
};
static u8 music_play_energy_lever(u8 *val)
{
	u8 lever = GetMusicEnergyLevel();
	if(lever > 3)
		lever = 3;
	/* printf("lever = %d\n", lever); */
	(*val) &= ~(play_speed_tab[lever][1]);
	return play_speed_tab[lever][0];
}

_led_picture led_pictureEffect_play = 
{
	.pictureNum = sizeof(pictureEffect_play)/sizeof(pictureEffect_play[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect_play,
	.timeChangeNum_cb = music_play_energy_lever,
};
/************音柱快速效果外调接口***********/
void Effect_play(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_play)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;	
	led_pictureEffect_play.pictureNum = sizeof(pictureEffect_play)/sizeof(pictureEffect_play[0]);
	led_pictureEffect_play.changeTime = 40;
	led_pictureEffect_play.picture = pictureEffect_play;
	
	ledPictureCtl.led_picture = &led_pictureEffect_play;
	OS_EXIT_CRITICAL();
};
/********************全熄灭灯**********************/
const u8 pictureEffect10[][LED_COLUMN_NUM+1] =
{
	{0x00,0x00,0x00,0x00,0x00,0x00,0x10},
};
_led_picture led_pictureEffect10 = 
{
	.pictureNum = sizeof(pictureEffect10)/sizeof(pictureEffect10[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect10,
	.timeChangeNum_cb = NULL,
};
/************关闭所有led外调接口***********/
void EffectCloseAllLed(void)
{
	OS_ENTER_CRITICAL();
	
	if (ledPictureCtl.led_picture == &led_pictureEffect10)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;
	
	led_pictureEffect10.pictureNum = sizeof(pictureEffect10)/sizeof(pictureEffect10[0]);
	led_pictureEffect10.changeTime = 40;
	led_pictureEffect10.picture = pictureEffect10;
	
	ledPictureCtl.led_picture = &led_pictureEffect10;
	OS_EXIT_CRITICAL();
};

/********************全亮**********************/
const u8 pictureEffect_open_all[][LED_COLUMN_NUM+1] =
{
	{0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x10},
};
_led_picture led_pictureEffect_open_all = 
{
	.pictureNum = sizeof(pictureEffect_open_all)/sizeof(pictureEffect_open_all[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect_open_all,
	.timeChangeNum_cb = NULL,
};
/************关闭所有led外调接口***********/
static void EffectopenAllLed(void)
{
	OS_ENTER_CRITICAL();
	
	if (ledPictureCtl.led_picture == &led_pictureEffect_open_all)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;
	
	led_pictureEffect_open_all.pictureNum = sizeof(pictureEffect_open_all)/sizeof(pictureEffect_open_all[0]);
	led_pictureEffect_open_all.changeTime = 40;
	led_pictureEffect_open_all.picture = pictureEffect_open_all;
	
	ledPictureCtl.led_picture = &led_pictureEffect_open_all;
	OS_EXIT_CRITICAL();
};
/*************喜欢效果****************************/
#define EFFECTEXPRESSIONCNT    0X20
const u8 pictureEffect12[][LED_COLUMN_NUM+1] =
{
	{0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect12 = 
{
	.pictureNum = sizeof(pictureEffect12)/sizeof(pictureEffect12[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect12,
	.timeChangeNum_cb = NULL,
};
/************喜欢表情外调接口***********/
void EffectLove(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect12)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect12;
	OS_EXIT_CRITICAL();
};
/*************喜欢效果****************************/
#define EFFECTEXPRESSIONCNT1    0X2e
/* const u8 pictureEffect25[][LED_COLUMN_NUM+1] = */
/* { */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,EFFECTEXPRESSIONCNT1}, */
	/* {0X0c,0X00,0X00,0X00,0X00,0X0c,EFFECTEXPRESSIONCNT1}, */
	/* {0X0c,0X12,0X00,0X00,0X12,0X0c,EFFECTEXPRESSIONCNT1}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT1}, */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,0x10}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT}, */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,0x10}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT}, */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,0x10}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT}, */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,0x10}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT}, */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,0x10}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT}, */
	/* {0X00,0X00,0X00,0X00,0X00,0X00,0x10}, */
	/* {0X0c,0X12,0X09,0X09,0X12,0X0c,EFFECTEXPRESSIONCNT1}, */
	/* {0X0c,0X12,0X00,0X00,0X12,0X0c,EFFECTEXPRESSIONCNT1}, */
	/* {0X0c,0X00,0X00,0X00,0X00,0X0c,EFFECTEXPRESSIONCNT1}, */
/* }; */

const u8 pictureEffect25[][LED_COLUMN_NUM+1] =
{
	{0X0c,0X12,0X09,0X09,0X12,0X0c,50},
	{0X00,0X00,0X00,0X00,0X00,0X00,50},
};
_led_picture led_pictureEffect25 = 
{
	.pictureNum = sizeof(pictureEffect25)/sizeof(pictureEffect25[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect25,
	.timeChangeNum_cb = NULL,
};
/************喜欢表情外调接口***********/
void EffectLoveTest(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect25)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect25;
	OS_EXIT_CRITICAL();
};
/************微笑外调接口***********/
const u8 pictureEffect13[][LED_COLUMN_NUM+1] =
{
	{0X04,0X08,0X10,0X10,0X08,0X04,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect13 = 
{
	.pictureNum = sizeof(pictureEffect13)/sizeof(pictureEffect13[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect13,
	.timeChangeNum_cb = NULL,
};
/************微笑表情外调接口***********/
void EffectSmile(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect13)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect13;
	OS_EXIT_CRITICAL();
};

/************害羞外调接口***********/
const u8 pictureEffect14[][LED_COLUMN_NUM+1] =
{
	{0X04,0X02,0X01,0X01,0X02,0X04,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect14 = 
{
	.pictureNum = sizeof(pictureEffect14)/sizeof(pictureEffect14[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect14,
	.timeChangeNum_cb = NULL,
};
/************害羞表情外调接口***********/
void EffectShy(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect14)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect14;
	OS_EXIT_CRITICAL();
};
/************冥想外调接口***********/
const u8 pictureEffect15[][LED_COLUMN_NUM+1] =
{
	{0X04,0X04,0X04,0X04,0X04,0X04,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect15 = 
{
	.pictureNum = sizeof(pictureEffect15)/sizeof(pictureEffect15[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect15,
	.timeChangeNum_cb = NULL,
};
/************冥想表情外调接口***********/
void EffectMeditation(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect15)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect15;
	OS_EXIT_CRITICAL();
};
/************惊讶外调接口***********/
const u8 pictureEffect16[][LED_COLUMN_NUM+1] =
{
	{0X00,0X0e,0X11,0X11,0X0e,0X00,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect16 = 
{
	.pictureNum = sizeof(pictureEffect16)/sizeof(pictureEffect16[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect16,
	.timeChangeNum_cb = NULL,
};
/************惊讶表情外调接口***********/
void EffectSurprise(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect16)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect16;
	OS_EXIT_CRITICAL();
};
/************愤怒外调接口***********/
const u8 pictureEffect17[][LED_COLUMN_NUM+1] =
{
	{0X08,0X04,0X02,0X01,0X01,0X02,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect17 = 
{
	.pictureNum = sizeof(pictureEffect17)/sizeof(pictureEffect17[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect17,
	.timeChangeNum_cb = NULL,
};
/************愤怒表情外调接口***********/
void EffectAnger(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect17)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect17;
	OS_EXIT_CRITICAL();
};
/************悲伤外调接口***********/
const u8 pictureEffect18[][LED_COLUMN_NUM+1] =
{
	{0X02,0X01,0X01,0X02,0X04,0X08,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect18 = 
{
	.pictureNum = sizeof(pictureEffect18)/sizeof(pictureEffect18[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect18,
	.timeChangeNum_cb = NULL,
};
/************悲伤表情外调接口***********/
void EffectSad(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect18)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect18;
	OS_EXIT_CRITICAL();
};

/************囧外调接口***********/
const u8 pictureEffect19[][LED_COLUMN_NUM+1] =
{
	{0X01,0X02,0X04,0X08,0X04,0X02,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect19 = 
{
	.pictureNum = sizeof(pictureEffect19)/sizeof(pictureEffect19[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect19,
	.timeChangeNum_cb = NULL,
};
/************囧表情外调接口***********/
void EffectJiong(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect19)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect19;
	OS_EXIT_CRITICAL();
};
/************晕外调接口***********/
const u8 pictureEffect20[][LED_COLUMN_NUM+1] =
{
	{0X00,0X0e,0X1d,0X15,0X0c,0X00,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect20 = 
{
	.pictureNum = sizeof(pictureEffect20)/sizeof(pictureEffect20[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect20,
	.timeChangeNum_cb = NULL,
};
/************晕表情外调接口***********/
void EffectDizzy(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect20)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect20;
	OS_EXIT_CRITICAL();
};
/************哭外调接口***********/
const u8 pictureEffect21[][LED_COLUMN_NUM+1] =
{
	{0X08,0X08,0X0f,0X0f,0X08,0X18,EFFECTEXPRESSIONCNT},
};
_led_picture led_pictureEffect21 = 
{
	.pictureNum = sizeof(pictureEffect21)/sizeof(pictureEffect21[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = pictureEffect21,
	.timeChangeNum_cb = NULL,
};
/************哭表情外调接口***********/
void EffectCry(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect21)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect21;
	OS_EXIT_CRITICAL();
};


/************fail***********/
#define PICTURE_FAIL_REFRESH_CNT		(40)
const u8 picture_fail[][LED_COLUMN_NUM+1] =
{
	{0X00,0X0e,0X15,0X15,0X0d,0X00,PICTURE_FAIL_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X00,PICTURE_FAIL_REFRESH_CNT},

	/* {0X1f,0X1f,0X1f,0X1f,0X1f,0X0f,PICTURE_POWER_ON_REFRESH_CNT}, */
};
_led_picture led_pictureEffect_fail = 
{
	.pictureNum = sizeof(picture_fail)/sizeof(picture_fail[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_fail,
	.timeChangeNum_cb = NULL,
};
void Effect_fail(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_fail)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_fail;
	OS_EXIT_CRITICAL();
};


/************power_on***********/
#define PICTURE_POWER_ON_REFRESH_CNT		(4)
const u8 picture_poweron[][LED_COLUMN_NUM+1] =
{
	{0X00,0X00,0X04,0X00,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X04,0X04,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X04,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X0c,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X0c,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X08,0X0c,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0c,0X0c,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0c,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X0c,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X0e,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X0e,0X02,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X0e,0X06,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X0e,0X0e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X0e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X0e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X0e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X1e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X10,0X1e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X18,0X1e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1c,0X1e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1e,0X1e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1e,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1e,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1e,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1e,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1f,0X00,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1f,0X01,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1f,0X03,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1f,0X07,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1f,0X0f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1f,0X1f,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1B,0X1f,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1B,0X1B,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X1B,0X13,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X1f,0X13,0X13,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X17,0X13,0X13,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X13,0X13,0X13,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X13,0X13,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X13,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X11,0X1f,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X11,0X1d,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X11,0X19,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X11,0X11,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X11,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X11,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X11,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X1f,0X01,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X0f,0X01,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X07,0X01,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X03,0X01,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X01,0X01,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X01,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X01,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X01,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X01,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X1f,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X1e,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X1c,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X18,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X10,PICTURE_POWER_ON_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X00,PICTURE_POWER_ON_REFRESH_CNT},

	/* {0X1f,0X1f,0X1f,0X1f,0X1f,0X0f,PICTURE_POWER_ON_REFRESH_CNT}, */
};
_led_picture led_pictureEffect_poweron = 
{
	.pictureNum = sizeof(picture_poweron)/sizeof(picture_poweron[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_poweron,
	.timeChangeNum_cb = NULL,
};
void Effect_poweron(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_poweron)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_poweron;
	OS_EXIT_CRITICAL();
};
/************暂停/播放***********/
#define PICTURE_PP_REFRESH_CNT		(80)
const u8 picture_pp[][LED_COLUMN_NUM+1] =
{
	/* {0X04,0X08,0X10,0X10,0X08,0X04,PICTURE_PP_REFRESH_CNT}, */
	/* {0X04,0X02,0X01,0X01,0X02,0X04,PICTURE_PP_REFRESH_CNT}, */

	{0X00,0X0e,0X11,0X11,0X0e,0X00,PICTURE_PP_REFRESH_CNT},
	{0X00,0X00,0X00,0X00,0X00,0X00,PICTURE_PP_REFRESH_CNT},
};
_led_picture led_pictureEffect_pp = 
{
	.pictureNum = sizeof(picture_pp)/sizeof(picture_pp[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_pp,
	.timeChangeNum_cb = NULL,
};
void Effect_pp(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_pp)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_pp;
	OS_EXIT_CRITICAL();
};


/************prev***********/
#define PICTURE_PREV_REFRESH_CNT		(80)
const u8 picture_prev[][LED_COLUMN_NUM+1] =
{
	{0X04,0X0e,0X04,0X04,0X04,0X04,PICTURE_PREV_REFRESH_CNT},
};
_led_picture led_pictureEffect_prev = 
{
	.pictureNum = sizeof(picture_prev)/sizeof(picture_prev[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_prev,
	.timeChangeNum_cb = NULL,
};
void Effect_prev(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_prev)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_prev;
	OS_EXIT_CRITICAL();
};

/************next***********/
#define PICTURE_NEXT_REFRESH_CNT		(80)
const u8 picture_next[][LED_COLUMN_NUM+1] =
{
	{0X04,0X04,0X04,0X04,0X0e,0X04,PICTURE_NEXT_REFRESH_CNT},
};
_led_picture led_pictureEffect_next = 
{
	.pictureNum = sizeof(picture_next)/sizeof(picture_next[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_next,
	.timeChangeNum_cb = NULL,
};
void Effect_next(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_next)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_next;
	OS_EXIT_CRITICAL();
};

/************connect***********/
#define PICTURE_CONNECT_REFRESH_CNT		(80)
const u8 picture_connect[][LED_COLUMN_NUM+1] =
{
	{0X04,0X0a,0X11,0X11,0X0a,0X04,PICTURE_CONNECT_REFRESH_CNT},
	{0X04,0X04,0X04,0X04,0X04,0X04,PICTURE_CONNECT_REFRESH_CNT},
};
_led_picture led_pictureEffect_connect = 
{
	.pictureNum = sizeof(picture_connect)/sizeof(picture_connect[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_connect,
	.timeChangeNum_cb = NULL,
};
void Effect_connect(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_connect)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_connect;
	OS_EXIT_CRITICAL();
};


/************update***********/
#define PICTURE_UPDATE_REFRESH_CNT		(50)
const u8 picture_update[][LED_COLUMN_NUM+1] =
{
	{0X04,0X00,0X00,0X00,0X00,0X00,PICTURE_UPDATE_REFRESH_CNT},
	{0X04,0X04,0X00,0X00,0X00,0X00,PICTURE_UPDATE_REFRESH_CNT},
	{0X04,0X04,0X04,0X00,0X00,0X00,PICTURE_UPDATE_REFRESH_CNT},
};
_led_picture led_pictureEffect_update = 
{
	.pictureNum = sizeof(picture_update)/sizeof(picture_update[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_update,
	.timeChangeNum_cb = NULL,
};
static void Effect_update(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_update)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_update;
	OS_EXIT_CRITICAL();
};



/************search***********/
#define PICTURE_SEARCH_REFRESH_CNT		(80)
const u8 picture_search[][LED_COLUMN_NUM+1] =
{
	{0X04,0X08,0X10,0X10,0X08,0X04,PICTURE_SEARCH_REFRESH_CNT},
	{0X04,0X02,0X01,0X01,0X02,0X04,PICTURE_SEARCH_REFRESH_CNT},
};
_led_picture led_pictureEffect_search = 
{
	.pictureNum = sizeof(picture_search)/sizeof(picture_search[0]),
	/********级别是ms*********/
	.changeTime = 40,
	.picture = picture_search,
	.timeChangeNum_cb = NULL,
};
static void Effect_search(void)
{
	OS_ENTER_CRITICAL();	
	if (ledPictureCtl.led_picture == &led_pictureEffect_search)
	{
		OS_EXIT_CRITICAL();
		return;
	};

	ledPictureCtl.curPictrue = 0;
	ledPictureCtl.timeCnt = 0;

	ledPictureCtl.led_picture = &led_pictureEffect_search;
	OS_EXIT_CRITICAL();
};

void EyeLedScan(void *parm)
{
	/***********行数据扫描指引*****************/
	static u8 pictureColumnIndex = 0;
	u16 timeChangeNum = 0;
	if (pictureColumnIndex >= LED_COLUMN_NUM)
	{
		pictureColumnIndex = 0;
		ledPictureCtl.timeCnt++;
	}

	if ((ledPictureCtl.led_picture==NULL) || (ledPictureCtl.led_picture->picture==NULL))
	{/***********没有初始化*************/
		EYELED_ALL_OFF();
		return;
	}
	
	switch(ledPictureCtl.led_picture->snakeen)
	{
		case 1:
			   if ((ledPictureCtl.curPictrue % 1) == 0)
			   {
			   		timeChangeNum = 0x20;
			   }
			   else
			   {
			   		timeChangeNum = (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][LED_COLUMN_NUM];
			   }
			break;
		case 2:
			   if ((ledPictureCtl.curPictrue % 1) == 0)
			   {
			   		timeChangeNum = 0x18;
			   }
			   else
			   {
			   		timeChangeNum = (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][LED_COLUMN_NUM];
			   }
			break;
		case 3:
			   if ((ledPictureCtl.curPictrue % 1) == 0)
			   {
			   		timeChangeNum = 0x10;
			   }
			   else
			   {
			   		timeChangeNum = (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][LED_COLUMN_NUM];
			   }
			break;

		default:
			timeChangeNum = (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][LED_COLUMN_NUM];
			break;
	}


	u8 num = (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][pictureColumnIndex];	
	if(ledPictureCtl.led_picture->timeChangeNum_cb)
	{
		timeChangeNum = (u16)ledPictureCtl.led_picture->timeChangeNum_cb(&num);
	}

	/* if (ledPictureCtl.timeCnt >= (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][LED_COLUMN_NUM]) */
	if (ledPictureCtl.timeCnt >= timeChangeNum)
	{/*****************切换下一幅图片*********************/
		ledPictureCtl.curPictrue++;
		ledPictureCtl.timeCnt = 0;
	}

	if(ledPictureCtl.curPictrue >= ledPictureCtl.led_picture->pictureNum)
	{/*************图片扫描完毕，进入下一轮扫描**************/
		ledPictureCtl.curPictrue = 0;
		ledPictureCtl.timeCnt = 0;
	}

	/************先熄灭所有的LED，再点亮对应的列led**************/
	EYELED_ALL_OFF();

	switch(pictureColumnIndex)
	{
		case 0:
			/****************扫描第一列**************/
			LED_Y1_ON();
			break;
		case 1:
			/****************扫描第二列**************/
			LED_Y2_ON();
			break;
		case 2:
			/****************扫描第三列**************/
			LED_Y3_ON();
			break;
		case 3:
			/****************扫描第四列**************/
			LED_Y4_ON();
			break;
		case 4:
			/****************扫描第五列**************/
			LED_Y5_ON();
			break;
		case 5:
			/****************扫描第六列**************/
			LED_Y6_ON();
			break;
		default:
			break;
	}

	/* num = (ledPictureCtl.led_picture->picture)[ledPictureCtl.curPictrue][pictureColumnIndex];	 */
	if (num & BIT(0))
	{
		LED_X1_ON();
	}
	if (num & BIT(1))
	{
		LED_X2_ON();
	}
	if (num & BIT(2))
	{
		LED_X3_ON();
	}
	if (num & BIT(3))
	{
		LED_X4_ON();
	}
	if (num & BIT(4))
	{
		LED_X5_ON();
	}

	pictureColumnIndex++;
}

/****************所有效果控制接口********************/
static _EYEEFFECT cur_eye_effect = EFFECT_UNACTIVE;
_EYEEFFECT EyeEffect_get_cur(void)
{
	return cur_eye_effect;		
}
void EyeEffectCtl(_EYEEFFECT eyeEffect)
{
	cur_eye_effect = eyeEffect;
	switch(eyeEffect)
	{
	case EFFECTCLOSEALLLED:
		EffectCloseAllLed();
		break;

	case EFFECTOPENALLLED:
		EffectopenAllLed();
		break;

	case EFFECTLOVE:
		EffectLove();
		break;
	case EFFECTSMILE:
		EffectSmile();
		break;
	case EFFECTSHY:
		EffectShy();
		break;
	case EFFECTMEDITATION:
		EffectMeditation();
		break;
	case EFFECTSURPRISE:
		EffectSurprise();
		break;
	case EFFECTANGER:
		EffectAnger();
		break;
	case EFFECTSAD:
		EffectSad();
		break;
	case EFFECTJIONG:
		EffectJiong();
		break;
	case EFFECTDIZZY:
		EffectDizzy();
		break;
	case EFFECTCRY:
		EffectCry();
		break;	
	case EFFECTLOVETEST:
		EffectLoveTest();
		break;

	case EFFECT_POWER_ON:
		printf("--EFFECT_POWER_ON--\n");
		Effect_poweron();
		/* Effect_search(); */
/* Effect_update(); */
/* EffectopenAllLed(); */
/* while(1) */
/* { */
	/* clear_wdt(); */
	/* os_time_dly(100);		 */
/* } */
	   break;	
	case EFFECT_POWER_OFF:
		printf("--EFFECT_POWER_OFF--\n");
		EffectSad();
	   break;
	case EFFECT_PLAY:
		Effect_play();
		printf("--EF_PLAY--\n");
		break;
	case EFFECT_PAUSE:
		printf("--EFFECT_PAUSE--\n");
		Effect_pp();
		break;
	case EFFECT_PREV:
		printf("--EFFECT_PREV--\n");
		Effect_prev();
		break;
	case EFFECT_NEXT:
		printf("--EFFECT_NEXT--\n");
		Effect_next();
		break;
	case EFFECT_CONNECT:
		printf("--EFFECT_CONNECT--\n");
		Effect_connect();
		break;
	case EFFECT_CONNECT_OK:
		printf("--EFFECT_CONNECT_OK--\n");
		EffectLoveTest();
		break;
	case EFFECT_NO_CONNECT:
		printf("--EFFECT_NO_CONNECT--\n");
		/* Effect_connect(); */
		Effect_fail();
		break;
	case EFFECT_SPEECH_INPUT:
		printf("--EFFECT_SPEECH_INPUT--\n");
		EffectMeditation();
		break;
	case EFFECT_SPEECH_RESULT_OK:
		printf("--EFFECT_SPEECH_RESULT_OK--\n");
		break;
	case EFFECT_SPEECH_RESULT_FAIL:
		printf("--EFFECT_SPEECH_RESULT_FAIL--\n");
		break;
	case EFFECT_ALARM_ON:
		EffectSquare();
		break;

	case EFFECT_UPDATE_ING:
		/* printf("--EFFECT_ALARM_ON--\n"); */
		Effect_update();
		break;

	case EFFECT_SEARCH_ING:
		Effect_search();
		break;
	default:
		break;
	}
}


#define EYE_LED_OFF_TIMEOUT 		(10*10)//10s
static volatile u16 eye_led_display_timeout = 0;
static volatile u16 eye_led_off_timeout = EYE_LED_OFF_TIMEOUT;
static volatile u8 eye_led_off_enable = 0;
static _EYEEFFECT last_eye_display = EFFECT_UNACTIVE;
static _EYEEFFECT curt_eye_display = EFFECT_UNACTIVE;

extern volatile u8 g_powerdown_flag;

void eye_led_api(_EYEEFFECT type, u16 timeout, u8 flag)
{
	if(curt_eye_display == type)
	{
		return;		
	}

	if(g_powerdown_flag)
	{
		if(type != EFFECTCLOSEALLLED)
		{
			return ;	
		}
	}

	EyeEffectCtl(type);

	OS_ENTER_CRITICAL();	
	eye_led_display_timeout = timeout;
	eye_led_off_timeout = EYE_LED_OFF_TIMEOUT;
	if(timeout)
	{
		last_eye_display = curt_eye_display;
	}
	curt_eye_display = type;
	eye_led_off_enable = flag;
	OS_EXIT_CRITICAL();
}

void eye_led_set_back_api(_EYEEFFECT type)
{
	OS_ENTER_CRITICAL();	
	curt_eye_display = type;
	OS_EXIT_CRITICAL();
}


void eye_led_api_scan(void *p)
{
	if(eye_led_off_enable)
	{
		if(eye_led_off_timeout)		
		{
			eye_led_off_timeout --;
		}
		else
		{
			eye_led_off_enable = 0;
			eye_led_display_timeout = 0;
			/* last_eye_display = EFFECT_UNACTIVE; */
			EffectCloseAllLed();
		}
	}

	if(eye_led_off_timeout)
	{
		/* printf("---%d-------\n", eye_led_display_timeout); */
		if(eye_led_display_timeout)	
		{
			eye_led_display_timeout --;
		}
		else
		{
			if(last_eye_display != EFFECT_UNACTIVE)
			{
				/* printf("turn back lask display\n"); */
				EyeEffectCtl(last_eye_display);
				last_eye_display = EFFECT_UNACTIVE;
			}
		}
	}
}


void EyeLedInit(void)
{
	s32 ret;
	ret = timer_reg_isr_fun(timer0_hl,1,EyeLedScan,NULL);
	if(ret != TIMER_NO_ERR)
	{
		printf("led_scan err = %x\n",ret);
	}

	ret = timer_reg_isr_fun(timer0_hl,50,eye_led_api_scan,NULL);
	if(ret != TIMER_NO_ERR)
	{
		printf("eye_led_api_scan err = %x\n",ret);
	}
}
