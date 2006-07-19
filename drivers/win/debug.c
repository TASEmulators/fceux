#include "common.h"
#ifdef FCEUDEF_DEBUGGER
#include "../../x6502.h"
#include "../../debug.h"
#include "debug.h"

void UpdateDebugger(void);
static int discallb(uint16 a, char *s);
static void crs(HWND hParent);
static void Disyou(uint16 a);
static void UpdateDMem(uint16 a);
int BlockingCheck(void); //mbg merge 7/18/06 yech had to add this

static HWND mwin=0;
static int32 cmsi;
static int instep=-1;
static X6502 *Xsave;

HWND dwin;

static uint16 StrToU16(char *s)
{
 unsigned int ret=0;
 sscanf(s,"%4x",&ret);
 return ret;
}

static uint8 StrToU8(char *s)
{
 unsigned int ret=0;
 sscanf(s,"%2x",&ret);
 return ret;
}

/* Need to be careful where these functions are used. */
static char *U16ToStr(uint16 a)
{
 static char TempArray[16];
 sprintf(TempArray,"%04X",a);
 return TempArray;
}

static char *U8ToStr(uint8 a)
{
 static char TempArray[16];
 sprintf(TempArray,"%02X",a);
 return TempArray;
}

static uint16 asavers[27];


static void crs(HWND hParent)
{
// SetDlgItemText(hParent,320,(LPTSTR)(instep>=0)?"Stopped.":"Running...");
 EnableWindow( GetDlgItem(hParent,301),(instep>=0)?1:0);
}

static void Disyou(uint16 a)
{
 int discount;

 SendDlgItemMessage(dwin,100,LB_RESETCONTENT,0,0);

 discount = 0;
 while(discount < 25)
 {
  char buffer[256];
  uint16 prea = a;

  asavers[discount]=a;
  sprintf(buffer,"%04x: ",a);

  a = FCEUI_Disassemble(Xsave,a,buffer + strlen(buffer));
  if(prea==Xsave->PC)
   SendDlgItemMessage(dwin,100,LB_SETCURSEL,discount,0);

  SendDlgItemMessage(dwin,100,LB_ADDSTRING,0,(LPARAM)(LPSTR)buffer);
  discount++;
 }
 //FCEUI_Disassemble(Xsave,a,discallb);
}

static void SSI(X6502 *X)
{
 SCROLLINFO si;

 memset(&si,0,sizeof(si));
 si.cbSize=sizeof(si);
 si.fMask=SIF_ALL;
 si.nMin=-32768;
 si.nMax=32767;
 si.nPage=27;
 si.nPos=X->PC-32768;
 SetScrollInfo(GetDlgItem(dwin,101),SB_CTL,&si,1);
}

static void RFlags(X6502 *X)
{
 int x;

 SetDlgItemText(dwin,202,(LPTSTR)U8ToStr(X->P&~0x30));
 for(x=0;x<8;x++)
 {
  if(x!=4 && x!=5)
   CheckDlgButton(dwin,400+x,((X->P>>x)&1)?BST_CHECKED:BST_UNCHECKED);
 }
}

static void RRegs(X6502 *X)
{
 SetDlgItemText(dwin,200,(LPTSTR)U16ToStr(X->PC));
 SetDlgItemText(dwin,201,(LPTSTR)U8ToStr(X->S));
 SetDlgItemText(dwin,210,(LPTSTR)U8ToStr(X->A));
 SetDlgItemText(dwin,211,(LPTSTR)U8ToStr(X->X));
 SetDlgItemText(dwin,212,(LPTSTR)U8ToStr(X->Y));
}

static void DoVecties(HWND hParent)
{
  uint16 m[3];
  int x;

  FCEUI_GetIVectors(&m[2],&m[0],&m[1]);
  for(x=0;x<3;x++)
   SetDlgItemText(hParent,220+x,(LPTSTR)U16ToStr(m[x]));
}

static void Redrawsy(X6502 *X)
{
 SSI(X);
 RFlags(X);
 RRegs(X);

 Disyou(X->PC);

 DoVecties(dwin);
}

static void MultiCB(X6502 *X)
{
 Xsave=X;
 if(instep>=0)
 {  
  Redrawsy(X);
  if(mwin)
   UpdateDMem(cmsi);
 }

 while(instep>=0)
 {
  if(instep>=1)
  {
   instep--;
   return;
  }
  StopSound();
  if(!BlockingCheck())		/* Whoops, need to exit for some reason. */
  {
   instep=-1;
   FCEUI_SetCPUCallback(0);
   return;
  }
  Sleep(50);
 }
}

static int multistep=0;
static void cpucb(X6502 *X)
{
 multistep=0;
 MultiCB(X);
}

static void BPointHandler(X6502 *X, int type, unsigned int A)
{
 if(multistep) return;  /* To handle instructions that can cause multiple
                           breakpoints per instruction.
                        */
 if(!dwin) return;      /* Window is closed, don't do breakpoints. */
 multistep=1;
 instep=0;
 crs(dwin);
 MultiCB(X);
}

static HWND hWndCallB;
static int RBPCallBack(int type, unsigned int A1, unsigned int A2,
        void (*Handler)(X6502 *, int type, unsigned int A) )
{
 char buf[128];
 sprintf(buf,"%s%s%s $%04x - $%04x",(type&BPOINT_READ)?"R":" ",
        (type&BPOINT_WRITE)?"W":" ",(type&BPOINT_PC)?"P":" ",A1,A2);
 SendDlgItemMessage(hWndCallB,510,LB_ADDSTRING,0,(LPARAM)(LPSTR)buf);
 return(1);
}

static void RebuildBPointList(HWND hwndDlg)
{
 SendDlgItemMessage(hwndDlg,510,LB_RESETCONTENT,0,0);
 hWndCallB=hwndDlg;
 FCEUI_ListBreakPoints(RBPCallBack);
}

static void FetchBPDef(HWND hwndDlg, uint16 *A1, uint16 *A2, int *type)
{
 char tmp[256];

 GetDlgItemText(hwndDlg,520,tmp,256);
 *A1=StrToU16(tmp);
 GetDlgItemText(hwndDlg,521,tmp,256);
 if(tmp[0]==0)
  *A2=*A1;
 else
  *A2=StrToU16(tmp);

 *type=0;
 *type|=(IsDlgButtonChecked(hwndDlg,530)==BST_CHECKED)?BPOINT_READ:0;
 *type|=(IsDlgButtonChecked(hwndDlg,531)==BST_CHECKED)?BPOINT_WRITE:0;
 *type|=(IsDlgButtonChecked(hwndDlg,532)==BST_CHECKED)?BPOINT_PC:0;
}

static void SetBPDef(HWND hwndDlg, int32 A1, int32 A2, int type)
{
 if(A1>=0)
  SetDlgItemText(hwndDlg,520,(LPTSTR)U16ToStr(A1));
 if(A2>=0)
  SetDlgItemText(hwndDlg,521,(LPTSTR)U16ToStr(A2));
 if(type>=0)
 {
 // CheckDlgButton(hwndDlg,123,(soundoptions&SO_SECONDARY)?BST_CHECKED:BST_UNCHECKED);  
 }
}

static BOOL CALLBACK DebugCon(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int32 scrollindex=0,ced=0;
  DSMFix(uMsg);
  switch(uMsg)
  {
   case WM_INITDIALOG:
                crs(hwndDlg);
                DoVecties(hwndDlg);
                FCEUI_SetCPUCallback(cpucb);
                RebuildBPointList(hwndDlg);
                break;
   case WM_VSCROLL:
                if(0!=instep) break;
                switch((int)LOWORD(wParam))
                {
                  case SB_TOP:
                        scrollindex=-32768;
                        ced=1;
                        break;
                  case SB_BOTTOM:
                        scrollindex=32767;
                        ced=1;
                        break;
                  case SB_LINEUP:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,101),SB_CTL);
                        if(scrollindex>-32768)
                        {
                         scrollindex--;
                         ced=1;
                        }
                        break;

                  case SB_PAGEUP:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,101),SB_CTL);
                        scrollindex-=27;
                        if(scrollindex<-32768)
                         scrollindex=-32768;
                        ced=1;
                        break;

                  case SB_LINEDOWN:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,101),SB_CTL);
                        if(scrollindex<32767)
                        {
                         scrollindex++;
                         ced=1;
                        }
                        break;

                  case SB_PAGEDOWN:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,101),SB_CTL);
                        scrollindex+=27;
                        if(scrollindex>32767)
                        {
                         scrollindex=32767;
                        }
                        ced=1;
                        break;

                  case SB_THUMBPOSITION:
                  case SB_THUMBTRACK:
                        scrollindex=(int16)(wParam>>16);
                        ced=1;
                        break;
                 }
                 if(ced)
                 {
                  SendDlgItemMessage(hwndDlg,101,SBM_SETPOS,scrollindex,1);
                  Disyou(scrollindex+32768);
                 }
                 break;


   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(HIWORD(wParam))
                {
                 case LBN_DBLCLK:
                         if(0==instep)
                         {
                          Xsave->PC=asavers[SendDlgItemMessage(hwndDlg,100,LB_GETCURSEL,0,0)];
                          RRegs(Xsave);
                          Disyou(Xsave->PC);
                         }
                         break;
                 case EN_KILLFOCUS:
                        {
                         char TempArray[64];
                         int id=LOWORD(wParam);
                         GetDlgItemText(hwndDlg,id,TempArray,64);

                         if(0==instep)
                         {
                          int tscroll=32768+SendDlgItemMessage(hwndDlg,101,SBM_GETPOS,0,0);

                          switch(id)
                          {
                           case 200:                                    
                                    if(Xsave->PC!=StrToU16(TempArray))
                                    {
                                     Xsave->PC=StrToU16(TempArray);
                                     Disyou(Xsave->PC);
                                     RRegs(Xsave);
                                    }
                                    break;
                           case 201:Xsave->S=StrToU8(TempArray);
                                    RRegs(Xsave);
                                    Disyou(tscroll);
                                    break;
                           case 202:Xsave->P&=0x30;
                                    Xsave->P|=StrToU8(TempArray)&~0x30;
                                    RFlags(Xsave);
                                    Disyou(tscroll);
                                    break;
                           case 210:Xsave->A=StrToU8(TempArray);
                                    RRegs(Xsave);
                                    Disyou(tscroll);
                                    break;
                           case 211:Xsave->X=StrToU8(TempArray);
                                    RRegs(Xsave);
                                    Disyou(tscroll);
                                    break;
                           case 212:Xsave->Y=StrToU8(TempArray);
                                    RRegs(Xsave);
                                    Disyou(tscroll);
                                    break;
                          }
                         }
                        }
                        break;

                 case BN_CLICKED:
                         if(LOWORD(wParam)>=400 && LOWORD(wParam)<=407)
                         {
                          if(0==instep)
                          {
                           Xsave->P^=1<<(LOWORD(wParam)&7);
                           RFlags(Xsave);
                          }
                         }
                         else switch(LOWORD(wParam))
                         {
                          case 300:
                           instep=1;
                           crs(hwndDlg);
                           break;
                          case 301:
                           instep=-1;
                           crs(hwndDlg);
                           break;
                          case 302:FCEUI_NMI();break;
                          case 303:FCEUI_IRQ();break;
                          case 310:FCEUI_ResetNES();
                                   if(instep>=0)
                                    instep=1;
                                   crs(hwndDlg);
                                   break;
                          case 311:DoMemmo(hwndDlg);break;

                          case 540:
                                   {
                                    LONG t;
                                    t=SendDlgItemMessage(hwndDlg,510,LB_GETCURSEL,0,0);
                                    if(t!=LB_ERR)
                                    {
                                     FCEUI_DeleteBreakPoint(t);
                                     SendDlgItemMessage(hWndCallB,510,LB_DELETESTRING,t,(LPARAM)(LPSTR)0);
                                    }
                                   }
                                   break;
                          case 541:
                                   {
                                    uint16 A1,A2;
                                    int type;
                                    FetchBPDef(hwndDlg, &A1, &A2, &type);
                                    FCEUI_AddBreakPoint(type, A1,A2,BPointHandler);
                                    hWndCallB=hwndDlg;  /* Hacky hacky. */
                                    RBPCallBack(type, A1,A2,0);
                                   }
                                   break;
                         }
                        break;
                }


                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:                        
                        instep=-1;
                        FCEUI_SetCPUCallback(0);
                        DestroyWindow(dwin);
                        dwin=0;
                        break;
                }
              }
  return 0;
}

void BeginDSeq(HWND hParent)
{
 if(dwin)
 {
  SetFocus(dwin);
  return;
 }
 if(!GI)
 {
  FCEUD_PrintError("You must have a game loaded before you can screw up a game.");
  return;
 }

 dwin=CreateDialog(fceu_hInstance,"DEBUGGER",0,DebugCon);
}

/* 16 numbers per line times 3 then minus one(no space at end) and plus
   6 for "8000: "-like string and plus 2 for crlf.  *16 for 16 lines, and
   +1 for a null.
*/
static uint8 kbuf[(16*3-1+6+2)*16+1];
static void sexycallb(uint16 a, uint8 v)
{
 if((a&15)==15)
  sprintf((char*)kbuf+strlen((char*)kbuf),"%02X\r\n",v); //mbg merge 7/18/06 added casts
 else if((a&15)==0)
  sprintf((char*)kbuf+strlen((char*)kbuf),"%03xx: %02X ",a>>4,v); //mbg merge 7/18/06 added casts
 else
  sprintf((char*)kbuf+strlen((char*)kbuf),"%02X ",v); //mbg merge 7/18/06 added casts
}

static void MDSSI(void)
{
 SCROLLINFO si;

 memset(&si,0,sizeof(si));
 si.cbSize=sizeof(si);
 si.fMask=SIF_ALL;
 si.nMin=0;
 si.nMax=0xFFFF>>4;
 si.nPage=16;
 cmsi=si.nPos=0;
 SetScrollInfo(GetDlgItem(mwin,103),SB_CTL,&si,1);
}

static void UpdateDMem(uint16 a)
{
 kbuf[0]=0;
 FCEUI_MemDump(a<<4,256,sexycallb);
 SetDlgItemText(mwin,100,(char*)kbuf); //mbg merge 7/18/06 added cast
}

int CreateDumpSave(uint32 a1, uint32 a2)
{
 const char filter[]="Raw dump(*.dmp)\0*.dmp\0";
 char nameo[2048];
 OPENFILENAME ofn;

 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="Dump Memory As...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 if(GetSaveFileName(&ofn))
 {
  FCEUI_DumpMem(nameo,a1,a2);
  return(1);
 }
 return 0;
}

int LoadSave(uint32 a)
{
 const char filter[]="Raw dump(*.dmp)\0*.dmp\0";
 char nameo[2048];
 OPENFILENAME ofn;

 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="Load Memory...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 if(GetOpenFileName(&ofn))
 {
  FCEUI_LoadMem(nameo,a,0); /* LL Load */
  return(1);
 }
 return 0;
}

static BOOL CALLBACK MemCon(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int32 scrollindex=0,ced=0;
  char TempArray[64];

  DSMFix(uMsg);

  switch(uMsg)
  {
   case WM_CLOSE:
   case WM_QUIT: goto gornk;

   case WM_COMMAND:
                switch(HIWORD(wParam))
                {
                 case BN_CLICKED:
                         switch(LOWORD(wParam))
                         {
                          case 203:
                          case 202:
                          {
                           uint16 a;
                           uint8 v;
                           GetDlgItemText(hwndDlg,200,TempArray,64);
                           a=StrToU16(TempArray);
                           GetDlgItemText(hwndDlg,201,TempArray,64);
                           v=StrToU8(TempArray);
                           FCEUI_MemPoke(a,v,LOWORD(wParam)&1);
                           UpdateDMem(cmsi);
                          
                           if(dwin && 0==instep)
                           {
                            int tscroll=32768+SendDlgItemMessage(dwin,101,SBM_GETPOS,0,0);
                            Disyou(tscroll);
                           }
                          }
                          break;
                          case 212:
                          {
                           uint16 a1;
                           uint16 a2;
                           GetDlgItemText(hwndDlg,210,TempArray,64);
                           a1=StrToU16(TempArray);
                           GetDlgItemText(hwndDlg,211,TempArray,64);
                           a2=StrToU16(TempArray);
                           CreateDumpSave(a1,a2);
                          }
                          break;
                          case 222:
                          {
                           uint16 a;
                           GetDlgItemText(hwndDlg,220,TempArray,64);
                           a=StrToU16(TempArray);
                           LoadSave(a);                        
                           UpdateDMem(cmsi);
                           if(dwin && 0==instep)
                           {
                            int tscroll=32768+SendDlgItemMessage(dwin,101,SBM_GETPOS,0,0);
                            Disyou(tscroll);
                           }
                          }
                          break;

                         }
                        break;
                }

                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:
                        DestroyWindow(mwin);
                        mwin=0;
                        break;
                }
                break;
   case WM_INITDIALOG:
                      return(1);
   case WM_VSCROLL:
                switch((int)LOWORD(wParam))
                {
                  case SB_TOP:
                        scrollindex=0;
                        ced=1;
                        break;
                  case SB_BOTTOM:
                        scrollindex=0xffff>>4;
                        ced=1;
                        break;
                  case SB_LINEUP:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,103),SB_CTL);
                        if(scrollindex>0)
                        {
                         scrollindex--;
                         ced=1;
                        }
                        break;

                  case SB_PAGEUP:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,103),SB_CTL);
                        scrollindex-=16;
                        if(scrollindex<0)
                         scrollindex=0;
                        ced=1;
                        break;

                  case SB_LINEDOWN:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,103),SB_CTL);
                        if(scrollindex<(0xFFFF>>4))
                        {
                         scrollindex++;
                         ced=1;
                        }
                        break;

                  case SB_PAGEDOWN:
                        scrollindex=GetScrollPos(GetDlgItem(hwndDlg,103),SB_CTL);
                        scrollindex+=16;
                        if(scrollindex>(0xFFFF>>4))
                        {
                         scrollindex=(65535>>4);
                        }
                        ced=1;
                        break;

                  case SB_THUMBPOSITION:
                  case SB_THUMBTRACK:
                        scrollindex=HIWORD(wParam);
                        ced=1;
                        break;
                 }
                 if(ced)
                 {
                  SendDlgItemMessage(hwndDlg,103,SBM_SETPOS,scrollindex,1);
                  UpdateDMem(scrollindex);
                  cmsi=scrollindex;
                 }
                 break;



  }

 return 0;
}

void DoMemmo(HWND hParent)
{
 if(mwin)
 {
  SetFocus(mwin);
  return;
 }
 mwin=CreateDialog(fceu_hInstance,"MEMVIEW",0,MemCon);
 MDSSI();
 UpdateDMem(0);
}

void UpdateDebugger(void)
{
 if(mwin)
  UpdateDMem(cmsi);
}

void KillDebugger(void)
{
 if(mwin)
  DestroyWindow(mwin);
 if(dwin)
  DestroyWindow(dwin);
 dwin=mwin=0;
}
#endif
