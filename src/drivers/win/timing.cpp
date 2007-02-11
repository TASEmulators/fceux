#include "common.h"
#include "main.h"

/**
* Callback function of the Timing configuration dialog.
**/
BOOL CALLBACK TimingConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 int x;

  switch(uMsg) {
   case WM_INITDIALOG:                
                if(eoptions&EO_HIGHPRIO)
                 CheckDlgButton(hwndDlg,105,BST_CHECKED);
                if(eoptions&EO_NOTHROTTLE)
                 CheckDlgButton(hwndDlg,101,BST_CHECKED);
                for(x=0;x<10;x++)
                {
                 char buf[8];
                 sprintf(buf,"%d",x);
                 SendDlgItemMessage(hwndDlg,110,CB_ADDSTRING,0,(LPARAM)(LPSTR)buf);
                 SendDlgItemMessage(hwndDlg,111,CB_ADDSTRING,0,(LPARAM)(LPSTR)buf);
                }
                SendDlgItemMessage(hwndDlg,110,CB_SETCURSEL,maxconbskip,(LPARAM)(LPSTR)0);
                SendDlgItemMessage(hwndDlg,111,CB_SETCURSEL,ffbskip,(LPARAM)(LPSTR)0);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:
                        if(IsDlgButtonChecked(hwndDlg,105)==BST_CHECKED)
                         eoptions|=EO_HIGHPRIO;
                        else
                         eoptions&=~EO_HIGHPRIO;

                        if(IsDlgButtonChecked(hwndDlg,101)==BST_CHECKED)
                         eoptions|=EO_NOTHROTTLE;
                        else
                         eoptions&=~EO_NOTHROTTLE;

                        maxconbskip=SendDlgItemMessage(hwndDlg,110,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                        ffbskip=SendDlgItemMessage(hwndDlg,111,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

void DoTimingConfigFix()
{
	DoPriority();
}

/**
* Shows the Timing configuration dialog.
**/
void ConfigTiming()
{
	DialogBox(fceu_hInstance, "TIMINGCONFIG", hAppWnd, TimingConCallB);  
	DoTimingConfigFix();
}

