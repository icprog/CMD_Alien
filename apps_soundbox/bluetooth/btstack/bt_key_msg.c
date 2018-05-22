#include "includes.h"
#include "sdk_cfg.h"
#include "bt_key_msg.h"
#include "common/msg.h"


/*�������ⰴ����*/
#define IRFF00_BT_SHORT			\
                                /*00*/    MSG_POWER_OFF,\
							    /*01*/    MSG_CHANGE_WORKMODE,\
								/*02*/    MSG_MUTE,\
								/*03*/    MSG_BT_PP,\
								/*04*/    MSG_BT_PREV_FILE,\
								/*05*/    MSG_BT_NEXT_FILE,\
								/*06*/    MSG_BT_MUSIC_EQ,\
								/*07*/    MSG_VOL_DOWN,\
								/*08*/    MSG_VOL_UP,\
								/*09*/    MSG_BT_HID_CTRL,\
                                /*10*/    MSG_BT_HID_TAKE_PIC,\
								/*11*/    NO_MSG,\
								/*12*/    NO_MSG,\
								/*13*/    NO_MSG,\
								/*14*/    NO_MSG,\
								/*15*/    NO_MSG,\
								/*16*/    NO_MSG,\
								/*17*/    NO_MSG,\
								/*18*/    NO_MSG,\
								/*19*/    NO_MSG,\
								/*20*/    NO_MSG

#define IRFF00_BT_LONG			\
								/*00*/    NO_MSG,\
							    /*01*/    NO_MSG,\
								/*02*/    NO_MSG,\
								/*03*/    NO_MSG,\
								/*04*/    NO_MSG,\
								/*05*/    NO_MSG,\
								/*06*/    NO_MSG,\
								/*07*/    MSG_ENTER_MENULIST,\
								/*08*/    MSG_ENTER_MENUMAIN,\
								/*09*/    NO_MSG,\
                                /*10*/    NO_MSG,\
								/*11*/    NO_MSG,\
								/*12*/    NO_MSG,\
								/*13*/    NO_MSG,\
								/*14*/    NO_MSG,\
								/*15*/    NO_MSG,\
								/*16*/    NO_MSG,\
								/*17*/    NO_MSG,\
								/*18*/    NO_MSG,\
								/*19*/    NO_MSG,\
								/*20*/    NO_MSG

#define IRFF00_BT_HOLD			\
								/*00*/    NO_MSG,\
							    /*01*/    NO_MSG,\
								/*02*/    NO_MSG,\
								/*03*/    NO_MSG,\
								/*04*/    NO_MSG,\
								/*05*/    NO_MSG,\
								/*06*/    NO_MSG,\
								/*07*/    MSG_VOL_DOWN,\
								/*08*/    MSG_VOL_UP,\
								/*09*/    NO_MSG,\
                                /*10*/    NO_MSG,\
								/*11*/    NO_MSG,\
								/*12*/    NO_MSG,\
								/*13*/    NO_MSG,\
								/*14*/    NO_MSG,\
								/*15*/    NO_MSG,\
								/*16*/    NO_MSG,\
								/*17*/    NO_MSG,\
								/*18*/    NO_MSG,\
								/*19*/    NO_MSG,\
								/*20*/    NO_MSG


#define IRFF00_BT_LONG_UP 		\
								/*00*/    NO_MSG,\
                                /*01*/    NO_MSG,\
								/*02*/    NO_MSG,\
								/*03*/    NO_MSG,\
								/*04*/    NO_MSG,\
								/*05*/    NO_MSG,\
								/*06*/    NO_MSG,\
								/*07*/    NO_MSG,\
								/*08*/    NO_MSG,\
								/*09*/    NO_MSG,\
								/*10*/    NO_MSG,\
								/*11*/    NO_MSG,\
								/*12*/    NO_MSG,\
								/*13*/    NO_MSG,\
                                /*14*/    NO_MSG,\
								/*15*/    NO_MSG,\
								/*16*/    NO_MSG,\
								/*17*/    NO_MSG,\
								/*18*/    NO_MSG,\
								/*19*/    NO_MSG,\
								/*20*/    NO_MSG

const u8 bt_ir_table[4][21] =				//����ң��ת����
{
    /*�̰�*/	    {IRFF00_BT_SHORT},
    /*����*/		{IRFF00_BT_LONG},
    /*����*/		{IRFF00_BT_HOLD},
    /*����̧��*/	{IRFF00_BT_LONG_UP},
};

#define ADKEY_BT_SHORT		\
						/*00  220k �׶�	 */   NO_MSG,\
						/*01  100k Сѧ	 */   NO_MSG,\
						/*02  51k  ����	 */   NO_MSG,\
						/*03  33k  Ӣ����  */    NO_MSG,\
						/*04  24K  ����Ӣ  */    NO_MSG,\
						/*05  15k  ����ת��*/ MSG_CHANGE_WORKMODE,\
						/*06  9.1K ��һ��  */    MSG_BT_PREV_FILE,\
						/*07  6.2k ��һ��  */    MSG_BT_NEXT_FILE,\
						/*08  3K   ��ͣ����*/ MSG_BT_PP,\
						/*09  0K   �����㲥*/     MSG_SPEECH_INPUT,

#define ADKEY_BT_LONG		\
                        /*00*/    NO_MSG,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    MSG_CHANGE_NET_RES_SUB_JUNIOR,\
                        /*06*/    MSG_EN_2_CH,\
                        /*07*/    MSG_CH_2_EN,\
                        /*08*/    MSG_CHANGE_NET_RES_CHILD,\
                        /*09*/    MSG_CHANGE_NET_RES_SUB_PRIMARY,


#define ADKEY_BT_HOLD		\
                        /*00*/    NO_MSG,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

#define ADKEY_BT_LONG_UP	\
                        /*00*/    NO_MSG,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

const u8 bt_ad_table[4][10] =				//AD&IO����ת����
{
    /*�̰�*/	    {ADKEY_BT_SHORT},
    /*����*/		{ADKEY_BT_LONG},
    /*����*/		{ADKEY_BT_HOLD},
    /*����̧��*/	{ADKEY_BT_LONG_UP},
};

#define IOKEY_BT_SHORT		\
                        /*00*/    MSG_BT_PP,\
                        /*01*/    MSG_BT_PREV_FILE,\
                        /*02*/    MSG_BT_NEXT_FILE,\
                        /*03*/    MSG_CHANGE_WORKMODE,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    MSG_BT_CALL_LAST_NO,

#define IOKEY_BT_LONG		\
                        /*00*/    MSG_POWER_OFF/*MSG_BT_CALL_REJECT*/,\
                        /*01*/    MSG_VOL_DOWN,\
                        /*02*/    MSG_VOL_UP,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,


#define IOKEY_BT_HOLD		\
                        /*00*/    MSG_POWER_OFF_HOLD,\
                        /*01*/    MSG_VOL_DOWN,\
                        /*02*/    MSG_VOL_UP,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

#define IOKEY_BT_LONG_UP	\
                        /*00*/    MSG_POWER_KEY_UP,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

const u8 bt_io_table[4][10] =				//AD&IO����ת����
{
    /*�̰�*/	    {IOKEY_BT_SHORT},
    /*����*/		{IOKEY_BT_LONG},
    /*����*/		{IOKEY_BT_HOLD},
    /*����̧��*/	{IOKEY_BT_LONG_UP},
};
