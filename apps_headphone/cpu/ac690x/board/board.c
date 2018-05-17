#include "sdk_cfg.h"
#include "timer.h"
#include "key_drv/key.h"
#include "key_drv/key.h"
#include "common/flash_cfg.h"
#include "vm/vm_api.h"
#include "rtos/os_api.h"
#include "common/common.h"
#include "clock.h"
#include "sys_detect.h"
#include "uart.h"
#include "hw_cpu.h"
#include "cpu.h"
#include "dac/dac.h"
#include "dac/ladc.h"
#include "irq_api.h"
#include "uart_api.h"
#include "uart_param.h"
#include "rtc/rtc_api.h"
#include "update.h"
#include "charge.h"
#include "timer/power_timer.h"
#define FLASH_BASE_ADDR 0x1000000

extern OS_TCB  *OSTCBCur[1];
extern u8 vmm_fat[192];
extern u32 pm_usd_map[(MAX_SDRAM_SIZE + ((32*PAGE_SIZE)-1))/(32*PAGE_SIZE)];
extern void set_sydf_header_base(u32 base);
extern void set_cache_base(u32 base);
extern void bt_app_cfg();
extern void set_bt_osc_internal_cfg(u8 sel_l,u8 sel_r );
extern void set_pwrmd(u8 sm);
extern u8 read_reset_power();
extern int main_task_init(void);
extern int TaskBtTest();

APP_USE_FLASH_SYS_CFG app_use_flash_cfg;

AT(.ver_magic_start)
const char ver_start_magic[] = {0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x5F, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00};

AT(.ver_magic_end)
const char ver_end_magic[] = {0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x5F, 0x65, 0x6E, 0x64, 0x00};


const u8 irq_index_2_prio_tab[][2] = 
{
	{IRQ_TIME0_IDX      ,0},
	{IRQ_TIME1_IDX      ,0},
	{IRQ_TIME2_IDX      ,0},
	{IRQ_TIME3_IDX      ,0},
	{IRQ_USB_SOF_IDX    ,1},
	{IRQ_USB_CTRL_IDX   ,1},
	{IRQ_RTC_IDX        ,0},
	{IRQ_ALINK_IDX      ,2},
	{IRQ_DAC_IDX        ,2},
	{IRQ_PORT_IDX       ,0},
	{IRQ_SPI0_IDX       ,0},
	{IRQ_SPI1_IDX       ,0},
	{IRQ_SD0_IDX        ,0},
	{IRQ_SD1_IDX        ,0},
	{IRQ_UART0_IDX      ,0},
	{IRQ_UART1_IDX      ,0},
	{IRQ_UART2_IDX      ,0},
	{IRQ_PAP_IDX        ,0},
	{IRQ_IIC_IDX        ,0},
	{IRQ_SARADC_IDX     ,0},
	{IRQ_FM_HWFE_IDX    ,1},
	{IRQ_FM_IDX         ,1},
	{IRQ_FM_LOFC_IDX    ,1},
	{IRQ_BREDR_IDX      ,2},
	{IRQ_BT_CLKN_IDX    ,2},
	{IRQ_BT_DBG_IDX     ,1},
	{IRQ_BT_PCM_IDX     ,2},
	{IRQ_SRC_IDX        ,2},
	{IRQ_EQ_IDX         ,2},
	{IRQ_BLE_IDX        ,2},
	{IRQ_NFC_IDX        ,1},
	{IRQ_CTM_IDX		,1},
	{IRQ_SOFT0_IDX      ,0},
	{IRQ_SOFT_IDX       ,0},
	{IRQ_EXCEPTION_IDX  ,0},
};

/*----------------------------------------------------------------------------*/
/**@brief  �쳣�����жϷ������
   @param
   @return
   @note
 */
/*----------------------------------------------------------------------------*/
void exception_isr(void )
{
    u32 tmp;

    ///printf rets & reti
    asm("mov %0, RETS " : "=r" (tmp));

	printf("\nRETS=0x%x\n", tmp);

	printf("DEBUG_MSG = 0x%x\n", JL_DEBUG->MSG);
	printf("PRP MMU_MSG = 0x%x\n", JL_DEBUG->PRP_MMU_MSG);
	printf("LSB MMU MSG = 0x%x\n", JL_DEBUG->LSB_MMU_MSG_CH);
	printf("PRP WR LIMIT MSG = 0x%x\n", JL_DEBUG->PRP_WR_LIMIT_MSG);
	printf("LSB WR LIMIT MSG = 0x%x\n", JL_DEBUG->LSB_WR_LIMIT_CH);

	asm("mov %0, RETI " : "=r" (tmp));

	printf("\nRETI=0x%x\n", tmp);
//	printf_buf((u8 *)MMU_TAG_ADDR0, 256);
     //JL_POWER->CON |= BIT(4); //�쳣����λ
    while(1);
    /* JL_POWER->CON |= BIT(4); //�쳣����λ */
}
/* IRQ_REGISTER(IRQ_EXCEPTION_IDX, exception_isr);
 */

/*----------------------------------------------------------------------------*/
/**@brief  poweroff_ram_fun
   @param
   @return
   @note

 */
/*----------------------------------------------------------------------------*/
void poweroff_ram_fun()
{
    // close_wdt();//�ؿ��Ź�
    uart_init(UART_BAUD_RAE); //<���ڲ�����
    delay(10000);
//    putchar('X');
//    putchar('K');
//    putchar('O');
    //puts("\n  ------ run ram ------- \n");
}


/*$PAGE*/


void set_g_sys_cfg(FLASH_SYS_CFG *sys_cfg,u32 cfg_addr)
{
    u8 cfg_data[16];
    u8 *cfg_ptr;
    u32 i;
    printf("cfg_addr =  %08x \n",cfg_addr);
    memcpy(sys_cfg, (void*)(FLASH_BASE_ADDR + cfg_addr), sizeof(FLASH_SYS_CFG));
    app_use_flash_cfg.cfg_zone_addr = sys_cfg->flash_cfg.cfg_zone_addr;
    app_use_flash_cfg.flash_size = sys_cfg->flash_cfg.flash_size;
    app_use_flash_cfg.cfg_zone_size = sys_cfg->flash_cfg.cfg_zone_size;
   
    printf("cfg_zone_addr =%08x \n",app_use_flash_cfg.cfg_zone_addr);
    printf("cfg_zone_size =%08x \n",app_use_flash_cfg.cfg_zone_size);

    cfg_ptr=(u8*)(FLASH_BASE_ADDR+app_use_flash_cfg.cfg_zone_addr);
    for(i=0;i<app_use_flash_cfg.cfg_zone_size;i+=16)
    {
        u32 *tmp;
        memcpy(cfg_data,cfg_ptr,16);
        printf_buf(cfg_data,16);
        if(memcmp((const char *)cfg_data,"BTIF",4)==0)
        {
            tmp=(u32 *)(cfg_data+4);
            app_use_flash_cfg.btif_addr=*tmp;
            tmp=(u32 *)(cfg_data+8);
            app_use_flash_cfg.btif_len=*tmp;
            printf("btif_addr =%08x \n",app_use_flash_cfg.btif_addr);
            printf("btif_len =%08x \n",app_use_flash_cfg.btif_len);
        }
        else if(memcmp((const char *)cfg_data,"VMIF",4)==0)
		{
			tmp=(u32 *)(cfg_data+4);
			app_use_flash_cfg.vmif_addr=*tmp;
			tmp=(u32 *)(cfg_data+8);
			app_use_flash_cfg.vmif_len=*tmp;
			printf("VM_addr =%08x \n",app_use_flash_cfg.vmif_addr);
			printf("VM_len =%08x \n",app_use_flash_cfg.vmif_len);
		}
		else
		{
			tmp=(u32 *)(cfg_data+4);
			printf("USER_addr =%08x \n",*tmp);
			tmp=(u32 *)(cfg_data+8);
			printf("USER_len =%08x \n",*tmp);
		}
        cfg_ptr+=16;
    } 

#if 0
    printf("flash_id : %x\n",   sys_cfg->flash_cfg.flash_id);
    printf("flash_size : %x\n",   sys_cfg->flash_cfg.flash_size);
    printf("flash_file_size : %x\n",   sys_cfg->flash_cfg.flash_file_size);
    printf("sdfile_head_addr : %x\n",   sys_cfg->flash_cfg.sdfile_head_addr);
    printf("spi_run_mode : %x\n",   sys_cfg->flash_cfg.spi_run_mode);
    printf("spi_div : %x\n",   sys_cfg->flash_cfg.spi_div);
    printf("flash_base : %x\n",   sys_cfg->flash_cfg.flash_base);
    printf("protected_arg : %x\n",   sys_cfg->flash_cfg.protected_arg);
    printf("cfg_zone_addr : %x\n",   sys_cfg->flash_cfg.cfg_zone_addr);
    printf("cfg_zone_size : %x\n",   sys_cfg->flash_cfg.cfg_zone_size);
/*
    printf("sdram_refresh_cycle : %x\n",   sys_cfg->sdr_cfg.sdram_refresh_cycle);
    printf("sdram_refresh_time : %x\n",   sys_cfg->sdr_cfg.sdram_refresh_time);
    printf("sdram_cas_latency : %x\n",   sys_cfg->sdr_cfg.sdram_cas_latency);
    printf("sdram_trfc : %x\n",   sys_cfg->sdr_cfg.sdram_trfc);
    printf("sdram_trp : %x\n",   sys_cfg->sdr_cfg.sdram_trp);
    printf("sdram_trcd : %x\n",   sys_cfg->sdr_cfg.sdram_trcd);
    printf("sdram_colum : %x\n",   sys_cfg->sdr_cfg.sdram_colum);
    printf("sdram_clk_hd_en : %x\n",   sys_cfg->sdr_cfg.sdram_clk_hd_en);
    printf("sdram_trrd : %x\n",   sys_cfg->sdr_cfg.sdram_trrd);
    printf("sdram_clkoe : %x\n",   sys_cfg->sdr_cfg.sdram_clkoe);
    printf("sdram_size : %x\n",   sys_cfg->sdr_cfg.sdram_size);
    printf("sdram_fast_clock_rate : %x\n",   sys_cfg->sdr_cfg.sdram_fast_clock_rate);
*/
    printf("pll_sel : %x\n",   sys_cfg->clk_cfg.pll_sel);
    printf("osc_freq : %x\n",   sys_cfg->clk_cfg.osc_freq);
    printf("osc_src :%x\n",   sys_cfg->clk_cfg.osc_src);
    printf("osc_hc_en : %x\n",   sys_cfg->clk_cfg.osc_hc_en);
    printf("osc_1pin_en : %x\n",   sys_cfg->clk_cfg.osc_1pin_en);
    printf("---------------------------------------------\r\n");
#endif
}

void wake_io_check(void)
{
	u8 reg;
 	reg = get_wake_up_io();
	printf("wake up io:%x\n",reg);

    if(reg & BIT(0))
    {
		puts("-WK_PR0\n");        
    }
    else if(reg & BIT(1))
    {
		puts("-WK_PR1\n");       
    }
    else if(reg & BIT(2))
    {
		puts("-WK_PR2\n");  
    }
    else if(reg & BIT(3))
    {
		puts("-WK_PR3\n");  
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  system start enter
   @param  cfg_addr - g_sys_cfg's addr �� addr - app_file's addr��res - no used
   @return
   @note

 */

/*----------------------------------------------------------------------------*/
extern void power_init(u8 mode , u32 sys_freq , u8 charge_flag);
extern void set_poweroff_wakeup_io();
extern void set_poweroff_wakeup_io_handle_register(void (*handle)(),void (*sleep_io_handle)(),void (*sleep_handle)());
extern void low_power_mode(u16 wakeup_cfg , u8 wakeup_edge);
extern void set_sleep_mode_wakeup_io();
extern void set_sleep_before_close_irq();
extern void enter_sleep_mode();
extern void set_sys_ldo_level(u8 level);
extern void set_use_vdc15_level(u8 level);
void  board_main(u32 cfg_addr, u32 addr, u32 res,u32 update_flag)
{
    u32 tmp;
    FLASH_SYS_CFG sys_cfg;

	irq_index_tab_reg((void *)irq_index_2_prio_tab, sizeof(irq_index_2_prio_tab));

	update_check(update_flag);

	set_bt_osc_internal_cfg(0x11,0x11);/*internal capacitor range:0x00~0x1F,ʱ�ӳ�ʼ��ǰ����*/
    clock_init_app(SYS_CLOCK_IN, OSC_Hz, INIT_SYS_Hz);

	open_wdt(0x0f);			//set wd 32s
    uart_init(UART_BAUD_RAE); //<���ڲ�����

    /* printf("SFC_BAUD=%d\n", JL_SFC->BAUD); */
    /* printf("hsb_clk_hz= %u HZ\n", clock_get_sys_freq()); */
    /* printf("lsb_clk_hz= %u HZ\n", clock_get_lsb_freq()); */

	puts("\n\n***************************AC69_PowerUp**********************************\n");

	//�鿴�ĸ�PR�ڻ���softpoweroff
	wake_io_check();

    u8 *protect_info;
    protect_info=(u8 *)0x40000;
    printf_buf(protect_info,32);

    set_g_sys_cfg(&sys_cfg, cfg_addr);

	__asm__ volatile ("mov %0,ie0" : "=r"(tmp));
	printf("ie0 =%08x \n",tmp);
	__asm__ volatile ("mov %0,icfg" : "=r"(tmp));
	printf("icfg =%08x \n",tmp);
	
	/* printf("syd_cfg_addr=0x%x\n", cfg_addr); */
    set_sydf_header_base(sys_cfg.flash_cfg.sdfile_head_addr);

    set_cache_base(FLASH_BASE_ADDR);

    if(vm_init_api(&sys_cfg.flash_cfg))
	{
		while(1)
		{
		   puts("vm_init_api err\n");	
		}
	}

    vm_open_all();
    bt_app_cfg();

#ifdef USE_USB_DM_PRINTF  
	JL_IOMAP->CON1 &= ~(BIT(8)|BIT(9)|BIT(10));

	USB_DM_PU(1);
	USB_DM_PD(1);
	USB_DM_DIR(0);
#endif

    printf("--JL_POWER->CON-- : 0x%x\n", JL_POWER->CON>>5);//0:PWR_ON 1:LVD 2:WDT 3:SRST 4:POWEROFF 

	/* 2:LDO  3:DCDC*/
	set_use_vdc15_level(VDC15_LEV);
	power_init(PWR_MODE_SELECT , INIT_SYS_Hz , CHARGE_PROTECT_EN);

	/*LDOģʽĬ��VDDIO��3.3V DCDCģʽĬ����3.0V*/
	set_sys_ldo_level(SYS_LDO_NORMAL_LEVEL); //include VDDIO && RTC_LDO

    clock_init_app(SYS_CLOCK_IN, OSC_Hz, SYS_Hz);
	uart_set_baud(UART_BAUD_RAE);

	extern void set_power_delay(u32 sys_freq);
	set_power_delay(SYS_Hz);

	charge_mode_detect_ctl(1);

	set_poweroff_wakeup_io_handle_register(set_poweroff_wakeup_io,set_sleep_mode_wakeup_io,set_sleep_before_close_irq); 

	//extern void enter_poweroff_mode(u32 usec);
	//enter_poweroff_mode(1000000);

	if(device_is_first_start()||device_is_updata_over())
	{
		puts("device first_start or updata_over\n");	
	}
	else
	{
	   ldo5v_detect_deal(POWER_ON);
	}

    timer0_init();
    sys_init();
#ifdef POWER_TIMER_HW_ENABLE
	    power_timer_hw_init();
#endif

	/* enter_sleep_mode(); */

    main_task_init();
    OSStart();
    irq_global_enable();
    __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
    printf("ie0 =%08x \n",tmp);
    __asm__ volatile ("mov %0,ie1" : "=r"(tmp));
    printf("ie1 =%08x \n",tmp);
	__asm__ volatile ("mov %0,icfg" : "=r"(tmp));
    printf("icfg =%08x \n",tmp);
    while(1)
    {
        __asm__ volatile ("idle");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
    }
}
volatile unsigned long jiffies;
void set_jiffies_10ms(u32 timer_ms)
{
	jiffies += timer_ms / 10;
}


