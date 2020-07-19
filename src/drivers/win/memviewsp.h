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

extern HexBookmark hexBookmarks[64];
extern int hexBookmarkShortcut[10];
extern int numHexBookmarkShortcut;
extern int nextBookmark;

int findBookmark(unsigned int address, int editmode);
int addBookmark(HWND hwnd, unsigned int address, int editmode);
int editBookmark(HWND hwnd, unsigned int index);
int removeBookmark(unsigned int index);
// int toggleBookmark(HWND hwnd, uint32 address, int mode);
void updateBookmarkMenus(HMENU menu);
int handleBookmarkMenu(int bookmark);
void removeAllBookmarks(HMENU menu);
