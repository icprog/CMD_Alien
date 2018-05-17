#ifndef _ENCHO_API_H_
#define _ENCHO_API_H_

#include "includes.h"
#include "dac/ladc.h"

#if ECHO_EN
    #define echo_msg_deal_api(x,y)   echo_msg_deal(x,y)
    #define echo_exit_api(x)      echo_exit(x)
#else
    #define echo_msg_deal_api(...)
    #define echo_exit_api(...)
#endif


extern u32 cbuf_write_echo(void * buf,u32 len);
extern u32 cbuf_read_echo(void *buf, u32 len);
extern void *cbuf_read_alloc_echo(u32 *len);
extern void cbuf_read_updata_echo(u32 len);
extern u32 cbuf_get_data_len_echo(void);
extern u32 cbuf_get_space_len_echo(void);
extern u32 cbuf_mixture_echo(void *buf, u32 len);
extern void echo_msg_deal(void **mode_reverb, int* msg);
extern void echo_exit(void **mode_reverb);
extern void set_echo_ad_sr(u16 ad_sr,void* param);

#endif
