/********************************************************************

	emf.c

	----------------------------------------------------------------
    ��������֤ �� GPL
	��Ȩ���� (C) 2001 VCHelp coPathway ISee workgroup.
	----------------------------------------------------------------
	��һ������������������������������������������GNU ͨ�ù�����
	��֤�������޸ĺ����·�����һ���򡣻���������֤�ĵڶ��棬���ߣ���
	�����ѡ�����κθ��µİ汾��

    ������һ�����Ŀ����ϣ�������ã���û���κε���������û���ʺ��ض�
	Ŀ�ص������ĵ���������ϸ����������GNUͨ�ù�������֤��

    ��Ӧ���Ѿ��ͳ���һ���յ�һ��GNUͨ�ù�������֤�ĸ�������Ŀ¼
	GPL.txt�ļ����������û�У�д�Ÿ���
    The Free Software Foundation, Inc.,  675  Mass Ave,  Cambridge,
    MA02139,  USA
	----------------------------------------------------------------
	�������ʹ�ñ�����ʱ��ʲô������飬�������µ�ַ������ȡ����ϵ��

			http://isee.126.com
			http://cosoft.org.cn/projects/iseeexplorer

	���ŵ���

			isee##vip.163.com
	----------------------------------------------------------------
	���ļ���;��	ISeeͼ���������EMFͼ���дģ��ʵ���ļ�

					��ȡ���ܣ�	�ɶ�ȡ��ǿ��Ԫ�ļ���EMF���������ļ����ı���ʾ������
							  
					���湦�ܣ�	��֧�ֱ��湦��
							   

	���ļ���д�ˣ�	YZ			yzfree##yeah.net
					swstudio	swstudio##sohu.com

	���ļ��汾��	20621
	����޸��ڣ�	2002-6-21

	ע������E-Mail��ַ�е�##����@�滻����������Ϊ�˵��ƶ����E-Mail
	    ��ַ�ռ�������
  	----------------------------------------------------------------
	������ʷ��

		2002-6		�ڶ������������°棩
		2001-1		��������ע����Ϣ
		2000-7		�����ദBUG������ǿ��ģ����ݴ���
		2000-6		��һ���汾����


********************************************************************/


#ifndef WIN32
#if defined(_WIN32)||defined(_WINDOWS)
#define WIN32
#endif
#endif /* WIN32 */

/*###################################################################

  ��ֲע�ͣ����´���ʹ����WIN32ϵͳ��SEH(�ṹ���쳣����)�����߳�ͬ��
			���󡰹ؼ��Ρ�������ֲʱӦתΪLinux����Ӧ��䡣

  #################################################################*/


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN				/* ����windows.h�ļ��ı���ʱ�� */
#include <windows.h>
#endif /* WIN32 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "emf.h"


IRWP_INFO			emf_irwp_info;			/* �����Ϣ�� */

#ifdef WIN32
CRITICAL_SECTION	emf_get_info_critical;	/* emf_get_image_info�����Ĺؼ��� */
CRITICAL_SECTION	emf_load_img_critical;	/* emf_load_image�����Ĺؼ��� */
CRITICAL_SECTION	emf_save_img_critical;	/* emf_save_image�����Ĺؼ��� */
#else
/* Linux��Ӧ����� */
#endif


/* �ڲ����ֺ��� */
void CALLAGREEMENT _init_irwp_info(LPIRWP_INFO lpirwp_info);
int CALLAGREEMENT _calcu_scanline_size(int w/* ���� */, int bit/* λ�� */);

static int _verify_file(ISFILE* pfile);
static int _get_info(ISFILE* pfile, LPINFOSTR pinfo_str);
static int _load_enh_metafile(ISFILE* pfile, HENHMETAFILE* phout);
static int _enh_meta_to_raster(HENHMETAFILE hemf, LPINFOSTR pinfo_str);




#ifdef WIN32
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			/* ��ʼ�������Ϣ�� */
			_init_irwp_info(&emf_irwp_info);

			/* ��ʼ�����ʹؼ��� */
			InitializeCriticalSection(&emf_get_info_critical);
			InitializeCriticalSection(&emf_load_img_critical);
			InitializeCriticalSection(&emf_save_img_critical);

			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			/* ���ٷ��ʹؼ��� */
			DeleteCriticalSection(&emf_get_info_critical);
			DeleteCriticalSection(&emf_load_img_critical);
			DeleteCriticalSection(&emf_save_img_critical);
			break;
    }
    return TRUE;
}
#endif	/* WIN32 */



#ifdef WIN32

EMF_API LPIRWP_INFO CALLAGREEMENT is_irw_plugin()
{
	return (LPIRWP_INFO)&emf_irwp_info;
}

#else

EMF_API LPIRWP_INFO CALLAGREEMENT emf_get_plugin_info()
{
	_init_irwp_info(&emf_irwp_info);

	return (LPIRWP_INFO)&emf_irwp_info;
}

EMF_API void CALLAGREEMENT emf_init_plugin()
{
	/* ��ʼ�����߳�ͬ������ */
}

EMF_API void CALLAGREEMENT emf_detach_plugin()
{
	/* ���ٶ��߳�ͬ������ */
}

#endif /* WIN32 */


/* ��ʼ�������Ϣ�ṹ */
void CALLAGREEMENT _init_irwp_info(LPIRWP_INFO lpirwp_info)
{
	assert(lpirwp_info);

	/* ��ʼ���ṹ���� */
	memset((void*)lpirwp_info, 0, sizeof(IRWP_INFO));

	/* �汾�š���ʮ����ֵ��ʮλΪ���汾�ţ���λΪ���汾��*/
	lpirwp_info->irwp_version = MODULE_BUILDID;
	/* ������� */
	strcpy((char*)(lpirwp_info->irwp_name), MODULE_NAME);
	/* ��ģ�麯��ǰ׺ */
	strcpy((char*)(lpirwp_info->irwp_func_prefix), MODULE_FUNC_PREFIX);


	/* ����ķ������͡�0�����԰�����1���������� */
#ifdef _DEBUG
	lpirwp_info->irwp_build_set = 0;
#else
	lpirwp_info->irwp_build_set = 1;
#endif


	/* ���ܱ�ʶ ��##���ֶ������� */
	lpirwp_info->irwp_function = IRWP_READ_SUPP;

	/* ����ģ��֧�ֵı���λ�� */
	lpirwp_info->irwp_save.bitcount = 0;
	lpirwp_info->irwp_save.img_num = 0;
	lpirwp_info->irwp_save.count = 0;

	/* ����������������������Ϣ����Ч��ĸ�������##���ֶ�������*/
	lpirwp_info->irwp_author_count = 2;	


	/* ��������Ϣ��##���ֶ������� */
	/* ---------------------------------[0] �� ��һ�� -------------- */
	strcpy((char*)(lpirwp_info->irwp_author[0].ai_name), 
				(const char *)"YZ");
	strcpy((char*)(lpirwp_info->irwp_author[0].ai_email), 
				(const char *)"yzfree##yeah.net");
	strcpy((char*)(lpirwp_info->irwp_author[0].ai_message), 
				(const char *)"Hi��EMFģ�鹤����������^_^");
	/* ---------------------------------[1] �� �ڶ��� -------------- */
	strcpy((char*)(lpirwp_info->irwp_author[1].ai_name), 
				(const char *)"swstudio");
	strcpy((char*)(lpirwp_info->irwp_author[1].ai_email), 
				(const char *)"swstudio##sohu.com");
	strcpy((char*)(lpirwp_info->irwp_author[1].ai_message), 
				(const char *)"EMF��ʾ�ü�^_^");
	/* ---------------------------------[2] �� ������ -------------- */
	/* ������������Ϣ�ɼ��ڴ˴���
	strcpy((char*)(lpirwp_info->irwp_author[2].ai_name), 
				(const char *)"");
	strcpy((char*)(lpirwp_info->irwp_author[2].ai_email), 
				(const char *)"@");
	strcpy((char*)(lpirwp_info->irwp_author[2].ai_message), 
				(const char *)":)");
	*/
	/* ------------------------------------------------------------- */


	/* ���������Ϣ����չ����Ϣ��*/
	strcpy((char*)(lpirwp_info->irwp_desc_info.idi_currency_name), 
				(const char *)MODULE_FILE_POSTFIX);

	lpirwp_info->irwp_desc_info.idi_rev = 0;

	/* ����������##���ֶ������� */
	lpirwp_info->irwp_desc_info.idi_synonym_count = 0;

	/* ���ó�ʼ����ϱ�־ */
	lpirwp_info->init_tag = 1;

	return;
}



/* ��ȡͼ����Ϣ */
EMF_API int CALLAGREEMENT emf_get_image_info(PISADDR psct, LPINFOSTR pinfo_str)
{
#	ifdef WIN32
	ISFILE			*pfile = (ISFILE*)0;

	enum EXERESULT	b_status = ER_SUCCESS;

	assert(psct&&pinfo_str);
	assert(pinfo_str->sct_mark == INFOSTR_DBG_MARK);
	assert(pinfo_str->data_state < 2);	/* ������ݰ���������ͼ��λ���ݣ������ٸı���е�ͼ����Ϣ */	

	__try
	{
		__try
		{
			/* ����ؼ��� */
			EnterCriticalSection(&emf_get_info_critical);

			/* ��ָ���� */
			if ((pfile = isio_open((const char *)psct, "rb")) == (ISFILE*)0)
			{ 
				b_status = ER_FILERWERR; __leave;	
			}
			
			/* ��ȡ�ļ�ͷ�ṹ */
			if (isio_seek(pfile, 0, SEEK_SET) == -1)
			{
				b_status = ER_FILERWERR; __leave;
			}

			if ((b_status = _verify_file(pfile)) != ER_SUCCESS)
			{
				__leave;
			}

			if ((b_status = _get_info(pfile, pinfo_str)) != ER_SUCCESS)
			{
				__leave;
			}

			
			/* �趨���ݰ�״̬ */
			pinfo_str->data_state = 1;
		}
		__finally
		{
			if (pfile)
				isio_close(pfile);

			LeaveCriticalSection(&emf_get_info_critical);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		pinfo_str->data_state = 0;
		b_status = ER_SYSERR;
	}

#	else

	b_status = ER_NOTSUPPORT;

#	endif /* WIN32 */

	return (int)b_status;
}


/* ��ȡͼ��λ���� */
EMF_API int CALLAGREEMENT emf_load_image(PISADDR psct, LPINFOSTR pinfo_str)
{
#	ifdef WIN32
	ISFILE			*pfile = (ISFILE*)0;

	enum EXERESULT	b_status = ER_SUCCESS;

	HENHMETAFILE hemf;

	assert(psct&&pinfo_str);
	assert(pinfo_str->sct_mark == INFOSTR_DBG_MARK);
	assert(pinfo_str->data_state < 2);	/* ���ݰ��в��ܴ���ͼ��λ���� */	

	__try
	{
		__try
		{
			EnterCriticalSection(&emf_load_img_critical);

			/* ���� */
			if ((pfile = isio_open((const char *)psct, "rb")) == (ISFILE*)0)
			{
				b_status = ER_FILERWERR; __leave;
			}
			
			pinfo_str->process_total = pinfo_str->height;
			pinfo_str->process_current = 0;

			if (pinfo_str->break_mark)
			{
				b_status = ER_USERBREAK; __leave;
			}

			/* ���ļ�ͷ�ṹ */
			if (isio_seek(pfile, 0, SEEK_SET) == -1)
			{
				b_status = ER_FILERWERR; __leave;
			}

			/* ��֤emf�ļ�����Ч�� */
			if ((b_status = _verify_file(pfile)) != ER_SUCCESS)
			{
				__leave;
			}
			
			/* ����ǿյ����ݰ������Ȼ�ȡͼ���Ҫ��Ϣ���ɹ����ٶ�ȡͼ�� */
			if (pinfo_str->data_state == 0)
			{
				b_status = _get_info(pfile, pinfo_str);
				if (b_status != ER_SUCCESS)
				{
					__leave;
				}
				pinfo_str->data_state = 1;
			}
			
			assert(pinfo_str->data_state == 1);

			if ((b_status = _load_enh_metafile(pfile, &hemf)) != ER_SUCCESS)
			{
				__leave;
			}

			assert(hemf);

			if ((b_status = _enh_meta_to_raster(hemf, pinfo_str)) != ER_SUCCESS)
			{
				__leave;
			}
			
			/* ��д���ݰ��������� */
			/*!! need to be modified */
			pinfo_str->pal_count = 0;
			pinfo_str->imgnumbers = 1;
			pinfo_str->psubimg = NULL;	
						
			/* ########################################################## */

			/* �������� */
			pinfo_str->process_current = pinfo_str->process_total;

			pinfo_str->data_state = 2;
		}
		__finally
		{
			if ((b_status != ER_SUCCESS)||(AbnormalTermination()))
			{
				if (pinfo_str->p_bit_data)
				{
					free(pinfo_str->p_bit_data);
					pinfo_str->p_bit_data = (unsigned char *)0;
				}
				if (pinfo_str->pp_line_addr)
				{
					free(pinfo_str->pp_line_addr);
					pinfo_str->pp_line_addr = (void**)0;
				}
				if (pinfo_str->data_state == 2)
					pinfo_str->data_state =1;	/* �Զ����� */
			}

			if (pfile)
				isio_close(pfile);
			
			LeaveCriticalSection(&emf_load_img_critical);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		b_status = ER_SYSERR;
	}

#	else

	b_status = ER_NOTSUPPORT;

#	endif /* WIN32 */
	
	return (int)b_status;
}


/* ����ͼ�� */
EMF_API int CALLAGREEMENT emf_save_image(PISADDR psct, LPINFOSTR pinfo_str, LPSAVESTR lpsave)
{
	enum EXERESULT	b_status = ER_SUCCESS;
	
	assert(psct&&lpsave&&pinfo_str);
	
	__try
	{
		__try
		{
			EnterCriticalSection(&emf_save_img_critical);
	
			/* ��ǰ������֧�ֱ��湦�� */
			b_status = ER_NOTSUPPORT;
			
			/* �������� */
			pinfo_str->process_current = pinfo_str->process_total;
		}
		__finally
		{
			LeaveCriticalSection(&emf_save_img_critical);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		b_status = ER_SYSERR;
	}
	
	return (int)b_status;
}





/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/* �ڲ��������� */

/* ����ɨ���гߴ�(���ֽڶ���) */
int CALLAGREEMENT _calcu_scanline_size(int w/* ���� */, int bit/* λ�� */)
{
	return DIBSCANLINE_WIDTHBYTES(w*bit);
}



static int _verify_file(ISFILE* pfile)
{
	enum EXERESULT	b_status = ER_SUCCESS;

	unsigned int	filesize;
	ENHMETAHEADER	emh;

	__try
	{
		__try
		{
			/* ������֤�ļ���С����Ӧ��С��emf�ļ�ͷ�Ĵ�С */
			if (isio_seek(pfile, 0, SEEK_END) == -1
				|| (filesize = isio_tell(pfile)) == -1)
			{
				b_status = ER_FILERWERR; __leave;
			}
			if (filesize < sizeof(emh))
			{
				b_status = ER_BADIMAGE; __leave;
			}
            
			/* ��ȡemf�ļ�ͷ */
			if (isio_seek(pfile, 0, SEEK_SET) == -1
				|| isio_read(&emh, sizeof(emh), 1, pfile) != 1) 
			{
				b_status = ER_FILERWERR; __leave;
			}

			/* ȷ�����ļ�ͷ��¼,����֤�ļ����� */
			if (emh.iType != EMR_HEADER
				|| emh.dSignature != ENHMETA_SIGNATURE
				|| emh.nBytes > filesize)
			{
				b_status = ER_BADIMAGE; __leave;
			}

			/* ��֤�汾�� */
			if (emh.nVersion != 0x10000)
			{
				b_status = ER_NONIMAGE; __leave;
			}
		}
		__finally
		{
			
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		b_status = ER_SYSERR;
	}

	return (int)b_status;
}



static int _get_info(ISFILE* pfile, LPINFOSTR pinfo_str)
{
	enum EXERESULT	b_status = ER_SUCCESS;
	
	HDC				hdc = NULL;
	
	assert(pfile);
	assert(pinfo_str);

	__try
	{
		__try
		{
			hdc = GetDC(GetDesktopWindow());
			if (hdc == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}
			
			/* ����ͼ������߶� */
			{
				int width, height, t;
				ENHMETAHEADER	emh;

				if (isio_seek(pfile, 0, SEEK_SET) == -1
					|| isio_read(&emh, sizeof(emh), 1, pfile) != 1)
				{
					b_status = ER_FILERWERR; __leave;
				}

				/* ����� 0.01 mm Ϊ��λ��ͼ��ߴ� */
				width = emh.rclFrame.right - emh.rclFrame.left;
				height = emh.rclFrame.bottom - emh.rclFrame.top;

				/* ת��Ϊ������Ϊ��λ�ĳߴ� */
                t = GetDeviceCaps(hdc, HORZSIZE) * 100;
				width = (width * GetDeviceCaps(hdc, HORZRES) + t / 2) / t;
				t = GetDeviceCaps(hdc, VERTSIZE) * 100;
				height = (height * GetDeviceCaps(hdc, VERTRES) + t / 2) / t;
				
				pinfo_str->width = width;
				pinfo_str->height = height;
			}

			/* ����ͼ�����͡��洢��ʽ������ѹ����ʽ */
			pinfo_str->imgtype     = IMT_VECTORSTATIC;
			pinfo_str->imgformat   = IMF_EMF;
			pinfo_str->compression = ICS_GDIRECORD;

			/* ����ͼ��Ϊ������� */
			pinfo_str->order = 1;

			/* ����λ��ȣ�ֱ���趨Ϊ24bit */
			pinfo_str->bitcount = 24;

			/* ���ø���ɫ�������� */
			pinfo_str->r_mask = 0xff0000;
			pinfo_str->g_mask = 0xff00;
			pinfo_str->b_mask = 0xff;
			pinfo_str->a_mask = 0x0;
		}
		__finally
		{
			if (hdc != NULL)
			{
				ReleaseDC(GetDesktopWindow(), hdc);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		b_status = ER_SYSERR;
	}
	
	return (int)b_status;
	
}




static int _load_enh_metafile(ISFILE* pfile, HENHMETAFILE* phout)
{
	enum EXERESULT	b_status = ER_SUCCESS;

	ENHMETAHEADER	emh;
	unsigned char*	embits = NULL;
	
	assert(pfile);

	__try
	{
		__try
		{
			/* ��ȡemf�ļ�ͷ������ļ���С */
			if (isio_seek(pfile, 0, SEEK_SET) == -1
				|| isio_read(&emh, sizeof(emh), 1, pfile) != 1)
			{
				b_status = ER_FILERWERR; __leave;
			}

			/* ��ȡ�ļ����ݣ�����emf handle */
			embits = (unsigned char*)malloc(emh.nBytes);
			if (embits == NULL)
			{
				b_status = ER_MEMORYERR; __leave;
			}

			if (isio_seek(pfile, 0, SEEK_SET) == -1
				|| isio_read(embits, 1, emh.nBytes, pfile) != emh.nBytes)
			{
				b_status = ER_FILERWERR; __leave;
			}

			*phout = SetEnhMetaFileBits(emh.nBytes, embits);
			if (*phout == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}

		}
		__finally
		{
			if (embits != NULL)
			{
				free(embits);
			}
			if (b_status != ER_SUCCESS || AbnormalTermination())
			{
				*phout = NULL;
			}

		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		b_status = ER_SYSERR;
	}
	
	return (int)b_status;
}



static int _enh_meta_to_raster(HENHMETAFILE hemf, LPINFOSTR pinfo_str)
{
	enum EXERESULT	b_status = ER_SUCCESS;

	unsigned char	*dib_bits = NULL;
	HDC				hdc = NULL, hComDC = NULL;
	HBITMAP			bmp = NULL, oldbmp = NULL;
	BITMAPINFO		bmi;
	RECT			rect;
	int				linesize;
	unsigned		i;
	unsigned char	**ppt;

	assert(hemf);
	assert(pinfo_str);

	__try
	{
		__try
		{
			/* ����������ļ���DC */
			if ((hdc = GetDC(GetDesktopWindow())) == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}

			memset(&bmi, 0, sizeof(bmi));
			bmi.bmiHeader.biSize          = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth         = pinfo_str->width;
			bmi.bmiHeader.biHeight        = pinfo_str->height;
			bmi.bmiHeader.biPlanes        = 1;
			bmi.bmiHeader.biBitCount      = (WORD)pinfo_str->bitcount;
			bmi.bmiHeader.biCompression   = BI_RGB;
			bmi.bmiHeader.biXPelsPerMeter = GetDeviceCaps(hdc, HORZRES)*1000/GetDeviceCaps(hdc, HORZSIZE);
			bmi.bmiHeader.biYPelsPerMeter = GetDeviceCaps(hdc, VERTRES)*1000/GetDeviceCaps(hdc, VERTSIZE);

			/* ����λͼ��� */
            bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &dib_bits, NULL, 0);
			if (bmp == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}

			/* ��������DC */
			if ((hComDC = CreateCompatibleDC(hdc)) == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}

			oldbmp = SelectObject(hComDC, bmp);
			
			/* ��ͼ�󱳾����Ϊ��ɫ */
			rect.left = rect.top = 0;
			rect.right = pinfo_str->width;
			rect.bottom = pinfo_str->height;
			FillRect(hComDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

			/* �����DC����emf���� */
			PlayEnhMetaFile(hComDC, hemf, &rect);

			/* ���õõ���λͼ����������ݰ������ */
			linesize = _calcu_scanline_size(pinfo_str->width, pinfo_str->bitcount);

			if ((pinfo_str->p_bit_data = (unsigned char*)malloc(linesize*pinfo_str->height)) == NULL
                || (pinfo_str->pp_line_addr = malloc(sizeof(unsigned char*)*pinfo_str->height)) == NULL)
			{
				b_status = ER_MEMORYERR; __leave;
			}

			/* ���������ݿ��������ݰ� */
			memcpy(pinfo_str->p_bit_data, dib_bits, linesize*pinfo_str->height);

			/* ��д���׵�ַ���� */
			ppt = (unsigned char**)pinfo_str->pp_line_addr;
			for (i = 0; i < pinfo_str->height; i++)
			{
				ppt[i] = (void *)(pinfo_str->p_bit_data+((pinfo_str->height-i-1)*linesize));
			}
		}
		__finally
		{
			if (hdc != NULL)
			{
				ReleaseDC(GetDesktopWindow(), hdc);
			}
			if (hComDC != NULL)
			{
				DeleteDC(hComDC);
			}
			if (bmp != NULL)
			{
				DeleteObject(bmp);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		b_status = ER_SYSERR;
	}

	return (int)b_status;
}

