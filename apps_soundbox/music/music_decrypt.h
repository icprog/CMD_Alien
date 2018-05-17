#ifndef __MUSIC_DECRYPT_H__
#define __MUSIC_DECRYPT_H__

#include "typedef.h"


typedef struct _CIPHER CIPHER;

CIPHER *cipher_obj_creat(u32 key, const char *file_type);
void cipher_obj_destroy(CIPHER **obj);
void cipher_analysis_buff(CIPHER *obj, void* buf, u32 faddr, u32 len);
tbool cipher_check_file_type(CIPHER *obj, void *file);

#endif //__MUSIC_DECRYPT_H__
