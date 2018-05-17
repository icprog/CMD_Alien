#ifndef __SPEEX_ENCODE_H__
#define __SPEEX_ENCODE_H__
#include "typedef.h"
#include "speex/speex_encode_api.h"

typedef struct __SPEEX_ENCODE SPEEX_ENCODE;

void __speex_encode_set_file_io(SPEEX_ENCODE *obj, SPEEX_FILE_IO *_io, void *hdl);
void __speex_encode_set_samplerate(SPEEX_ENCODE *obj, u16 samplerate);
void __speex_encode_set_father_name(SPEEX_ENCODE *obj, const char *father_name);
void __speex_encode_set_timeout(SPEEX_ENCODE *obj, u32 timeout, u32 no_speak_timeout, u32 speak_timeout, u32 speak_threshold, u32 no_speak_threshold, u32 delay, void (*timeout_cbk)(void *priv), void *priv);
void __speex_encode_set_vad_reg(SPEEX_ENCODE *obj, s16 sp_threshold, s16 nsp_threshold, u32 sp_timeout, u32 nsp_timeout, u32 min_time,  void (*timeout_cbk)(void *priv, u8 is_timeout), void *priv);
void  speex_encode_ladc_isr_callback(void *ladc_buf, u32 buf_flag, u32 buf_len);
tbool speex_encode_process(SPEEX_ENCODE *obj, void *task_name, u8 prio);
tbool speex_encode_write_process(SPEEX_ENCODE *obj, void *task_name, u8 prio);
tbool speex_encode_start(SPEEX_ENCODE *obj);
tbool speex_encode_stop(SPEEX_ENCODE *obj);
SPEEX_ENCODE *speex_encode_creat(void);
void speex_encode_destroy(SPEEX_ENCODE **obj);

#endif

