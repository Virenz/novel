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
		// ���öԻ����ͼ�� 
		//SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hgInst, MAKEINTRESOURCE(IDI_ICON1)));
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
		case IDC_NOVEL_LIST://IDΪlistview��ID  
			LPNMITEMACTIVATE now = (LPNMITEMACTIVATE)lParam;//�õ�NMITEMACTIVATE�ṹָ��  
			//now->iItem��Ŀ��� 
			switch (now->hdr.code) {//�ж�֪ͨ��  
			case NM_CLICK:
				
				break;
			case NM_DBLCLK:
			{
				// ��ȡѡ�����С˵�½�����
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