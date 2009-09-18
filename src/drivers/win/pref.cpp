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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "debuggersp.h"
#include "memviewsp.h"
#include "common.h"
#include "debugger.h"
#include "../../debug.h"

extern char symbDebugEnabled;
bool wasinDebugger = false;

/**
* Stores debugger preferences in a file
*
* @param f File to write the preferences to
* @return 0 if everything went fine. An error code if something went wrong.
**/	
int storeDebuggerPreferences(FILE* f)
{
	int i;

	// Flag that says whether symbolic debugging should be enabled
	if (fwrite(&symbDebugEnabled, 1, 1, f) != 1) return 1;

	// Write the number of active CPU bookmarks
	if (fwrite(&bookmarks, sizeof(unsigned int), 1, f) != 1) return 1;
	// Write the addresses of those bookmarks
	if (fwrite(bookmarkData, sizeof(unsigned short), bookmarks, f) != bookmarks) return 1;

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
	
	return 0;
}

/**
* Stores the preferences from the Hex window
*
* @param f File to write the preferences to
* @return 0 if everything went fine. An error code if something went wrong.
**/
int storeHexPreferences(FILE* f)
{
	int i;

	// Writes the number of bookmarks to save
	if (fwrite(&nextBookmark, sizeof(nextBookmark), 1, f) != 1) return 1;
	
	for (i=0;i<nextBookmark;i++)
	{
		unsigned int len;

		// Writes the bookmark address
		if (fwrite(&hexBookmarks[i].address, sizeof(hexBookmarks[i].address), 1, f) != 1) return 1;
		
		len = strlen(hexBookmarks[i].description);
		// Writes the length of the bookmark description
		if (fwrite(&len, sizeof(len), 1, f) != 1) return 1;
		// Writes the actual bookmark description
		if (fwrite(hexBookmarks[i].description, 1, len, f) != len) return 1;
	}
	
	return 0;
}

/**
* Stores the debugging preferences to a file name romname.nes.deb
*
* @param romname Name of the ROM
* @return 0 on success or an error code.
**/
int storePreferences(char* romname)
{
	FILE* f;
	char* filename;
	int result;
	int Counter = 0;

	// Prevent any attempts at file usage if the debugger is open
	//if (inDebugger) return 0;
	
	wasinDebugger = inDebugger;
	
	// Prevent any attempts at file usage if the debugger is open
	if (wasinDebugger) {
		DebuggerExit();
	}

	while ((inDebugger) || (Counter == 300000)) {
	Counter++;
	}

	if (!debuggerWasActive)
	{
		return 0;
	}

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
	sprintf(filename, "%s.deb", romname);

	f = fopen(filename, "wb");

	free(filename);
	
	result = !f || storeDebuggerPreferences(f) || storeHexPreferences(f);

	if (f) {
		fclose(f);
	}

	return result;
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
	unsigned int i;

	// Read flag that says if symbolic debugging is enabled
	if (fread(&symbDebugEnabled, sizeof(symbDebugEnabled), 1, f) != 1) return 1;
	
	// Read the number of CPU bookmarks
	if (fread(&bookmarks, sizeof(bookmarks), 1, f) != 1) return 1;
	
	bookmarkData = (unsigned short*)malloc(bookmarks * sizeof(unsigned short));
	
	// Read the offsets of the bookmarks
	for (i=0;i<bookmarks;i++)
	{
		if (fread(&bookmarkData[i], sizeof(bookmarkData[i]), 1, f) != 1) return 1;
	}
	
	myNumWPs = 0;
	
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
		
		// Read the length of the BP description
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

	return 0;
}


/**
* Loads HexView preferences from a file
*
* @param f File to write the preferences to
* @return 0 if everything went fine. An error code if something went wrong.
**/	
int loadHexPreferences(FILE* f)
{
	int i;

	// Read number of bookmarks
	fread(&nextBookmark, sizeof(nextBookmark), 1, f);

	for (i=0;i<nextBookmark;i++)
	{
		unsigned int len;
		
		// Read address
		if (fread(&hexBookmarks[i].address, sizeof(hexBookmarks[i].address), 1, f) != 1) return 1;
		// Read length of description
		if (fread(&len, sizeof(len), 1, f) != 1) return 1;
		// Read the bookmark description
		if (fread(hexBookmarks[i].description, 1, len, f) != len) return 1;
	}
	
	return 0;
}

/**
* Loads the debugging preferences from disk
*
* @param romname Name of the active ROM file
* @return 0 or an error code.
**/
int loadPreferences(char* romname)
{
	FILE* f;
	int result;
	int i;

	myNumWPs = 0;

	// Get the name of the preferences file
	char* filename = (char*)malloc(strlen(romname) + 5);
	sprintf(filename, "%s.deb", romname);
	
	f = fopen(filename, "rb");
	free(filename);
	
	result = f ? loadDebuggerPreferences(f) || loadHexPreferences(f) : 0;

	if (f) {
		fclose(f);
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

	if (wasinDebugger){
		DoDebug(0);
	}

	// Attempt to load the preferences
	return result;
}
