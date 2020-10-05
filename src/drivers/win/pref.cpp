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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils/xstring.h"
#include <assert.h>
#include "common.h"
#include "debugger.h"
#include "debuggersp.h"
#include "memviewsp.h"
#include "../../debug.h"

extern bool break_on_cycles;
extern uint64 break_cycles_limit;
extern bool break_on_instructions;
extern uint64 break_instructions_limit;

/**
* Stores debugger preferences in a file
*
* @param f File to write the preferences to
* @return 0 if everything went fine. An error code if something went wrong.
**/	
int storeDebuggerPreferences(FILE* f)
{
	int i;
	unsigned int size, len;
	uint8 tmp;

	// Write the number of CPU bookmarks
	size = bookmarks.size();
	bookmarks.resize(size);
	if (fwrite(&size, sizeof(unsigned int), 1, f) != 1) return 1;
	// Write the data of those bookmarks
	for (i = 0; i < (int)size; ++i)
	{
		if (fwrite(&bookmarks[i].first, sizeof(unsigned int), 1, f) != 1) return 1;
		len = bookmarks[i].second.size();
		if (fwrite(&len, sizeof(unsigned int), 1, f) != 1) return 1;
		if (fwrite(bookmarks[i].second.c_str(), 1, len, f) != len) return 1;
	}

	// Write all breakpoints
	for (i=0;i<65;i++)
	{
		unsigned int len;

		// Write the start address of a BP
		if (fwrite(&watchpoint[i].address, sizeof(watchpoint[i].address), 1, f) != 1) return 1;
		// Write the end address of a BP
		if (fwrite(&watchpoint[i].endaddress, sizeof(watchpoint[i].endaddress), 1, f) != 1) return 1;
		// Write the flags of a BP
		if (fwrite(&watchpoint[i].flags, sizeof(watchpoint[i].flags), 1, f) != 1) return 1;
		
		// Write the length of the BP condition
		len = watchpoint[i].condText ? strlen(watchpoint[i].condText) : 0;
		if (fwrite(&len, sizeof(len), 1, f) != 1) return 1;
		
		// Write the text of the BP condition
		if (len)
		{
			if (fwrite(watchpoint[i].condText, 1, len, f) != len) return 1;
		}
		
		len = watchpoint[i].desc ? strlen(watchpoint[i].desc) : 0;
		
		// Write the length of the BP description
		if (fwrite(&len, sizeof(len), 1, f) != 1) return 1;
		
		// Write the actual BP description
		if (len)
		{
			if (fwrite(watchpoint[i].desc, 1, len, f) != len) return 1;
		}
	}

	// write "Break on Bad Opcode" flag
	if (FCEUI_Debugger().badopbreak)
		tmp = 1;
	else
		tmp = 0;
	if (fwrite(&tmp, 1, 1, f) != 1) return 1;
	// write "Break when exceed" data
	if (break_on_cycles)
		tmp = 1;
	else
		tmp = 0;
	if (fwrite(&tmp, 1, 1, f) != 1) return 1;
	if (fwrite(&break_cycles_limit, sizeof(break_cycles_limit), 1, f) != 1) return 1;
	if (break_on_instructions)
		tmp = 1;
	else
		tmp = 0;
	if (fwrite(&tmp, 1, 1, f) != 1) return 1;
	if (fwrite(&break_instructions_limit, sizeof(break_instructions_limit), 1, f) != 1) return 1;
	
	return 0;
}

/**
* Stores the preferences from the Hex window
*
* @param f File to write the preferences to
* @param source The source the preferences are read from
* @return 0 if everything went fine. An error code if something went wrong.
**/
int storeHexPreferences(FILE* f, HexBookmarkList& source = hexBookmarks)
{
	int i;

	// Writes the number of bookmarks to save
	if (fwrite(&source.bookmarkCount, sizeof(source.bookmarkCount), 1, f) != 1) return 1;
	
	for (i=0;i<source.bookmarkCount;i++)
	{
		unsigned int len;

		// Writes the bookmark address
		if (fwrite(&source[i].address, sizeof(source[i].address), 1, f) != 1) return 1;
		
		len = strlen(source[i].description);
		// Writes the length of the bookmark description
		if (fwrite(&len, sizeof(len), 1, f) != 1) return 1;
		// Writes the actual bookmark description
		if (fwrite(source[i].description, 1, len, f) != len) return 1;
	}

	// Optional Section 1: Save bookmark shortcut matches
	if (source.shortcutCount)
	{
		fwrite(&source.shortcutCount, sizeof(source.shortcutCount), 1, f);
		for (int i = 0; i < 10; ++i)
			if (source.shortcuts[i] != -1)
			{
				fwrite(&source.shortcuts[i], sizeof(source.shortcuts[i]), 1, f);
				fwrite(&i, sizeof(i), 1, f);
			}
	}

	/* Optional Section 2: Edit mode
	   The Hex Editor used to have a bug, it doesn't store the edit mode to
	   the preferences, which make the bookmarks outside NES memory are all
	   treated as NES memory bookmarks.
	   However, for the consideration of backward compatibility of the older
	   version of FCEUX, we can only add the extra data to the last of the file
	   to fix this problem.
	*/
	for (int i = 0; i < source.bookmarkCount; ++i)
		if (fwrite(&source[i].editmode, sizeof(source[i].editmode), 1, f) != 1)
			return 1;

	return 0;
}

/**
* Stores the debugging preferences to a file name romname.nes.deb
*
* @param romname Name of the ROM
* @return 0 on success or an error code.
**/
int storePreferences(const char* romname)
{

	if (debuggerSaveLoadDEBFiles == false)
		return 0;

	FILE* f;
	char* filename;
	int result;
	int Counter = 0;
	
	// Prevent any attempts at file usage if the debugger is open
	// Moved debugger exit code due to complaints and the Debugger menu option being enabled

	if (!debuggerWasActive)
		return 0;

	/*
	// With some work, this could be made to prevent writing empty .deb files.
	// Currently, it doesn't account for fully-deleted lists when deciding to write.
	int i;
	int makeDebugFile = 0;

	for (i=0;i<65;i++)
	{
		if (watchpoint[i].address || watchpoint[i].endaddress || watchpoint[i].flags || hexBookmarks[i].address || hexBookmarks[i].description[0])
		makeDebugFile = 1;	
	}

	if (!makeDebugFile) 
	{
		return 0;
	}
	*/

	filename = (char*)malloc(strlen(romname) + 5);
	strcpy(filename, romname);
	strcat(filename, ".deb");

	f = fopen(filename, "wb");

	free(filename);
	
	result = !f || storeDebuggerPreferences(f) || storeHexPreferences(f);

	if (f) {
		fclose(f);
	}

	return result;
}

void DoDebuggerDataReload()
{
	if (debuggerSaveLoadDEBFiles == false)
		return;
	
	extern HWND hDebug;
	LoadGameDebuggerData(hDebug);
}

int myNumWPs = 0;
int loadDebugDataFailed = 0;

/**
* Loads debugger preferences from a file
*
* @param f File to write the preferences to
* @return 0 if everything went fine. An error code if something went wrong.
**/	
int loadDebuggerPreferences(FILE* f)
{
	unsigned int i, size, len;
	uint8 tmp;

	// Read the number of CPU bookmarks
	if (fread(&size, sizeof(unsigned int), 1, f) != 1) return 1;
	bookmarks.resize(size);
	// Read the data of those bookmarks
	char buffer[256];
	for (i = 0; i < (int)size; ++i)
	{
		if (fread(&bookmarks[i].first, sizeof(unsigned int), 1, f) != 1) return 1;
		if (fread(&len, sizeof(unsigned int), 1, f) != 1) return 1;
		if (len >= 256) return 1;
		if (fread(&buffer, 1, len, f) != len) return 1;
		buffer[len] = 0;
		bookmarks[i].second = buffer;
	}

	myNumWPs = 0;
	// Ugetab:
	// This took far too long to figure out...
	// Nullifying the data is better than using free(), because
	// a simple if(watchpoint[i].cond) can still evaluate to true.
	// This causes several tests to fail, one of which kills
	// conditional text loading when reusing a used condText.
	for (i=0;i<65;i++)
	{
		watchpoint[i].cond = 0;
		watchpoint[i].condText = 0;
		watchpoint[i].desc = 0;
	}

	// Read the breakpoints
	for (i=0;i<65;i++)
	{
		uint16 start, end;
		uint8 flags;
		unsigned int len;
		
		// Read the start address of the BP
		if (fread(&start, sizeof(start), 1, f) != 1) return 1;
		// Read the end address of the BP
		if (fread(&end, sizeof(end), 1, f) != 1) return 1;
		// Read the flags of the BP
		if (fread(&flags, sizeof(flags), 1, f) != 1) return 1;
		
		// Read the length of the BP condition
		if (fread(&len, sizeof(len), 1, f) != 1) return 1;
		
		// Delete eventual older conditions
		if (watchpoint[myNumWPs].condText)
			free(watchpoint[myNumWPs].condText);
				
		watchpoint[myNumWPs].condText = (char*)malloc(len + 1);
		watchpoint[myNumWPs].condText[len] = 0;
		if (len)
		{
			// Read the breakpoint condition
			if (fread(watchpoint[myNumWPs].condText, 1, len, f) != len) return 1;
			// TODO: Check return value
			checkCondition(watchpoint[myNumWPs].condText, myNumWPs);
		}
		
		// Read length of the BP description
		if (fread(&len, sizeof(len), 1, f) != 1) return 1;
		
		// Delete eventual older description
		if (watchpoint[myNumWPs].desc)
			free(watchpoint[myNumWPs].desc);
				
		watchpoint[myNumWPs].desc = (char*)malloc(len + 1);
		watchpoint[myNumWPs].desc[len] = 0;
		if (len)
		{
			// Read breakpoint description
			if (fread(watchpoint[myNumWPs].desc, 1, len, f) != len) return 1;
		}
		
		watchpoint[i].address = 0;
		watchpoint[i].endaddress = 0;
		watchpoint[i].flags = 0;

		// Activate breakpoint
		if (start || end || flags)
		{
			watchpoint[myNumWPs].address = start;
			watchpoint[myNumWPs].endaddress = end;
			watchpoint[myNumWPs].flags = flags;

			myNumWPs++;
		}
	}

	// Read "Break on Bad Opcode" flag
	if (fread(&tmp, 1, 1, f) != 1) return 1;
	FCEUI_Debugger().badopbreak = (tmp != 0);
	// Read "Break when exceed" data
	if (fread(&tmp, 1, 1, f) != 1) return 1;
	break_on_cycles = (tmp != 0);
	if (fread(&break_cycles_limit, sizeof(break_cycles_limit), 1, f) != 1) return 1;
	if (fread(&tmp, 1, 1, f) != 1) return 1;
	break_on_instructions = (tmp != 0);
	if (fread(&break_instructions_limit, sizeof(break_instructions_limit), 1, f) != 1) return 1;

	return 0;
}


/**
* Loads HexView preferences from a file
*
* @param f File to write the preferences to
* @param target The target to load the preferences to
* @return 0 if everything went fine. An error code if something went wrong.
**/	
int loadHexPreferences(FILE* f, HexBookmarkList& target = hexBookmarks)
{
	int i;

	// Read number of bookmarks
	if (fread(&target.bookmarkCount, sizeof(target.bookmarkCount), 1, f) != 1) return 1;
	if (target.bookmarkCount >= 64) return 1;

	// clean the garbage values
	memset(&target.bookmarks, 0, sizeof(HexBookmark) * target.bookmarkCount);


	for (i=0;i<target.bookmarkCount;i++)
	{
		unsigned int len;
		
		// Read address
		if (fread(&target[i].address, sizeof(target[i].address), 1, f) != 1) return 1;
		// Read length of description
		if (fread(&len, sizeof(len), 1, f) != 1) return 1;
		// Read the bookmark description
		if (fread(target[i].description, 1, len, f) != len) return 1;
	}

	/* Optional Section 1: read bookmark shortcut matches
	   read number of shortcuts
	   older versions of .deb file don't have this section, so the file would reach the end.
	*/
	if (!feof(f))
	{
		fread(&target.shortcutCount, sizeof(target.shortcutCount), 1, f);
	
		unsigned int bookmark_index, shortcut_index;
		// read the matching index list of the shortcuts
		for (unsigned int i = 0; i < target.shortcutCount; ++i)
			if (fread(&bookmark_index, sizeof(bookmark_index), 1, f) != EOF && fread(&shortcut_index, sizeof(shortcut_index), 1, f) != EOF)
				target.shortcuts[shortcut_index % 10] = bookmark_index;
			else
				break;
	}
	else {
		// use the default configruation based on the order of the bookmark list
		target.shortcutCount = target.bookmarkCount > 10 ? 10 : target.bookmarkCount;
		for (int i = 0; i < target.shortcutCount; ++i)
			target.shortcuts[i] = i;
	}

	/*
       Optional Section 2: Edit mode
	   The Hex Editor used to have a bug, it doesn't store the edit mode to 
	   the preferences, which make the bookmarks outside NES memory are all
	   treated as NES memory bookmarks.
	   However, for the consideration of backward compatibility of the older
	   version of FCEUX, we can only add the extra data to the last of the file
	   to fix this problem.
	*/
	int editmode;
	if (!feof(f))
		for (int i = 0; i < target.bookmarkCount; i++)
		{
			if (fread(&editmode, sizeof(editmode), 1, f) != EOF)
				target[i].editmode = editmode;
			else break;
		}
	return 0;
}

/**
* Loads the debugging preferences from disk
*
* @param romname Name of the active ROM file
* @return 0 or an error code.
**/
int loadPreferences(const char* romname)
{
	if (debuggerSaveLoadDEBFiles == false) {
		return 0;
	}

	FILE* f;
	int result;
	int i;

	myNumWPs = 0;

	// Get the name of the preferences file
	char* filename = (char*)malloc(strlen(romname) + 5);
	strcpy(filename, romname);
	strcat(filename, ".deb");
	
	f = fopen(filename, "rb");
	free(filename);
	
	if (f)
	{
		result = loadDebuggerPreferences(f) || loadHexPreferences(f);
		if (result)
		{
			// there was error when loading the .deb, reset the data to default
			DeleteAllDebuggerBookmarks();
			myNumWPs = 0;
			break_on_instructions = break_on_cycles = FCEUI_Debugger().badopbreak = false;
			break_instructions_limit = break_cycles_limit = 0;
			hexBookmarks.bookmarkCount = 0;
		}
		fclose(f);
	} else
	{
		result = 0;
	}

	// This prevents information from traveling between debugger interations.
	// It needs to be run pretty much regardless of whether there's a .deb file or not,
	// successful read or failure. Otherwise, it's basically leaving previous info around.
	for (i=myNumWPs;i<=64;i++)
	{
		watchpoint[i].address = 0;
		watchpoint[i].endaddress = 0;
		watchpoint[i].flags = 0;
	}

	// Attempt to load the preferences
	return result;
}
