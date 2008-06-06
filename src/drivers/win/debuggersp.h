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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
 #include <windows.h>

//mbg merge 7/17/06 made struct sane c++
struct Name 
{
	Name* next;
	char* offset;
	char* name;
	char* comment;
};

extern char symbDebugEnabled;
extern unsigned int bookmarks;
extern unsigned short* bookmarkData;
extern int debuggerWasActive;

int checkCondition(const char* buffer, int num);
void loadNameFiles();
void decorateAddress(unsigned int addr, char* str, const char* chr, UINT);
void replaceNames(Name* list, char* str);
void AddDebuggerBookmark(HWND hwnd);
void AddDebuggerBookmark2(HWND hwnd, char* buffer);
void DeleteDebuggerBookmark(HWND hwnd);
void GoToDebuggerBookmark(HWND hwnd);
int getBank(int offs);
void dumpBookmarks(HWND hwmd);
int isHex(char c);
