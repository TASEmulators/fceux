/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"

#include "cheat.h"

static HWND acwin=0;

static int selcheat;
static int scheatmethod=0;
static uint8 cheatval1=0;
static uint8 cheatval2=0;

#define CSTOD   24

static uint16 StrToU16(char *s)
{
 unsigned int ret=0;
 sscanf(s,"%4x",&ret);
 return ret;
}

static uint8 StrToU8(char *s)
{
 unsigned int ret=0;
 sscanf(s,"%d",&ret);
 return ret;
}

static int StrToI(char *s)
{
 int ret=0;
 sscanf(s,"%d",&ret);
 return ret;
}


//This is slow
static char *U16ToStr(uint16 a)
{
 static char TempArray[16];
 sprintf(TempArray,"%04X",a);
 return TempArray;
}

//Rewritten to be fast for MemWatch (--Luke)
static char *U8ToStr(uint8 a)
{
 static char TempArray[8];
 TempArray[0] = '0' + a/100;
 TempArray[1] = '0' + (a%100)/10;
 TempArray[2] = '0' + (a%10);
 TempArray[3] = 0;
 return TempArray;
}

static char *IToStr(int a)
{
 static char TempArray[32];
 sprintf(TempArray,"%d",a);
 return TempArray;
}

static HWND RedoCheatsWND;
static int RedoCheatsCallB(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
 char tmp[512];
 sprintf(tmp,"%s %s",s?"+":"-",name);
 SendDlgItemMessage(RedoCheatsWND,300,LB_ADDSTRING,0,(LPARAM)(LPSTR)tmp);
 return(1);
}

static void RedoCheatsLB(HWND hwndDlg)
{
 SendDlgItemMessage(hwndDlg,300,LB_RESETCONTENT,0,0);
 RedoCheatsWND=hwndDlg;
 FCEUI_ListCheats(RedoCheatsCallB, 0);
}

int cfcallb(uint32 a, uint8 last, uint8 current)
{
 char temp[16];

 sprintf(temp,"%04X:%03d:%03d",(unsigned int)a,last,current);
 SendDlgItemMessage(acwin,108,LB_ADDSTRING,0,(LPARAM)(LPSTR)temp);
 return(1);
}

static int scrollindex;
static int scrollnum;
static int scrollmax;

int cfcallbinsert(uint32 a, uint8 last, uint8 current)
{
 char temp[16];

 sprintf(temp,"%04X:%03d:%03d",(unsigned int)a,last,current);
 SendDlgItemMessage(acwin,108,LB_INSERTSTRING,(CSTOD-1),(LPARAM)(LPSTR)temp);
 return(1);
}

int cfcallbinsertt(uint32 a, uint8 last, uint8 current)
{
 char temp[16];

 sprintf(temp,"%04X:%03d:%03d",(unsigned int)a,last,current);
 SendDlgItemMessage(acwin,108,LB_INSERTSTRING,0,(LPARAM)(LPSTR)temp);
 return(1);
}


void AddTheThing(HWND hwndDlg, char *s, int a, int v)
{
 if(FCEUI_AddCheat(s,a,v,-1,0))
  MessageBox(hwndDlg,"Cheat Added","Cheat Added",MB_OK);
}


static void DoGet(void)
{
 int n=FCEUI_CheatSearchGetCount();
 int t;
 scrollnum=n;
 scrollindex=-32768;

 SendDlgItemMessage(acwin,108,LB_RESETCONTENT,0,0);
 FCEUI_CheatSearchGetRange(0,(CSTOD-1),cfcallb);

 t=-32768+n-1-(CSTOD-1);
 if(t<-32768)
  t=-32768;
 scrollmax=t;
 SendDlgItemMessage(acwin,120,SBM_SETRANGE,-32768,t);
 SendDlgItemMessage(acwin,120,SBM_SETPOS,-32768,1);
}

static void FixCheatSelButtons(HWND hwndDlg, int how)
{
 /* Update Cheat Button */
 EnableWindow(GetDlgItem(hwndDlg,251),how);

 /* Delete Cheat Button */
 EnableWindow(GetDlgItem(hwndDlg,252),how);
}

static BOOL CALLBACK AddCheatCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int lbfocus;
  static HWND hwndLB;

  switch(uMsg)
  {                                                                               
   case WM_VSCROLL:
                if(scrollnum>(CSTOD-1))
                {
                 switch((int)LOWORD(wParam))
                 {
                  case SB_TOP:
                        scrollindex=-32768;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,(CSTOD-1),0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+(CSTOD-1),cfcallb);
                        break;
                  case SB_BOTTOM:
                        scrollindex=scrollmax;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,(CSTOD-1),0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+(CSTOD-1),cfcallb);
                        break;
                  case SB_LINEUP:
                        if(scrollindex>-32768)
                        {
                         scrollindex--;
                         SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                         SendDlgItemMessage(hwndDlg,108,LB_DELETESTRING,(CSTOD-1),0);
                         FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768,cfcallbinsertt);
                        }
                        break;

                  case SB_PAGEUP:
                        scrollindex-=CSTOD;
                        if(scrollindex<-32768) scrollindex=-32768;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,(CSTOD-1),0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+(CSTOD-1),cfcallb);
                        break;

                  case SB_LINEDOWN:
                        if(scrollindex<scrollmax)
                        {
                         scrollindex++;
                         SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                         SendDlgItemMessage(hwndDlg,108,LB_DELETESTRING,0,0);
                         FCEUI_CheatSearchGetRange(scrollindex+32768+(CSTOD-1),scrollindex+32768+(CSTOD-1),cfcallbinsert);
                        }
                        break;

                  case SB_PAGEDOWN:
                        scrollindex+=CSTOD;
                        if(scrollindex>scrollmax)
                         scrollindex=scrollmax;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,0,0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+(CSTOD-1),cfcallb);
                        break;

                  case SB_THUMBPOSITION:
                  case SB_THUMBTRACK:
                        scrollindex=(short int)HIWORD(wParam);
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,0,0);
                        FCEUI_CheatSearchGetRange(32768+scrollindex,32768+scrollindex+(CSTOD-1),cfcallb);
                        break;
                 }

                }                
                break;

   case WM_INITDIALOG:
                selcheat = -1;
                FixCheatSelButtons(hwndDlg, 0);
                acwin=hwndDlg;
                SetDlgItemText(hwndDlg,110,(LPTSTR)U8ToStr(cheatval1));
                SetDlgItemText(hwndDlg,111,(LPTSTR)U8ToStr(cheatval2));
                DoGet();
                CheckRadioButton(hwndDlg,115,120,scheatmethod+115);
                lbfocus=0;
                hwndLB=0;

                RedoCheatsLB(hwndDlg);
                break;

   case WM_VKEYTOITEM:
                if(lbfocus)
                {
                 int real;

                 real=SendDlgItemMessage(hwndDlg,108,LB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                 switch((int)LOWORD(wParam))
                 {
                  case VK_UP: 
                              /* mmmm....recursive goodness */
                              if(!real)
                               SendMessage(hwndDlg,WM_VSCROLL,SB_LINEUP,0);
                              return(-1);
                              break;
                  case VK_DOWN:
                              if(real==(CSTOD-1))
                               SendMessage(hwndDlg,WM_VSCROLL,SB_LINEDOWN,0);
                              return(-1);
                              break;
                  case VK_PRIOR:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_PAGEUP,0);
                              break;
                  case VK_NEXT:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_PAGEDOWN,0);
                              break;
                  case VK_HOME:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_TOP,0);
                              break;
                  case VK_END:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_BOTTOM,0);
                              break;
                 }
                 return(-2);
                }
                break;

   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(LOWORD(wParam))
                {
                 case 300:      /* List box selection changed. */
                        if(HIWORD(wParam)==LBN_SELCHANGE)
                        {
                         char *s;
                         uint32 a;
                         uint8 v;
			 int status;
                         int c,type;

                         selcheat=SendDlgItemMessage(hwndDlg,300,LB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         if(selcheat<0)
                         {
                          FixCheatSelButtons(hwndDlg,0);
                          break;
                         }
                         FixCheatSelButtons(hwndDlg, 1);

                         FCEUI_GetCheat(selcheat,&s,&a,&v,&c,&status,&type);
                         SetDlgItemText(hwndDlg,200,(LPTSTR)s);
                         SetDlgItemText(hwndDlg,201,(LPTSTR)U16ToStr(a));
                         SetDlgItemText(hwndDlg,202,(LPTSTR)U8ToStr(v));
                         SetDlgItemText(hwndDlg,203,(c==-1)?(LPTSTR)"":(LPTSTR)IToStr(c));
                         CheckDlgButton(hwndDlg,204,type?BST_CHECKED:BST_UNCHECKED);
                        }
                        break;
                 case 108:
                        switch(HIWORD(wParam))
                        {
                         case LBN_SELCHANGE:
                                 {
                                  char TempArray[32];
                                  SendDlgItemMessage(hwndDlg,108,LB_GETTEXT,SendDlgItemMessage(hwndDlg,108,LB_GETCURSEL,0,(LPARAM)(LPSTR)0),(LPARAM)(LPCTSTR)TempArray);
                                  TempArray[4]=0;
                                  SetDlgItemText(hwndDlg,201,(LPTSTR)TempArray);                                 
                                 }
                                 break;
                         case LBN_SETFOCUS:
                                 lbfocus=1;
                                 break;
                         case LBN_KILLFOCUS:
                                 lbfocus=0;
                                 break;
                        }
                        break;
                }

                switch(HIWORD(wParam))
                {
                 case LBN_DBLCLK:
                        if(selcheat>=0)
                        {
                         if(LOWORD(wParam)==300)
                          FCEUI_ToggleCheat(selcheat);
                         RedoCheatsLB(hwndDlg);
                         SendDlgItemMessage(hwndDlg,300,LB_SETCURSEL,selcheat,0);
                        }
                        break;

                 case BN_CLICKED:
                        if(LOWORD(wParam)>=115 && LOWORD(wParam)<=120)
                         scheatmethod=LOWORD(wParam)-115;
                        else switch(LOWORD(wParam))        
                        {
                         case 112:
                          FCEUI_CheatSearchBegin();
                          DoGet();
                          break;
                         case 113:
                          FCEUI_CheatSearchEnd(scheatmethod,cheatval1,cheatval2);
                          DoGet();
                          break;
                         case 114:
                          FCEUI_CheatSearchSetCurrentAsOriginal();
                          DoGet();
                          break;
                         case 107:
                          FCEUI_CheatSearchShowExcluded();
                          DoGet();
                          break;
                         case 250:      /* Add Cheat Button */
                          {
                           int a,v,c,t;
                           char name[257];
                           char temp[16];

                           GetDlgItemText(hwndDlg,200,name,256+1);
                           GetDlgItemText(hwndDlg,201,temp,4+1);
                           a=StrToU16(temp);
                           GetDlgItemText(hwndDlg,202,temp,3+1);
                           v=StrToU8(temp);
                           GetDlgItemText(hwndDlg,203,temp,3+1);
                           if(temp[0]==0)
                            c=-1;
                           else
                            c=StrToI(temp);
                           t=(IsDlgButtonChecked(hwndDlg,204)==BST_CHECKED)?1:0;
                           FCEUI_AddCheat(name,a,v,c,t);
                           RedoCheatsLB(hwndDlg);
                           SendDlgItemMessage(hwndDlg,300,LB_SETCURSEL,selcheat,0);
                          }
                          break;
                         case 253:      /* Add GG Cheat Button */
                          {
                           uint16 a;
                           int c;
                           uint8 v;
                           char name[257];

                           GetDlgItemText(hwndDlg,200,name,256+1);

                           if(FCEUI_DecodeGG(name,&a,&v,&c))
                           {
                            FCEUI_AddCheat(name,a,v,c,1);
                            RedoCheatsLB(hwndDlg);
                            SendDlgItemMessage(hwndDlg,300,LB_SETCURSEL,selcheat,0);
                           }
                          }
                          break;

                         case 251:      /* Update Cheat Button */
                          if(selcheat>=0)
                          {
                           int a,v,c,t;
                           char name[257];
                           char temp[16];

                           GetDlgItemText(hwndDlg,200,name,256+1);
                           GetDlgItemText(hwndDlg,201,temp,4+1);
                           a=StrToU16(temp);
                           GetDlgItemText(hwndDlg,202,temp,3+1);
                           v=StrToU8(temp);
                           GetDlgItemText(hwndDlg,203,temp,3+1);
                           if(temp[0]==0)
                            c=-1;
                           else
                            c=StrToI(temp);
                           t=(IsDlgButtonChecked(hwndDlg,204)==BST_CHECKED)?1:0;
                           FCEUI_SetCheat(selcheat,name,a,v,c,-1,t);
                           RedoCheatsLB(hwndDlg);
                           SendDlgItemMessage(hwndDlg,300,LB_SETCURSEL,selcheat,0);
                          }
                          break;
                         case 252:      /* Delete cheat button */
                          if(selcheat>=0)
                          {
                           FCEUI_DelCheat(selcheat);
                           SendDlgItemMessage(hwndDlg,300,LB_DELETESTRING,selcheat,0);
                           FixCheatSelButtons(hwndDlg, 0);
                           selcheat=-1;
                           SetDlgItemText(hwndDlg,200,(LPTSTR)"");
                           SetDlgItemText(hwndDlg,201,(LPTSTR)"");
                           SetDlgItemText(hwndDlg,202,(LPTSTR)"");
                           SetDlgItemText(hwndDlg,203,(LPTSTR)"");
                           CheckDlgButton(hwndDlg,204,BST_UNCHECKED);
                          }
                          break;
                         case 106:
                          gornk:
                          EndDialog(hwndDlg,0);
                          acwin=0;
                          break;
                        }
                        break;
                 case EN_CHANGE:
                        {
                         char TempArray[256];
                         GetDlgItemText(hwndDlg,LOWORD(wParam),TempArray,256);
                         switch(LOWORD(wParam))
                         {
                          case 110:cheatval1=StrToU8(TempArray);break;
                          case 111:cheatval2=StrToU8(TempArray);break;
                         }
                        }
                        break;
                }
              }
  return 0;
}

void ConfigAddCheat(HWND wnd)
{
 if(!GI)
 {
  FCEUD_PrintError("You must have a game loaded before you can manipulate cheats.");
  return;
 }

 if(GI->type==GIT_NSF)
 {
  FCEUD_PrintError("Sorry, you can't cheat with NSFs.");
  return;
 }

 DialogBox(fceu_hInstance,"ADDCHEAT",wnd,AddCheatCallB);
}

