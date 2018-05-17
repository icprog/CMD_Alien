#include "speex_encode.h"
#include "sdk_cfg.h"
#include "common/app_cfg.h"
#include "common/msg.h"
#include "cbuf/circular_buf.h"
#include "cpu/dac_param.h"
#include "dac/ladc_api.h"
#include "dac/dac_api.h"
#include "dac/ladc.h"
#include "timer.h"
#include "speex/DigitalAGC.h"
#include "vad/vad_api.h"

#define SPEEX_ENCODE_DEBUG_ENABLE
#ifdef SPEEX_ENCODE_DEBUG_ENABLE
#define speex_encode_printf	printf
#else
#define speex_encode_printf(...)
#endif


#define SPEEX_ENCODE_BUF_SIZE			(1024*2)//(512)//
#define SPEEX_ENCODE_GET_DATA_SIZE		(SPEEX_ENCODE_BUF_SIZE/4)//(SPEEX_ENCODE_BUF_SIZE/8)//
#define SPEEX_ENCODE_WRITE_BUF_MIN   	(1024)//(256)//
#define SPEEX_ENCODE_WRITE_BUF_SIZE 	(SPEEX_ENCODE_WRITE_BUF_MIN*8)//(SPEEX_ENCODE_WRITE_BUF_MIN*8)
#define SPEEX_ENCODE_WRITE_FULL_LIMIT	(SPEEX_ENCODE_WRITE_BUF_SIZE - 40)
#define SPEEX_ENCODE_DELAY_LIMIT		(0)

#define SPEEX_ENCODE_AGC_EN				0
#if (SPEEX_ENCODE_AGC_EN)
#define AGC_TMP_BUF_LEN					(160*4)
#endif//SPEEX_ENCODE_AGC_EN

#define SPEEX_VAD_ENABLE 				 1//语音检测使能

#if (SPEEX_VAD_ENABLE)
#define VAD_TMP_BUF_LEN					(160*4)
#endif//SPEEX_VAD_ENABLE

#define SPEEX_AGC_SD_DEBUG       		0//AEC SD DEBUG MODE

#if (SPEEX_AGC_SD_DEBUG)
extern void aec_tst_write(u8* buf,int len);
extern void aec_tst_task_init(char *p,void *ptr);
#endif//SPEEX_AGC_SD_DEBUG


#define SPEEX_ENCODE_SIZEOF_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

typedef struct __SPEEX_VAD_OT
{
	void 			 *priv;
	void 			 (*cbk)(void *priv, u8 is_timeout);
}SPEEX_VAD_OT;

typedef struct __SPEEX_VAD
{
	void 			 *hdl;	
	SPEEX_VAD_OT	 timeout_cb;
	volatile u32	 timeout_count;
	volatile u8		 timeout_flag;
	volatile u8		 start;
	volatile u8		 end;
	u32	  			 sp_timeout_threshold;
	u32	  			 nsp_timeout_threshold;
	u32	  			 min_time_threshold;
	s16				 sp_threshold;
	s16				 nsp_threshold;
}SPEEX_VAD;


struct __SPEEX_ENCODE {
    void 			*father_name;
    void 			*en_name;
	void 			*wr_name;
	/* void 			*vad; */
	DigitalAGCVar   *agc;
    void 			*ops_hdl;
    speex_enc_ops 	*ops;
	SPEEX_VAD		 vad;
    SPEEX_EN_FILE_IO _en_io;
    SPEEX_FILE 		 file;
    cbuffer_t 		 en_cbuf;
    cbuffer_t 		 wr_cbuf;
    cbuffer_t 		 agc_cbuf;
    cbuffer_t 		 vad_cbuf;
    OS_SEM		 	 en_sem;
	OS_SEM		 	 wr_sem;
    u32				 wr_delay;
    u32				 pcm_lost;
    u16				 samplerate;
    volatile u8 	 write_st;
    volatile u8 	 encode_st;
    volatile u8 	 is_play;
    volatile u8 	 agc_res;
};

//static u8 pcm_16k_test_data[64] = {
//		0x00, 0x00, 0xFB, 0x30, 0x82, 0x5A, 0x40, 0x76, 0xFF, 0x7F, 0x41, 0x76, 0x82, 0x5A, 0xFB, 0x30,
//		0x00, 0x00, 0x04, 0xCF, 0x7F, 0xA5, 0xC0, 0x89, 0x01, 0x80, 0xBF, 0x89, 0x7F, 0xA5, 0x05, 0xCF,
//		0xFF, 0xFF, 0xFB, 0x30, 0x82, 0x5A, 0x40, 0x76, 0xFF, 0x7F, 0x41, 0x76, 0x82, 0x5A, 0xFB, 0x30,
//		0x00, 0x00, 0x05, 0xCF, 0x7E, 0xA5, 0xBF, 0x89, 0x01, 0x80, 0xC0, 0x89, 0x7E, 0xA5, 0x05, 0xCF
//};
//
//static u32 get_test_data_len = 0;
//static u8 pcm_500hz_data[128] = {
//	0x00, 0x00, 0xF8, 0x18, 0xFC, 0x30, 0x1C, 0x47, 0x82, 0x5A, 0x6D, 0x6A, 0x41, 0x76, 0x89, 0x7D,
//	0xFF, 0x7F, 0x89, 0x7D, 0x41, 0x76, 0x6D, 0x6A, 0x82, 0x5A, 0x1C, 0x47, 0xFC, 0x30, 0xF9, 0x18,
//	0x00, 0x00, 0x08, 0xE7, 0x05, 0xCF, 0xE3, 0xB8, 0x7E, 0xA5, 0x93, 0x95, 0xBF, 0x89, 0x76, 0x82,
//	0x01, 0x80, 0x77, 0x82, 0xBF, 0x89, 0x93, 0x95, 0x7E, 0xA5, 0xE3, 0xB8, 0x05, 0xCF, 0x07, 0xE7,
//	0x00, 0x00, 0xF8, 0x18, 0xFB, 0x30, 0x1D, 0x47, 0x82, 0x5A, 0x6D, 0x6A, 0x41, 0x76, 0x8A, 0x7D,
//	0xFF, 0x7F, 0x89, 0x7D, 0x40, 0x76, 0x6C, 0x6A, 0x82, 0x5A, 0x1C, 0x47, 0xFB, 0x30, 0xF8, 0x18,
//	0x01, 0x00, 0x07, 0xE7, 0x05, 0xCF, 0xE4, 0xB8, 0x7E, 0xA5, 0x94, 0x95, 0xC0, 0x89, 0x76, 0x82,
//	0x01, 0x80, 0x77, 0x82, 0xBF, 0x89, 0x93, 0x95, 0x7E, 0xA5, 0xE3, 0xB8, 0x05, 0xCF, 0x07, 0xE7
//};

/* 编码调用示例 */
/* { */
/* speex_enc_ops *tst_ops=get_speex_enc_obj(); */
/* bufsize=tst_ops->need_buf(); */
/* st=malloc(bufsize); */
/* tst_ops->open(st,&speex_en_io,0);         */
/* while(!tst_ops->run(st))               //读不到数的时候就会返回1了。 */
/* { */
/* } */
/* free(st); */
/* } */

/* u16 (*input_data)(void *priv,s16 *buf,u8 len);           //这里的长度是多少个short */
/* void (*output_data)(void *priv,u8 *buf,u16 len);         //这里的长度是多少个bytes */
SPEEX_ENCODE *g_speex_encode = NULL;


void speex_encode_ladc_isr_callback(void *ladc_buf, u32 buf_flag, u32 buf_len)
{
    if (g_speex_encode != NULL) {

		s16 *res;
		s16 *ladc_mic;

		//dual buf
		res = (s16 *)ladc_buf;
		res = res + (buf_flag * DAC_SAMPLE_POINT);
		ladc_mic = res + (2 * DAC_DUAL_BUF_LEN);


        if ((cbuf_get_data_size(&(g_speex_encode->wr_cbuf))) > SPEEX_ENCODE_WRITE_FULL_LIMIT) { //判断是否压缩完能写进写缓存
            /* speex_encode_printf("f"); */
            return ;
        }

        cbuffer_t *cbuffer = &(g_speex_encode->en_cbuf);

        if ((cbuffer->total_len - cbuffer->data_len) < DAC_DUAL_BUF_LEN) {
            g_speex_encode->pcm_lost ++;
            /* speex_encode_printf("x"); */
        } else {

			cbuf_write(cbuffer, (void *)ladc_mic, DAC_DUAL_BUF_LEN);
        }
        if (cbuffer->data_len >= SPEEX_ENCODE_GET_DATA_SIZE) {

            os_sem_set(&(g_speex_encode->en_sem), 0);
            os_sem_post(&(g_speex_encode->en_sem));
        }
    }
}

/* void speex_test_dac(void *buf, u8 len) */
/* { */
	/* if (g_speex_encode != NULL)  */
	/* { */
		/* cbuf_read(&(g_speex_encode->en_cbuf), (void *)buf, len); */
	/* } */
/* } */
/* #define AGC_TMP_BUF_LEN		(80) */
/* static u8 agc_tmp_buf[AGC_TMP_BUF_LEN]; */
/* static u8 cur_agc_buf_index = 0; */

/* static u8 tmp_buf[160]; */
static u16 speex_encode_input(void *priv, s16 *buf, u8 len)
{
	s16 tmp_buf[160];
    SPEEX_ENCODE *obj = (SPEEX_ENCODE *)priv;
    if (obj == NULL) {
        return 0;
    }
    /* if(len!=160) */
    /* return 0; */
    u32 rlen = len << 1;
	while(1)
	{
		int vad_res = 0;
		u32 tick;

        if ((obj->encode_st == 0) || (obj->is_play == 0)) {
            break;
        }
		
		tick = get_sys_2msCount();
		if(obj->vad.start)
		{
			if((tick - obj->vad.timeout_count) >= obj->vad.sp_timeout_threshold)
			{
				if(obj->vad.timeout_flag == 0)
				{
					obj->vad.timeout_flag = 1;
					if(obj->vad.end == 0)
					{
						if(obj->vad.timeout_cb.cbk)
						{
							speex_encode_printf("speak timeout !!!!\n");
							obj->vad.timeout_cb.cbk(obj->vad.timeout_cb.priv, 1);
						}
					}
				}
			}
		}
		else
		{
			if((tick - obj->vad.timeout_count) >= obj->vad.nsp_timeout_threshold)
			{
				if(obj->vad.timeout_flag == 0)
				{
					obj->vad.timeout_flag = 1;
					if(obj->vad.timeout_cb.cbk)
					{
						speex_encode_printf("no speak timeout !!!!\n");
						obj->vad.timeout_cb.cbk(obj->vad.timeout_cb.priv, 2);
					}
				}
			}
		}

		if (cbuf_get_data_size(&(obj->vad_cbuf)) >= rlen) 
		{
            cbuf_read(&(obj->vad_cbuf), (void *)buf, rlen);
			return rlen;
		}
		else
		{
			if (cbuf_get_data_size(&(obj->en_cbuf)) >= 320) 
			{
				cbuf_read(&(obj->en_cbuf), (void *)tmp_buf, 320);

				cbuf_write(&(obj->vad_cbuf), tmp_buf, 320);

				vad_res = vad_main(obj->vad.hdl, (int *)tmp_buf);

				tick = get_sys_2msCount();

				if(vad_res == 1)
				{
					if(obj->vad.start == 0)
					{
						speex_encode_printf("++++++++++++sentence start++++++++++++++\n");		
						obj->vad.start = 1;
						obj->vad.timeout_count = tick;
					}
				}
				else if(vad_res == 2)
				{
					if((tick - obj->vad.timeout_count) >= obj->vad.min_time_threshold)
					{
						if(obj->vad.end == 0)
						{
							obj->vad.end = 1;
							speex_encode_printf("++++++++++++sentence end++++++++++++++\n");		
							/* if(obj->vad.timeout_cb.cbk) */
							/* { */
								/* obj->vad.timeout_cb.cbk(obj->vad.timeout_cb.priv, 0); */
							/* } */
						}
					}
				}
				else if(vad_res == 0)
				{
					/* printf("-0-");		 */
				}
				continue ;
			}

        	os_sem_pend(&(obj->en_sem), 0);
		}

	}
    return 0;
}

static void speex_encode_output(void *priv, u8 *buf, u16 len)
{
    SPEEX_ENCODE *obj = (SPEEX_ENCODE *)priv;
    if (obj == NULL) {
        return ;
    }
#if (SPEEX_AGC_SD_DEBUG)
	return ;
#endif//SPEEX_AGC_SD_DEBUG

	/* printf("len = %d\n", len); */
    if (len <= cbuf_is_write_able(&(obj->wr_cbuf), len)) {
#if 0
		static u32 test_cnt = 0;
		u16 i;
		for(i=0; i<(len/4); i++)
		{
			cbuf_write(&(obj->wr_cbuf), (void *)&test_cnt, 4);
			test_cnt ++;		
		}
#else
		cbuf_write(&(obj->wr_cbuf), (void *)buf, len);
#endif
    } else {
        speex_encode_printf("l");
    }

    if (obj->wr_delay) {
        if (obj->wr_delay > len) {
            obj->wr_delay -= len;
            return ;
        }
    }

    os_sem_set(&(obj->wr_sem), 0);
    os_sem_post(&(obj->wr_sem));
}

static int speex_encode_write(SPEEX_ENCODE *obj)
{
    u8 *addr  = NULL;
    u32 rlen = 0;
    addr = cbuf_read_alloc(&(obj->wr_cbuf), &rlen);
    if (addr == NULL || rlen == 0) {
        return 1;
    }

	/* printf_buf(addr, rlen); */

    if (obj->write_st == 0) {
        return 1;
    }

    if (obj->file._io == NULL) {
        speex_encode_printf("file io is null !!!, fun = %s, line = %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (obj->file._io->write == NULL) {
        speex_encode_printf("file write io is null !!!\n");
        return -1;
    }

	if(rlen > 512)
		rlen = 512;

    if (obj->file._io->write(obj->file.hdl, addr, rlen) == 0) {
        speex_encode_printf("file write err !!!\n");
        return -1;
    }
    cbuf_read_updata(&(obj->wr_cbuf), rlen);
	
	if(obj->vad.end)
	{
		if(cbuf_get_data_size(&(obj->wr_cbuf)) == 0)			
		{
			if(obj->vad.timeout_cb.cbk)
			{
				printf("speed data send end\n");
				obj->vad.timeout_cb.cbk(obj->vad.timeout_cb.priv, 0);
			}
		}
	}

    return 0;
}



static void speex_encode_deal(void *parg)
{
    u8 ret;
    u32 err = 0;
    int msg[3] = {0};
    SPEEX_ENCODE *obj = (SPEEX_ENCODE *)parg;


#if (SPEEX_ENCODE_AGC_EN)
	/* int agc_err = DigitalAGC_Init(obj->agc, 6, 6, 0, -33, -33); */
	int agc_err = DigitalAGC_Init(obj->agc, 6, 6, 0, -36, -36);
	speex_encode_printf("agc_err = %d\n", agc_err);
#endif//SPEEX_ENCODE_AGC_EN

#if (SPEEX_VAD_ENABLE)
	speex_encode_printf("obj->vad.sp_threshold = %d, obj->vad.nsp_threshold = %d\n", obj->vad.sp_threshold, obj->vad.nsp_threshold);
	vad_init(obj->vad.hdl, obj->vad.sp_threshold, obj->vad.nsp_threshold);
#endif//SPEEX_VAD_ENABLE


    while (1) {
        if ((obj->encode_st == 1) && (obj->vad.end == 0)) {
            ret = OSTaskQAccept(ARRAY_SIZE(msg), msg);
            if (ret == OS_Q_EMPTY) {
                err = obj->ops->run(obj->ops_hdl);
                if (err == 0) {
                    continue;
                }
            }
        } else {
            os_taskq_pend(0, ARRAY_SIZE(msg), msg);
        }

        switch (msg[0]) {
        case SYS_EVENT_DEL_TASK:
            if (os_task_del_req_name(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
                os_task_del_res_name(OS_TASK_SELF);
            }
            break;
        case MSG_SPEEX_START:
            /* speex_encode_printf("fun = %s, line = %d\n", __func__, __LINE__); */
            obj->encode_st = 1;
            if (obj->wr_name) {
                os_taskq_post(obj->wr_name, 1, MSG_SPEEX_START);
                while (obj->write_st == 0) {
                    OSTimeDly(1);
                }
            }

            /* speex_encode_printf("fun = %s, line = %d\n", __func__, __LINE__); */
            break;
        case MSG_SPEEX_STOP:
            obj->encode_st = 0;
            if (obj->wr_name) {
				os_taskq_post(obj->wr_name, 1, MSG_SPEEX_STOP);
				while (obj->write_st) {
					os_sem_set(&(obj->wr_sem), 0);
					os_sem_post(&(obj->wr_sem));
					OSTimeDly(1);
				}
            }

            break;
        default:
            break;
        }
    }
}




static void speex_encode_write_deal(void *parg)
{
    int err = 0;
    u8 ret;
    u8 write_err_flag = 0;
    int msg[3] = {0};
    SPEEX_ENCODE *obj = (SPEEX_ENCODE *)parg;
    /* obj->write_st = 1; */
    while (1) {
        if ((write_err_flag == 0) && (obj->write_st == 1)) {
            ret = OSTaskQAccept(ARRAY_SIZE(msg), msg);
            if (ret == OS_Q_EMPTY) {

                err = speex_encode_write(obj);
                if (err == 1) {
                    os_sem_pend(&(obj->wr_sem), 0);
                } else if (err == 0) {
                    continue;
                } else {
                    write_err_flag = 1;
                    os_taskq_post(obj->father_name, 1, MSG_SPEEX_ENCODE_ERR);
                }
            }
        } else {
            /* obj->w_busy = 0; */
            os_taskq_pend(0, ARRAY_SIZE(msg), msg);
        }

        switch (msg[0]) {
        case SYS_EVENT_DEL_TASK:
            if (os_task_del_req_name(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
                os_task_del_res_name(OS_TASK_SELF);
            }
            break;
        case MSG_SPEEX_START:
            /* speex_encode_printf("fun = %s, line = %d\n", __func__, __LINE__); */
            obj->write_st = 1;
            break;
        case MSG_SPEEX_STOP:
            obj->write_st = 0;
            break;
        default:
            break;
        }
    }
}


tbool speex_encode_process(SPEEX_ENCODE *obj, void *task_name, u8 prio)
{
    if (obj == NULL) {
        return false;
    }

    if (obj->file._io == NULL) {
        return false;
    }

    u32 err;
    err = os_task_create(speex_encode_deal,
                         (void *)obj,
                         prio,
                         10
#if OS_TIME_SLICE_EN > 0
                         , 1
#endif
                         , task_name);

    if (err) {
        speex_encode_printf("speex encode task err\n");
        return false;
    }

    obj->en_name = task_name;
    return true;
}


tbool speex_encode_write_process(SPEEX_ENCODE *obj, void *task_name, u8 prio)
{
    if (obj == NULL) {
        return false;
    }

    if (obj->file._io == NULL) {
        return false;
    }

    u32 err;
    err = os_task_create(speex_encode_write_deal,
                         (void *)obj,
                         prio,
                         10
#if OS_TIME_SLICE_EN > 0
                         , 1
#endif
                         , task_name);

    if (err) {
        speex_encode_printf("speex encode write task err %d\n", err);
        return false;
    }

    obj->wr_name = task_name;
    return true;
}


void __speex_encode_set_file_io(SPEEX_ENCODE *obj, SPEEX_FILE_IO *_io, void *hdl)
{
    if (obj == NULL) {
        return ;
    }

    obj->file.hdl = hdl;
    obj->file._io = _io;
}

void __speex_encode_set_samplerate(SPEEX_ENCODE *obj, u16 samplerate)
{
    if (obj == NULL) {
        return ;
    }

    obj->samplerate = samplerate;
}

void __speex_encode_set_father_name(SPEEX_ENCODE *obj, const char *father_name)
{
    if (obj == NULL) {
        return ;
    }

    obj->father_name = (void *)father_name;
}


void __speex_encode_set_vad_reg(SPEEX_ENCODE *obj, s16 sp_threshold, s16 nsp_threshold, u32 sp_timeout, u32 nsp_timeout, u32 min_time,  void (*timeout_cbk)(void *priv, u8 is_timeout), void *priv)
{
    if (obj == NULL) {
        return ;
    }
	obj->vad.sp_timeout_threshold = sp_timeout;
	obj->vad.nsp_timeout_threshold = nsp_timeout;
	obj->vad.min_time_threshold = min_time;
	obj->vad.timeout_cb.priv = priv;
	obj->vad.timeout_cb.cbk = timeout_cbk;;
	obj->vad.sp_threshold = sp_threshold;
	obj->vad.nsp_threshold = nsp_threshold;
	obj->vad.start = 0;
	obj->vad.end = 0;
}

extern void ladc_reg_init(u8 ch_sel, u16 sr);
tbool speex_encode_start(SPEEX_ENCODE *obj)
{
    if (obj == NULL) {
        return false;
    }
    if (obj->encode_st == 1) {
        speex_encode_printf("speex encode is aready start !!\n");
        return false;
    }
    obj->is_play = 1;
	obj->wr_delay = SPEEX_ENCODE_DELAY_LIMIT;
	obj->vad.timeout_count = get_sys_2msCount();

    g_speex_encode = obj;
    if (obj->en_name) {
		/* ladc_reg_isr_cb_api(speex_encode_ladc_isr_callback); */
#if (SPEEX_AGC_SD_DEBUG)
		aec_tst_task_init("tst_sd", 0);
#endif//SPEEX_AGC_SD_DEBUG

		ladc_reg_init(ENC_MIC_CHANNEL, LADC_SR16000);
        ladc_mic_gain(60, 0); //设置mic音量,这一句要在ladc_enable(ENC_MIC_CHANNEL,LADC_SR16000, VCOMO_EN);后面

        os_taskq_post(obj->en_name, 1, MSG_SPEEX_START);
        while (obj->encode_st == 0) {
            OSTimeDly(1);
        }
        /* speex_encode_printf("fun = %s, line = %d\n", __func__, __LINE__); */
        return true;
    }

    return false;
}


tbool speex_encode_stop(SPEEX_ENCODE *obj)
{
    if (obj == NULL) {
        return false;
    }
    if (obj->encode_st == 0) {
        speex_encode_printf("speex encode is aready stop!!\n");
        return false;
    }

    obj->is_play = 0;
    if (obj->en_name) {

        os_sem_set(&(obj->en_sem), 0);
        os_sem_post(&(obj->en_sem));

        os_taskq_post(obj->en_name, 1, MSG_SPEEX_STOP);
        while ((obj->encode_st == 1) || (obj->write_st == 1)) {
            OSTimeDly(1);
        }

		/* ladc_disable(ENC_MIC_CHANNEL); */
		ladc_close(ENC_MIC_CHANNEL);
        g_speex_encode = NULL;
        return true;
    }

    return false;
}



SPEEX_ENCODE *speex_encode_creat(void)
{
    u8 *e_buf = NULL;
    u8 *w_buf = NULL;
    u8 *agc_tmp_buf = NULL;
    u8 *vad_tmp_buf = NULL;
    u8 *need_buf;
    u32 need_buf_len;
    u32 speex_enc_need_buf_len;
    speex_enc_ops *tmp_ops = get_speex_enc_obj();
    speex_enc_need_buf_len = tmp_ops->need_buf();

#if (SPEEX_ENCODE_AGC_EN)
	u32 agc_need_buf_len = (u32)DigitalAGC_QueryNeedSize(0);
	speex_encode_printf("agc_need_buf_len = %d\n", agc_need_buf_len);
#endif//SPEEX_ENCODE_AGC_EN

#if (SPEEX_VAD_ENABLE)
	u32 vad_need_buf_len = vad_get_need_buf_size();
	speex_encode_printf("vad_need_buf_len = %d\n", vad_need_buf_len);
#endif//SPEEX_VAD_ENABLE
    need_buf_len = SPEEX_ENCODE_SIZEOF_ALIN(sizeof(SPEEX_ENCODE), 4)
                   + SPEEX_ENCODE_SIZEOF_ALIN(speex_enc_need_buf_len, 4)
                   + SPEEX_ENCODE_SIZEOF_ALIN(SPEEX_ENCODE_BUF_SIZE, 4)
                   + SPEEX_ENCODE_SIZEOF_ALIN(SPEEX_ENCODE_WRITE_BUF_SIZE, 4)
#if (SPEEX_ENCODE_AGC_EN)
                   + SPEEX_ENCODE_SIZEOF_ALIN(agc_need_buf_len, 4)
                   + SPEEX_ENCODE_SIZEOF_ALIN(AGC_TMP_BUF_LEN, 4)
#endif//SPEEX_ENCODE_AGC_EN

#if (SPEEX_VAD_ENABLE)
                   + SPEEX_ENCODE_SIZEOF_ALIN(vad_need_buf_len, 4)
                   + SPEEX_ENCODE_SIZEOF_ALIN(VAD_TMP_BUF_LEN, 4)
#endif//SPEEX_VAD_ENABLE
				   ;

    speex_encode_printf("need_buf_len = %d!!\n", need_buf_len);
    need_buf = (u8 *)calloc(1, need_buf_len);
    if (need_buf == NULL) {
        return NULL;
    }
    SPEEX_ENCODE *obj = (SPEEX_ENCODE *)need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(sizeof(SPEEX_ENCODE), 4);

    obj->ops_hdl = (void *)need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(speex_enc_need_buf_len, 4);

    e_buf = (u8 *)need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(SPEEX_ENCODE_BUF_SIZE, 4);


    w_buf = (u8 *)need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(SPEEX_ENCODE_WRITE_BUF_SIZE, 4);

#if (SPEEX_ENCODE_AGC_EN)
	obj->agc = (DigitalAGCVar *)need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(agc_need_buf_len, 4);

	agc_tmp_buf = need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(AGC_TMP_BUF_LEN, 4);
#endif//SPEEX_ENCODE_AGC_EN

#if (SPEEX_VAD_ENABLE)
	obj->vad.hdl = (DigitalAGCVar *)need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(vad_need_buf_len, 4);

	vad_tmp_buf = need_buf;
    need_buf += SPEEX_ENCODE_SIZEOF_ALIN(VAD_TMP_BUF_LEN, 4);
#endif//SPEEX_VAD_ENABLE
    obj->ops = tmp_ops;

    obj->_en_io.priv =  obj;
    obj->_en_io.input_data = (void *)speex_encode_input;
    obj->_en_io.output_data = (void *)speex_encode_output;
    obj->ops->open(obj->ops_hdl, &(obj->_en_io), 0);

    cbuf_init(&(obj->en_cbuf), e_buf, SPEEX_ENCODE_BUF_SIZE);
    cbuf_init(&(obj->wr_cbuf), w_buf, SPEEX_ENCODE_WRITE_BUF_SIZE);

#if (SPEEX_ENCODE_AGC_EN)
    cbuf_init(&(obj->agc_cbuf), agc_tmp_buf, AGC_TMP_BUF_LEN);
#endif

#if (SPEEX_VAD_ENABLE)
    cbuf_init(&(obj->vad_cbuf), vad_tmp_buf, VAD_TMP_BUF_LEN);
#endif

    if (os_sem_create(&obj->en_sem, 0) != OS_NO_ERR) {
        free(need_buf);
        return NULL;
    }

    if (os_sem_create(&obj->wr_sem, 0) != OS_NO_ERR) {
        free(need_buf);
        return NULL;
    }

    obj->encode_st = 0;
    obj->write_st = 0;
    obj->is_play = 0;

//#if (SPEEX_ENCODE_AGC_EN)
//	/* int agc_err = DigitalAGC_Init(obj->agc, 6, 6, 0, -33, -33); */
//	int agc_err = DigitalAGC_Init(obj->agc, 6, 6, 0, -36, -36);
//	printf("agc_err = %d\n", agc_err);
//#endif//SPEEX_ENCODE_AGC_EN
//
//#if (SPEEX_VAD_ENABLE)
//	vad_init(obj->vad.hdl);
//#endif//SPEEX_VAD_ENABLE

    speex_encode_printf("speex_encode_creat ok!!\n");
	
    return obj;
}


void speex_encode_destroy(SPEEX_ENCODE **obj)
{
    if ((obj == NULL) || ((*obj) == NULL)) {
        return;
    }
    SPEEX_ENCODE *tmp_obj = *obj;
    speex_encode_stop(tmp_obj);
    if (tmp_obj->en_name) {
        if (os_task_del_req(tmp_obj->en_name) != OS_TASK_NOT_EXIST) {
            os_taskq_post_event(tmp_obj->en_name, 1, SYS_EVENT_DEL_TASK);
            do {
                OSTimeDly(1);
                /* speex_encode_printf("fun = %s, line = %d\n", __FUNCTION__, __LINE__); */
            } while (os_task_del_req(tmp_obj->en_name) != OS_TASK_NOT_EXIST);
        }
    }

    if (tmp_obj->wr_name) {
        if (os_task_del_req(tmp_obj->wr_name) != OS_TASK_NOT_EXIST) {
            os_taskq_post_event(tmp_obj->wr_name, 1, SYS_EVENT_DEL_TASK);
            do {
                OSTimeDly(1);
                /* speex_encode_printf("fun = %s, line = %d\n", __FUNCTION__, __LINE__); */
            } while (os_task_del_req(tmp_obj->wr_name) != OS_TASK_NOT_EXIST);
        }
    }
    free(*obj);
    *obj = NULL;
}



#if 0
void demo_test(void)
{
	tbool ret;
	SPEEX_ENCODE *obj = speex_encode_creat();
	ASSERT(obj);
	__speex_encode_set_father_name(obj, "FATHER_NAME");//注意这里填写控制线程的名称
	__speex_encode_set_samplerate(obj, 16000L);//只能配置8K或者16K
	__speex_encode_set_file_io(obj, (SPEEX_FILE_IO *)&speex_encode_file, NULL);
	ret = speex_encode_process(obj, "SPEEX_ENCODE_TASK", TaskEncRunPrio);
	if(ret == true)
	{
		ret = speex_encode_write_process(obj, "SPEEX_ENCODE_WR_TASK", TaskEncRunPrio);
		if(ret == true)
		{
			speex_encode_printf("speex encode init ok\n");	
		}
		else
		{
			speex_encode_destroy(&obj);	
			return ;
		}
	}
	else
	{
		speex_encode_destroy(&obj);	
		return ;
	}

	//speex_encode_start(obj);
	//speex_encode_stop(obj);

	speex_encode_destroy(&obj);	
}
#endif
