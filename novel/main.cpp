#include <windows.h>
#include <commctrl.h>
#include <thread>
#include <gdiplus.h>
#include "resource.h"
#include "tool.h"

#pragma comment(lib, "gdiplus.lib")
ULONG_PTR m_gdiplusToken;
BOOL NovelContent(HWND hwnd, int contents);

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowNovelInfo(HWND hwnd);
void ShowChapterInfo(HWND hwnd, int chapter);

// 清理干净ListView
void ClearShowListView(HWND hwnd, WORD childWindow);
// 切换显示指定ListView
void SwitchShowWindows(HWND hwnd, WORD prWindow, WORD newWindow);

HINSTANCE hgInst;

int WINAPI WinMain(HINSTANCE hThisApp, HINSTANCE hPrevApp, LPSTR lpCmd, int nShow)
{
	// Init GDI+    
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	hgInst = hThisApp;
	HWND hdlg = CreateDialog(hThisApp, MAKEINTRESOURCE(IDD_NOVEL), NULL, (DLGPROC)DlgProc);

	if (!hdlg)
	{
		return 0;
	}
	ShowWindow(hdlg, SW_SHOW);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// delete GDI
	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	/*case WM_CREATE:
	{
		DWORD exStyle = ::GetWindowLong(hDlg, GWL_EXSTYLE);
		exStyle |= WS_EX_LAYERED;
		::SetWindowLong(hDlg, GWL_EXSTYLE, exStyle);
		::SetLayeredWindowAttributes(hDlg, RGB(0, 0, 0), 0, LWA_ALPHA);
		return 0;
	}*/
	case WM_INITDIALOG:
	{
		// 设置对话框的图标 
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hgInst, MAKEINTRESOURCE(IDI_ICON1)));

		//	获取ListView控件的句柄  
		HWND hListview = GetDlgItem(hDlg, IDC_NOVEL_LIST);
		//	设置ListView的列  
		LVCOLUMN vcl;
		vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		//	第一列  
		vcl.pszText = L"序号";//列标题  
		vcl.cx = 40;//列宽  
		vcl.iSubItem = 0;//子项索引，第一列无子项  
		ListView_InsertColumn(hListview, 0, &vcl);
		//	第二列  
		vcl.pszText = L"资源名称";
		vcl.cx = 250;
		vcl.iSubItem = 1;//子项索引  
		ListView_InsertColumn(hListview, 1, &vcl);
		//	第三列  
		vcl.pszText = L"资源链接";
		vcl.cx = 250;
		vcl.iSubItem = 2;
		ListView_InsertColumn(hListview, 2, &vcl); 
		ListView_SetExtendedListViewStyle(hListview, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);//设置listview扩展风格  

		return 0;
	}
	case WM_PAINT:
	{
		// 绘制背景图
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);
		Gdiplus::Image* image = Gdiplus::Image::FromFile(L"background2.jpg");
		if (image->GetLastStatus() != Gdiplus::Ok)
		{
			MessageBox(hDlg, L"加载图片失败!", L"提示", MB_OK);
			return -1;
		}
		RECT videoimgrect;
		GetClientRect(hDlg, &videoimgrect);
		//取得宽度和高度  
		Gdiplus::Image* pThumbnail = image->GetThumbnailImage(
			videoimgrect.right - videoimgrect.left,
			videoimgrect.bottom - videoimgrect.top,
			NULL,
			NULL);

		//绘制背景图  
		Gdiplus::Graphics graphics(hdc);
		graphics.DrawImage(image, videoimgrect.left, videoimgrect.top, pThumbnail->GetWidth(), pThumbnail->GetHeight());

		delete pThumbnail;
		delete image;
		EndPaint(hDlg, &ps);
	}
	case WM_SYSCOMMAND:
	{
		if (wParam == SC_CLOSE)
		{
			PostQuitMessage(0);//退出     
		}
		return 0;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// 加载完成，激活“开始搜索按钮”
			EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			
			// 清除listview数据
			HWND m_list = GetDlgItem(hDlg, IDC_NOVEL_LIST);
			ListView_DeleteAllItems(m_list);

			// 按切换到搜索小说的listview
			HWND videoimg = GetDlgItem(hDlg, IDC_NOVELCONTENT);
			if (IsWindowVisible(videoimg))
			{
				SwitchShowWindows(hDlg, IDC_NOVELCONTENT, IDC_NOVEL_LIST);
			}
			else
			{
				SwitchShowWindows(hDlg, IDC_CHAPTERS, IDC_NOVEL_LIST);
			}

			std::thread action(ShowNovelInfo, hDlg);
			action.detach();
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		case IDC_BACK:
		{
			HWND videoimg = GetDlgItem(hDlg, IDC_NOVELCONTENT);
			if (IsWindowVisible(videoimg))
			{
				// 按返回键切换到先前的listview
				SwitchShowWindows(hDlg, IDC_NOVELCONTENT, IDC_CHAPTERS);
			}
			else
			{
				// 按返回键切换到先前的listview
				SwitchShowWindows(hDlg, IDC_CHAPTERS, IDC_NOVEL_LIST);
			}
			break;
		}		
		default:
			break;
		}
		break;
	}
	case WM_NOTIFY:
	{
		switch (wParam) {
		case IDC_CHAPTERS:
		{	//ID为listview的ID  
			LPNMITEMACTIVATE now1 = (LPNMITEMACTIVATE)lParam;//得到NMITEMACTIVATE结构指针  
			switch (now1->hdr.code) {//判断通知码  
			case NM_DBLCLK:
			{
				SetDlgItemText(hDlg, IDC_NOVELCONTENT, L"");
				SwitchShowWindows(hDlg, IDC_CHAPTERS, IDC_NOVELCONTENT);

				std::thread action(NovelContent, hDlg, now1->iItem);
				action.detach();
				break;
			}
			default:
				break;
			}
			break;
		}
		case IDC_NOVEL_LIST://ID为listview的ID  
			LPNMITEMACTIVATE now = (LPNMITEMACTIVATE)lParam;//得到NMITEMACTIVATE结构指针  
			//now->iItem项目序号 
			switch (now->hdr.code) {//判断通知码  
			case NM_CLICK:
				
				break;
			case NM_DBLCLK:
			{
			
				// 清除listview数据
				ClearShowListView(hDlg, IDC_CHAPTERS);

				// 按返回键切换到先前的listview
				SwitchShowWindows(hDlg, IDC_NOVEL_LIST, IDC_CHAPTERS);

				std::thread action(ShowChapterInfo, hDlg, now->iItem);
				action.detach();

				break;
			}
			case NM_RCLICK:
				break;
			}
			break;
		}
		break;
	}
	/*case WM_CTLCOLORSTATIC:
	{
		SetBkMode((HDC)wParam, TRANSPARENT);
		SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
		return (BOOL)((HBRUSH)GetStockObject(NULL_BRUSH));
	}*/
	case NM_CUSTOMDRAW:
	{
		LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
		// TODO: 在此添加控件通知处理程序代码
		LRESULT *pResult = 0;

		//This code based on Michael Dunn's excellent article on
		//list control custom draw at http://www.codeproject.com/listctrl/lvcustomdraw.asp

		NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);

		// Take the default processing unless we set this to something else below.
		*pResult = CDRF_DODEFAULT;
		// First thing - check the draw stage. If it's the control's prepaint
		// stage, then tell Windows we want messages for every item.
		if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
		}
		else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
		{
			// This is the notification message for an item.  We'll request
			// notifications before each subitem's prepaint stage.

			*pResult = CDRF_NOTIFYSUBITEMDRAW;
		}
		else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
		{

			COLORREF clrNewTextColor, clrNewBkColor;

			int    nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

			BOOL bSelect = TRUE;
			
			if (bSelect)
			{
				//clrNewTextColor = RGB(255,0,0);     //Set the text to red
				clrNewTextColor = RGB(0, 255, 0);     //Set the text to red
				clrNewBkColor = RGB(0, 0, 255);     //Set the bkgrnd color to blue
			}
			else
			{
				clrNewTextColor = RGB(0, 0, 0);     //Leave the text black
				clrNewBkColor = RGB(255, 255, 255);    //leave the bkgrnd color white
			}

			pLVCD->clrText = clrNewTextColor;
			pLVCD->clrTextBk = clrNewBkColor;


			// Tell Windows to paint the control itself.
			*pResult = CDRF_DODEFAULT;

		}
		break;
	}
	default:
		break;
	}
	return (INT_PTR)FALSE;
}

void ShowNovelInfo(HWND hwnd)										
{
	// 获取ListView控件的句柄  
	HWND hListview = GetDlgItem(hwnd, IDC_NOVEL_LIST);

	std::vector<DownResourceInfo> vtDownInfo;
	std::wstring id;
		
	TCHAR m_search[MAX_PATH];
	GetDlgItemText(hwnd, IDC_SEARCH, m_search, MAX_PATH);
	vtDownInfo = GetNovelList(m_search);

	if (vtDownInfo.size() > 0)
	{
		//求出数组中元素的个数  
		int arrCount = vtDownInfo.size();
		LVITEM vitem;
		vitem.mask = LVIF_TEXT;
		for (int i = 0; i < arrCount; i++)
		{
			/*
			策略：
			先添加项再设置子项内容
			*/
			id = std::to_wstring(vtDownInfo[i].rowNum);
			vitem.pszText = (LPWSTR)id.c_str();
			vitem.iItem = i;
			vitem.iSubItem = 0;
			ListView_InsertItem(hListview, &vitem);
			// 设置子项  
			vitem.pszText = (LPWSTR)vtDownInfo[i].strResourceName.c_str();
			vitem.iSubItem = 1;
			ListView_SetItem(hListview, &vitem);
			vitem.iSubItem = 2;
			vitem.pszText = (LPWSTR)vtDownInfo[i].strResourceLink.c_str();
			ListView_SetItem(hListview, &vitem);
		}
	}
	
	// 加载完成，激活“开始搜索按钮”
	EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
}
void ShowChapterInfo(HWND hwnd, int chapter)
{
	// 获取ListView控件的句柄  
	HWND hListview = GetDlgItem(hwnd, IDC_CHAPTERS);

	// 获取选定项的小说章节链接
	TCHAR pbuf[256];
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT;
	lvi.cchTextMax = 256;
	lvi.iItem = chapter;
	lvi.iSubItem = 2;
	lvi.pszText = pbuf;
	ListView_GetItem(GetDlgItem(hwnd, IDC_NOVEL_LIST), &lvi);

	if (wcscmp(L"", pbuf) == 0)
	{
		return;
	}

	//	设置ListView的列  
	LVCOLUMN vcl;
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	//	第一列  
	vcl.pszText = L"序号";//列标题  
	vcl.cx = 40;//列宽  
	vcl.iSubItem = 0;//子项索引，第一列无子项  
	ListView_InsertColumn(hListview, 0, &vcl);
	//	第二列  
	vcl.pszText = L"章节名称";
	vcl.cx = 250;
	vcl.iSubItem = 1;//子项索引  
	ListView_InsertColumn(hListview, 1, &vcl);
	//	第三列  
	vcl.pszText = L"章节链接";
	vcl.cx = 250;
	vcl.iSubItem = 2;
	ListView_InsertColumn(hListview, 2, &vcl);
	ListView_SetExtendedListViewStyle(hListview, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);//设置listview扩展风格  

	std::wstring id;
	std::vector<NovelChapter> vtDownInfo;
	vtDownInfo = GetNoveChapters(pbuf);

	if (vtDownInfo.size() > 0)
	{
		//求出数组中元素的个数  
		int arrCount = vtDownInfo.size();
		LVITEM vitem;
		vitem.mask = LVIF_TEXT;
		for (int i = 0; i < arrCount; i++)
		{
			/*
			策略：
			先添加项再设置子项内容
			*/
			id = std::to_wstring(i+1);
			vitem.pszText = (LPWSTR)id.c_str();
			vitem.iItem = i;
			vitem.iSubItem = 0;
			ListView_InsertItem(hListview, &vitem);
			// 设置子项  
			vitem.pszText = (LPWSTR)vtDownInfo[i].strChapterName.c_str();
			vitem.iSubItem = 1;
			ListView_SetItem(hListview, &vitem);
			vitem.iSubItem = 2;
			vitem.pszText = (LPWSTR)vtDownInfo[i].strChapterLink.c_str();
			ListView_SetItem(hListview, &vitem);
		}
	}
}

/*
第二个参数：需要隐藏的控件
第三个参数：需要显示的控件
*/
void SwitchShowWindows(HWND hwnd, WORD prWindow, WORD newWindow)
{
	HWND hPreListview = GetDlgItem(hwnd, prWindow);
	HWND hNewListview = GetDlgItem(hwnd, newWindow);

	ShowWindow(hPreListview, SW_HIDE);
	ShowWindow(hNewListview, SW_SHOWNORMAL);
}

/*
第一个参数 hwnd
第二个参数 指定ListView
*/
void ClearShowListView(HWND hwnd, WORD childWindow)
{
	int nCols;
	HWND hWndListView, hWndListViewHeader;
	hWndListView = GetDlgItem(hwnd, childWindow);
	
	//删除所有行
	ListView_DeleteAllItems(hWndListView);

	//得到ListView的Header窗体
	hWndListViewHeader = ListView_GetHeader(hWndListView);
	//得到列的数目
	nCols = Header_GetItemCount(hWndListViewHeader);

	nCols--;
	//删除所有列
	for (; nCols >= 0; nCols--)
		ListView_DeleteColumn(hWndListView, nCols);
}

BOOL NovelContent(HWND hDlg, int index)
{
	// 获取选定项的小说章节链接
	TCHAR pbuf[256];
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT;
	lvi.cchTextMax = 256;
	lvi.iItem = index;
	lvi.iSubItem = 2;
	lvi.pszText = pbuf;
	ListView_GetItem(GetDlgItem(hDlg, IDC_CHAPTERS), &lvi);
	std::wstring contents = GetChapterContent(pbuf);

	HWND videoimg = GetDlgItem(hDlg, IDC_NOVELCONTENT);

	LOGFONT LogFont;
	memset(&LogFont, 0, sizeof(LOGFONT));
	lstrcpy(LogFont.lfFaceName, L"微软雅黑");
	LogFont.lfWeight = FW_NORMAL;//FW_BLACK
	LogFont.lfHeight = -18; // 字体大小 
	LogFont.lfCharSet = 134;
	LogFont.lfOutPrecision = 3;
	LogFont.lfClipPrecision = 2;
	LogFont.lfOrientation = 45;
	LogFont.lfQuality = 1;
	LogFont.lfPitchAndFamily = 2;
	// 创建字体 
	HFONT hFont = CreateFontIndirect(&LogFont);
	// 取得控件句柄 
	SendMessage(videoimg, WM_SETFONT, (WPARAM)hFont, TRUE);

	SetDlgItemText(hDlg, IDC_NOVELCONTENT, contents.c_str());
	
	//HDC hdc = GetDC(videoimg);//BeginPaint(hDlg, &ps);

	////加载图像  
	//Gdiplus::Image* image = Gdiplus::Image::FromFile(L"background.jpg");
	//if (image->GetLastStatus() != Gdiplus::Ok)
	//{
	//	MessageBox(hDlg, L"加载图片失败!", L"提示", MB_OK);
	//	return -1;
	//}

	//RECT videoimgrect;
	//GetClientRect(videoimg, &videoimgrect);

	////取得宽度和高度  
	//Gdiplus::Image* pThumbnail = image->GetThumbnailImage(
	//	videoimgrect.right - videoimgrect.left,
	//	videoimgrect.bottom - videoimgrect.top,
	//	NULL,
	//	NULL);

	//Gdiplus::Graphics graphics(hdc);
	////绘制小说阅读背景图  
	////graphics.DrawImage(image, videoimgrect.left, videoimgrect.top, pThumbnail->GetWidth(), pThumbnail->GetHeight());
	////绘制小说内容
	//// Initialize arguments.
	//Gdiplus::Font myFont(L"微软雅黑", 10);
	//Gdiplus::RectF layoutRect(videoimgrect.left, videoimgrect.top, pThumbnail->GetWidth(), pThumbnail->GetHeight());
	//Gdiplus::StringFormat format;
	//format.SetAlignment(Gdiplus::StringAlignmentCenter);
	//Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 0, 0, 0));

	//graphics.DrawString(
	//	contents,
	//	wcslen(contents),
	//	&myFont,
	//	layoutRect,
	//	&format,
	//	&blackBrush);

	//delete pThumbnail;
	//delete image;
	//graphics.ReleaseHDC(hdc);

	////EndPaint(hDlg, &ps);

	return TRUE;
}
