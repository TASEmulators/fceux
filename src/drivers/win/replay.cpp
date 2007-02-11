#include "replay.h"
#include "common.h"
#include "main.h"
#include "window.h"

extern int movieConvertOffset1, movieConvertOffset2, movieConvertOK;
extern FCEUGI *GameInfo;

static int ReplayDialogReadOnlyStatus = 0;
static int ReplayDialogStopFrame = 0;

static int movieHackType = 3;

char *md5_asciistr(uint8 digest[16]);
void RefreshThrottleFPS();

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

void UpdateReplayDialog(HWND hwndDlg)
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

			SetWindowText(GetDlgItem(hwndDlg,308),md5_asciistr(GameInfo->MD5));
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
		SetWindowText(GetDlgItem(hwndDlg,308),md5_asciistr(GameInfo->MD5));
		SetDlgItemText(hwndDlg,1003,"");
		EnableWindow(GetDlgItem(hwndDlg,201),FALSE);
		SendDlgItemMessage(hwndDlg,201,BM_SETCHECK,BST_UNCHECKED,0);
		EnableWindow(GetDlgItem(hwndDlg,1),FALSE);
	}
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

BOOL CALLBACK ReplayDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
						if(fcm) strcpy(md51, md5_asciistr(GameInfo->MD5));
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
			if(!strlen(szMd5Text) || !strcmp(szMd5Text, "unknown") || !strcmp(szMd5Text, "00000000000000000000000000000000") || !strcmp(szMd5Text, md5_asciistr(GameInfo->MD5)))
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

/**
* Show movie replay dialog and replay the movie if necessary.
**/
void FCEUD_MovieReplayFrom(void)
{
	char* fn;

	fn = (char*)DialogBox(fceu_hInstance, "IDD_REPLAYINP", hAppWnd, ReplayDialogProc);

	if(fn)
	{
		FCEUI_LoadMovie(fn, ReplayDialogReadOnlyStatus, ReplayDialogStopFrame);

		free(fn);

		pal_emulation = FCEUI_GetCurrentVidSystem(0,0);
		UpdateCheckedMenuItems();
		FixFL();
		SetMainWindowStuff();
		RefreshThrottleFPS();

		extern int movie_readonly;
		moviereadonly = movie_readonly; // for prefs
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

/**
* Show the record movie dialog and record a movie.
**/
void FCEUD_MovieRecordTo()
{
	struct CreateMovieParameters p;
	p.szFilename = FCEUI_MovieGetCurrentName(0);

	if(DialogBoxParam(fceu_hInstance, "IDD_RECORDINP", hAppWnd, RecordDialogProc, (LPARAM)&p))
	{
		// turn WCHAR into UTF8
		char meta[MOVIE_MAX_METADATA << 2];
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

		FCEUI_SaveMovie(
			p.szFilename,
			(p.recordFrom == 0) ? MOVIE_FLAG_FROM_POWERON : ((p.recordFrom == 1) ? MOVIE_FLAG_FROM_RESET : 0),
			meta);
	}

	free(p.szFilename);
}

