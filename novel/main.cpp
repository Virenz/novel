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

// ����ɾ�ListView
void ClearShowListView(HWND hwnd, WORD childWindow);
// �л���ʾָ��ListView
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
		// ���öԻ����ͼ�� 
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hgInst, MAKEINTRESOURCE(IDI_ICON1)));

		//	��ȡListView�ؼ��ľ��  
		HWND hListview = GetDlgItem(hDlg, IDC_NOVEL_LIST);
		//	����ListView����  
		LVCOLUMN vcl;
		vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		//	��һ��  
		vcl.pszText = L"���";//�б���  
		vcl.cx = 40;//�п�  
		vcl.iSubItem = 0;//������������һ��������  
		ListView_InsertColumn(hListview, 0, &vcl);
		//	�ڶ���  
		vcl.pszText = L"��Դ����";
		vcl.cx = 250;
		vcl.iSubItem = 1;//��������  
		ListView_InsertColumn(hListview, 1, &vcl);
		//	������  
		vcl.pszText = L"��Դ����";
		vcl.cx = 250;
		vcl.iSubItem = 2;
		ListView_InsertColumn(hListview, 2, &vcl); 
		ListView_SetExtendedListViewStyle(hListview, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);//����listview��չ���  

		return 0;
	}
	case WM_PAINT:
	{
		// ���Ʊ���ͼ
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);
		Gdiplus::Image* image = Gdiplus::Image::FromFile(L"background2.jpg");
		if (image->GetLastStatus() != Gdiplus::Ok)
		{
			MessageBox(hDlg, L"����ͼƬʧ��!", L"��ʾ", MB_OK);
			return -1;
		}
		RECT videoimgrect;
		GetClientRect(hDlg, &videoimgrect);
		//ȡ�ÿ�Ⱥ͸߶�  
		Gdiplus::Image* pThumbnail = image->GetThumbnailImage(
			videoimgrect.right - videoimgrect.left,
			videoimgrect.bottom - videoimgrect.top,
			NULL,
			NULL);

		//���Ʊ���ͼ  
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
			PostQuitMessage(0);//�˳�     
		}
		return 0;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// ������ɣ������ʼ������ť��
			EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			
			// ���listview����
			HWND m_list = GetDlgItem(hDlg, IDC_NOVEL_LIST);
			ListView_DeleteAllItems(m_list);

			// ���л�������С˵��listview
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
				// �����ؼ��л�����ǰ��listview
				SwitchShowWindows(hDlg, IDC_NOVELCONTENT, IDC_CHAPTERS);
			}
			else
			{
				// �����ؼ��л�����ǰ��listview
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
		{	//IDΪlistview��ID  
			LPNMITEMACTIVATE now1 = (LPNMITEMACTIVATE)lParam;//�õ�NMITEMACTIVATE�ṹָ��  
			switch (now1->hdr.code) {//�ж�֪ͨ��  
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
		case IDC_NOVEL_LIST://IDΪlistview��ID  
			LPNMITEMACTIVATE now = (LPNMITEMACTIVATE)lParam;//�õ�NMITEMACTIVATE�ṹָ��  
			//now->iItem��Ŀ��� 
			switch (now->hdr.code) {//�ж�֪ͨ��  
			case NM_CLICK:
				
				break;
			case NM_DBLCLK:
			{
			
				// ���listview����
				ClearShowListView(hDlg, IDC_CHAPTERS);

				// �����ؼ��л�����ǰ��listview
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
		// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// ��ȡListView�ؼ��ľ��  
	HWND hListview = GetDlgItem(hwnd, IDC_NOVEL_LIST);

	std::vector<DownResourceInfo> vtDownInfo;
	std::wstring id;
		
	TCHAR m_search[MAX_PATH];
	GetDlgItemText(hwnd, IDC_SEARCH, m_search, MAX_PATH);
	vtDownInfo = GetNovelList(m_search);

	if (vtDownInfo.size() > 0)
	{
		//���������Ԫ�صĸ���  
		int arrCount = vtDownInfo.size();
		LVITEM vitem;
		vitem.mask = LVIF_TEXT;
		for (int i = 0; i < arrCount; i++)
		{
			/*
			���ԣ�
			���������������������
			*/
			id = std::to_wstring(vtDownInfo[i].rowNum);
			vitem.pszText = (LPWSTR)id.c_str();
			vitem.iItem = i;
			vitem.iSubItem = 0;
			ListView_InsertItem(hListview, &vitem);
			// ��������  
			vitem.pszText = (LPWSTR)vtDownInfo[i].strResourceName.c_str();
			vitem.iSubItem = 1;
			ListView_SetItem(hListview, &vitem);
			vitem.iSubItem = 2;
			vitem.pszText = (LPWSTR)vtDownInfo[i].strResourceLink.c_str();
			ListView_SetItem(hListview, &vitem);
		}
	}
	
	// ������ɣ������ʼ������ť��
	EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
}
void ShowChapterInfo(HWND hwnd, int chapter)
{
	// ��ȡListView�ؼ��ľ��  
	HWND hListview = GetDlgItem(hwnd, IDC_CHAPTERS);

	// ��ȡѡ�����С˵�½�����
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

	//	����ListView����  
	LVCOLUMN vcl;
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	//	��һ��  
	vcl.pszText = L"���";//�б���  
	vcl.cx = 40;//�п�  
	vcl.iSubItem = 0;//������������һ��������  
	ListView_InsertColumn(hListview, 0, &vcl);
	//	�ڶ���  
	vcl.pszText = L"�½�����";
	vcl.cx = 250;
	vcl.iSubItem = 1;//��������  
	ListView_InsertColumn(hListview, 1, &vcl);
	//	������  
	vcl.pszText = L"�½�����";
	vcl.cx = 250;
	vcl.iSubItem = 2;
	ListView_InsertColumn(hListview, 2, &vcl);
	ListView_SetExtendedListViewStyle(hListview, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);//����listview��չ���  

	std::wstring id;
	std::vector<NovelChapter> vtDownInfo;
	vtDownInfo = GetNoveChapters(pbuf);

	if (vtDownInfo.size() > 0)
	{
		//���������Ԫ�صĸ���  
		int arrCount = vtDownInfo.size();
		LVITEM vitem;
		vitem.mask = LVIF_TEXT;
		for (int i = 0; i < arrCount; i++)
		{
			/*
			���ԣ�
			���������������������
			*/
			id = std::to_wstring(i+1);
			vitem.pszText = (LPWSTR)id.c_str();
			vitem.iItem = i;
			vitem.iSubItem = 0;
			ListView_InsertItem(hListview, &vitem);
			// ��������  
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
�ڶ�����������Ҫ���صĿؼ�
��������������Ҫ��ʾ�Ŀؼ�
*/
void SwitchShowWindows(HWND hwnd, WORD prWindow, WORD newWindow)
{
	HWND hPreListview = GetDlgItem(hwnd, prWindow);
	HWND hNewListview = GetDlgItem(hwnd, newWindow);

	ShowWindow(hPreListview, SW_HIDE);
	ShowWindow(hNewListview, SW_SHOWNORMAL);
}

/*
��һ������ hwnd
�ڶ������� ָ��ListView
*/
void ClearShowListView(HWND hwnd, WORD childWindow)
{
	int nCols;
	HWND hWndListView, hWndListViewHeader;
	hWndListView = GetDlgItem(hwnd, childWindow);
	
	//ɾ��������
	ListView_DeleteAllItems(hWndListView);

	//�õ�ListView��Header����
	hWndListViewHeader = ListView_GetHeader(hWndListView);
	//�õ��е���Ŀ
	nCols = Header_GetItemCount(hWndListViewHeader);

	nCols--;
	//ɾ��������
	for (; nCols >= 0; nCols--)
		ListView_DeleteColumn(hWndListView, nCols);
}

BOOL NovelContent(HWND hDlg, int index)
{
	// ��ȡѡ�����С˵�½�����
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
	lstrcpy(LogFont.lfFaceName, L"΢���ź�");
	LogFont.lfWeight = FW_NORMAL;//FW_BLACK
	LogFont.lfHeight = -18; // �����С 
	LogFont.lfCharSet = 134;
	LogFont.lfOutPrecision = 3;
	LogFont.lfClipPrecision = 2;
	LogFont.lfOrientation = 45;
	LogFont.lfQuality = 1;
	LogFont.lfPitchAndFamily = 2;
	// �������� 
	HFONT hFont = CreateFontIndirect(&LogFont);
	// ȡ�ÿؼ���� 
	SendMessage(videoimg, WM_SETFONT, (WPARAM)hFont, TRUE);

	SetDlgItemText(hDlg, IDC_NOVELCONTENT, contents.c_str());
	
	//HDC hdc = GetDC(videoimg);//BeginPaint(hDlg, &ps);

	////����ͼ��  
	//Gdiplus::Image* image = Gdiplus::Image::FromFile(L"background.jpg");
	//if (image->GetLastStatus() != Gdiplus::Ok)
	//{
	//	MessageBox(hDlg, L"����ͼƬʧ��!", L"��ʾ", MB_OK);
	//	return -1;
	//}

	//RECT videoimgrect;
	//GetClientRect(videoimg, &videoimgrect);

	////ȡ�ÿ�Ⱥ͸߶�  
	//Gdiplus::Image* pThumbnail = image->GetThumbnailImage(
	//	videoimgrect.right - videoimgrect.left,
	//	videoimgrect.bottom - videoimgrect.top,
	//	NULL,
	//	NULL);

	//Gdiplus::Graphics graphics(hdc);
	////����С˵�Ķ�����ͼ  
	////graphics.DrawImage(image, videoimgrect.left, videoimgrect.top, pThumbnail->GetWidth(), pThumbnail->GetHeight());
	////����С˵����
	//// Initialize arguments.
	//Gdiplus::Font myFont(L"΢���ź�", 10);
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
