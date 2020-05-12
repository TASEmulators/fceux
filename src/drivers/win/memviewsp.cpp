/* FCEUXD SP - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 Sebastian Porst
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "memviewsp.h"
#include "memview.h"
#include "common.h"

HexBookmark hexBookmarks[64];
int nextBookmark = 0;

/// Finds the bookmark for a given address
/// @param address The address to find.
/// @param editmode The editing mode of the hex editor (RAM/PPU/OAM/ROM)
/// @return The index of the bookmark at that address or -1 if there's no bookmark at that address.
int findBookmark(unsigned int address, int editmode)
{
	int i;

	if (address > 0xFFFF)
	{
		MessageBox(0, "Error: Invalid address was specified as parameter to findBookmark", "Error", MB_OK | MB_ICONERROR);
		return -1;
	}
	
	for (i=0;i<nextBookmark;i++)
	{
		if (hexBookmarks[i].address == address && hexBookmarks[i].editmode == editmode)
			return i;
	}
	
	return -1;
}

BOOL CenterWindow(HWND hwndDlg);

/// Callback function for the name bookmark dialog
INT_PTR CALLBACK nameBookmarkCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static char* description;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			// Limit bookmark descriptions to 50 characters
			SendDlgItemMessage(hwndDlg,IDC_BOOKMARK_DESCRIPTION,EM_SETLIMITTEXT,50,0);
			
			// Put the current bookmark description into the edit field
			// and set focus to that edit field.
			
			description = (char*)lParam;
			SetDlgItemText(hwndDlg, IDC_BOOKMARK_DESCRIPTION, description);
			SetFocus(GetDlgItem(hwndDlg, IDC_BOOKMARK_DESCRIPTION));
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg, 0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case BN_CLICKED:
					switch(LOWORD(wParam))
					{
						case IDOK:
						{
							// Update the bookmark description
							GetDlgItemText(hwndDlg, IDC_BOOKMARK_DESCRIPTION, description, 50);
							EndDialog(hwndDlg, 1);
							break;
						}
					}
			}
			break;
	}
	
	return FALSE;
}

/// Attempts to add a new bookmark to the bookmark list.
/// @param hwnd HWND of the FCEU window
/// @param address Address of the new bookmark
/// @param editmode The editing mode of the hex editor (RAM/PPU/OAM/ROM)
/// @return Returns 0 if everything's OK, 1 if user canceled and an error flag otherwise.
int addBookmark(HWND hwnd, unsigned int address, int editmode)
{
	// Enforce a maximum of 64 bookmarks
	if (nextBookmark < 64)
	{
		char description[51] = { 0 };
		sprintf(description, "%s %04X", EditString[editmode], address);
	
		// Show the bookmark name dialog
		if (DialogBoxParam(fceu_hInstance, "NAMEBOOKMARKDLG", hwnd, nameBookmarkCallB, (LPARAM)description))
		{
			// Add the bookmark
			hexBookmarks[nextBookmark].address = address;
			hexBookmarks[nextBookmark].editmode = editmode;
			strcpy(hexBookmarks[nextBookmark].description, description);
			nextBookmark++;
			return 0;
		}
		else
			return 1;
	}
	else
		return -1;
}

/// Edit a bookmark in the bookmark list
/// @param hwnd HWND of the FCEU window
/// @param index Index of the bookmark to edit
/// @return Returns 0 if everything's OK, 1 if user canceled and an error flag otherwise.
int editBookmark(HWND hwnd, unsigned int index)
{
	if (index >= 64) return -1;

	char description[51] = { 0 };
	strcpy(description, hexBookmarks[index].description);

	// Show the bookmark name dialog
	if (DialogBoxParam(fceu_hInstance, "NAMEBOOKMARKDLG", hwnd, nameBookmarkCallB, (LPARAM)description))
	{
		// Update the bookmark information
		strcpy(hexBookmarks[index].description, description);
		return 0;
	}
	else
		return 1;

	return -1;
}

/// Removes a bookmark from the bookmark list
/// @param index Index of the bookmark to remove
/// @return Returns 0 if everything's OK, 1 if user canceled and an error flag otherwise.
int removeBookmark(unsigned int index)
{
	if (index >= 64) return -1;

	// At this point it's necessary to move the content of the bookmark list
	for (int i=index;i<nextBookmark - 1;i++)
	{
		hexBookmarks[i] = hexBookmarks[i+1];
	}
	
	--nextBookmark;

	return 0;
}
/*
/// Adds or removes a bookmark from a given address
/// @param hwnd HWND of the emu window
/// @param address Address of the bookmark
/// @param editmode The editing mode of the hex editor (RAM/PPU/OAM/ROM)
int toggleBookmark(HWND hwnd, uint32 address, int editmode)
{
	int val = findBookmark(address, editmode);
	
	// If there's no bookmark at the given address add one.
	if (val == -1)
	{
		return addBookmark(hwnd, address, editmode);
	}
	else // else remove the bookmark
	{
		removeBookmark(val);
		return 0;
	}
}
*/
/// Updates the bookmark menu in the hex window
/// @param menu Handle of the bookmark menu
void updateBookmarkMenus(HMENU menu)
{
	// Remove all bookmark menus
	for (int i = 0; i<nextBookmark + 1; i++)
		RemoveMenu(menu, ID_FIRST_BOOKMARK + i, MF_BYCOMMAND);
	RemoveMenu(menu, ID_BOOKMARKLIST_SEP, MF_BYCOMMAND);

	if (nextBookmark != 0)
	{
		// Add the menus again
		AppendMenu(menu, MF_SEPARATOR, ID_BOOKMARKLIST_SEP, NULL);
		for (int i = 0;i<nextBookmark;i++)
		{
			// Get the text of the menu
			char buffer[0x100];
			sprintf(buffer, i < 10 ? "&%d. $%04X - %s\tCtrl+%d" : "%d. $%04X - %s",i, hexBookmarks[i].address, hexBookmarks[i].description, i);
		
			AppendMenu(menu, MF_STRING, ID_FIRST_BOOKMARK + i, buffer);
		}
	}
}

/// Returns the address to scroll to if a given bookmark was activated
/// @param bookmark Index of the bookmark
/// @return The address to scroll to or -1 if the bookmark index is invalid.
int handleBookmarkMenu(int bookmark)
{
	if (bookmark < nextBookmark)
	{
		return hexBookmarks[bookmark].address;
	}
	
	return -1;
}

/// Removes all bookmarks
/// @param menu Handle of the bookmark menu
void removeAllBookmarks(HMENU menu)
{
	for (int i = 0;i<nextBookmark;i++)
	{
		RemoveMenu(menu, ID_FIRST_BOOKMARK + i, MF_BYCOMMAND);
	}
	RemoveMenu(menu, ID_BOOKMARKLIST_SEP, MF_BYCOMMAND);
	
	nextBookmark = 0;
}