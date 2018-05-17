#ifndef _LYRICS_H_
#define _LYRICS_H_

#include "typedef.h"

typedef struct __LRC_FILE_IO
{
	bool (*seek)(void *hdl ,u8 type ,u32 offsize);
	u16 (*read)(void *hdl, u8 _xdata *buff, u16 len);
}LRC_FILE_IO;

typedef struct __LRC_CFG
{
    u16 once_read_len;///һ�ζ�ȡ��������
    u16 once_disp_len;///һ����ʾ���泤������
    u16 label_temp_buf_len;///ʱ���ǩ�����ܳ�������
    u8   lrc_text_id;         ///text id
    u8   read_next_lrc_flag;    ///�Ƿ�Ԥ����һ�����lrc
	u8	 enable_save_lable_to_flash;//ʹ�ܱ���ʱ���ǩ��flash����Ҫ�ǽ��������ļ���֧������
    void (*roll_speed_ctrl_cb)(u8 lrc_len, u32 time_gap, u8 *roll_speed);///�ٶȿ�������
    void (*clr_lrc_disp_cb)(void);///�����ص�
}LRC_CFG;


extern void lrc_destroy(void);
extern int lrc_param_init(LRC_CFG *cfg);
// extern bool lrc_analysis(void *lrc_handle);
extern bool lrc_analysis(void *lrc_handle, const LRC_FILE_IO *file_io);
extern int lrc_show(u16 dbtime_s , u8 btime_100ms);
extern void lrc_label_save_to_flash_init(u32 addr, u32 len, u32 base_addr);

#endif
