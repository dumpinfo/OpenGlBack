/********************************************************************

	filter.h - ISee图像浏览器—图像处理模块实现代码头文件

    版权所有(C) VCHelp-coPathway-ISee workgroup 2000 all member's

    这一程序是自由软件，你可以遵照自由软件基金会出版的GNU 通用许可证
	条款来修改和重新发布这一程序。或者用许可证的第二版，或者（根据你
	的选择）用任何更新的版本。

    发布这一程序的目的是希望它有用，但没有任何担保。甚至没有适合特定
	目地的隐含的担保。更详细的情况请参阅GNU通用许可证。

    你应该已经和程序一起收到一份GNU通用许可证(GPL)的副本。如果还没有，
	写信给：
    The Free Software Foundation, Inc.,  675  Mass Ave,  Cambridge,
    MA02139,  USA

	如果你在使用本软件时有什么问题或建议，用以下地址可以与我们取得联
	系：
		http://isee.126.com
		http://www.vchelp.net
	或：
		iseesoft@china.com

	作者：临风
   e-mail:ringphone@sina.com

   功能实现：图像旋转处理功能函数定义

	文件版本：
		Build 00617
		Date  2000-6-17

********************************************************************/

extern LPIMAGEPROCSTR lpProcInfo;
extern LPVOID lpPreViewData,lpBakData;
extern HBITMAP hbm;         				//对话框背景图像

int WINAPI FAR _export AccessPropFilter(LPIMAGEPROCSTR lpInfo);

int ShowWriterMessage();
void ShowCopyright();

void PaintDlgBk(HWND hWnd);						//绘制对话框背景

int DoRotate(LPIMAGEPROCSTR lpInfo);	//旋转
int Output_Rotate();                   //输出旋转后的图像数据

int DoGreyScale(LPIMAGEPROCSTR lpInfo);//灰度转换

int DoResize(LPIMAGEPROCSTR lpInfo);
int Output_Resize(int width,int height,LPBYTE lpDestData);//重定义尺寸

