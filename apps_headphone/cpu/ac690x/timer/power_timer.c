#include "timer/power_timer.h"
#include "timer.h"
#include "cpu/power_timer_hw.h"
#include "common/sys_timer.h"
#include "sdk_cfg.h"
#include "rtos/os_api.h"

#ifdef POWER_TIMER_HW_ENABLE
///这里的timer选择要确保所选用的timer没有被使用，否则无法正常使用
#define POWER_TIMER0	0
#define POWER_TIMER1	1
#define POWER_TIMER2	2
#define POWER_TIMER3	3
#define POWER_TIMER_SEL	POWER_TIMER1

static void power_timer_unit_scan(void)
{
    /* printf("OSTIMETICK_STEP = %d\n", OSTIMETICK_STEP); */
    u32 i;
    for (i = 0; i < TIMETICK_STEP ; i++) {
        OSTimeTick();
        sys_timer_schedule();
    }
}



//允许进入powerdown的条件下， 如果要推灯， 检查下static void in_low_pwr_port_deal(u8 mode)函数的io设置,保证进入powerdown和非powerdown不被改变即可, 否则亮灯会不规则，请注意
extern void led_scan(void *param);
static void __led_scan(void)
{
   led_scan(NULL);
}

void power_timer_hw_init(void)
{
    __periodic_timer_init(POWER_TIMER_HW_UNIT, POWER_TIMER_SEL);//timer最小时间间隔单元为POWER_TIMER_HW_UNIT, 选用timer1

    periodic_timer_add(POWER_TIMER_HW_UNIT, power_timer_unit_scan);

	periodic_timer_add(POWER_TIMER_HW_UNIT, __led_scan);

}

#endif//POWER_TIMER_HW_ENABLE


