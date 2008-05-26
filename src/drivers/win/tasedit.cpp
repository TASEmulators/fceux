#include "common.h"
#include "tasedit.h"
#include "../../fceu.h"

HWND hTasEdit = 0;

void KillTasEdit()
{
	DestroyWindow(hTasEdit);
	hTasEdit = 0;
}

BOOL CALLBACK WndprocTasEdit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			break;
		case WM_CLOSE:
		case WM_QUIT:
			KillTasEdit();

	/*	case WM_NOTIFY:
		case LVN_GETDISPINFO:*/
	}

	return FALSE;
}

void DoTasEdit()
{
	if(!hTasEdit) 
		hTasEdit = CreateDialog(fceu_hInstance,"TASEDIT",NULL,WndprocTasEdit);

	if(hTasEdit)
	{
		SetWindowPos(hTasEdit,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		//update?
		//FCEUD_UpdatePPUView(-1,1);
	}
}
