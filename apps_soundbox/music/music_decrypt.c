#include "music_decrypt.h"
#include "music.h"
#include "fat/fs_io.h"
#include "fat/tff.h"
#include "sdk_cfg.h"


#if 1
#define decrypt_puts(...)
#define decrypt_put_buf(...)
#define decrypt_printf(...)
#else
#define decrypt_puts puts
#define decrypt_put_buf put_buf
#define decrypt_printf printf
#endif

#define ALIN_SIZE	4


struct _CIPHER
{
	void *file_type;		  //文件后缀
	u32   cipher_code;        ///>解密key
	u8    cipher_enable;      ///>解密读使能
};

static tbool __cipher_match(char *fname, char const *ext)
{
    char *str = (char *)ext;

    if(!ext)//不匹配
    {
        return false;
    }

    while (1)
    {
        if (memcmp(&fname[8], str, 3))
        {
            return false;
        }

        if (*(str + 3) == '\0')									//end of string?
            return true;
        else
            str += 3;											//next extension
    }
}


CIPHER *cipher_obj_creat(u32 key, const char *file_type)
{
	CIPHER *obj = calloc(1, sizeof(CIPHER));	
	if(obj == NULL)
	{
		ASSERT(obj);
		return NULL;
	}

	obj->cipher_code = key;
	obj->file_type = (void *)file_type;

	return obj;
}

void cipher_obj_destroy(CIPHER **obj)
{
	if(obj == NULL || *obj == NULL)		
		return ;

	free(*obj);
	*obj = NULL;
}

void cipher_analysis_buff(CIPHER *obj, void* buf, u32 faddr, u32 len)
{
	u32 i;
	u8 j;
	u8	head_rem;//
	u8  tail_rem;//
	u32 len_ali;

//	u32 *buf_4b_ali;
	u8  *buf_1b_ali;
	u8  *cipher_code = NULL;

	if(obj == NULL)
		return ;

	/* if(!cipher_file.cipher_enable) */
	if(obj->cipher_enable == 0)
	{
		return;
	}

	cipher_code = (u8 *)&(obj->cipher_code);//cipher_file.cipher_code;

	/* printf("----faddr = %d \n",faddr); */
	/* put_buf(buf,len); */

	decrypt_printf("buf_addr = %d \n",buf);

	head_rem = ALIN_SIZE-(faddr%ALIN_SIZE);
	if(head_rem == ALIN_SIZE)
	{
		head_rem = 0;
	}

	if(head_rem > len){
		head_rem = len;
	}

	if(len - head_rem){
		tail_rem = (faddr+len)%ALIN_SIZE;
	}
	else{
		tail_rem = 0;
	}

	decrypt_printf("head_rem = %d tail_rem = %d \n",head_rem,tail_rem);

	decrypt_puts("deal_head_buf\n");
	buf_1b_ali = buf;
	j = 3;
	for(i = head_rem; i>0; i--)
	{
		buf_1b_ali[i-1] ^= cipher_code[j--];
		decrypt_printf("i = %d \n",i-1);
		decrypt_printf("buf_1b_ali[i] = %x \n",buf_1b_ali[i-1]);
	}
	decrypt_puts("\n\n-----------TEST_HEAD-----------------");
	decrypt_put_buf(buf_1b_ali,head_rem);

	decrypt_puts("deal_main_buf\n");
	buf_1b_ali = buf;
	buf_1b_ali = (u8 *)(buf_1b_ali+head_rem);
	len_ali = len - head_rem-tail_rem;
	decrypt_printf("len_ali = %d \n",len_ali);
	decrypt_printf("buf_1b_ali = %d \n",buf_1b_ali);
	decrypt_printf("buf_4b_ali = %d \n",buf_4b_ali);
	/* if(cipher_file.cipher_database <= lba) */
	{
		for(i = 0; i<(len_ali/4);i++)
		{
			/* buf_4b_ali[i] = cipher_file.cipher_code; */
			buf_1b_ali[0+i*4] ^= cipher_code[0];
			buf_1b_ali[1+i*4] ^= cipher_code[1];
			buf_1b_ali[2+i*4] ^= cipher_code[2];
			buf_1b_ali[3+i*4] ^= cipher_code[3];
		}
	}
	decrypt_puts("\n\n-----------TEST_MAIN-----------------");
	decrypt_put_buf(buf_1b_ali,len_ali);

	decrypt_puts("deal_tail_buf\n");
	buf_1b_ali = buf;
	buf_1b_ali+=len-tail_rem;
	j = 0;
	for(i = 0 ; i<tail_rem; i++)
	{
		buf_1b_ali[i] ^=cipher_code[j++];
	}
	decrypt_puts("\n\n-----------TEST_TAIL-----------------");
	decrypt_put_buf(buf_1b_ali,tail_rem);
	
	decrypt_puts("\n\n-----------TEST-----------------");
	/* put_buf(buf,len); */
}


tbool cipher_check_file_type(CIPHER *obj, void *file)
{
	_FIL_HDL *f_h=(_FIL_HDL *)(file);
	if(obj == NULL)
		return false;

	if(f_h->io->type == FAT_FS_T)
	{
		FIL *p_f=(FIL *)(f_h->hdl);
		u8 *file_name;
		file_name =(u8 *)p_f->dir_info.fname;
		printf("---name = %s---\n",p_f->dir_info.fname);

		if(true == __cipher_match((char *)file_name, (const char *)obj->file_type))
		{
			obj->cipher_enable = 1;
			return true;			
		}
	}
	obj->cipher_enable = 0;
	return false;
}


