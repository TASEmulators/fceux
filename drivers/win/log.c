#include <stdlib.h>
#include "common.h"

static HWND logwin=0;

static char *logtext[64];
static int logcount=0;

static void RedoText(void)
{
 char textbuf[65536];
 int x;

 textbuf[0]=0;
 if(logcount>=64)
 {
  x=logcount&63;
  for(;;)
  {
   strcat(textbuf,logtext[x]);
   x=(x+1)&63;
   if(x==(logcount&63)) break;
  }
 }
 else
  for(x=0;x<logcount;x++)
  {
   strcat(textbuf,logtext[x]);
  }
 SetDlgItemText(logwin,100,textbuf);
 SendDlgItemMessage(logwin,100,EM_LINESCROLL,0,200);
}

static BOOL CALLBACK LogCon(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DSMFix(uMsg);
  switch(uMsg)
  {
   case WM_INITDIALOG:logwin=hwndDlg;
                      RedoText();break;
   case WM_COMMAND:
                   if(HIWORD(wParam)==BN_CLICKED)
                   {
                    DestroyWindow(hwndDlg);
                    logwin=0;
                   }
                   break;

  }
 return 0;
}

void MakeLogWindow(void)
{
 if(!logwin)
  CreateDialog(fceu_hInstance,"MESSAGELOG",0,LogCon);
}

void AddLogText(char *text,int newline)
{
 int x;
 char *t;

 if(logcount>=64) free(logtext[logcount&63]);

 x=0;
 t=text;
 while(*t)
 {
  if(*t=='\n') x++;
  t++;
 }

 if(!(logtext[logcount&63]=malloc(strlen(text)+1+x+newline*2)))
  return;

 t=logtext[logcount&63];

 while(*text)
 {
  if(*text=='\n')
  {
   *t='\r';
   t++;
  }
  *t=*text;
  t++;
  text++;
 }
 if(newline)
 {
  *t='\r';
  t++;
  *t='\n';
  t++;
 }
 *t=0;
 logcount++;
 if(logwin)
  RedoText();
}

void FCEUD_Message(char *text)
{
 AddLogText(text,0);
}
