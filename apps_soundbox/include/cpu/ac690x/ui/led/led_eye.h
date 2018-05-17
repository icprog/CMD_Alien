#ifndef _LED_EYE_H
#define _LED_EYE_H
#define LED_Y1_ON()  do{JL_PORTA->DIR&=~BIT(15);JL_PORTA->OUT&=~BIT(15);}while(0)
#define LED_Y2_ON()  do{JL_PORTC->DIR&=~BIT(1);JL_PORTC->OUT&=~BIT(1);}while(0)
#define LED_Y3_ON()  do{JL_PORTC->DIR&=~BIT(2);JL_PORTC->OUT&=~BIT(2);}while(0)
#define LED_Y4_ON()  do{JL_PORTC->DIR&=~BIT(3);JL_PORTC->OUT&=~BIT(3);}while(0)
#define LED_Y5_ON()  do{JL_PORTC->DIR&=~BIT(4);JL_PORTC->OUT&=~BIT(4);}while(0)
#define LED_Y6_ON()  do{JL_PORTC->DIR&=~BIT(5);JL_PORTC->OUT&=~BIT(5);}while(0)

#define LED_X1_ON()  do{JL_PORTA->DIR&=~BIT(14);JL_PORTA->OUT|=BIT(14);}while(0)
#define LED_X2_ON()  do{JL_PORTC->DIR&=~BIT(0);JL_PORTC->OUT|=BIT(0);}while(0)
#define LED_X3_ON()  do{JL_PORTA->DIR&=~BIT(11);JL_PORTA->OUT|=BIT(11);}while(0)
#define LED_X4_ON()  do{JL_PORTA->DIR&=~BIT(10);JL_PORTA->OUT|=BIT(10);}while(0)
#define LED_X5_ON()  do{JL_PORTA->DIR&=~BIT(9);JL_PORTA->OUT|=BIT(9);}while(0)

#define EYELED_ALL_OFF() do{JL_PORTA->DIR&=~(BIT(9)|BIT(10)|BIT(11)|BIT(14)|BIT(15));\
						JL_PORTA->OUT&=~(BIT(9)|BIT(10)|BIT(11)|BIT(14));\
						JL_PORTA->OUT|=BIT(15);\
						JL_PORTC->DIR&=~(BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5));\
						JL_PORTC->OUT&=~(BIT(0));\
						JL_PORTC->OUT|=(BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5));\
						}while(0)

/***************列数量**********************/
#define LED_COLUMN_NUM 6

typedef struct __led_picture
{
	volatile u8 snakeen;
	volatile u8 pictureNum;
	volatile u16 changeTime;
	volatile const u8 (*picture)[LED_COLUMN_NUM+1];
	u8 (*timeChangeNum_cb)(u8 *val);
}_led_picture;

typedef struct __led_picture_ctl
{
	volatile u8 curPictrue;
	volatile u16 timeCnt;
	volatile _led_picture *led_picture;
}_led_picture_ctl;

typedef enum
{
	EFFECTCLOSEALLLED = 0x0,
	EFFECTOPENALLLED,
	EFFECTLOVE,
	EFFECTSMILE,
	EFFECTSHY,
	EFFECTMEDITATION,
	EFFECTSURPRISE,
	EFFECTANGER,
	EFFECTSAD,
	EFFECTJIONG,
	EFFECTDIZZY,
	EFFECTCRY,
	EFFECTLOVETEST,


	EFFECT_POWER_ON,
	EFFECT_POWER_OFF,
	EFFECT_LOW_POWER_WARMING,
	EFFECT_PLAY,
	EFFECT_PAUSE,
	EFFECT_PREV,
	EFFECT_NEXT,
	EFFECT_NO_DEV,
	EFFECT_NO_FILE,
	EFFECT_CONNECT,
	EFFECT_CONNECT_OK,
	EFFECT_CONNECT_FAIL,
	EFFECT_NO_CONNECT,
	EFFECT_SPEECH_INPUT,
	EFFECT_SPEECH_RESULT_OK,
	EFFECT_SPEECH_RESULT_FAIL,
	EFFECT_ALARM_ON,
	EFFECT_UPDATE_ING,
	EFFECT_SEARCH_ING,


	EFFECTMAX,
	EFFECT_UNACTIVE,
}_EYEEFFECT;
void EyeLedInit(void);
/************三角形效果外调接口***********/
void EffectTriangle(void);
/************正方形效果外调接口***********/
void EffectSquare(void);
/************眼睛效果外调接口***********/
void EffectEye(void);
/************贪吃蛇慢速效果外调接口***********/
void EffectSnakeSlow(void);
/************贪吃蛇正常速度效果外调接口***********/
void EffectSnakeNormal(void);
/************贪吃蛇快速效果外调接口***********/
void EffectSnakeFast(void);
/************音柱慢速效果外调接口***********/
void EffectSoundColumnSlow(void);
/************音柱正常速度效果外调接口***********/
void EffectSoundColumnNormal(void);
/************音柱快速效果外调接口***********/
void EffectSoundColumnFast(void);
/************关闭所有led外调接口***********/
void EffectCloseAllLed(void);
/************圆形led外调接口***********/
void EffectAround(void);
/****************所有效果控制接口********************/
void EyeEffectCtl(_EYEEFFECT eyeEffect);
_EYEEFFECT EyeEffect_get_cur(void);

void eye_led_api(_EYEEFFECT type, u16 timeout, u8 flag);
void eye_led_set_back_api(_EYEEFFECT type);

#endif

