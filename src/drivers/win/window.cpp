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

#include "state.cpp"      /* Save/Load state AS */

extern char *md5_asciistr(uint8 digest[16]);
extern FCEUGI *FCEUGameInfo;
extern int EnableRewind;

void DSMFix(UINT msg)
{
 switch(msg)
 {
    case WM_VSCROLL:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_ENTERMENULOOP:StopSound();break;
 }
}
static void ConfigGUI(void);
static void ConfigTiming(void);
static void ConfigPalette(void);
static void ConfigDirectories(void);

static HMENU fceumenu=0;

static int tog=0;
static int CheckedAutoFirePattern=40004;
static int CheckedAutoFireOffset=40016;
static int EnableBackgroundInput=0;

void ShowCursorAbs(int w)
{
 static int stat=0;
 if(w)
 {
  if(stat==-1) {stat++; ShowCursor(1);}
 }
 else
 {
  if(stat==0) {stat--; ShowCursor(0);}
 }
}


void CalcWindowSize(RECT *al)
{
 al->left=0;
 al->right=VNSWID*winsizemulx;
 al->top=0;
 al->bottom=totallines*winsizemuly;

 AdjustWindowRectEx(al,GetWindowLong(hAppWnd,GWL_STYLE),GetMenu(hAppWnd)!=NULL,GetWindowLong(hAppWnd,GWL_EXSTYLE));

 al->right-=al->left;
 al->left=0;
 al->bottom-=al->top;
 al->top=0;
}


void RedoMenuGI(FCEUGI *gi)
{
 int simpled[]={101,111,110,200,201,204,203,141,142,143,151,152,300,40003,40028, 0};
 int x;

 x = 0;
 while(simpled[x])
 {
  #ifndef FCEUDEF_DEBUGGER
  if(simpled[x] == 203)
   EnableMenuItem(fceumenu,simpled[x],MF_BYCOMMAND | MF_GRAYED);
  else
  #endif
  #ifndef _USE_SHARED_MEMORY_
  if(simpled[x] == 40002 || simpled[x] == 40003)
	  EnableMenuItem(fceumenu,simpled[x],MF_BYCOMMAND| MF_GRAYED);
  else
  #endif
  EnableMenuItem(fceumenu,simpled[x],MF_BYCOMMAND | (gi?MF_ENABLED:MF_GRAYED));
  x++;
 }
}

void UpdateMenu(void)
{
	static int *polo[3]={&genie,&palyo,&status_icon};
	static int polo2[3]={310,311,303};
	int x;

	for(x=0;x<3;x++)
		CheckMenuItem(fceumenu,polo2[x],*polo[x]?MF_CHECKED:MF_UNCHECKED);

	if(eoptions&EO_BGRUN)
		CheckMenuItem(fceumenu,301,MF_CHECKED);
	else
		CheckMenuItem(fceumenu,301,MF_UNCHECKED);

	if(FCEU_BotMode())
		CheckMenuItem(fceumenu,40003, MF_CHECKED);
	else
		CheckMenuItem(fceumenu,40003, MF_UNCHECKED);

	if(GetAutoFireDesynch())
		CheckMenuItem(fceumenu,40025, MF_CHECKED);
	else
		CheckMenuItem(fceumenu,40025, MF_UNCHECKED);

	CheckMenuItem(fceumenu,302, EnableBackgroundInput?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(fceumenu,40029, EnableRewind?MF_CHECKED:MF_UNCHECKED);

	int AutoFirePatternIDs[]={40004,40005,40006,40007,40008,40009,40010,40011,40012,40013,40014,40015,40022,40023,40024,0};
	int AutoFireOffsetIDs[]={40016,40017,40018,40019,40020,40021,0};

	x = 0;
	while(AutoFirePatternIDs[x])
	{
		CheckMenuItem(fceumenu,AutoFirePatternIDs[x],
			AutoFirePatternIDs[x]==CheckedAutoFirePattern?MF_CHECKED:MF_UNCHECKED);
		x++;
	}

	x = 0;
	while(AutoFireOffsetIDs[x])
	{
		CheckMenuItem(fceumenu,AutoFireOffsetIDs[x],
			AutoFireOffsetIDs[x]==CheckedAutoFireOffset?MF_CHECKED:MF_UNCHECKED);
		x++;
	}
}

static HMENU recentmenu, recentdmenu;
char *rfiles[10]={0,0,0,0,0,0,0,0,0,0};
char *rdirs[10]={0,0,0,0,0,0,0,0,0,0};

void UpdateRMenu(HMENU menu, char **strs, int mitem, int baseid)
{
 MENUITEMINFO moo;
 int x;

 moo.cbSize=sizeof(moo);
 moo.fMask=MIIM_SUBMENU|MIIM_STATE;

 GetMenuItemInfo(GetSubMenu(fceumenu,0),mitem,FALSE,&moo);
 moo.hSubMenu=menu;
 moo.fState=strs[0]?MFS_ENABLED:MFS_GRAYED;

 SetMenuItemInfo(GetSubMenu(fceumenu,0),mitem,FALSE,&moo);

 for(x=0;x<10;x++)
  RemoveMenu(menu,baseid+x,MF_BYCOMMAND);
 for(x=9;x>=0;x--)
 {  
  char tmp[128+5];
  if(!strs[x]) continue;

  moo.cbSize=sizeof(moo);
  moo.fMask=MIIM_DATA|MIIM_ID|MIIM_TYPE;

  if(strlen(strs[x])<128)
  {
   sprintf(tmp,"&%d. %s",(x+1)%10,strs[x]);
  }
  else
   sprintf(tmp,"&%d. %s",(x+1)%10,strs[x]+strlen(strs[x])-127);

  moo.cch=strlen(tmp);
  moo.fType=0;
  moo.wID=baseid+x;
  moo.dwTypeData=tmp;
  InsertMenuItem(menu,0,1,&moo);
 }
 DrawMenuBar(hAppWnd);
}

void AddRecent(char *fn)
{
 int x;

 for(x=0;x<10;x++)
  if(rfiles[x])
   if(!strcmp(rfiles[x],fn))    // Item is already in list.
   {
    int y;
    char *tmp;

    tmp=rfiles[x];              // Save pointer.
    for(y=x;y;y--)
     rfiles[y]=rfiles[y-1];     // Move items down.

    rfiles[0]=tmp;              // Put item on top.
    UpdateRMenu(recentmenu, rfiles, 102, 600);
    return;
   }

 if(rfiles[9]) free(rfiles[9]);
 for(x=9;x;x--) rfiles[x]=rfiles[x-1];
 rfiles[0]=(char*)malloc(strlen(fn)+1); //mbg merge 7/17/06 added cast
 strcpy(rfiles[0],fn);
 UpdateRMenu(recentmenu, rfiles, 102, 600);
}

void AddRecentDir(char *fn)
{
 int x;

 for(x=0;x<10;x++)
  if(rdirs[x])
   if(!strcmp(rdirs[x],fn))    // Item is already in list.
   {
    int y;
    char *tmp;

    tmp=rdirs[x];              // Save pointer.
    for(y=x;y;y--)
     rdirs[y]=rdirs[y-1];     // Move items down.

    rdirs[0]=tmp;              // Put item on top.
    UpdateRMenu(recentdmenu, rdirs, 103, 700);
    return;
   }

 if(rdirs[9]) free(rdirs[9]);
 for(x=9;x;x--) rdirs[x]=rdirs[x-1];
 rdirs[0]=(char *)malloc(strlen(fn)+1); //mbg merge 7/17/06 added  cast
 strcpy(rdirs[0],fn);
 UpdateRMenu(recentdmenu, rdirs, 103, 700);
}



void HideMenu(int h)
{
  if(h)
  {
   SetMenu(hAppWnd,0);   
  }
  else
  {
   SetMenu(hAppWnd,fceumenu);
  }
}

static LONG WindowXC=1<<30,WindowYC;
void HideFWindow(int h)
{
 LONG desa;

 if(h)  /* Full-screen. */
 {
   RECT bo;
   GetWindowRect(hAppWnd,&bo);
   WindowXC=bo.left;
   WindowYC=bo.top;

   SetMenu(hAppWnd,0);
   desa=WS_POPUP|WS_CLIPSIBLINGS;  
 }
 else
 {
   desa=WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS;
   HideMenu(tog);
   /* Stupid DirectDraw bug(I think?) requires this.  Need to investigate it. */
   SetWindowPos(hAppWnd,HWND_NOTOPMOST,0,0,0,0,SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE);
 }
 
 SetWindowLong(hAppWnd,GWL_STYLE,desa|(GetWindowLong(hAppWnd,GWL_STYLE)&WS_VISIBLE));
 SetWindowPos(hAppWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_NOZORDER);
}

void ToggleHideMenu(void)
{ 
 extern FCEUGI *FCEUGameInfo;
 if(!fullscreen && (FCEUGameInfo || tog))
 {
  tog^=1;
  HideMenu(tog);
  SetMainWindowStuff();
 }
}

void FCEUD_HideMenuToggle(void)
{
	ToggleHideMenu();
}

static void ALoad(char *nameo)
{
  if((GI=FCEUI_LoadGame(nameo,1)))
  {
   palyo=FCEUI_GetCurrentVidSystem(0,0);
   UpdateMenu();
   FixFL();
   SetMainWindowStuff();
   AddRecent(nameo);
   RefreshThrottleFPS();
   if(eoptions&EO_HIDEMENU && !tog)
    ToggleHideMenu();
   if(eoptions&EO_FSAFTERLOAD)
    SetFSVideoMode();
  }
  else
   StopSound();
  ParseGIInput(GI);
  RedoMenuGI(GI);
}

void LoadNewGamey(HWND hParent, char *initialdir)
{
 const char filter[]="All usable files(*.nes,*.nsf,*.fds,*.unf,*.zip,*.gz)\0*.nes;*.nsf;*.fds;*.unf;*.zip;*.gz\0All non-compressed usable files(*.nes,*.nsf,*.fds,*.unf)\0*.nes;*.nsf;*.fds;*.unf\0All files (*.*)\0*.*\0";
 char nameo[2048];
 OPENFILENAME ofn;
 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="FCE Ultra Open File...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.hwndOwner=hParent;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY; //OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK;
 ofn.lpstrInitialDir=initialdir?initialdir:gfsdir;
 if(GetOpenFileName(&ofn))
 {
  char *tmpdir;

  if((tmpdir=(char *)malloc(ofn.nFileOffset+1))) //mbg merge 7/17/06 added cast
  {
   strncpy(tmpdir,ofn.lpstrFile,ofn.nFileOffset);
   tmpdir[ofn.nFileOffset]=0;
   AddRecentDir(tmpdir);

   if(!initialdir)              // Prevent setting the File->Open default
                                // directory when a "Recent Directory" is selected.
   {
    if(gfsdir)
     free(gfsdir);
    gfsdir = tmpdir;
   }
   else
    free(tmpdir);
  }
  ALoad(nameo);
 }
}

static uint32 mousex,mousey,mouseb;
void GetMouseData(uint32 *md)
{
 if(FCEUI_IsMovieActive()<0)
   return;

 md[0]=mousex;
 md[1]=mousey;
 if(!fullscreen)
 {
  if(ismaximized)
  {
   RECT t;
   GetClientRect(hAppWnd, &t);
   md[0] = md[0] * VNSWID / (t.right?t.right:1);
   md[1] = md[1] * totallines / (t.bottom?t.bottom:1);
  }
  else
  {
   md[0]/=winsizemulx;
   md[1]/=winsizemuly;
  }
  md[0]+=VNSCLIP;
 }

 md[1]+=srendline;
 md[2]=((mouseb==MK_LBUTTON)?1:0)|((mouseb==MK_RBUTTON)?2:0);
}

//static int sizchange=0;
static int vchanged=0;

extern void RestartMovieOrReset(int pow);

int KeyboardSetBackgroundAccess(int on); //mbg merge 7/17/06 YECH had to add
void SetJoystickBackgroundAccess(int background); //mbg merge 7/17/06 YECH had to add
void ShowNetplayConsole(void); //mbg merge 7/17/06 YECH had to add
int FCEUMOV_IsPlaying(void); //mbg merge 7/17/06 YECH had to add
void DoPPUView();//mbg merge 7/19/06 yech had to add

void MapInput(void);

LRESULT FAR PASCAL AppWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  DSMFix(msg);
  switch(msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
                  mouseb=wParam;
                  goto proco;
    case WM_MOUSEMOVE:
                  {
                   mousex=LOWORD(lParam);
                   mousey=HIWORD(lParam);
                  }
                  goto proco;
    case WM_ERASEBKGND:
                  if(xbsave)
                   return(0);
                  else goto proco;
    case WM_PAINT:if(xbsave)
                  {
                   PAINTSTRUCT ps;
                   BeginPaint(hWnd,&ps);
                   FCEUD_BlitScreen(xbsave);
                   EndPaint(hWnd,&ps);
                   return(0);
                  }
                  goto proco;
    case WM_SIZE:
                if(!fullscreen && !changerecursive)
                 switch(wParam)
                 {
                  case SIZE_MAXIMIZED: ismaximized = 1;SetMainWindowStuff();break;
                  case SIZE_RESTORED: ismaximized = 0;SetMainWindowStuff();break;
                 }
                 break;
    case WM_SIZING:
                 {
                  RECT *wrect=(RECT *)lParam;
                  RECT srect;

                  int h=wrect->bottom-wrect->top;
                  int w=wrect->right-wrect->left;
                  int how;

                  if(wParam == WMSZ_BOTTOM || wParam == WMSZ_TOP)
                   how = 2;
                  else if(wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT)
                   how = 1;
                  else if(wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT
                        || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_TOPLEFT)
                   how = 3;
                  if(how & 1)
                   winsizemulx*= (double)w/winwidth;
                  if(how & 2)
                   winsizemuly*= (double)h/winheight;
                  if(how & 1) FixWXY(0);
                  else FixWXY(1);

                  CalcWindowSize(&srect);
                  winwidth=srect.right;
                  winheight=srect.bottom;
                  wrect->right = wrect->left + srect.right;
                  wrect->bottom = wrect->top + srect.bottom;
                 }
                 //sizchange=1;
                 //break;
                 goto proco;
    case WM_DISPLAYCHANGE:
                if(!fullscreen && !changerecursive)
                 vchanged=1;
                goto proco;
    case WM_DROPFILES:
                {
                 UINT len;
                 char *ftmp;

                 len=DragQueryFile((HDROP)wParam,0,0,0)+1; //mbg merge 7/17/06 changed (HANDLE) to (HDROP)
                 if((ftmp=(char*)malloc(len))) //mbg merge 7/17/06 added cast
                 {
                  DragQueryFile((HDROP)wParam,0,ftmp,len); //mbg merge 7/17/06 changed (HANDLE) to (HDROP)
                  ALoad(ftmp);
                  free(ftmp);
                 }                 
                }
                break;
    case WM_COMMAND:
                if(!(wParam>>16))
                {
                 wParam&=0xFFFF;
                 if(wParam>=600 && wParam<=609) // Recent files
                 {
                  if(rfiles[wParam-600]) ALoad(rfiles[wParam-600]);
                 }
                 else if(wParam >= 700 && wParam <= 709) // Recent dirs
                 {
                  if(rdirs[wParam-700])
                   LoadNewGamey(hWnd, rdirs[wParam - 700]);
                 }
                 switch(wParam)
                 {
					 //-------
					//mbg merge 7/18/06 added XD tools
				 case ID_DEBUG_DEBUGGER: DoDebug(0); break;
				 case ID_DEBUG_PPUVIEWER: DoPPUView(); break;
				 case ID_DEBUG_NAMETABLEVIEWER: DoNTView(); break;
				 case ID_DEBUG_HEXEDITOR: DoMemView(); break;
				 case ID_DEBUG_TRACELOGGER: DoTracer(); break;
				 case ID_DEBUG_GAMEGENIEDECODER: DoGGConv(); break;
				 case ID_DEBUG_CDLOGGER: DoCDLogger(); break;
				 
				 case 40004:
					 SetAutoFirePattern(1,1);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40005:
					 SetAutoFirePattern(1,2);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40006:
					 SetAutoFirePattern(1,3);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40007:
					 SetAutoFirePattern(1,4);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40008:
					 SetAutoFirePattern(1,5);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40009:
					 SetAutoFirePattern(2,1);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40010:
					 SetAutoFirePattern(2,2);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40011:
					 SetAutoFirePattern(2,3);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40012:
					 SetAutoFirePattern(2,4);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40013:
					 SetAutoFirePattern(3,1);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40014:
					 SetAutoFirePattern(3,2);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40015:
					 SetAutoFirePattern(3,3);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40022:
					 SetAutoFirePattern(4,1);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40023:
					 SetAutoFirePattern(4,2);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40024:
					 SetAutoFirePattern(5,1);
					 CheckedAutoFirePattern = wParam;
					 UpdateMenu();
					 break;
				 case 40016:
				 case 40017:
				 case 40018:
				 case 40019:
				 case 40020:
				 case 40021:
					 SetAutoFireOffset(wParam - 40016);
					 CheckedAutoFireOffset = wParam;
					 UpdateMenu();
					 break;
				 case 40025:
					 SetAutoFireDesynch(GetAutoFireDesynch()^1);
					 UpdateMenu();
					 break;
                  case 300:ToggleHideMenu();break;
                  case 301:
					  eoptions^=EO_BGRUN;
					  if((eoptions & EO_BGRUN) == 0)
					  {
						  EnableBackgroundInput = 0;
						  KeyboardSetBackgroundAccess(EnableBackgroundInput);
						  SetJoystickBackgroundAccess(EnableBackgroundInput);
					  }
					  UpdateMenu();
					  break;
				  case 302:EnableBackgroundInput ^= 1;
					  eoptions |= EO_BGRUN*EnableBackgroundInput;
					  KeyboardSetBackgroundAccess(EnableBackgroundInput);
					  SetJoystickBackgroundAccess(EnableBackgroundInput);
					  UpdateMenu();
					  break;
				  case 40029:
					  EnableRewind^= 1;
					  UpdateMenu();
					  break;
				  case 303:status_icon=!status_icon;UpdateMenu();break;
                  case 310:genie^=1;FCEUI_SetGameGenie(genie);UpdateMenu();break;
                  case 311:palyo^=1;
                           FCEUI_SetVidSystem(palyo);
                           RefreshThrottleFPS();
                           UpdateMenu();
                           FixFL();
//						   DoVideoConfigFix();
                           SetMainWindowStuff();
                           break;
				  case 40003: FCEU_SetBotMode(1^FCEU_BotMode());
					          UpdateMenu(); break;
				  case 40002: CreateBasicBot();break;
				  // case 40028: DoMemmo(0); break; //mbg merge 7/18/06 removed as part of old debugger
                  case 320:StopSound();ConfigDirectories();break;
                  case 327:StopSound();ConfigGUI();break;
                  case 321:StopSound();ConfigInput(hWnd);break;
                  case 322:ConfigTiming();break;
                  case 323:StopSound();ShowNetplayConsole();break;
                  case 324:StopSound();ConfigPalette();break;
                  case 325:StopSound();ConfigSound();break;
                  case 326:ConfigVideo();break;
				  case 328:MapInput();break;

                  case 200:RestartMovieOrReset(0);break;
                  case 201:RestartMovieOrReset(1);break;
				  case 40026: FCEUI_FDSSelect();break;
				  case 40001: FCEUI_FDSInsert();break;
				  case 40027: FCEUI_VSUniCoin();break;
			

		  #ifdef FCEUDEF_DEBUGGER
		  //case 203:BeginDSeq(hWnd);break; //mbg merge 7/18/06 removed as part of old debugger
		  #endif

                  //case 204:ConfigAddCheat(hWnd);break; //mbg merge TODO 7/17/06 - had to remove this
					  //mbg merge TODO 7/17/06 - had to remove this
                  //case 205:CreateMemWatch(hWnd);break;
                  case 100:StopSound();
                           LoadNewGamey(hWnd, 0);
                           break;
                  case 101:if(GI)
                           {
			    #ifdef FCEUDEF_DEBUGGER
			    //KillDebugger(); //mbg merge 7/18/06 removed as part of old debugger
			    #endif
                            FCEUI_CloseGame();                            
                            GI=0;
                            RedoMenuGI(GI);
                           }
                           break;
                  case 110:FCEUD_SaveStateAs();break;
                  case 111:FCEUD_LoadStateFrom();break;

                  case 40120: //mbg merge 7/18/06 changed ID from 120
                           {
                            MENUITEMINFO mi;
                            char *str;
             
                            StopSound();
                            if(CreateSoundSave())
                             str="Stop Sound Logging";
                            else
                             str="Log Sound As...";
                            memset(&mi,0,sizeof(mi));
                            mi.fMask=MIIM_DATA|MIIM_TYPE;
                            mi.cbSize=sizeof(mi);
                            GetMenuItemInfo(fceumenu,120,0,&mi);                           
                            mi.fMask=MIIM_DATA|MIIM_TYPE;
                            mi.cbSize=sizeof(mi);
                            mi.dwTypeData=str;
                            mi.cch=strlen(str);
                            SetMenuItemInfo(fceumenu,120,0,&mi);
                           }
                           break;
                  case 130:DoFCEUExit();break;

                  case 141:FCEUD_MovieRecordTo();break;
                  case 142:FCEUD_MovieReplayFrom();break;
                  case 143:FCEUI_StopMovie();break;

				  case 151:FCEUD_AviRecordTo();break;
				  case 152:FCEUD_AviStop();break;

                  case 400:StopSound();ShowAboutBox();break;
                  case 401:MakeLogWindow();break;
                 }
                }
                break;


    case WM_SYSCOMMAND:
               if(GI && wParam == SC_SCREENSAVE && (goptions & GOO_DISABLESS))
                return(0);

               if(wParam==SC_KEYMENU)
               {
                if(GI && InputType[2]==SIFC_FKB && cidisabled)
                 break;
                if(lParam == VK_RETURN || fullscreen || tog) break;
               }
               goto proco;
    case WM_SYSKEYDOWN:
               if(GI && InputType[2]==SIFC_FKB && cidisabled)
                break; /* Hopefully this won't break DInput... */

                if(fullscreen || tog)
                {
                 if(wParam==VK_MENU)
                  break;
                }
                if(wParam==VK_F10)
                {
					return 0;
				/*
                 if(!moocow) FCEUD_PrintError("Iyee");
                 if(!(lParam&0x40000000))
                  FCEUI_ResetNES();
                 break;
				*/
                }

                if(wParam == VK_RETURN)
		{
                 if(!(lParam&(1<<30)))
                 {
                  UpdateMenu();
                  changerecursive=1;
                  if(!SetVideoMode(fullscreen^1))
                   SetVideoMode(fullscreen);
                  changerecursive=0;
                 }
                 break;
		}
                goto proco;

    case WM_KEYDOWN:
              if(GI)
	      {
	       /* Only disable command keys if a game is loaded(and the other
		  conditions are right, of course). */
               if(InputType[2]==SIFC_FKB)
	       {
		if(wParam==VK_SCROLL)
		{
 		 cidisabled^=1;
		 FCEUI_DispMessage("Family Keyboard %sabled.",cidisabled?"en":"dis");
		}
		if(cidisabled)
                 break; /* Hopefully this won't break DInput... */
	       }
	      }
                goto proco;
    case WM_CLOSE:
    case WM_DESTROY:
    case WM_QUIT:DoFCEUExit();break;
    case WM_ACTIVATEAPP:       
       if((BOOL)wParam)
       {
        nofocus=0;
       }
       else
       {
        nofocus=1;
       }
       goto proco;
    case WM_ENTERMENULOOP:
      EnableMenuItem(fceumenu,143,MF_BYCOMMAND | (FCEUI_IsMovieActive()?MF_ENABLED:MF_GRAYED));
      EnableMenuItem(fceumenu,152,MF_BYCOMMAND | (FCEUI_AviIsRecording()?MF_ENABLED:MF_GRAYED));
    default:
      proco:
      return DefWindowProc(hWnd,msg,wParam,lParam);
   }
  return 0;
}

void FixWXY(int pref)
{
   if(eoptions&EO_FORCEASPECT)
   {
    /* First, make sure the ratio is valid, and if it's not, change
       it so that it doesn't break everything.
    */
    if(saspectw < 0.01) saspectw = 0.01;
    if(saspecth < 0.01) saspecth = 0.01;
    if((saspectw / saspecth) > 100) saspecth = saspectw;
    if((saspecth / saspectw) > 100) saspectw = saspecth;

    if((saspectw / saspecth) < 0.1) saspecth = saspectw;
    if((saspecth / saspectw) > 0.1) saspectw = saspecth;

    if(!pref)
    {
     winsizemuly = winsizemulx * 256 / 240 * 3 / 4 * saspectw / saspecth;
    }
    else
    {
     winsizemulx = winsizemuly * 240 / 256 * 4 / 3 * saspecth / saspectw;
    }
   }
   if(winspecial)
   {
    int mult;
    if(winspecial == 1 || winspecial == 2) mult = 2;
    else mult = 3;
    if(winsizemulx < mult)
    {
     if(eoptions&EO_FORCEASPECT)
      winsizemuly *= mult / winsizemulx;
     winsizemulx = mult;
    }
    if(winsizemuly < mult)
    {
     if(eoptions&EO_FORCEASPECT)
      winsizemulx *= mult / winsizemuly;
     winsizemuly = mult;
    }
   }

   if(winsizemulx<0.1)
    winsizemulx=0.1;
   if(winsizemuly<0.1)
    winsizemuly=0.1;

   if(eoptions & EO_FORCEISCALE)
   {
    int x,y;

    x = winsizemulx * 2;
    y = winsizemuly * 2;

    x = (x>>1) + (x&1);
    y = (y>>1) + (y&1);

    if(!x) x=1;
    if(!y) y=1;

    winsizemulx = x;
    winsizemuly = y;    
   }

   if(winsizemulx > 100) winsizemulx = 100;
   if(winsizemuly > 100) winsizemuly = 100;
}

void UpdateFCEUWindow(void)
{
  //int w,h; //mbg merge 7/17/06 removed
 // RECT wrect; //mbg merge 7/17/06 removed

  if(vchanged && !fullscreen && !changerecursive && !nofocus)
  {
   SetVideoMode(0);
   vchanged=0;
  }

  BlockingCheck();

  //mbg merge 7/18/06 removed as part of old debugger
  //#ifdef FCEUDEF_DEBUGGER
  //UpdateDebugger(); 
  //#endif

  if(!(eoptions&EO_BGRUN))
   while(nofocus)
   {
    StopSound();
    Sleep(75);
    BlockingCheck();
   }

   //mbg merge 7/19/06 removing this since I think its not used
 //if(_userpause)   //mbg merge 7/18/06 this changed. even though theres nothing setting this..
 //{
 // StopSound();
 // while(_userpause)   //mbg merge 7/18/06 this changed. even though theres nothing setting this..
 // {
 //  Sleep(50);
 //  BlockingCheck();   
 // }
 //}
}

void ByebyeWindow(void)
{
 SetMenu(hAppWnd,0);
 DestroyMenu(fceumenu);
 DestroyWindow(hAppWnd);
}

int CreateMainWindow(void)
{
  WNDCLASSEX winclass;
  RECT tmp;

  memset(&winclass,0,sizeof(winclass));
  winclass.cbSize=sizeof(WNDCLASSEX);
  winclass.style=CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_SAVEBITS;
  winclass.lpfnWndProc=AppWndProc;
  winclass.cbClsExtra=0;
  winclass.cbWndExtra=0;
  winclass.hInstance=fceu_hInstance;
  winclass.hIcon=LoadIcon(fceu_hInstance, "ICON_1");
  winclass.hIconSm=LoadIcon(fceu_hInstance, "ICON_1");
  winclass.hCursor=LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH); //mbg merge 7/17/06 added cast
  //winclass.lpszMenuName="FCEUMENU";
  winclass.lpszClassName="FCEULTRA";

  if(!RegisterClassEx(&winclass))
    return FALSE;

  AdjustWindowRectEx(&tmp,WS_OVERLAPPEDWINDOW,1,0);

  fceumenu=LoadMenu(fceu_hInstance,"FCEUMENU");

  recentmenu=CreateMenu();
  recentdmenu = CreateMenu();

  UpdateRMenu(recentmenu, rfiles, 102, 600);
  UpdateRMenu(recentdmenu, rdirs, 103, 700);

  RedoMenuGI(NULL);

  hAppWnd = CreateWindowEx(0,"FCEULTRA","FCE Ultra",
                        WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,  /* Style */
                        CW_USEDEFAULT,CW_USEDEFAULT,256,240,  /* X,Y ; Width, Height */
                        NULL,fceumenu,fceu_hInstance,NULL );  
  DragAcceptFiles(hAppWnd, 1);
  SetMainWindowStuff();
  return 1;
}


int SetMainWindowStuff(void)
{
  RECT tmp;

  GetWindowRect(hAppWnd,&tmp);

  if(ismaximized)
  {
   winwidth=tmp.right - tmp.left;
   winheight=tmp.bottom - tmp.top;

   ShowWindow(hAppWnd, SW_SHOWMAXIMIZED);
  }
  else
  {
   RECT srect;
   if(WindowXC!=(1<<30))
   {
    /* Subtracting and adding for if(eoptions&EO_USERFORCE) below. */
    tmp.bottom-=tmp.top;
    tmp.bottom+=WindowYC;

    tmp.right-=tmp.left;
    tmp.right+=WindowXC;
   

    tmp.left=WindowXC;
    tmp.top=WindowYC;
    WindowXC=1<<30;
   }

   CalcWindowSize(&srect);
   SetWindowPos(hAppWnd,HWND_TOP,tmp.left,tmp.top,srect.right,srect.bottom,SWP_SHOWWINDOW);
   winwidth=srect.right;
   winheight=srect.bottom;

   ShowWindow(hAppWnd, SW_SHOWNORMAL);
  }
  return 1;
}

int GetClientAbsRect(LPRECT lpRect)
{
  POINT point;
  point.x=point.y=0;
  if(!ClientToScreen(hAppWnd,&point)) return 0;

  lpRect->top=point.y;
  lpRect->left=point.x;
  
  if(ismaximized)
  {
   RECT al;
   GetClientRect(hAppWnd, &al);
   lpRect->right = point.x + al.right;
   lpRect->bottom = point.y + al.bottom;
  }
  else
  {
   lpRect->right=point.x+VNSWID*winsizemulx;
   lpRect->bottom=point.y+totallines*winsizemuly;
  }
  return 1;
}


int LoadPaletteFile(void)
{
 FILE *fp;
 const char filter[]="All usable files(*.pal)\0*.pal\0All files (*.*)\0*.*\0";
 char nameo[2048];
 OPENFILENAME ofn;
 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="FCE Ultra Open Palette File...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 ofn.lpstrInitialDir=0;
 if(GetOpenFileName(&ofn))
 {
  if((fp=FCEUD_UTF8fopen(nameo,"rb")))
  {
   fread(cpalette,1,192,fp);
   fclose(fp);
   FCEUI_SetPaletteArray(cpalette);
   eoptions|=EO_CPALETTE;
   return(1);
  }
  else
   FCEUD_PrintError("Error opening palette file!");
 }
 return(0);
}
static HWND pwindow;
static BOOL CALLBACK PaletteConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DSMFix(uMsg);
  switch(uMsg) {
   case WM_INITDIALOG:                
                if(ntsccol)
                 CheckDlgButton(hwndDlg,100,BST_CHECKED);
                SendDlgItemMessage(hwndDlg,500,TBM_SETRANGE,1,MAKELONG(0,128));
                SendDlgItemMessage(hwndDlg,501,TBM_SETRANGE,1,MAKELONG(0,128));
		FCEUI_GetNTSCTH(&ntsctint,&ntschue);
                SendDlgItemMessage(hwndDlg,500,TBM_SETPOS,1,ntsctint);
                SendDlgItemMessage(hwndDlg,501,TBM_SETPOS,1,ntschue);
                EnableWindow(GetDlgItem(hwndDlg,201),(eoptions&EO_CPALETTE)?1:0);
                break;
   case WM_HSCROLL:
                ntsctint=SendDlgItemMessage(hwndDlg,500,TBM_GETPOS,0,(LPARAM)(LPSTR)0);
                ntschue=SendDlgItemMessage(hwndDlg,501,TBM_GETPOS,0,(LPARAM)(LPSTR)0);
		FCEUI_SetNTSCTH(ntsccol,ntsctint,ntschue);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 100:ntsccol^=1;FCEUI_SetNTSCTH(ntsccol,ntsctint,ntschue);break;
                 case 200:
                          StopSound();
                          if(LoadPaletteFile())
                           EnableWindow(GetDlgItem(hwndDlg,201),1);
                          break;
                 case 201:FCEUI_SetPaletteArray(0);
                          eoptions&=~EO_CPALETTE;
                          EnableWindow(GetDlgItem(hwndDlg,201),0);
                          break;
                 case 1:
                        gornk:
                        DestroyWindow(hwndDlg);
                        pwindow=0; // Yay for user race conditions.
                        break;
               }
              }
  return 0;
}

static void ConfigPalette(void)
{
 if(!pwindow)
  pwindow=CreateDialog(fceu_hInstance,"PALCONFIG",0,PaletteConCallB);
 else
  SetFocus(pwindow);
}


static BOOL CALLBACK TimingConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

void DoTimingConfigFix(void)
{
  DoPriority();
}

static void ConfigTiming(void)
{
  DialogBox(fceu_hInstance,"TIMINGCONFIG",hAppWnd,TimingConCallB);  
  DoTimingConfigFix();
}

static BOOL CALLBACK GUIConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:                
                if(eoptions&EO_FOAFTERSTART)
                 CheckDlgButton(hwndDlg,102,BST_CHECKED);
                if(eoptions&EO_HIDEMENU)
                 CheckDlgButton(hwndDlg,104,BST_CHECKED);
                if(goptions & GOO_CONFIRMEXIT)
                 CheckDlgButton(hwndDlg,110,BST_CHECKED);
                if(goptions & GOO_DISABLESS)
                 CheckDlgButton(hwndDlg,111,BST_CHECKED);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:
                        if(IsDlgButtonChecked(hwndDlg,102)==BST_CHECKED)
                         eoptions|=EO_FOAFTERSTART;
                        else
                         eoptions&=~EO_FOAFTERSTART;
                        if(IsDlgButtonChecked(hwndDlg,104)==BST_CHECKED)
                         eoptions|=EO_HIDEMENU;
                        else
                         eoptions&=~EO_HIDEMENU;

                        goptions &= ~(GOO_CONFIRMEXIT | GOO_DISABLESS);

                        if(IsDlgButtonChecked(hwndDlg,110)==BST_CHECKED)
                         goptions |= GOO_CONFIRMEXIT;
                        if(IsDlgButtonChecked(hwndDlg,111)==BST_CHECKED)
                         goptions |= GOO_DISABLESS;
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

static void ConfigGUI(void)
{
  DialogBox(fceu_hInstance,"GUICONFIG",hAppWnd,GUIConCallB);  
}


static int BrowseForFolder(HWND hParent, char *htext, char *buf)
{
 BROWSEINFO bi;
 LPCITEMIDLIST pidl;
 int ret=1;

 buf[0]=0;

 memset(&bi,0,sizeof(bi));
                
 bi.hwndOwner=hParent;
 bi.lpszTitle=htext;
 bi.ulFlags=BIF_RETURNONLYFSDIRS; 

 if(FAILED(CoInitialize(0)))
  return(0);

 if(!(pidl=SHBrowseForFolder(&bi)))
 {
  ret=0;
  goto end1;
 }

 if(!SHGetPathFromIDList(pidl,buf))
 {
  ret=0;
  goto end2;
 }

 end2:
 /* This probably isn't the best way to free the memory... */
 CoTaskMemFree((PVOID)pidl);

 end1:
 CoUninitialize();
 return(ret);
}

static BOOL CALLBACK DirConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int x;

  switch(uMsg){
   case WM_INITDIALOG:                
	   SetDlgItemText(hwndDlg,65508,"The settings in the \"Individual Directory Overrides\" group will override the settings in the \"Base Directory Override\" group.  To delete an override, delete the text from the text edit control.  Note that the directory the configuration file is in cannot be overridden");
                for(x=0;x<6;x++)
                 SetDlgItemText(hwndDlg,100+x,DOvers[x]);
                if(eoptions&EO_SNAPNAME)
                 CheckDlgButton(hwndDlg,300,BST_CHECKED);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                {
                 if((wParam&0xFFFF)>=200 && (wParam&0xFFFF)<=205)
                 {
                  static char *helpert[6]={"Cheats","Miscellaneous","Nonvolatile Game Data","Save States","Screen Snapshots","Base Directory"};
                  char name[MAX_PATH];

                  if(BrowseForFolder(hwndDlg,helpert[((wParam&0xFFFF)-200)],name))
                   SetDlgItemText(hwndDlg,100+((wParam&0xFFFF)-200),name);
                 }
                 else switch(wParam&0xFFFF)
                 {
                  case 1:
                        gornk:

                        if(IsDlgButtonChecked(hwndDlg,300)==BST_CHECKED)
                         eoptions|=EO_SNAPNAME;
                        else
                         eoptions&=~EO_SNAPNAME;

                        RemoveDirs();   // Remove empty directories.

                        for(x=0;x<6;x++)
                        {
                         LONG len;
                         len=SendDlgItemMessage(hwndDlg,100+x,WM_GETTEXTLENGTH,0,0);
                         if(len<=0)
                         {
                          if(DOvers[x]) free(DOvers[x]);
                          DOvers[x]=0;
                          continue;
                         }
                         len++; // Add 1 for null character.
                         if(!(DOvers[x]=(char*)malloc(len))) //mbg merge 7/17/06 added cast
                          continue;
                         if(!GetDlgItemText(hwndDlg,100+x,DOvers[x],len))
                         {
                          free(DOvers[x]);
                          DOvers[x]=0;
                          continue;
                         }

                        }

                        CreateDirs();   // Create needed directories.
                        SetDirs();      // Set the directories in the core.
                        EndDialog(hwndDlg,0);
                        break;
                 }
                }
              }
  return 0;
}


static void ConfigDirectories(void)
{
  DialogBox(fceu_hInstance,"DIRCONFIG",hAppWnd,DirConCallB);
}

static int ReplayDialogReadOnlyStatus = 0;
static int ReplayDialogStopFrame = 0;

static char* GetReplayPath(HWND hwndDlg)
{
	char* fn=0;
	char szChoice[MAX_PATH];
	LONG lIndex = SendDlgItemMessage(hwndDlg, 200, CB_GETCURSEL, 0, 0);
	LONG lCount = SendDlgItemMessage(hwndDlg, 200, CB_GETCOUNT, 0, 0);

	// NOTE: lCount-1 is the "Browse..." list item
	if(lIndex != CB_ERR && lIndex != lCount-1)
	{
		LONG lStringLength = SendDlgItemMessage(hwndDlg, 200, CB_GETLBTEXTLEN, (WPARAM)lIndex, 0);
		if(lStringLength < MAX_PATH)
		{
			char szDrive[MAX_PATH]={0};
			char szDirectory[MAX_PATH]={0};
			char szFilename[MAX_PATH]={0};
			char szExt[MAX_PATH]={0};
			char szTemp[MAX_PATH]={0};

			SendDlgItemMessage(hwndDlg, 200, CB_GETLBTEXT, (WPARAM)lIndex, (LPARAM)szTemp);
			if(szTemp[0] && szTemp[1]!=':')
				sprintf(szChoice, ".\\%s", szTemp);
			else
				strcpy(szChoice, szTemp);

			SetCurrentDirectory(BaseDirectory);

			_splitpath(szChoice, szDrive, szDirectory, szFilename, szExt);
			if(szDrive[0]=='\0' && szDirectory[0]=='\0')
				fn=FCEU_MakePath(FCEUMKF_MOVIE, szChoice);		// need to make a full path
			else
				fn=strdup(szChoice);							// given a full path
		}
	}

	return fn;
}

static char* GetRecordPath(HWND hwndDlg)
{
	char* fn=0;
	char szChoice[MAX_PATH];
	char szDrive[MAX_PATH]={0};
	char szDirectory[MAX_PATH]={0};
	char szFilename[MAX_PATH]={0};
	char szExt[MAX_PATH]={0};

	GetDlgItemText(hwndDlg, 200, szChoice, sizeof(szChoice));

	_splitpath(szChoice, szDrive, szDirectory, szFilename, szExt);
	if(szDrive[0]=='\0' && szDirectory[0]=='\0')
		fn=FCEU_MakePath(FCEUMKF_MOVIE, szChoice);		// need to make a full path
	else
		fn=strdup(szChoice);							// given a full path

	return fn;
}

static char* GetSavePath(HWND hwndDlg)
{
	char* fn=0;
	char szDrive[MAX_PATH]={0};
	char szDirectory[MAX_PATH]={0};
	char szFilename[MAX_PATH]={0};
	char szExt[MAX_PATH]={0};
	LONG lIndex = SendDlgItemMessage(hwndDlg, 301, CB_GETCURSEL, 0, 0);
	LONG lStringLength = SendDlgItemMessage(hwndDlg, 301, CB_GETLBTEXTLEN, (WPARAM)lIndex, 0);

	fn = (char*)malloc(lStringLength);
	SendDlgItemMessage(hwndDlg, 301, CB_GETLBTEXT, (WPARAM)lIndex, (LPARAM)fn);

	_splitpath(fn, szDrive, szDirectory, szFilename, szExt);
	if(szDrive[0]=='\0' && szDirectory[0]=='\0')
	{
		char* newfn=FCEU_MakePath(FCEUMKF_MOVIE, fn);		// need to make a full path
		free(fn);
		fn=newfn;
	}

	return fn;
}


// C:\fceu\movies\bla.fcm  +  C:\fceu\fceu\  ->  C:\fceu\movies\bla.fcm
// movies\bla.fcm  +  fceu\  ->  movies\bla.fcm

// C:\fceu\movies\bla.fcm  +  C:\fceu\  ->  movies\bla.fcm
void AbsoluteToRelative(char *const dst, const char *const dir, const char *const root)
{
	int i, igood=0;

	for(i = 0 ; ; i++)
	{
		int a = tolower(dir[i]);
		int b = tolower(root[i]);
		if(a == '/' || a == '\0' || a == '.') a = '\\';
		if(b == '/' || b == '\0' || b == '.') b = '\\';

		if(a != b)
		{
			igood = 0;
			break;
		}

		if(a == '\\')
			igood = i+1;

		if(!dir[i] || !root[i])
			break;
	}

//	if(igood)
//		sprintf(dst, ".\\%s", dir + igood);
//	else
		strcpy(dst, dir + igood);
}

extern int movieConvertOffset1, movieConvertOffset2,movieConvertOK;
static int movieHackType=3;

static void UpdateReplayDialog(HWND hwndDlg)
{
	movieConvertOffset1=0, movieConvertOffset2=0,movieConvertOK=0;

	int doClear=1;
	char *fn=GetReplayPath(hwndDlg);

	// remember the previous setting for the read-only checkbox
	if(IsWindowEnabled(GetDlgItem(hwndDlg, 201)))
		ReplayDialogReadOnlyStatus = (SendDlgItemMessage(hwndDlg, 201, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

	if(fn)
	{
		MOVIE_INFO info;
		char metadata[MOVIE_MAX_METADATA];
		char rom_name[MAX_PATH];

		memset(&info, 0, sizeof(info));
		info.metadata = metadata;
		info.metadata_size = sizeof(metadata);
		info.name_of_rom_used = rom_name;
		info.name_of_rom_used_size = sizeof(rom_name);

		if(FCEUI_MovieGetInfo(fn, &info))
		{
#define MOVIE_FLAG_NOSYNCHACK          (1<<4) // set in newer version, used for old movie compatibility
			extern int resetDMCacc;
			extern int justAutoConverted;
			int noNoSyncHack=!(info.flags&MOVIE_FLAG_NOSYNCHACK) && resetDMCacc;
			EnableWindow(GetDlgItem(hwndDlg,1000),justAutoConverted || noNoSyncHack);
			EnableWindow(GetDlgItem(hwndDlg,1001),justAutoConverted || noNoSyncHack);
			if(justAutoConverted)
			{
				// use values as nesmock offsets
				if(movieHackType != 0)
				{
					movieHackType=0;
					SendDlgItemMessage(hwndDlg, 1000, WM_SETTEXT, 0,(LPARAM)"2");
					SendDlgItemMessage(hwndDlg, 1001, WM_SETTEXT, 0,(LPARAM)"0");
					SendDlgItemMessage(hwndDlg, 2000, WM_SETTEXT, 0,(LPARAM)"Offset:");
					SendDlgItemMessage(hwndDlg, 2001, WM_SETTEXT, 0,(LPARAM)"from");
				}
			}
			else if(noNoSyncHack)
			{
				// use values as sound reset hack values
				if(movieHackType != 1)
				{
					movieHackType=1;
//					extern int32 DMCacc;
//					extern int8 DMCBitCount;
//					char str[256];
//					sprintf(str, "%d", DMCacc);
//					SendDlgItemMessage(hwndDlg, 1000, WM_SETTEXT, 0,(LPARAM)str);
//					sprintf(str, "%d", DMCBitCount);
//					SendDlgItemMessage(hwndDlg, 1001, WM_SETTEXT, 0,(LPARAM)str);
					SendDlgItemMessage(hwndDlg, 1000, WM_SETTEXT, 0,(LPARAM)"8");
					SendDlgItemMessage(hwndDlg, 1001, WM_SETTEXT, 0,(LPARAM)"0");
					SendDlgItemMessage(hwndDlg, 2000, WM_SETTEXT, 0,(LPARAM)"Missing data: acc=");
					SendDlgItemMessage(hwndDlg, 2001, WM_SETTEXT, 0,(LPARAM)"bc=");
				}
			}
			else if(movieHackType != 2)
			{
				movieHackType=2;
				SendDlgItemMessage(hwndDlg, 1000, WM_SETTEXT, 0,(LPARAM)"");
				SendDlgItemMessage(hwndDlg, 1001, WM_SETTEXT, 0,(LPARAM)"");
				SendDlgItemMessage(hwndDlg, 2000, WM_SETTEXT, 0,(LPARAM)"");
				SendDlgItemMessage(hwndDlg, 2001, WM_SETTEXT, 0,(LPARAM)"");
			}

/*			{	// select away to autoconverted movie... but actually we don't want to do that now that there's an offset setting in the dialog
				extern char lastMovieInfoFilename [512];
				char relative[MAX_PATH];
				AbsoluteToRelative(relative, lastMovieInfoFilename, BaseDirectory);

				LONG lOtherIndex = SendDlgItemMessage(hwndDlg, 200, CB_FINDSTRING, (WPARAM)-1, (LPARAM)relative);
				if(lOtherIndex != CB_ERR)
				{
					// select already existing string
					SendDlgItemMessage(hwndDlg, 200, CB_SETCURSEL, lOtherIndex, 0);
				} else {
					LONG lIndex = SendDlgItemMessage(hwndDlg, 200, CB_GETCURSEL, 0, 0);
					SendDlgItemMessage(hwndDlg, 200, CB_INSERTSTRING, lIndex, (LPARAM)relative);
					SendDlgItemMessage(hwndDlg, 200, CB_SETCURSEL, lIndex, 0);
				}

				// restore focus to the dialog
//				SetFocus(GetDlgItem(hwndDlg, 200));
			}*/


			char tmp[256];
			uint32 div;
			
			sprintf(tmp, "%lu", info.num_frames);
			SetWindowTextA(GetDlgItem(hwndDlg,301), tmp);                   // frames
			SetDlgItemText(hwndDlg,1003,tmp);
			
			div = (FCEUI_GetCurrentVidSystem(0,0)) ? 50 : 60;				// PAL timing
			info.num_frames += (div>>1);                                    // round up
			sprintf(tmp, "%02d:%02d:%02d", (info.num_frames/(div*60*60)), (info.num_frames/(div*60))%60, (info.num_frames/div) % 60);
			SetWindowTextA(GetDlgItem(hwndDlg,300), tmp);                   // length
			
			sprintf(tmp, "%lu", info.rerecord_count);
			SetWindowTextA(GetDlgItem(hwndDlg,302), tmp);                   // rerecord
			
			{
				// convert utf8 metadata to windows widechar
				WCHAR wszMeta[MOVIE_MAX_METADATA];
				if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, metadata, -1, wszMeta, MOVIE_MAX_METADATA))
				{
					if(wszMeta[0])
						SetWindowTextW(GetDlgItem(hwndDlg,303), wszMeta);              // metadata
					else
						SetWindowTextW(GetDlgItem(hwndDlg,303), L"(this movie has no author info)");				   // metadata
				}
			}
			
			EnableWindow(GetDlgItem(hwndDlg,201),(info.read_only)? FALSE : TRUE); // disable read-only checkbox if the file access is read-only
			SendDlgItemMessage(hwndDlg,201,BM_SETCHECK,info.read_only ? BST_CHECKED : (ReplayDialogReadOnlyStatus ? BST_CHECKED : BST_UNCHECKED), 0);
			
			SetWindowText(GetDlgItem(hwndDlg,306),(info.flags & MOVIE_FLAG_FROM_RESET) ? "Reset or Power-On" : "Savestate");
			if(info.movie_version > 1)
			{
				char emuStr[128];
				SetWindowText(GetDlgItem(hwndDlg,304),info.name_of_rom_used);
				SetWindowText(GetDlgItem(hwndDlg,305),md5_asciistr(info.md5_of_rom_used));
				if(info.emu_version_used > 64)
					sprintf(emuStr, "FCEU %d.%02d.%02d%s", info.emu_version_used/10000, (info.emu_version_used/100)%100, (info.emu_version_used)%100, info.emu_version_used < 9813 ? " (blip)" : "");
				else
				{
					if(info.emu_version_used == 1)
						strcpy(emuStr, "Famtasia");
					else if(info.emu_version_used == 2)
						strcpy(emuStr, "Nintendulator");
					else if(info.emu_version_used == 3)
						strcpy(emuStr, "VirtuaNES");
					else
					{
						strcpy(emuStr, "(unknown)");
						char* dot = strrchr(fn,'.');
						if(dot)
						{
							if(!stricmp(dot,".fmv"))
								strcpy(emuStr, "Famtasia? (unknown version)");
							else if(!stricmp(dot,".nmv"))
								strcpy(emuStr, "Nintendulator? (unknown version)");
							else if(!stricmp(dot,".vmv"))
								strcpy(emuStr, "VirtuaNES? (unknown version)");
							else if(!stricmp(dot,".fcm"))
								strcpy(emuStr, "FCEU? (unknown version)");
						}
					}
				}
				SetWindowText(GetDlgItem(hwndDlg,307),emuStr);
			}
			else
			{
				SetWindowText(GetDlgItem(hwndDlg,304),"unknown");
				SetWindowText(GetDlgItem(hwndDlg,305),"unknown");
				SetWindowText(GetDlgItem(hwndDlg,307),"FCEU 0.98.10 (blip)");
			}

			SetWindowText(GetDlgItem(hwndDlg,308),md5_asciistr(FCEUGameInfo->MD5));
			EnableWindow(GetDlgItem(hwndDlg,1),TRUE);                     // enable OK
			
			doClear = 0;
		}

		free(fn);
	}
	else
	{
		EnableWindow(GetDlgItem(hwndDlg,1000),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,1001),FALSE);
	}

	if(doClear)
	{
		SetWindowText(GetDlgItem(hwndDlg,300),"");
		SetWindowText(GetDlgItem(hwndDlg,301),"");
		SetWindowText(GetDlgItem(hwndDlg,302),"");
		SetWindowText(GetDlgItem(hwndDlg,303),"");
		SetWindowText(GetDlgItem(hwndDlg,304),"");
		SetWindowText(GetDlgItem(hwndDlg,305),"");
		SetWindowText(GetDlgItem(hwndDlg,306),"Nothing (invalid movie)");
		SetWindowText(GetDlgItem(hwndDlg,307),"");
		SetWindowText(GetDlgItem(hwndDlg,308),md5_asciistr(FCEUGameInfo->MD5));
		SetDlgItemText(hwndDlg,1003,"");
		EnableWindow(GetDlgItem(hwndDlg,201),FALSE);
		SendDlgItemMessage(hwndDlg,201,BM_SETCHECK,BST_UNCHECKED,0);
		EnableWindow(GetDlgItem(hwndDlg,1),FALSE);
	}
}



#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))

static BOOL CALLBACK ReplayDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			movieHackType=3;
			SendDlgItemMessage(hwndDlg, 201, BM_SETCHECK, moviereadonly?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, 1002,BM_SETCHECK, BST_UNCHECKED, 0);

			char* findGlob[2] = {FCEU_MakeFName(FCEUMKF_MOVIEGLOB, 0, 0),
								 FCEU_MakeFName(FCEUMKF_MOVIEGLOB2, 0, 0)};

			extern int suppress_scan_chunks;
			suppress_scan_chunks=1;

			int i=0, j=0;

			for(j=0;j<2;j++)
			{
				char* temp=0;
				do {
					temp=strchr(findGlob[j],'/');
					if(temp)
						*temp = '\\';
				} while(temp);

				// disabled because... apparently something is case sensitive??
//				for(i=1;i<strlen(findGlob[j]);i++)
//					findGlob[j][i] = tolower(findGlob[j][i]);
			}

//			FCEU_PrintError(findGlob[0]);
//			FCEU_PrintError(findGlob[1]);

			for(j=0;j<2;j++)
			{
				// if the two directories are the same, only look through one of them to avoid adding everything twice
				if(j==1 && !strnicmp(findGlob[0],findGlob[1],MAX(strlen(findGlob[0]),strlen(findGlob[1]))-6))
					continue;

				char globBase[512];
				strcpy(globBase,findGlob[j]);
				globBase[strlen(globBase)-5]='\0';

				extern char FileBase[];
				//char szFindPath[512]; //mbg merge 7/17/06 removed
				WIN32_FIND_DATA wfd;
				HANDLE hFind;

				memset(&wfd, 0, sizeof(wfd));
				hFind = FindFirstFile(findGlob[j], &wfd);
				if(hFind != INVALID_HANDLE_VALUE)
				{
					do
					{
						if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							continue;

						// filter out everything that's not *.fcm, *.fmv, *.vmv, or *.nmv
						// (because FindFirstFile is too dumb to do that)
						{
							char* dot=strrchr(wfd.cFileName,'.');
							if(!dot)
								continue;
							char ext [512];
							strcpy(ext, dot+1);
							int k, extlen=strlen(ext);
							for(k=0;k<extlen;k++)
								ext[k]=tolower(ext[k]);
							if(strcmp(ext,"fcm") && strcmp(ext,"fmv") && strcmp(ext,"vmv") && strcmp(ext,"nmv"))
								continue;
						}

						MOVIE_INFO info;
						char metadata[MOVIE_MAX_METADATA];
						char rom_name[MAX_PATH];

						memset(&info, 0, sizeof(info));
						info.metadata = metadata;
						info.metadata_size = sizeof(metadata);
						info.name_of_rom_used = rom_name;
						info.name_of_rom_used_size = sizeof(rom_name);

						char filename [512];
						sprintf(filename, "%s%s", globBase, wfd.cFileName);

						char* dot = strrchr(filename, '.');
						int fcm = (dot && tolower(dot[1]) == 'f' && tolower(dot[2]) == 'c' && tolower(dot[3]) == 'm');

						if(fcm && !FCEUI_MovieGetInfo(filename, &info))
							continue;

						char md51 [256];
						char md52 [256];
						if(fcm) strcpy(md51, md5_asciistr(FCEUGameInfo->MD5));
						if(fcm) strcpy(md52, md5_asciistr(info.md5_of_rom_used));
						if(!fcm || strcmp(md51, md52))
						{
							if(fcm)
							{
								unsigned int k, count1=0, count2=0; //mbg merge 7/17/06 changed to uint
								for(k=0;k<strlen(md51);k++) count1 += md51[k]-'0';
								for(k=0;k<strlen(md52);k++) count2 += md52[k]-'0';
								if(count1 && count2)
									continue;
							}

							char* tlen1=strstr(wfd.cFileName, " (");
							char* tlen2=strstr(FileBase, " (");
							int tlen3=tlen1?(int)(tlen1-wfd.cFileName):strlen(wfd.cFileName);
							int tlen4=tlen2?(int)(tlen2-FileBase):strlen(FileBase);
							int len=MAX(0,MIN(tlen3,tlen4));
							if(strnicmp(wfd.cFileName, FileBase, len))
							{
								char temp[512];
								strcpy(temp,FileBase);
								temp[len]='\0';
								if(!strstr(wfd.cFileName, temp))
									continue;
							}
						}

						char relative[MAX_PATH];
						AbsoluteToRelative(relative, filename, BaseDirectory);
						SendDlgItemMessage(hwndDlg, 200, CB_INSERTSTRING, i++, (LPARAM)relative);
					} while(FindNextFile(hFind, &wfd));
					FindClose(hFind);
				}
			}

			suppress_scan_chunks=0;

			free(findGlob[0]);
			free(findGlob[1]);

			if(i>0)
				SendDlgItemMessage(hwndDlg, 200, CB_SETCURSEL, i-1, 0);
			SendDlgItemMessage(hwndDlg, 200, CB_INSERTSTRING, i++, (LPARAM)"Browse...");

			UpdateReplayDialog(hwndDlg);
		}

		SetFocus(GetDlgItem(hwndDlg, 200));
		return FALSE;

	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE)
		{
			UpdateReplayDialog(hwndDlg);
		}
		else if(HIWORD(wParam) == CBN_CLOSEUP)
		{
			LONG lCount = SendDlgItemMessage(hwndDlg, 200, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hwndDlg, 200, CB_GETCURSEL, 0, 0);
			if (lIndex != CB_ERR && lIndex == lCount-1)
				SendMessage(hwndDlg, WM_COMMAND, (WPARAM)IDOK, 0);		// send an OK notification to open the file browser
		}
		else
		{
			int wID = LOWORD(wParam);
			switch(wID)
			{
			case IDOK:
				{
					LONG lCount = SendDlgItemMessage(hwndDlg, 200, CB_GETCOUNT, 0, 0);
					LONG lIndex = SendDlgItemMessage(hwndDlg, 200, CB_GETCURSEL, 0, 0);
					if(lIndex != CB_ERR)
					{
						if(lIndex == lCount-1)
						{
							// pop open a file browser...
							char *pn=FCEU_GetPath(FCEUMKF_MOVIE);
							char szFile[MAX_PATH]={0};
							OPENFILENAME ofn;
							//int nRet; //mbg merge 7/17/06 removed

							memset(&ofn, 0, sizeof(ofn));
							ofn.lStructSize = sizeof(ofn);
							ofn.hwndOwner = hwndDlg;
							ofn.lpstrFilter = "Supported Movie Files (*.fcm|*.fmv|*.nmv|*.vmv)\0*.fcm;*.fmv;*.nmv;*.vmv\0FCEU Movie Files (*.fcm)\0*.fcm\0Famtasia Movie Files (*.fmv)\0*.fmv\0Nintendulator Movie Files (*.nmv)\0*.nmv\0VirtuaNES Movie Files (*.vmv)\0*.vmv\0All files(*.*)\0*.*\0\0";
							ofn.lpstrFile = szFile;
							ofn.nMaxFile = sizeof(szFile);
							ofn.lpstrInitialDir = pn;
							ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
							ofn.lpstrDefExt = "fcm";
							ofn.lpstrTitle = "Replay Movie from File";

							if(GetOpenFileName(&ofn))
							{
								char relative[MAX_PATH];
								AbsoluteToRelative(relative, szFile, BaseDirectory);

								LONG lOtherIndex = SendDlgItemMessage(hwndDlg, 200, CB_FINDSTRING, (WPARAM)-1, (LPARAM)relative);
								if(lOtherIndex != CB_ERR)
								{
									// select already existing string
									SendDlgItemMessage(hwndDlg, 200, CB_SETCURSEL, lOtherIndex, 0);
								} else {
									SendDlgItemMessage(hwndDlg, 200, CB_INSERTSTRING, lIndex, (LPARAM)relative);
									SendDlgItemMessage(hwndDlg, 200, CB_SETCURSEL, lIndex, 0);
								}

								// restore focus to the dialog
								SetFocus(GetDlgItem(hwndDlg, 200));
								UpdateReplayDialog(hwndDlg);
//								if (ofn.Flags & OFN_READONLY)
//									SendDlgItemMessage(hwndDlg, 201, BM_SETCHECK, BST_CHECKED, 0);
//								else
//									SendDlgItemMessage(hwndDlg, 201, BM_SETCHECK, BST_UNCHECKED, 0);
							}

							free(pn);
						}
						else
						{
							// user had made their choice
							// TODO: warn the user when they open a movie made with a different ROM
							char* fn=GetReplayPath(hwndDlg);
							//char TempArray[16]; //mbg merge 7/17/06 removed
							ReplayDialogReadOnlyStatus = (SendDlgItemMessage(hwndDlg, 201, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
							
							char offset1Str[32]={0};
							char offset2Str[32]={0};
							
							SendDlgItemMessage(hwndDlg, 1003, WM_GETTEXT, (WPARAM)32, (LPARAM)offset1Str);
							ReplayDialogStopFrame = (SendDlgItemMessage(hwndDlg, 1002, BM_GETCHECK,0,0) == BST_CHECKED)? strtol(offset1Str,0,10):0;

							SendDlgItemMessage(hwndDlg, 1000, WM_GETTEXT, (WPARAM)32, (LPARAM)offset1Str);
							SendDlgItemMessage(hwndDlg, 1001, WM_GETTEXT, (WPARAM)32, (LPARAM)offset2Str);

							movieConvertOffset1=strtol(offset1Str,0,10);
							movieConvertOffset2=strtol(offset2Str,0,10);
							movieConvertOK=1;

							EndDialog(hwndDlg, (INT_PTR)fn);
						}
					}
				}
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
			}
		}

	case WM_CTLCOLORSTATIC:
		if((HWND)lParam == GetDlgItem(hwndDlg, 308))
		{
			// draw the md5 sum in red if it's different from the md5 of the rom used in the replay
			HDC hdcStatic = (HDC)wParam;
			char szMd5Text[35];

			GetDlgItemText(hwndDlg, 305, szMd5Text, 35);
			if(!strlen(szMd5Text) || !strcmp(szMd5Text, "unknown") || !strcmp(szMd5Text, "00000000000000000000000000000000") || !strcmp(szMd5Text, md5_asciistr(FCEUGameInfo->MD5)))
				SetTextColor(hdcStatic, RGB(0,0,0));		// use black color for a match (or no comparison)
			else
				SetTextColor(hdcStatic, RGB(255,0,0));		// use red for a mismatch
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)GetStockObject(NULL_BRUSH);
		}
		else
			return FALSE;
	}

	return FALSE;
};

void FCEUD_MovieReplayFrom(void)
{
  char* fn;
	
  StopSound();

  fn=(char*)DialogBox(fceu_hInstance, "IDD_REPLAYINP", hAppWnd, ReplayDialogProc);
  if(fn)
  {
	  FCEUI_LoadMovie(fn, ReplayDialogReadOnlyStatus, ReplayDialogStopFrame);
	  free(fn);
      palyo=FCEUI_GetCurrentVidSystem(0,0);
      UpdateMenu();
      FixFL();
      SetMainWindowStuff();
      RefreshThrottleFPS();
   
	  extern int movie_readonly;
	  moviereadonly = movie_readonly; // for prefs
  }
}

static void UpdateRecordDialogPath(HWND hwndDlg, const char* fname)
{
	char* baseMovieDir = FCEU_GetPath(FCEUMKF_MOVIE);
	char* fn=0;

	// display a shortened filename if the file exists in the base movie directory
	if(!strncmp(fname, baseMovieDir, strlen(baseMovieDir)))
	{
		char szDrive[MAX_PATH]={0};
		char szDirectory[MAX_PATH]={0};
		char szFilename[MAX_PATH]={0};
		char szExt[MAX_PATH]={0};
		
		_splitpath(fname, szDrive, szDirectory, szFilename, szExt);
		fn=(char*)malloc(strlen(szFilename)+strlen(szExt)+1);
		_makepath(fn, "", "", szFilename, szExt);
	}
	else
		fn=strdup(fname);

	if(fn)
	{
		SetWindowText(GetDlgItem(hwndDlg,200),fn);				// FIXME:  make utf-8?
		free(fn);
	}
}

static void UpdateRecordDialog(HWND hwndDlg)
{
	int enable=0;
	char* fn=0;

	fn=GetRecordPath(hwndDlg);

	if(fn)
	{
		if(access(fn, F_OK) ||
			!access(fn, W_OK))
		{
			LONG lCount = SendDlgItemMessage(hwndDlg, 301, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hwndDlg, 301, CB_GETCURSEL, 0, 0);
			if(lIndex != lCount-1)
			{
				enable=1;
			}
		}

		free(fn);
	}

	EnableWindow(GetDlgItem(hwndDlg,1),enable ? TRUE : FALSE);
}

struct CreateMovieParameters
{
	char* szFilename;				// on Dialog creation, this is the default filename to display.  On return, this is the filename that the user chose.
	int recordFrom;				// 0 = "Power-On", 1 = "Reset", 2 = "Now", 3+ = savestate file in szSavestateFilename
	char* szSavestateFilename;
	WCHAR metadata[MOVIE_MAX_METADATA];
};

static BOOL CALLBACK RecordDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct CreateMovieParameters* p = NULL;
	
	switch(uMsg)
	{
	case WM_INITDIALOG:
		p = (struct CreateMovieParameters*)lParam;
		UpdateRecordDialogPath(hwndDlg, p->szFilename);
		free(p->szFilename);
		
		/* Populate the "record from..." dialog */
		{
			char* findGlob=FCEU_MakeFName(FCEUMKF_STATEGLOB, 0, 0);
			WIN32_FIND_DATA wfd;
			HANDLE hFind;
			int i=0;
			
			SendDlgItemMessage(hwndDlg, 301, CB_INSERTSTRING, i++, (LPARAM)"Start");
			SendDlgItemMessage(hwndDlg, 301, CB_INSERTSTRING, i++, (LPARAM)"Reset");
			SendDlgItemMessage(hwndDlg, 301, CB_INSERTSTRING, i++, (LPARAM)"Now");
			
			memset(&wfd, 0, sizeof(wfd));
			hFind = FindFirstFile(findGlob, &wfd);
			if(hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
						(wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
						continue;
					
					if (strlen(wfd.cFileName) < 4 ||
						!strcmp(wfd.cFileName + (strlen(wfd.cFileName) - 4), ".fcm"))
						continue;

					SendDlgItemMessage(hwndDlg, 301, CB_INSERTSTRING, i++, (LPARAM)wfd.cFileName);
				} while(FindNextFile(hFind, &wfd));
				FindClose(hFind);
			}
			free(findGlob);
			
			SendDlgItemMessage(hwndDlg, 301, CB_INSERTSTRING, i++, (LPARAM)"Browse...");
			SendDlgItemMessage(hwndDlg, 301, CB_SETCURSEL, 0, 0);				// choose "from reset" as a default
		}
		UpdateRecordDialog(hwndDlg);
		
		return TRUE;
		
	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE)
		{
			LONG lIndex = SendDlgItemMessage(hwndDlg, 301, CB_GETCURSEL, 0, 0);
			if(lIndex == CB_ERR)
			{
				// fix listbox selection
				SendDlgItemMessage(hwndDlg, 301, CB_SETCURSEL, (WPARAM)0, 0);
			}
			UpdateRecordDialog(hwndDlg);
			return TRUE;
		}
		else if(HIWORD(wParam) == CBN_CLOSEUP)
		{
			LONG lCount = SendDlgItemMessage(hwndDlg, 301, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hwndDlg, 301, CB_GETCURSEL, 0, 0);
			if (lIndex != CB_ERR && lIndex == lCount-1)
			{
				OPENFILENAME ofn;
				char szChoice[MAX_PATH]={0};
				
				// pop open a file browser to choose the savestate
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = "FCE Ultra Save State(*.fc?)\0*.fc?\0\0";
				ofn.lpstrFile = szChoice;
				ofn.lpstrDefExt = "fcs";
				ofn.nMaxFile = MAX_PATH;
				if(GetOpenFileName(&ofn))
				{
					SendDlgItemMessage(hwndDlg, 301, CB_INSERTSTRING, lIndex, (LPARAM)szChoice);
					SendDlgItemMessage(hwndDlg, 301, CB_SETCURSEL, (WPARAM)lIndex, 0);
				}
				else
					UpdateRecordDialog(hwndDlg);
			}
			return TRUE;
		}
		else if(HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 200)
		{
			UpdateRecordDialog(hwndDlg);
		}
		else
		{
			switch(LOWORD(wParam))
			{
			case IDOK:
				{
					LONG lIndex = SendDlgItemMessage(hwndDlg, 301, CB_GETCURSEL, 0, 0);
					p->szFilename = GetRecordPath(hwndDlg);
					GetDlgItemTextW(hwndDlg,300,p->metadata,MOVIE_MAX_METADATA);
					p->recordFrom = (int)lIndex;
					if(lIndex>=3)
						p->szSavestateFilename = GetSavePath(hwndDlg);
					EndDialog(hwndDlg, 1);
				}
				return TRUE;
				
			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
				
			case 201:
				{
					OPENFILENAME ofn;
					char szChoice[MAX_PATH]={0};
					
					// browse button
					memset(&ofn, 0, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwndDlg;
					ofn.lpstrFilter = "FCE Ultra Movie File(*.fcm)\0*.fcm\0All files(*.*)\0*.*\0\0";
					ofn.lpstrFile = szChoice;
					ofn.lpstrDefExt = "fcm";
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
					if(GetSaveFileName(&ofn))
						UpdateRecordDialogPath(hwndDlg,szChoice);
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}

void FCEUD_MovieRecordTo(void)
{
	struct CreateMovieParameters p;
	p.szFilename=FCEUI_MovieGetCurrentName(0);
	
	StopSound();

	if(DialogBoxParam(fceu_hInstance,"IDD_RECORDINP",hAppWnd,RecordDialogProc,(LPARAM)&p))
	{
		// turn WCHAR into UTF8
		char meta[MOVIE_MAX_METADATA<<2];
		WideCharToMultiByte(CP_UTF8, 0, p.metadata, -1, meta, sizeof(meta), NULL, NULL);

		if(p.recordFrom >= 3)
		{
			// attempt to load the savestate
			// FIXME:  pop open a messagebox if this fails
			FCEUI_LoadState(p.szSavestateFilename);
			{
				extern int loadStateFailed;
				if(loadStateFailed)
				{
					char str [1024];
					sprintf(str, "Failed to load save state \"%s\".\nRecording from current state instead...", p.szSavestateFilename);
					FCEUD_PrintError(str);
				}
			}
			free(p.szSavestateFilename);
		}

		FCEUI_SaveMovie(p.szFilename, (p.recordFrom==0) ? MOVIE_FLAG_FROM_POWERON : ((p.recordFrom==1) ? MOVIE_FLAG_FROM_RESET : 0), meta);
	}

	free(p.szFilename);
}

void FCEUD_AviRecordTo(void)
{
	OPENFILENAME ofn;
	char szChoice[MAX_PATH];

	if(FCEUMOV_IsPlaying())
	{
		extern char curMovieFilename[];
		strcpy(szChoice, curMovieFilename);
		char* dot=strrchr(szChoice,'.');
		if(dot) *dot='\0';
		strcat(szChoice,".avi");
	}
	else
	{
		extern char FileBase[];
		sprintf(szChoice, "%s.avi", FileBase);
	}

	StopSound();

	// avi record file browser
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hAppWnd;
	ofn.lpstrFilter = "AVI Files (*.avi)\0*.avi\0\0";
	ofn.lpstrFile = szChoice;
	ofn.lpstrDefExt = "avi";
	ofn.lpstrTitle = "Save AVI as";

	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	if(GetSaveFileName(&ofn))
	{
		FCEUI_AviBegin(szChoice);
	}
}

void FCEUD_AviStop(void)
{
	FCEUI_AviEnd();
}

void FCEUD_CmdOpen(void)
{
	StopSound();
	LoadNewGamey(hAppWnd, 0);
}

