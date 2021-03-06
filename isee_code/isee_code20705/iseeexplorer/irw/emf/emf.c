/********************************************************************

	emf.c

	----------------------------------------------------------------
    软件许可证 － GPL
	版权所有 (C) 2001 VCHelp coPathway ISee workgroup.
	----------------------------------------------------------------
	这一程序是自由软件，你可以遵照自由软件基金会出版的GNU 通用公共许
	可证条款来修改和重新发布这一程序。或者用许可证的第二版，或者（根
	据你的选择）用任何更新的版本。

    发布这一程序的目的是希望它有用，但没有任何担保。甚至没有适合特定
	目地的隐含的担保。更详细的情况请参阅GNU通用公共许可证。

    你应该已经和程序一起收到一份GNU通用公共许可证的副本（本目录
	GPL.txt文件）。如果还没有，写信给：
    The Free Software Foundation, Inc.,  675  Mass Ave,  Cambridge,
    MA02139,  USA
	----------------------------------------------------------------
	如果你在使用本软件时有什么问题或建议，请用以下地址与我们取得联系：

			http://isee.126.com
			http://cosoft.org.cn/projects/iseeexplorer

	或发信到：

			isee##vip.163.com
	----------------------------------------------------------------
	本文件用途：	ISee图像浏览器—EMF图像读写模块实现文件

					读取功能：	可读取增强型元文件（EMF），部分文件的文本显示有问题
							  
					保存功能：	不支持保存功能
							   

	本文件编写人：	YZ			yzfree##yeah.net
					swstudio	swstudio##sohu.com

	本文件版本：	20621
	最后修改于：	2002-6-21

	注：以上E-Mail地址中的##请用@替换，这样做是为了抵制恶意的E-Mail
	    地址收集软件。
  	----------------------------------------------------------------
	修正历史：

		2002-6		第二版插件发布（新版）
		2001-1		修正部分注释信息
		2000-7		修正多处BUG，并加强了模块的容错性
		2000-6		第一个版本发布


********************************************************************/


#ifndef WIN32
#if defined(_WIN32)||defined(_WINDOWS)
#define WIN32
#endif
#endif /* WIN32 */

/*###################################################################

  移植注释：以下代码使用了WIN32系统的SEH(结构化异常处理)及多线程同步
			对象“关键段”，在移植时应转为Linux的相应语句。

  #################################################################*/


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN				/* 缩短windows.h文件的编译时间 */
#include <windows.h>
#endif /* WIN32 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "emf.h"


IRWP_INFO			emf_irwp_info;			/* 插件信息表 */

#ifdef WIN32
CRITICAL_SECTION	emf_get_info_critical;	/* emf_get_image_info函数的关键段 */
CRITICAL_SECTION	emf_load_img_critical;	/* emf_load_image函数的关键段 */
CRITICAL_SECTION	emf_save_img_critical;	/* emf_save_image函数的关键段 */
#else
/* Linux对应的语句 */
#endif


/* 内部助手函数 */
void CALLAGREEMENT _init_irwp_info(LPIRWP_INFO lpirwp_info);
int CALLAGREEMENT _calcu_scanline_size(int w/* 宽度 */, int bit/* 位深 */);

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
			/* 初始化插件信息表 */
			_init_irwp_info(&emf_irwp_info);

			/* 初始化访问关键段 */
			InitializeCriticalSection(&emf_get_info_critical);
			InitializeCriticalSection(&emf_load_img_critical);
			InitializeCriticalSection(&emf_save_img_critical);

			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			/* 销毁访问关键段 */
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
	/* 初始化多线程同步对象 */
}

EMF_API void CALLAGREEMENT emf_detach_plugin()
{
	/* 销毁多线程同步对象 */
}

#endif /* WIN32 */


/* 初始化插件信息结构 */
void CALLAGREEMENT _init_irwp_info(LPIRWP_INFO lpirwp_info)
{
	assert(lpirwp_info);

	/* 初始化结构变量 */
	memset((void*)lpirwp_info, 0, sizeof(IRWP_INFO));

	/* 版本号。（十进制值，十位为主版本号，个位为副版本，*/
	lpirwp_info->irwp_version = MODULE_BUILDID;
	/* 插件名称 */
	strcpy((char*)(lpirwp_info->irwp_name), MODULE_NAME);
	/* 本模块函数前缀 */
	strcpy((char*)(lpirwp_info->irwp_func_prefix), MODULE_FUNC_PREFIX);


	/* 插件的发布类型。0－调试版插件，1－发布版插件 */
#ifdef _DEBUG
	lpirwp_info->irwp_build_set = 0;
#else
	lpirwp_info->irwp_build_set = 1;
#endif


	/* 功能标识 （##需手动修正） */
	lpirwp_info->irwp_function = IRWP_READ_SUPP;

	/* 设置模块支持的保存位深 */
	lpirwp_info->irwp_save.bitcount = 0;
	lpirwp_info->irwp_save.img_num = 0;
	lpirwp_info->irwp_save.count = 0;

	/* 开发者人数（即开发者信息中有效项的个数）（##需手动修正）*/
	lpirwp_info->irwp_author_count = 2;	


	/* 开发者信息（##需手动修正） */
	/* ---------------------------------[0] － 第一组 -------------- */
	strcpy((char*)(lpirwp_info->irwp_author[0].ai_name), 
				(const char *)"YZ");
	strcpy((char*)(lpirwp_info->irwp_author[0].ai_email), 
				(const char *)"yzfree##yeah.net");
	strcpy((char*)(lpirwp_info->irwp_author[0].ai_message), 
				(const char *)"Hi，EMF模块工作的正常吗^_^");
	/* ---------------------------------[1] － 第二组 -------------- */
	strcpy((char*)(lpirwp_info->irwp_author[1].ai_name), 
				(const char *)"swstudio");
	strcpy((char*)(lpirwp_info->irwp_author[1].ai_email), 
				(const char *)"swstudio##sohu.com");
	strcpy((char*)(lpirwp_info->irwp_author[1].ai_message), 
				(const char *)"EMF显示好简单^_^");
	/* ---------------------------------[2] － 第三组 -------------- */
	/* 后续开发者信息可加在此处。
	strcpy((char*)(lpirwp_info->irwp_author[2].ai_name), 
				(const char *)"");
	strcpy((char*)(lpirwp_info->irwp_author[2].ai_email), 
				(const char *)"@");
	strcpy((char*)(lpirwp_info->irwp_author[2].ai_message), 
				(const char *)":)");
	*/
	/* ------------------------------------------------------------- */


	/* 插件描述信息（扩展名信息）*/
	strcpy((char*)(lpirwp_info->irwp_desc_info.idi_currency_name), 
				(const char *)MODULE_FILE_POSTFIX);

	lpirwp_info->irwp_desc_info.idi_rev = 0;

	/* 别名个数（##需手动修正） */
	lpirwp_info->irwp_desc_info.idi_synonym_count = 0;

	/* 设置初始化完毕标志 */
	lpirwp_info->init_tag = 1;

	return;
}



/* 获取图像信息 */
EMF_API int CALLAGREEMENT emf_get_image_info(PISADDR psct, LPINFOSTR pinfo_str)
{
#	ifdef WIN32
	ISFILE			*pfile = (ISFILE*)0;

	enum EXERESULT	b_status = ER_SUCCESS;

	assert(psct&&pinfo_str);
	assert(pinfo_str->sct_mark == INFOSTR_DBG_MARK);
	assert(pinfo_str->data_state < 2);	/* 如果数据包中已有了图像位数据，则不能再改变包中的图像信息 */	

	__try
	{
		__try
		{
			/* 进入关键段 */
			EnterCriticalSection(&emf_get_info_critical);

			/* 打开指定流 */
			if ((pfile = isio_open((const char *)psct, "rb")) == (ISFILE*)0)
			{ 
				b_status = ER_FILERWERR; __leave;	
			}
			
			/* 读取文件头结构 */
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

			
			/* 设定数据包状态 */
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


/* 读取图像位数据 */
EMF_API int CALLAGREEMENT emf_load_image(PISADDR psct, LPINFOSTR pinfo_str)
{
#	ifdef WIN32
	ISFILE			*pfile = (ISFILE*)0;

	enum EXERESULT	b_status = ER_SUCCESS;

	HENHMETAFILE hemf;

	assert(psct&&pinfo_str);
	assert(pinfo_str->sct_mark == INFOSTR_DBG_MARK);
	assert(pinfo_str->data_state < 2);	/* 数据包中不能存在图像位数据 */	

	__try
	{
		__try
		{
			EnterCriticalSection(&emf_load_img_critical);

			/* 打开流 */
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

			/* 读文件头结构 */
			if (isio_seek(pfile, 0, SEEK_SET) == -1)
			{
				b_status = ER_FILERWERR; __leave;
			}

			/* 验证emf文件的有效性 */
			if ((b_status = _verify_file(pfile)) != ER_SUCCESS)
			{
				__leave;
			}
			
			/* 如果是空的数据包，首先获取图像概要信息，成功后再读取图像 */
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
			
			/* 填写数据包的其他域 */
			/*!! need to be modified */
			pinfo_str->pal_count = 0;
			pinfo_str->imgnumbers = 1;
			pinfo_str->psubimg = NULL;	
						
			/* ########################################################## */

			/* 结束操作 */
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
					pinfo_str->data_state =1;	/* 自动降级 */
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


/* 保存图像 */
EMF_API int CALLAGREEMENT emf_save_image(PISADDR psct, LPINFOSTR pinfo_str, LPSAVESTR lpsave)
{
	enum EXERESULT	b_status = ER_SUCCESS;
	
	assert(psct&&lpsave&&pinfo_str);
	
	__try
	{
		__try
		{
			EnterCriticalSection(&emf_save_img_critical);
	
			/* 当前还不能支持保存功能 */
			b_status = ER_NOTSUPPORT;
			
			/* 结束操作 */
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
/* 内部辅助函数 */

/* 计算扫描行尺寸(四字节对齐) */
int CALLAGREEMENT _calcu_scanline_size(int w/* 宽度 */, int bit/* 位深 */)
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
			/* 首先验证文件大小，不应该小于emf文件头的大小 */
			if (isio_seek(pfile, 0, SEEK_END) == -1
				|| (filesize = isio_tell(pfile)) == -1)
			{
				b_status = ER_FILERWERR; __leave;
			}
			if (filesize < sizeof(emh))
			{
				b_status = ER_BADIMAGE; __leave;
			}
            
			/* 读取emf文件头 */
			if (isio_seek(pfile, 0, SEEK_SET) == -1
				|| isio_read(&emh, sizeof(emh), 1, pfile) != 1) 
			{
				b_status = ER_FILERWERR; __leave;
			}

			/* 确定是文件头记录,并验证文件长度 */
			if (emh.iType != EMR_HEADER
				|| emh.dSignature != ENHMETA_SIGNATURE
				|| emh.nBytes > filesize)
			{
				b_status = ER_BADIMAGE; __leave;
			}

			/* 验证版本号 */
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
			
			/* 设置图像宽、高度 */
			{
				int width, height, t;
				ENHMETAHEADER	emh;

				if (isio_seek(pfile, 0, SEEK_SET) == -1
					|| isio_read(&emh, sizeof(emh), 1, pfile) != 1)
				{
					b_status = ER_FILERWERR; __leave;
				}

				/* 获得以 0.01 mm 为单位的图像尺寸 */
				width = emh.rclFrame.right - emh.rclFrame.left;
				height = emh.rclFrame.bottom - emh.rclFrame.top;

				/* 转换为以像素为单位的尺寸 */
                t = GetDeviceCaps(hdc, HORZSIZE) * 100;
				width = (width * GetDeviceCaps(hdc, HORZRES) + t / 2) / t;
				t = GetDeviceCaps(hdc, VERTSIZE) * 100;
				height = (height * GetDeviceCaps(hdc, VERTRES) + t / 2) / t;
				
				pinfo_str->width = width;
				pinfo_str->height = height;
			}

			/* 设置图像类型、存储格式和数据压缩方式 */
			pinfo_str->imgtype     = IMT_VECTORSTATIC;
			pinfo_str->imgformat   = IMF_EMF;
			pinfo_str->compression = ICS_GDIRECORD;

			/* 设置图像为倒向放置 */
			pinfo_str->order = 1;

			/* 设置位深度，直接设定为24bit */
			pinfo_str->bitcount = 24;

			/* 设置各颜色分量掩码 */
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
			/* 读取emf文件头，获得文件大小 */
			if (isio_seek(pfile, 0, SEEK_SET) == -1
				|| isio_read(&emh, sizeof(emh), 1, pfile) != 1)
			{
				b_status = ER_FILERWERR; __leave;
			}

			/* 读取文件内容，创建emf handle */
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
			/* 创建与桌面的兼容DC */
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

			/* 创建位图句柄 */
            bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &dib_bits, NULL, 0);
			if (bmp == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}

			/* 创建兼容DC */
			if ((hComDC = CreateCompatibleDC(hdc)) == NULL)
			{
				b_status = ER_SYSERR; __leave;
			}

			oldbmp = SelectObject(hComDC, bmp);
			
			/* 将图象背景填充为白色 */
			rect.left = rect.top = 0;
			rect.right = pinfo_str->width;
			rect.bottom = pinfo_str->height;
			FillRect(hComDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

			/* 向兼容DC播放emf数据 */
			PlayEnhMetaFile(hComDC, hemf, &rect);

			/* 利用得到的位图数据填充数据包相关域 */
			linesize = _calcu_scanline_size(pinfo_str->width, pinfo_str->bitcount);

			if ((pinfo_str->p_bit_data = (unsigned char*)malloc(linesize*pinfo_str->height)) == NULL
                || (pinfo_str->pp_line_addr = malloc(sizeof(unsigned char*)*pinfo_str->height)) == NULL)
			{
				b_status = ER_MEMORYERR; __leave;
			}

			/* 将象素数据拷贝至数据包 */
			memcpy(pinfo_str->p_bit_data, dib_bits, linesize*pinfo_str->height);

			/* 填写行首地址数组 */
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


