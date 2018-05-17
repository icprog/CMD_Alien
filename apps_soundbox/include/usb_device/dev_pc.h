#ifndef __DEV_PC_H__
#define __DEV_PC_H__

#include "includes.h"
#include "usb/usb_slave_api.h"

#define MASSSTORAGE_CLASS   BIT(0)
#define SPEAKER_CLASS       BIT(1)
#define MIC_CLASS           BIT(2)
#define AUDIO_CLASS         (SPEAKER_CLASS|MIC_CLASS)
#define HID_CLASS           BIT(3)
#define VENDOR_CLASS      BIT(4)
 
#if VENDOR_CLASS_ENABLE 
#define CLASS_CONFIG        (VENDOR_CLASS)
#else
#define CLASS_CONFIG        (MASSSTORAGE_CLASS|AUDIO_CLASS)
#endif
s32 app_usb_slave_init(void);
s32 app_usb_slave_close(void);
s32 app_usb_slave_card_reader(u32 cmd);
s32 app_usb_slave_hid(u32 hid_cmd);
u8 	app_pc_set_speaker_vol(u32 pc_mute_status);
u8 	app_pc_set_mic_vol(u32 pc_mic_mute_status);
u32 app_usb_slave_online_status(void);
void pc_dac_mute(bool mute_status, u8 fade_en);
void pc_dac_channel_on(void);
void pc_dac_channel_off(void);
void pc_check_api(u32 info);

#endif/*__DEV_PC_H__*/
