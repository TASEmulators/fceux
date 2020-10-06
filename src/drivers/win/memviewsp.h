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

#ifndef MEMVIEWSP_H
#define MEMVIEWSP_H

#include "types.h"

#define ID_FIRST_BOOKMARK               30
#define ID_BOOKMARKLIST_SEP				(ID_FIRST_BOOKMARK - 1)

typedef struct
{
	char description[51];
	unsigned int address;
	int editmode;
} HexBookmark;

typedef struct
{
	HexBookmark* bookmark;
	int bookmark_index;
	int shortcut_index = -1;
} HexBookmarkMsg;

extern struct HexBookmarkList
{
	HexBookmark bookmarks[64];
	int shortcuts[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	int bookmarkCount = 0;
	int shortcutCount = 0;

	HexBookmark& operator[](int index);
} hexBookmarks;

#define IMPORT_OVERWRITE_NONE 0 // Overwrite nothing
#define IMPORT_OVERWRITE_BOOKMARK 1 // Overwrite duplicated bookmarks but don't overwrite duplicated shortcuts
#define IMPORT_OVERWRITE_SHORTCUT 2 // Overwrite duplicated shortcuts but don't overwrite duplicated bookmarks
#define IMPORT_OVERWRITE_ALL (IMPORT_OVERWRITE_BOOKMARK | IMPORT_OVERWRITE_SHORTCUT), // (3) Overwrite duplicated bookmarks and shortcuts
#define IMPORT_OVERWRITE_NO_PROMPT 4 // Not confirm for what to do when conflicts
#define IMPORT_DISCARD_ORIGINAL 8 // Discard all the original bookmarks

extern int importBookmarkProps;

int findBookmark(unsigned int address, int editmode);
int addBookmark(HWND hwnd, unsigned int address, int editmode);
int editBookmark(HWND hwnd, unsigned int index);
int removeBookmark(unsigned int index);
// int toggleBookmark(HWND hwnd, uint32 address, int mode);
void updateBookmarkMenus(HMENU menu);
int handleBookmarkMenu(int bookmark);
void removeAllBookmarks(HMENU menu);

extern LRESULT APIENTRY FilterEditCtrlProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP);
extern WNDPROC DefaultEditCtrlProc;

#endif