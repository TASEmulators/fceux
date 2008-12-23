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

#include "resource.h"
#include "window.h"

//internal variables
int pauseAfterPlayback = 0;		//Flag for pausing emulator when movie is finished


//external
extern int status_icon;		//In main.cpp - For displaying movie status icons (play,record,pause)
extern bool movieSubtitles; //In fceu.cpp - Toggle for displaying movie subtitles
extern bool subtitlesOnAVI; //In movie.cpp - Toggle for putting movie subtitles in an AVI
extern bool autoMovieBackup;//In fceu.cpp - Toggle that determines if movies should be backed up automatically before altering them
extern bool bindSavestate ;		//Toggle that determines if a savestate filename will include the movie filename

void UpdateCheckBoxes(HWND hwndDlg)
{
	CheckDlgButton(hwndDlg, IDC_MOVIE_PAUSEAFTERPLAYBACK, pauseAfterPlayback ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_MOVIE_BINDSAVESTATES, bindSavestate ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_MOVIE_DISPLAYSTATUSICON, status_icon ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_MOVIE_DISPLAYSUBTITLES, movieSubtitles ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_MOVIE_SUBTITLESINAVI, subtitlesOnAVI ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_MOVIE_AUTOBACKUP, autoMovieBackup ? BST_CHECKED : BST_UNCHECKED);
}

void CloseMovieOptionsDialog(HWND hwndDlg)
{
	EndDialog(hwndDlg, 0);
}

/**
* Callback function of the Timing configuration dialog.
**/
BOOL CALLBACK MovieOptionsCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:            

			UpdateCheckBoxes(hwndDlg);
			break;

		case WM_CLOSE:
		case WM_QUIT:
			CloseMovieOptionsDialog(hwndDlg);
			break;

		case WM_COMMAND:

			if(!(wParam >> 16))
			{
				switch(wParam & 0xFFFF)
				{
					case IDC_MOVIE_PAUSEAFTERPLAYBACK:
						pauseAfterPlayback = pauseAfterPlayback?0:1;
						break;

					case IDC_MOVIE_BINDSAVESTATES:
						bindSavestate ^= 1;
						break;

					case IDC_MOVIE_DISPLAYSTATUSICON:
						status_icon = status_icon?0:1;
						break;

					case IDC_MOVIE_DISPLAYSUBTITLES:
						movieSubtitles ^= 1;
						if (movieSubtitles)	FCEU_DispMessage("Movie subtitles on");
						else FCEU_DispMessage("Movie subtitles off");
						break;

					case IDC_MOVIE_SUBTITLESINAVI:
						subtitlesOnAVI ^= 1;
						break;

					case IDC_MOVIE_AUTOBACKUP:
						autoMovieBackup ^= 1;
						break;

					case IDC_MOVIE_CLOSE:
						CloseMovieOptionsDialog(hwndDlg);
						break;
				}
			}
	}

	return 0;
}

void OpenMovieOptions()
{
	DialogBox(fceu_hInstance, "MOVIEOPTIONS", hAppWnd, MovieOptionsCallB);  
}
