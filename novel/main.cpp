#include <windows.h>
#include <commctrl.h>
#include <thread>
#include "resource.h"
#include "tool.h"

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowNovelInfo(HWND hwnd);											

int WINAPI WinMain(HINSTANCE hThisApp, HINSTANCE hPrevApp, LPSTR lpCmd, int nShow)
{
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

	return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		// 设置对话框的图标 
		//SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hgInst, MAKEINTRESOURCE(IDI_ICON1)));
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

			std::thread action(ShowNovelInfo, hDlg);
			action.detach();
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		default:
			break;
		}
		break;
	}
	case WM_NOTIFY:
	{
		switch (wParam) {
		case IDC_NOVEL_LIST://ID为listview的ID  
			LPNMITEMACTIVATE now = (LPNMITEMACTIVATE)lParam;//得到NMITEMACTIVATE结构指针  
			//now->iItem项目序号 
			switch (now->hdr.code) {//判断通知码  
			case NM_CLICK:
				
				break;
			case NM_DBLCLK:
			{
				// 获取选定项的小说章节链接
				TCHAR pbuf[256];
				LV_ITEM lvi;
				lvi.mask = LVIF_TEXT;
				lvi.cchTextMax = 256;
				lvi.iItem = now->iItem;
				lvi.iSubItem = 2;
				lvi.pszText = pbuf;
				ListView_GetItem(GetDlgItem(hDlg, IDC_NOVEL_LIST), &lvi);
				GetNoveChapters(pbuf);
				break;
			}
			case NM_RCLICK:

				break;
			}
			break;
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