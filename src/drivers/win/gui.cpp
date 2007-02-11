#include "common.h"

#include <commctrl.h>
#include <shlobj.h>     // For directories configuration dialog.

/**
* Changes the state of the mouse cursor.
* 
* @param set_visible Determines the visibility of the cursor ( 1 or 0 )
**/
void ShowCursorAbs(int set_visible)
{
	static int stat = 0;

	if(set_visible)
	{
		if(stat == -1)
		{
			stat++;
			ShowCursor(1);
		}
	}
	else
	{
		if(stat == 0)
		{
			stat--;
			ShowCursor(0);
		}
	}
}

/**
* Displays a folder selection dialog.
* 
* @param hParent Handle of the parent window.
* @param htext Text
* @param buf Storage buffer for the name of the selected path.
*
* @return 0 or 1 to indicate failure or success.
**/
int BrowseForFolder(HWND hParent, const char *htext, char *buf)
{
	BROWSEINFO bi;
	LPCITEMIDLIST pidl;

	buf[0] = 0;

	memset(&bi, 0, sizeof(bi));

	bi.hwndOwner = hParent;
	bi.lpszTitle = htext;
	bi.ulFlags = BIF_RETURNONLYFSDIRS; 

	if(FAILED(CoInitialize(0)))
	{
		return 0;
	}

	if(!(pidl = SHBrowseForFolder(&bi)))
	{
		CoUninitialize();
		return 0;
	}

	if(!SHGetPathFromIDList(pidl, buf))
	{
		CoTaskMemFree((PVOID)pidl);
		CoUninitialize();

		return 0;
	}

	/* This probably isn't the best way to free the memory... */
	CoTaskMemFree((PVOID)pidl);
	CoUninitialize();

	return 1;
}

