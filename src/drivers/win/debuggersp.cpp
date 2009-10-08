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

#include "common.h"
#include "debuggersp.h"
#include "debugger.h"
#include "../../fceu.h"
#include "../../debug.h"
#include "../../conddebug.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

int GetNesFileAddress(int A);

Name* lastBankNames = 0;
Name* loadedBankNames = 0;
Name* ramBankNames = 0;
int lastBank = -1;
int loadedBank = -1;
extern char LoadedRomFName[2048];
char symbDebugEnabled = 0;
int debuggerWasActive = 0;

/**
* Tests whether a char is a valid hexadecimal character.
* 
* @param c The char to test
* @return True or false.
**/
int isHex(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/**
* Replaces all occurences of a substring in a string with a new string.
* The maximum size of the string after replacing is 1000.
* The caller must ensure that the src buffer is large enough.
*
* @param src Source string
* @param r String to replace
* @param w New string
**/
void replaceString(char* src, const char* r, const char* w)
{
	char buff[1001] = {0};
	char* pos = src;
	char* beg = src;

	// Check parameters

	if (!src || !r || !w)
	{
		MessageBox(0, "Error: Invalid parameter in function replaceString", "Error", MB_OK | MB_ICONERROR);
		return;
	}
		
	// Replace sub strings

	while ((pos = strstr(src, r)))
	{
		*pos = 0;
		strcat(buff, src);
		strcat(buff, w ? w : r);
		src = pos + strlen(r);
	}

	strcat(buff, src);

	strcpy(beg, buff);
}

/**
* Returns the bank for a given offset.
* Technically speaking this function does not calculate the actual bank
* where the offset resides but the 0x4000 bytes large chunk of the ROM of the offset.
* 
* @param offs The offset
* @return The bank of that offset or -1 if the offset is not part of the ROM.
**/
int getBank(int offs)
{
	//NSF data is easy to overflow the return on.
	//Anything over FFFFF will kill it.


	//GetNesFileAddress doesn't work well with Unif files
	int addr = GetNesFileAddress(offs)-16;

	if (GameInfo->type==GIT_NSF) {
	return addr != -1 ? addr / 0x1000 : -1;
	}
	return addr != -1 ? addr / 0x4000 : -1;
}

/**
* Parses a line from a NL file.
* @param line The line to parse
* @param n The name structure to write the information to
*
* @return 0 if everything went OK. Otherwise an error code is returned.
**/
int parseLine(char* line, Name* n)
{
	char* pos;
	int llen;

	// Check parameters
	
	if (!line)
	{
		MessageBox(0, "Invalid parameter \"line\" in function parseLine", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	
	if (!n)
	{
		MessageBox(0, "Invalid parameter \"n\" in function parseLine", "Error", MB_OK | MB_ICONERROR);
		return 2;
	}
	
	// Allow empty lines
	if (*line == '\r' || *line == '\n')
	{
		return -1;
	}
	
	// Attempt to tokenize the given line
	
	pos = strstr(line, "#");

	if (!pos)
	{
		// Found an invalid line
		return 3;
	}
	
	// Check if the first tokenized part (the offset) is valid
	
	*pos = 0;
	
	llen = (int)strlen(line);
	
	if (llen == 5) // Offset size of normal lines of the form $XXXX
	{
		if (line[0] != '$'
			|| !isHex(line[1])
			|| !isHex(line[2])
			|| !isHex(line[3])
			|| !isHex(line[4])
		)
		{
			return 4;
		}
	}
	else if (llen >= 7) // Offset size of array definition lines of the form $XXXX/YY
	{
		int i;
		if (line[0] != '$'
			|| !isHex(line[1])
			|| !isHex(line[2])
			|| !isHex(line[3])
			|| !isHex(line[4])
			|| line[5] != '/'
		)
		{
			return 5;
		}
		
		for (i=6;line[i];i++)
		{
			if (!isHex(line[i]))
			{
				return 6;
			}
		}
	}
	else // Lines that have an invalid size
	{
		return 7;
	}
	
	
	// TODO: Validate if the offset is in the correct NL file.

	// After validating the offset it's OK to store it
	n->offset = (char*)malloc(strlen(line) + 1);
	strcpy(n->offset, line);
	line = pos + 1;
	
	// Attempt to tokenize the string again to find the name of the address

	pos = strstr(line, "#");

	if (!pos)
	{
		// Found an invalid line
		return 8;
	}

	*pos = 0;
	
	// The only requirement for a name of an address is that it's not of size 0
	
	if (*line)
	{
		n->name = (char*)malloc(strlen(line) + 1);
		strcpy(n->name, line);
	}
	else
	{
		// Properly initialize zero-length names too
		n->name = 0;
	}
	
	// Now it's clear that the line was valid. The rest of the line is the comment
	// that belongs to that line. Once again a real string is required though.
	
	line = pos + 1;

	if (*line > 0x0D)
	{
		n->comment = (char*)malloc(strlen(line) + 1);
		strcpy(n->comment, line);
	}
	else
	{
		// Properly initialize zero-length comments too
		n->comment = 0;
	}
	
	// Everything went fine.
	return 0;
}

/**
* Parses an array of lines read from a NL file.
* @param lines The lines to parse
* @param filename The name of the file the lines were read from.
* 
* @return A pointer to the first Name structure built from the parsed lines.
**/
Name* parse(char* lines, const char* filename)
{
	char* pos, *size;
	Name* prev = 0, *cur, *first = 0;

	// Check the parameters
	if (!lines)
	{
		MessageBox(0, "Invalid parameter \"lines\" in function parse", "Error", MB_OK | MB_ICONERROR);
		return 0;
	}
	
	if (!filename)
	{
		MessageBox(0, "Invalid parameter \"filename\" in function parse", "Error", MB_OK | MB_ICONERROR);
		return 0;
	}
	
	// Begin the actual parsing
	
	do
	{
		int fail;

		// Allocate a name structure to hold the parsed data from the next line
		
		cur = (Name*)malloc(sizeof(Name));
		cur->offset = 0;
		cur->next = 0;
		cur->name = 0;
		cur->comment = 0;
		
		pos = lines;

		// This first loop attempts to read potential multi-line comments and add them
		// into a single comment.
		for(;;)
		{
			// Get the end of the next line
			pos = strstr(pos, "\n");

			// If there's no end of line or if the next line does not begin with a \ character
			// we can stop.
			if (!pos || pos[1] != '\\') break;
			
			// At this point we have the following situation. pos[0] and pos[1] can be overwritten
			/*
				pos	-1	0	1	2
					 ?	\n	\   ?
			*/
			
			// \r\n is needed in text boxes
			if (pos[-1] != '\r')
			{
				pos[0] = '\r';
				pos[1] = '\n';
				pos += 2;
			}
			else
			{
				// Remove backslash
				pos[1] = ' ';
				pos += 1;
			}
		}
		
		if (!pos)
		{
			// All lines were parsed
			break;
		}

		*pos = 0;

		// Attempt to parse the current line
		fail = parseLine(lines, cur);
		
		if (fail == -1)
		{
			continue;
		}
		else if (fail) // Show an error to allow the user to correct the defect line
		{
			const char* fmtString = "Error (Code: %d): Invalid line \"%s\" in NL file \"%s\"";
			char* msg = (char*)malloc(strlen(fmtString) + 8 + strlen(lines) + strlen(filename) + 1);
			sprintf(msg, fmtString, fail, lines, filename);
			MessageBox(0, msg, "Error", MB_OK | MB_ICONERROR);
			free(msg);
			lines = pos + 1;
			MessageBox(0, lines, "Error", MB_OK | MB_ICONERROR);
			continue;
		}
		
		lines = pos + 1;
		
		// Check if the line is an array definition line
		
		size = strstr(cur->offset, "/");
			
		if (size) // Array definition line
		{
			int arrlen, offset;
			
			*size = 0;
			
			// Attempt to read the length of the array and the array offset
			if (sscanf(size + 1, "%x", &arrlen) > 0 && sscanf(cur->offset + 1, "%x", &offset) > 0)
			{
				Name* nn = 0;
				int i;
				
				// Create a node for each element of the array
				for (i=0;i<=arrlen;i++)
				{
					char numbuff[10] = {0};

					nn = (Name*)malloc(sizeof(Name));
					nn->next = 0;
					
					// The comment is the same for each array element
					nn->comment = strdup(cur->comment);
					
					// The offset of the node
					nn->offset = (char*)malloc(10);
					sprintf(nn->offset, "$%04X", offset + i);
					
					// The name of an array address is of the form NAME[INDEX]
					sprintf(numbuff, "[%X]", i);
					
					nn->name = (char*)malloc(strlen(cur->name) + strlen(numbuff) + 1);
					strcpy(nn->name, cur->name);
					strcat(nn->name, numbuff);
					
					// Add the new node to the list of address nodes
					if (prev)
					{
						prev->next = nn;
						prev = prev->next;
					}
					else
					{
						first = prev = nn;
					}
				}
				
				// Free the allocated node
				free(cur->name);
				free(cur->comment);
				free(cur->offset);
				free(cur);
				
				cur = nn;
			}
			else
			{
				// I don't think it's possible to get here as the validity of
				// offset and array size has already been validated in parseLine
				continue;
			}
		}
		else
		{
			// Add the node to the list of address nodes
			if (prev)
			{
				prev->next = cur;
				prev = prev->next;
			}
			else
			{
				first = prev = cur;
			}
		}
	} while (pos);

	// Return the first node in the list of address nodes
	return first;
}

/**
* Load and parse an entire NL file
* @param filename Name of the file to parse
*
* @return A pointer to the first Name structure built from the file data.
**/
Name* parseNameFile(const char* filename)
{
	char* buffer;
	Name* n = 0;

	// Attempt to read the file
	FILE* f = fopen(filename, "rb");

	if (f)
	{
//		__asm (".byte 0xcc");
		// Get the file size
		int lSize;
		fseek (f , 0 , SEEK_END);
		lSize = ftell(f) + 1;
		
		// Allocate sufficient buffer space
		rewind (f);
		
		buffer = (char*)malloc(lSize);

		if (buffer)
		{
			// Read the file and parse it
			memset(buffer, 0, lSize);
			fread(buffer, 1, lSize - 1, f);
			n = parse(buffer, filename);

			fclose(f);
			
			free(buffer);
		}
	}

	return n;
}

/**
* Frees an entire list of Name nodes starting with the given node.
* 
* @param n The node to start with (0 is a valid value)
**/
void freeList(Name* n)
{
	Name* next;
	
	while (n)
	{
		if (n->offset) free(n->offset);
		if (n->name) free(n->name);
		if (n->comment) free(n->comment);
		
		next = n->next;
		free(n);
		n = next;
	}
}

/**
* Replaces all offsets in a string with the names that were given to those offsets
* The caller needs to make sure that str is large enough.
* 
* @param list NL list of address definitions
* @param str The string where replacing takes place.
**/
void replaceNames(Name* list, char* str)
{
	Name* beg = list;

	if (!str)
	{
		MessageBox(0, "Error: Invalid parameter \"str\" in function replaceNames", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	
	while (beg)
	{
		if (beg->name)
		{
			replaceString(str, beg->offset, beg->name);
		}
		
		beg = beg->next;
	}
}

/**
* Searches an address node in a list of address nodes. The found node
* has the same offset as the passed parameter offs.
*
* @param node The address node list
* @offs The offset to search
* @return The node that has the given offset or 0.
**/
Name* searchNode(Name* node, const char* offs)
{
	while (node)
	{
		if (!strcmp(node->offset, offs))
		{
			return node;
		}
		
		node = node->next;
	}
	
	return 0;
}

/**
* Loads the necessary NL files
**/
void loadNameFiles()
{
	int cb;
	char* fn = (char*)malloc(strlen(LoadedRomFName) + 20);

	if (ramBankNames)
		free(ramBankNames);
		
	// The NL file for the RAM addresses has the name nesrom.nes.ram.nl
	strcpy(fn, LoadedRomFName);
	strcat(fn, ".ram.nl");

	// Load the address descriptions for the RAM addresses
	ramBankNames = parseNameFile(fn);
			
	free(fn);

	// Find out which bank is loaded at 0xC000
	cb = getBank(0xC000);
	
	if (cb == -1) // No bank was loaded at that offset
	{
		free(lastBankNames);
		lastBankNames = 0;
	}
	else if (cb != lastBank)
	{
		char* fn = (char*)malloc(strlen(LoadedRomFName) + 12);

		// If the bank changed since loading the NL files the last time it's necessary
		// to load the address descriptions of the new bank.
		
		lastBank = cb;

		// Get the name of the NL file
		sprintf(fn, "%s.%X.nl", LoadedRomFName, lastBank);

		if (lastBankNames)
			freeList(lastBankNames);

		// Load new address definitions
		lastBankNames = parseNameFile(fn);
				
		free(fn);
	}
	
	// Find out which bank is loaded at 0x8000
	cb = getBank(0x8000);
	
	if (cb == -1) // No bank is loaded at that offset
	{
		free(loadedBankNames);
		loadedBankNames = 0;
	}
	else if (cb != loadedBank)
	{
		char* fn = (char*)malloc(strlen(LoadedRomFName) + 12);

		// If the bank changed since loading the NL files the last time it's necessary
		// to load the address descriptions of the new bank.
		
		loadedBank = cb;
		
		// Get the name of the NL file
		sprintf(fn, "%s.%X.nl", LoadedRomFName, loadedBank);
		
		if (loadedBankNames)
			freeList(loadedBankNames);
			
		// Load new address definitions
		loadedBankNames = parseNameFile(fn);
				
		free(fn);
	}
}

/**
* Adds label and comment to an offset in the disassembly output string
* 
* @param addr Address of the currently processed line
* @param str Disassembly output string
* @param chr Address in string format
* @param decorate Flag that indicates whether label and comment should actually be added
**/
void decorateAddress(unsigned int addr, char* str, const char* chr, UINT decorate)
{
	if (decorate)
	{
		Name* n;
		
		if (addr < 0x8000)
		{
			// Search address definition node for a RAM address
			n = searchNode(ramBankNames, chr);
		}
		else
		{
			// Search address definition node for a ROM address
			n = addr >= 0xC000 ? searchNode(lastBankNames, chr) : searchNode(loadedBankNames, chr);
		}
		
		// If a node was found there's a name or comment to add do so
		if (n && (n->name || n->comment))
		{
			// Add name
			if (n->name && *n->name)
			{
				strcat(str, "Name: ");
				strcat(str, n->name);
				strcat(str,"\r\n");
			}
			
			// Add comment
			if (n->comment && *n->comment)
			{
				strcat(str, "Comment: ");
				strcat(str, n->comment);
				strcat(str, "\r\n");
			}
		}
	}
	
	// Add address
	strcat(str, chr);
	strcat(str, ":");
}

/**
* Returns the bookmark address of a CPU bookmark identified by its index.
* The caller must make sure that the index is valid.
*
* @param hwnd HWND of the debugger window
* @param index Index of the bookmark
**/
unsigned int getBookmarkAddress(HWND hwnd, unsigned int index)
{
	int n;
	char buffer[5] = {0};
	
	SendDlgItemMessage(hwnd, LIST_DEBUGGER_BOOKMARKS, LB_GETTEXT, index, (LPARAM)buffer);
	
	sscanf(buffer, "%x", &n);
	
	return n;
}

unsigned int bookmarks;
unsigned short* bookmarkData = 0;

/**
* Stores all CPU bookmarks in a simple array to be able to store
* them between debugging sessions.
*
* @param hwnd HWND of the debugger window.
**/
void dumpBookmarks(HWND hwnd)
{
	unsigned int i;
	if (bookmarkData)
		free(bookmarkData);
		
	bookmarks = SendDlgItemMessage(hwnd, LIST_DEBUGGER_BOOKMARKS, LB_GETCOUNT, 0, 0);
	bookmarkData = (unsigned short*)malloc(bookmarks * sizeof(unsigned short));
	
	for (i=0;i<bookmarks;i++)
	{
		bookmarkData[i] = getBookmarkAddress(hwnd, i);
	}
}

/**
* Adds a debugger bookmark to the list on the debugger window.
*
* @param hwnd HWMD of the debugger window
* @param buffer Text of the debugger bookmark
**/
void AddDebuggerBookmark2(HWND hwnd, char* buffer)
{
	if (!buffer)
	{
		MessageBox(0, "Error: Invalid parameter \"buffer\" in function AddDebuggerBookmark2", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	
	SendDlgItemMessage(hwnd, LIST_DEBUGGER_BOOKMARKS, LB_ADDSTRING, 0, (LPARAM)buffer);
	
	dumpBookmarks(hwnd);
}

/**
* Takes the offset from the debugger bookmark edit field and adds a debugger
* bookmark with that offset to the bookmark list if the offset is valid.
*
* @param hwnd HWMD of the debugger window
**/
void AddDebuggerBookmark(HWND hwnd)
{
	int result;
	unsigned int n;
	char buffer[5] = {0};
	
	GetDlgItemText(hwnd, IDC_DEBUGGER_BOOKMARK, buffer, 5);
		
	result = sscanf(buffer, "%x", &n);
	
	// Make sure the offset is valid
	if (result != 1 || n > 0xFFFF)
	{
		MessageBox(hwnd, "Invalid offset", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	
	AddDebuggerBookmark2(hwnd, buffer);
}

/**
* Removes a debugger bookmark
* 
* @param hwnd HWND of the debugger window
**/
void DeleteDebuggerBookmark(HWND hwnd)
{
	// Get the selected bookmark
	int selectedItem = SendDlgItemMessage(hwnd, LIST_DEBUGGER_BOOKMARKS, LB_GETCURSEL, 0, 0);
	
	if (selectedItem == LB_ERR)
	{
		MessageBox(hwnd, "Please select a bookmark from the list", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	else
	{
		// Remove the selected bookmark
		
		SendDlgItemMessage(hwnd, LIST_DEBUGGER_BOOKMARKS, LB_DELETESTRING, selectedItem, 0);
		dumpBookmarks(hwnd);
	}
}

void Disassemble(HWND hWnd, int id, int scrollid, unsigned int addr);

/**
* Shows the code at the bookmark address in the disassembly window
*
* @param hwnd HWND of the debugger window
**/
void GoToDebuggerBookmark(HWND hwnd)
{
	unsigned int n;
	int selectedItem = SendDlgItemMessage(hwnd, LIST_DEBUGGER_BOOKMARKS, LB_GETCURSEL, 0, 0);
	
	// If no bookmark is selected just return
	if (selectedItem == LB_ERR) return;
	
	n = getBookmarkAddress(hwnd, selectedItem);
	
	//Disassemble(hwnd, 300, 301, n);
	//MBG TODO
}
