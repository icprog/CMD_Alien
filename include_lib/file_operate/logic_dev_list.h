#ifndef __LOGIC_DEV_LIST_H__
#define __LOGIC_DEV_LIST_H__

#include "common/list.h"
#include "rtos/os_api.h"
enum
{
    DEV_TYPE_SPI =0,            ///<�ڲ�spi flash
    DEV_TYPE_MASS,              ///<�ⲿ��洢
};

typedef struct _lg_dev_hdl_
{
    void *fs_hdl;               ///<�ļ�ϵͳָ��
    void *file_hdl;             ///<�ļ�ָ��
    void *phydev_item;          ///<�߼��̷���Ӧ�����豸�ڵ�
    void *ui_fhdl;             ///<�ļ�ָ��
    void *lrc_fhdl;             ///<�ļ�ָ��
    void *mutex;
} lg_dev_hdl;


typedef struct _lg_dev_info_
{
    OS_SEM del_sem;            ///<�豸ɾ�������ź�
    OS_MUTEX *p_mutex;           ///<IO����
    u32 total_file_num;          ///<�߼��豸���ļ�����
    u32 last_op_file_num;        ///<��һ�β������ļ����
    lg_dev_hdl  *lg_hdl;         ///<�߼��豸�������豸���ļ�ϵͳ���ļ�ָ��
    u8 fat_type;				 ///<�ļ�ϵͳ����
    u8 dev_let;				     ///<�̷�
    u8 snum_phydev; 		     ///<�߼����������豸�����
    u8 dev_type;				 ///<�豸����
    u8 allfileerr;              ///<��ʶ���豸�����ļ������ɽ���
}lg_dev_info;


typedef struct _lg_dev_list_ {
    lg_dev_info *p_cur_dev;
    u8 max_lg_hdl;
    volatile u8 dev_offline;
    struct list_head list;
} lg_dev_list;

#endif
