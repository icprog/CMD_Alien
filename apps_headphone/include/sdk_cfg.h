/*********************************************************************************************
    *   Filename        : sdk_cfg.h

    *   Description     : Config for Head Phone Case

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2016-12-01 17:26

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

#ifndef _CONFIG_
#define _CONFIG_

#include "includes.h"



///�����ջ��С���ò���
#define VM_TASK_STACK_SIZE          (1024 * 1)
#define MAIN_TASK_STACK_SIZE        (1024 * 2) //(1024 * 1)����
#define MUSIC_TASK_STACK_SIZE       (1024 * 4)
#define MUSIC_PHY_TASK_STACK_SIZE   (1024 * 4)
#define TONE_TASK_STACK_SIZE        (1024 * 4)
#define TONE_PHY_TASK_STACK_SIZE    (1024 * 2)
#define UI_TASK_STACK_SIZE          (1024 * 2)

//���ö���
///<�����Զ�ʹ��,�Զ���ʱ������ble
#define  BT_TWS                       0 
///�Զ��ӻ������Ƿ���뱻���ӻ��ǻ���״̬
#define    TWS_SLAVE_WAIT_CON         1  
///�Զ������Ƿ�һ��ػ�
#define    BT_TWS_POWEROFF_TOGETHER   1 

///�Զ�û���ӳɹ�֮ǰ��ͨ�����������ɷ���ʹ��,������ͬʱ������԰����Ž������
#define    BT_TWS_SCAN_ENBLE         0 

//charge enable,C�����ϵ�оƬ�ſ���
#define CHARGE_PROTECT_EN    0

#if CHARGE_PROTECT_EN
	#define KEEP_BT_CONNECT  0
#endif

/********************************************************************************/
/*
 *          --------����������
 */
////<����ϵͳ��ӡ���Թ���
#define __DEBUG         

#ifdef __DEBUG
	// #define USE_USB_DM_PRINTF
#endif

///<LEDָʾʹ��
#define LED_EN                  1       
///<��������
#define UART_UPDATA_EN          0      
///<�������
#define SYS_LVD_EN              1       

///<����˫������
#define KEY_DOUBLE_CLICK             1  
/********************************************************************************/

/********************************************************************************/
/*
 *           --------��Դ������
 */
///   0:  no change   
#define    PWR_NO_CHANGE        0     
///   1:  LDOIN 5v -> VDDIO 3.3v -> DVDD 1.2v
#define    PWR_LDO33            1     
///   2:  LDOIN 5v -> LDO   1.5v -> DVDD 1.2v, support bluetooth
#define    PWR_LDO15            2     
///   3:  LDOIN 5v -> DCDC  1.5v -> DVDD 1.2v, support bluetooth
#define    PWR_DCDC15           3     

///��ѡ���ã�PWR_NO_CHANGE/PWR_LDO33/PWR_LDO15/PWR_DCDC15
#define PWR_MODE_SELECT         PWR_DCDC15

//DCDCʱBTAVDD,��LDOģʽʱ���Ժ��� 0:1.61v  1:1.51v  2:1.43v  3:1.35v
//AC6904��װҪ��1.51v,��Ӧ����Ϊ1
#define VDC15_LEV     3      

//0:0.703V  1:0.675V  2:0.631V  3:0.592V
//4:0.559V  5:0.527V  6:0.493V  7:0.462V 
//LC6904��װ��������Ϊ5
#define POWER_DOWN_DVDD_LEV      7

///�����������Զ��ػ���ʱ��u16���ͣ�0��ʾ���Զ��ػ�
#define AUTO_SHUT_DOWN_TIME     (3*2*60)          

///<��ص����ͣ��Ƿ��л���Դ�������
#define SWITCH_PWR_CONFIG		0
/*
	SYS_LDO_Level:3.53v-3.34v-3.18v-3.04v-2.87v-2.73v-2.62v-2.52v
	FM_LDO_Level:3.3v-3.04v-2.76v-2.5v
*/
///<Normal LDO level
#define SYS_LDO_NORMAL_LEVEL	2	//range:0~7:FM_LDO��VDDIO��һ�𣬽���:level=2
#define FM_LDO_NORMAL_LEVEL		0	//range:0~3
///<Reduce LDO level
#define SYS_LDO_REDUCE_LEVEL	3	//range:0~7
#define FM_LDO_REDUCE_LEVEL		1	//range:0~3

/********************************************************************************/

/********************************************************************************/
/*
 *           --------DAC VCOMO ����
 */
///�Ƿ�ѡ��VCMOֱ�ƶ���
#define VCOMO_EN 	            1	
///��ѡ���ã�DAC_L_R_CHANNEL/DAC_L_CHANNEL/DAC_R_CHANNEL  
#define DAC_CHANNEL_SLECT       DAC_L_R_CHANNEL

//DAC ���������IIS
#define DAC2IIS_EN                      0     //1:ʹ��DAC������IIS�Թ̶�������44.1K�����
  #if(DAC2IIS_EN == 1) 
     #define DAC2IIS_OUTCLK_AUTO_CLOSE  O    //1:��û������ʱ���Զ��ر�MCLK,SCLK,LRCLKʱ������� 0:���ر�
     #define IISCHIP_WM8978_EN          1    //1: ʹ��WM8978 IISоƬ��
  #else
     #define DAC2IIS_OUTCLK_AUTO_CLOSE  0    //1:��û������ʱ���Զ��ر�MCLK,SCLK,LRCLKʱ������� 0:���ر�
     #define IISCHIP_WM8978_EN          0    //1: ʹ��WM8978 IISоƬ��
  #endif
/********************************************************************************/

/********************************************************************************/
/*
 *           --------��Ч������
 */
///<EQģ�鿪��
#define EQ_EN			        1       
///<EQ uart online debug
#define EQ_UART_BDG	    		0       
///<dac�����ϲ�
#define DAC_SOUNDTRACK_COMPOUND 0       
///<�Զ�mute
#define DAC_AUTO_MUTE_EN		1       
///<������
#define KEY_TONE_EN     	    1       
///<��0��ʾʹ��Ĭ������
#define SYS_DEFAULT_VOL         0      

///�绰�����͹Ҷ�������ʶ��  
#define  USE_HTK                0             

///1:����������ʾ������������ʶ��  0��һֱ������ʾ����������ʶ��,һֱ����ʾ��Ҫ��ϵͳʱ����ߵ�160M����
#define TONE_AFTER_HTK_START    1

/********************************************************************************/


/********************************************************************************/
/*
 *           --------����������
 */
#define SDMMC0_EN       0
#define SDMMC1_EN       0
#define USB_DISK_EN     0
#define USB_PC_EN       0

/********************************************************************************/




/********************************************************************************/
/*
 *           --------����������
 */
#include "bluetooth/bluetooth_api.h"

#define NFC_EN          0  ///<NFC ENABLE

///��ѡ���ã�NORMAL_MODE/TEST_BQB_MODE/TEST_FCC_MODE/TEST_FRE_OFF_MODE/TEST_BOX_MODE
#define BT_MODE             NORMAL_MODE

#if ((BT_MODE == TEST_FCC_MODE)||(BT_MODE == TEST_BQB_MODE))
  #undef PWR_MODE_SELECT
  #define PWR_MODE_SELECT         PWR_LDO15
#endif
#if (BT_MODE == TEST_FCC_MODE)
  #undef UART_UPDATA_EN
  #define UART_UPDATA_EN    0
#endif

///ģ������
#define BT_ANALOG_CFG       0
#define BT_XOSC             0//

///<������̨
#define BT_BACKMODE         0             
///dependency
#if (BT_BACKMODE == 0)
    ///<HID���յĶ���ģʽֻ֧�ַǺ�̨
    #define BT_HID_INDEPENDENT_MODE  0    
#endif

#if USE_HTK
///<���籨��
  #define BT_PHONE_NUMBER     0          
#else
   #define BT_PHONE_NUMBER     1          
#endif

///<����K�豦
#define BT_KTV_EN			0

/*
 *           --------�����͹������� 
 */
///��ѡ���ã�SNIFF_EN/SNIFF_TOW_CONN_ENTER_POWERDOWN_EN
#define SNIFF_MODE_CONF    SNIFF_EN

///��ѡ���ã�BT_POWER_DOWN_EN/BT_POWER_OFF_EN
#define BT_LOW_POWER_MODE  BT_POWER_DOWN_EN //BT_POWER_OFF_EN

#define BT_OSC              0
#define RTC_OSCH            1
#define RTC_OSCL            2

///��ѡ���ã�BT_OSC/RTC_OSCH/RTC_OSCL
#define LOWPOWER_OSC_TYPE   BT_OSC

///��ѡ���ã�32768L//24000000L
#define LOWPOWER_OSC_HZ     24000000L

///��ѡ���ã�BT_BREDR_EN/BT_BLE_EN/(BT_BREDR_EN|BT_BLE_EN)
#define BLE_BREDR_MODE      (BT_BREDR_EN)

///TWS �̶����Ҷ�ѡ�� ��ѡ���ã�TWS_CHANNEL_LEFT/TWS_CHANNEL_RIGHT)
#define    BT_TWS_CHANNEL_SELECT         0 ////�̶����Ҷ�ʱ,���Ϊ��,�Ҷ��������������

/********************************************************************************/

#if ((BT_LOW_POWER_MODE==BT_POWER_DOWN_EN)&&(BLE_BREDR_MODE&BT_BLE_EN))
    #define POWER_TIMER_HW_ENABLE //�ù�����Ҫ��Ϊ��Ҫ����powerdown���ܷ����ṩ��Ϊ׼ȷ��timer
#endif

#ifdef POWER_TIMER_HW_ENABLE

#define TIMETICK_UNIT					 (10)//�̶�ֵ. ���ɸ�
#define TIMETICK_STEP					 (5)//��0ֵ

#define POWER_TIMER_HW_UNIT				 (TIMETICK_UNIT * TIMETICK_STEP)
#else//POWER_TIMER_HW_ENABLE

#define TIMETICK_UNIT					 (10)//�̶�ֵ. ���ɸ�
#define TIMETICK_STEP					 (1)//�̶�ֵ. ���ɸ�

#endif//POWER_TIMER_HW_ENABLE





/********************************************************************************/
/*
 *           --------оƬ��װ���� 
 */
///RTCVDD��û�а����Ҫ��1,Ŀǰ��Ӧ��װоƬAC6905
#define RTCVDD_TYPE              0          
#define BTAVDD_TYPE              0          

/********************************************************************************/

/********************************************************************************/
/*
 *           --------MUSIC MACRO
 */
//SMP�����ļ�֧��
#define MUSIC_DECRYPT_EN 		1	
//AB�ϵ�֧��
#define MUSIC_AB_RPT_EN 		1	

///<MP3
#define DEC_TYPE_MP3_ENABLE     1
///<SBC
#define DEC_TYPE_SBC_ENABLE     1
///<AAC
#define DEC_TYPE_AAC_ENABLE		0

///<3K_code_space
#define DEC_TYPE_WAV_ENABLE     1
///<5K_code_space
#define DEC_TYPE_FLAC_ENABLE    0
///<8K_code_space
#define DEC_TYPE_APE_ENABLE     0
///<30K_code_space
#define DEC_TYPE_WMA_ENABLE     0
///<30K_code_space
#define DEC_TYPE_F1A_ENABLE     0

/********************************************************************************/

/********************************************************************************/
/*
 *           --------FM MACRO
 */
///<��׼SDK
#define FM_RADIO_EN         0       
///dependency
#if (FM_RADIO_EN == 1)
    ///<RDA5807FM
    #define RDA5807                 0       
    ///<BK1080FM
    #define BK1080                  0       
    ///<QN8035FM
    #define QN8035                  0       
    ///<оƬ�ڲ�FM
    #define FM_INSIDE               1       
#endif

/********************************************************************************/

/********************************************************************************/
/*
 *           --------ECHO MACRO
 */
///dependency
#if (BT_BACKMODE == 1)
    ///<��֧����������̨��������
    #define ECHO_EN             0       
    ///<����ģʽ ��׼SDK
    #define ECHO_TASK_EN        0       
#else
    ///<���칦�� ��׼SDK
    #define ECHO_EN             0       
    ///<����ģʽ ��׼SDK
    #define ECHO_TASK_EN        0       
#endif

/********************************************************************************/

/********************************************************************************/
/*
 *           --------RTC MACRO
 */
///<��׼SDK RTCʱ��ģʽ
#define RTC_CLK_EN          0       
///dependency
#if (RTC_CLK_EN == 1)
    ///<RTC����ģʽ
    #define RTC_ALM_EN          1       
#endif

/********************************************************************************/

/********************************************************************************/
/*
 *           --------REC MACRO
 */
///dependency
#if (BT_BACKMODE == 1)
    ///<��֧����������̨����¼��
    #define REC_EN             0    
#else
    ///<��׼SDK¼������
    #define REC_EN             0    
#endif

#if (REC_EN == 1)
    ///<����¼��ʹ��
	#define BT_REC_EN		1       
    ///<MIC¼��ʹ��
	#define MIC_REC_EN		1       
    ///<FM¼��ʹ��
	#define FM_REC_EN		1       
    ///<AUX¼��ʹ��
	#define AUX_REC_EN		1       
#endif

/********************************************************************************/

/********************************************************************************/
/*
 *           --------UI MACRO
 */
///<UI_��ʾ
#define UI_ENABLE                0     
///dependency
#if (UI_ENABLE == 1)
    #define LCD_128X64_EN        1      ///<lcd ֧��
    #define LED_7_EN             0      ///<led ֧��
#else
    ///����֧��
    #define LCD_128X64_EN        0
    #define LED_7_EN             0
#endif 
#if ((LCD_128X64_EN == 1) && (LED_7_EN == 1))
#error  "UI driver support only one"
#endif

#if (LCD_128X64_EN == 1)
    ///�˵���ʾ
    #define LCD_SUPPORT_MENU     1       
    ///LRC�����ʾ
    #define LRC_LYRICS_EN        1       
#else
    #define LCD_SUPPORT_MENU     0
    #define LRC_LYRICS_EN        0
#endif

/********************************************************************************/

#if (BT_MODE !=NORMAL_MODE)
  #if ((SNIFF_MODE_CONF)||(BT_LOW_POWER_MODE))
  #error  "-------BT MODE not support this config ,please check sdk_cfg.h------------"
  #endif
#endif


#endif
