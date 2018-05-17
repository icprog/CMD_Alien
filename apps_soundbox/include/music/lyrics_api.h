#ifndef __LYRICS_API_H__
#define __LYRICS_API_H__

#include "includes.h"
#include "dec/music_api.h"
#include "uicon/lyrics.h"
#include "uicon/ui.h"

///<������ʱ���ǩ��flash, ���Խ���ϳ�����ļ�����������,Ҫʹ�ܸù��ܣ���Ҫ�����¼��㣺
//1������isd_tools.cfg�ļ�(�ο�vm�������, LRIF_LEN Ĭ������Ϊ64K)
//		LRIF_ADDR=AUTO;
//		LRIF_LEN=0x10000;
//		LRIF_OPT=1;
//2��ʹ��LRC_ENABLE_SAVE_LABEL_TO_FLASH
//3������ʱ���ǩ��ʱ����buf��СLABEL_TEMP_BUF_LEN(2048��ʾ���Խ��������1024��ʱ���ǩ�ĸ���ļ�)
//ע�⣺���flash�ռ䲻���� ֻ�ܲ�����LRC_ENABLE_SAVE_LABEL_TO_FLASH,���ǲ����ݳ�����ļ�
#define LRC_ENABLE_SAVE_LABEL_TO_FLASH		0  //�Ƿ�ʹ�ܱ�����ʱ���ǩ��flash�� 1�����棬 0��������


//�궨����
#define LRC_DISPLAY_TEXT_ID   DVcTxt1_11
#define LRC_DISPLAY_TEXT_LEN   32

#define  ONCE_READ_LENGTH     ALIGN_4BYTE(128)   ///<�漰�ڴ�������⣬ֵ�����4�ı���(�������ֵΪ255)
#define  ONCE_DIS_LENGTH      ALIGN_4BYTE(64)    ///��ʾ��ʵĻ��泤��
#define  LABEL_TEMP_BUF_LEN   ALIGN_4BYTE(2048)  ///����2K����ʱ���ǩ


extern int lrc_init(void);
extern void lrc_exit(void);
extern int lrc_find(MUSIC_OP_API *m_op_api, void *ext_name);
#endif//__LYRICS_API_H__


