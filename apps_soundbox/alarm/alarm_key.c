#include "alarm_key.h"
#include "common/msg.h"

/*******************************************************************
                            AD������
*******************************************************************/
#define ADKEY_ALARM_SHORT		\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,

#define ADKEY_ALARM_LONG		\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,


#define ADKEY_ALARM_HOLD		\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,

#define ADKEY_ALARM_LONG_UP	\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,

const u8 alarm_ad_table[4][10] =
{
    /*�̰�*/	    {ADKEY_ALARM_SHORT},
    /*����*/		{ADKEY_ALARM_LONG},
    /*����*/		{ADKEY_ALARM_HOLD},
    /*����̧��*/	{ADKEY_ALARM_LONG_UP},
};

/*******************************************************************
                            I/O������
*******************************************************************/
#define IOKEY_ALARM_SHORT		\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,

#define IOKEY_ALARM_LONG		\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,


#define IOKEY_ALARM_HOLD		\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,

#define IOKEY_ALARM_LONG_UP	\
                        /*00*/    MSG_ALARM_OFF,\
                        /*01*/    MSG_ALARM_OFF,\
                        /*02*/    MSG_ALARM_OFF,\
                        /*03*/    MSG_ALARM_OFF,\
                        /*04*/    MSG_ALARM_OFF,\
                        /*05*/    MSG_ALARM_OFF,\
                        /*06*/    MSG_ALARM_OFF,\
                        /*07*/    MSG_ALARM_OFF,\
                        /*08*/    MSG_ALARM_OFF,\
                        /*09*/    MSG_ALARM_OFF,

const u8 alarm_io_table[4][10] =
{
    /*�̰�*/	    {IOKEY_ALARM_SHORT},
    /*����*/		{IOKEY_ALARM_LONG},
    /*����*/		{IOKEY_ALARM_HOLD},
    /*����̧��*/	{IOKEY_ALARM_LONG_UP},
};

/*******************************************************************
                            IR������
*******************************************************************/
#define IRFF00_ALARM_KEY_SHORT			\
                                /*00*/    MSG_POWER_OFF,\
							    /*01*/    MSG_CHANGE_WORKMODE,\
								/*02*/    MSG_MUTE,\
								/*03*/    MSG_AUX_MUTE,\
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


#define IRFF00_ALARM_KEY_LONG			\
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

#define IRFF00_ALARM_KEY_HOLD			\
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


#define IRFF00_ALARM_KEY_LONG_UP 		\
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
const u8 alarm_ir_table[4][21] =
{
    /*�̰�*/	    {IRFF00_ALARM_KEY_SHORT},
    /*����*/		{IRFF00_ALARM_KEY_LONG},
    /*����*/		{IRFF00_ALARM_KEY_HOLD},
    /*����̧��*/	{IRFF00_ALARM_KEY_LONG_UP},
};


/*******************************************************************
                            touchkey������
*******************************************************************/
#define TOUCHKEY_ALARM_SHORT		\
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

#define TOUCHKEY_ALARM_LONG		\
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


#define TOUCHKEY_ALARM_HOLD		\
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

#define TOUCHKEY_ALARM_LONG_UP	\
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

const u8 alarm_touch_table[4][10] =
{
    /*�̰�*/	    {TOUCHKEY_ALARM_SHORT},
    /*����*/		{TOUCHKEY_ALARM_LONG},
    /*����*/		{TOUCHKEY_ALARM_HOLD},
    /*����̧��*/	{TOUCHKEY_ALARM_LONG_UP},
};
