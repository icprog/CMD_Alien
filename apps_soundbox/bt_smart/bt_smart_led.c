#include "bt_smart_led.h"
#include "timer.h"


#define BT_SMART_LED_ENABLE
///这里默认已用JL_PORTB->DIR = ~BIT(9); 开发时需要留意，io冲突
#ifdef BT_SMART_LED_ENABLE
#define BT_SMART_LED_INIT_REG(io, x)		\
									do{	\
										JL_PORT##io->DIR &=  ~BIT(x);	\
										JL_PORT##io->PU &= ~BIT(x);	\
										JL_PORT##io->PD &= ~BIT(x);	\
									}while(0);

#define BT_SMART_LED_SET_STATUS(io, x, status)	\
											do{	\
												status ? (JL_PORT##io->OUT |= BIT(x)) : (JL_PORT##io->OUT &= ~BIT(x));	\
											}while(0);

#define BT_SMART_LED_SWAP_STATUS(io, x)	\
											do{	\
												JL_PORT##io->OUT ^= BIT(x);	\
											}while(0);
#else
#define BT_SMART_LED_INIT_REG(...)
#define BT_SMART_LED_SET_STATUS(...)
#define BT_SMART_LED_SWAP_STATUS(...)
#endif//BT_SMART_LED_ENABLE


#define  BT_SMART_LED_DEBUG_ENABLE
#ifdef BT_SMART_LED_DEBUG_ENABLE
#define bt_smart_printf	printf
#else
#define bt_smart_printf(...)
#endif//BT_SMART_LED_DEBUG_ENABLE

/* static u8 is_speex_cancel = 0; */
/* static u8 is_notice_plan = 0; */




typedef struct __BT_SMART_LED
{
	u32 		timeout_count;//ms
	u32 		timeout_threshold;
	u32 		blink_threshold;
	u32			freq;
	u32			count; 
	u32			blink_count; 
	volatile u8 mode;	
}BT_SMART_LED;



static BT_SMART_LED smart_led = 
{
	.mode = 0,		
	.count = 0,
	.blink_count = 0,
};

extern void AI_toy_led_init(void);
extern void AI_toy_led_set_status(u8 status);
extern void AI_toy_led_set_swap(void);

void bt_smart_led_on(void)
{
	/* BT_SMART_LED_SET_STATUS(B, 9, 1); */
	AI_toy_led_set_status(1);
	smart_led.mode = 1;
}


void bt_smart_led_off(void)
{
	/* BT_SMART_LED_SET_STATUS(B, 9, 0); */
	AI_toy_led_set_status(0);
	smart_led.count = 0;
	smart_led.mode = 0;
}


void bt_smart_led_flick(u32 freq, u32 timeout)
{
	freq = freq/2;

	if(smart_led.mode == 2)
	{
		if(freq == smart_led.freq && smart_led.timeout_threshold == timeout)		
		{
			/* printf("led the same param !!\n"); */
			return ;		
		}
	}

	/* BT_SMART_LED_SET_STATUS(B, 9, 1); */
	AI_toy_led_set_status(1);
	smart_led.count = 0;
	smart_led.freq = freq;
	smart_led.timeout_threshold = timeout;
	smart_led.mode = 2;
}

void bt_smart_led_blink(u32 cnt, u32 freq)
{
	bt_smart_led_off();
	freq = freq/2;
	smart_led.count = 0;
	smart_led.freq = freq;
	smart_led.blink_threshold = cnt;
	smart_led.blink_count = 0;
	smart_led.mode = 3;
}


static void bt_smart_led_scan(void *p)
{
	if(smart_led.mode == 2)	
	{
		smart_led.count++;
		if(smart_led.count >= smart_led.freq)		
		{
			smart_led.count = 0;
			/* BT_SMART_LED_SWAP_STATUS(B, 9); */
			AI_toy_led_set_swap();
		}

		if(smart_led.timeout_threshold)
		{
			smart_led.timeout_count++;
			if(smart_led.timeout_count >= smart_led.timeout_threshold)
			{
				bt_smart_printf("AI wait timeout !!!\n");
				smart_led.timeout_count = 0;	
				bt_smart_led_off();
			}
		}
	}
	else if(smart_led.mode == 3)
	{
		smart_led.count++;
		if(smart_led.count >= smart_led.freq)		
		{
			smart_led.count = 0;
			/* BT_SMART_LED_SWAP_STATUS(B, 9); */
			AI_toy_led_set_swap();
			smart_led.blink_count ++;
			if(smart_led.blink_count >= smart_led.blink_threshold)
			{
				smart_led.blink_count = 0;
				bt_smart_led_off();
			}
		}
	}
}

void bt_smart_led_init(void)
{
	/* BT_SMART_LED_INIT_REG(B, 9); */
	AI_toy_led_init();
	s32 ret = timer_reg_isr_fun(timer0_hl,1,bt_smart_led_scan, NULL);
	if(ret != TIMER_NO_ERR) {
		bt_smart_printf("reg bt_smart_led_scan err = %x\n",ret);
	}
}

