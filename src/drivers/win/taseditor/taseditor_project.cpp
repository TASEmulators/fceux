/* ---------------------------------------------------------------------------------
Implementation file of TASEDITOR_PROJECT class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Project - Manager of working project
[Singleton]

* stores the info about current project filename and about having unsaved changes
* implements saving and loading project files from filesystem
* implements autosave function
* stores resources: autosave period scale, default filename, fm3 format offsets
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "utils/xstring.h"
#include "version.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern MARKERS_MANAGER markers_manager;
extern BOOKMARKS bookmarks;
extern POPUP_DISPLAY popup_display;
extern GREENZONE greenzone;
extern PLAYBACK playback;
extern RECORDER recorder;
extern HISTORY history;
extern PIANO_ROLL piano_roll;
extern SELECTION selection;
extern SPLICER splicer;

extern FCEUGI *GameInfo;

extern void FCEU_PrintError(char *format, ...);
extern bool SaveProject();
extern bool SaveProjectAs();
extern int GetInputType(MovieData& md);
extern void SetInputType(MovieData& md, int new_input_type);

TASEDITOR_PROJECT::TASEDITOR_PROJECT()
{
}

void TASEDITOR_PROJECT::init()
{
	// default filename for a new project is blank
	projectFile = "";
	projectName = "";
	fm2FileName = "";
	reset();
}
void TASEDITOR_PROJECT::reset()
{
	changed = false;
}
void TASEDITOR_PROJECT::update()
{
	// if it's time to autosave - pop Save As dialog
	if (changed && taseditor_window.TASEditor_focus && taseditor_config.autosave_period && !projectFile.empty() && clock() >= next_save_shedule && piano_roll.drag_mode == DRAG_MODE_NONE)
	{
		if (taseditor_config.silent_autosave)
			SaveProject();
		else
			SaveProjectAs();
		// in case user pressed Cancel, postpone saving to next time
		SheduleNextAutosave();
	}
}

bool TASEDITOR_PROJECT::save(const char* different_name, bool save_binary, bool save_markers, bool save_bookmarks, bool save_greenzone, bool save_history, bool save_piano_roll, bool save_selection)
{
	if (!different_name && GetProjectFile().empty())
		// no different name specified, and there's no current filename of the project
		return false;
	
	// check MD5
	char md5_movie[256];
	char md5_rom[256];
	strcpy(md5_movie, md5_asciistr(currMovieData.romChecksum));
	strcpy(md5_rom, md5_asciistr(GameInfo->MD5));
	if (strcmp(md5_movie, md5_rom))
	{
		// checksums mismatch, check if they both aren't zero
		unsigned int k, count1 = 0, count2 = 0;
		for(k = 0; k < strlen(md5_movie); k++) count1 += md5_movie[k] - '0';
		for(k = 0; k < strlen(md5_rom); k++) count2 += md5_rom[k] - '0';
		if (count1 && count2)
		{
			// ask user if he wants to fix the checksum before saving
			char message[2048] = {0};
			strcpy(message, "Movie ROM:\n");
			strncat(message, currMovieData.romFilename.c_str(), 2047 - strlen(message));
			strncat(message, "\nMD5: ", 2047 - strlen(message));
			strncat(message, md5_movie, 2047 - strlen(message));
			strncat(message, "\n\nCurrent ROM:\n", 2047 - strlen(message));
			strncat(message, GameInfo->filename, 2047 - strlen(message));
			strncat(message, "\nMD5: ", 2047 - strlen(message));
			strncat(message, md5_rom, 2047 - strlen(message));
			strncat(message, "\n\nFix the movie header before saving? ", 2047 - strlen(message));
			int answer = MessageBox(taseditor_window.hwndTasEditor, message, "ROM Checksum Mismatch", MB_YESNOCANCEL);
			if (answer == IDCANCEL)
			{
				// cancel saving
				return false;
			} else if (answer == IDYES)
			{
				// change ROM data in the movie to current ROM
				currMovieData.romFilename = GameInfo->filename;
				currMovieData.romChecksum = GameInfo->MD5;
			}
		}
	}
	// open file for write
	EMUFILE_FILE* ofs = 0;
	if (different_name)
		ofs = FCEUD_UTF8_fstream(different_name, "wb");
	else
		ofs = FCEUD_UTF8_fstream(GetProjectFile().c_str(), "wb");
	if (ofs)
	{
		// change cursor to hourglass
		SetCursor(LoadCursor(0, IDC_WAIT));
		// save fm2 data to the project file
		currMovieData.loadFrameCount = currMovieData.records.size();
		currMovieData.emuVersion = FCEU_VERSION_NUMERIC;
		currMovieData.dump(ofs, save_binary);
		unsigned int taseditor_data_offset = ofs->ftell();
		// save header: fm3 version + saved_stuff
		write32le(PROJECT_FILE_CURRENT_VERSION, ofs);
		unsigned int saved_stuff_map = 0;
		if (save_markers) saved_stuff_map |= MARKERS_SAVED;
		if (save_bookmarks) saved_stuff_map |= BOOKMARKS_SAVED;
		if (save_greenzone) saved_stuff_map |= GREENZONE_SAVED;
		if (save_history) saved_stuff_map |= HISTORY_SAVED;
		if (save_piano_roll) saved_stuff_map |= PIANO_ROLL_SAVED;
		if (save_selection) saved_stuff_map |= SELECTION_SAVED;
		write32le(saved_stuff_map, ofs);
		unsigned int number_of_pointers = DEFAULT_NUMBER_OF_POINTERS;
		write32le(number_of_pointers, ofs);
		// write dummy zeros to the file, where the offsets will be
		for (unsigned int i = 0; i < number_of_pointers; ++i)
			write32le(0, ofs);
		// save specified modules
		unsigned int markers_offset = ofs->ftell();
		markers_manager.save(ofs, save_markers);
		unsigned int bookmarks_offset = ofs->ftell();
		bookmarks.save(ofs, save_bookmarks);
		unsigned int greenzone_offset = ofs->ftell();
		greenzone.save(ofs, save_greenzone);
		unsigned int history_offset = ofs->ftell();
		history.save(ofs, save_history);
		unsigned int piano_roll_offset = ofs->ftell();
		piano_roll.save(ofs, save_piano_roll);
		unsigned int selection_offset = ofs->ftell();
		selection.save(ofs, save_selection);
		// now write offsets (pointers)
		ofs->fseek(taseditor_data_offset + PROJECT_FILE_OFFSET_OF_POINTERS_DATA, SEEK_SET);
		write32le(markers_offset, ofs);
		write32le(bookmarks_offset, ofs);
		write32le(greenzone_offset, ofs);
		write32le(history_offset, ofs);
		write32le(piano_roll_offset, ofs);
		write32le(selection_offset, ofs);
		// finish
		delete ofs;
		playback.UpdateProgressbar();
		// also set project.changed to false, unless it was SaveCompact
		if (!different_name)
			reset();
		// restore cursor
		taseditor_window.must_update_mouse_cursor = true;
		return true;
	} else
	{
		return false;
	}
}
bool TASEDITOR_PROJECT::load(const char* fullname)
{
	bool load_all = true;
	unsigned int taseditor_data_offset = 0;
	EMUFILE_FILE ifs(fullname, "rb");

	if (ifs.fail())
	{
		FCEU_PrintError("Error opening %s!", fullname);
		return false;
	}

	// change cursor to hourglass
	SetCursor(LoadCursor(0, IDC_WAIT));
	// load fm2 data from the project file
	MovieData tempMovieData = MovieData();
	extern bool LoadFM2(MovieData& movieData, EMUFILE* fp, int size, bool stopAfterHeader);
	if (LoadFM2(tempMovieData, &ifs, ifs.size(), false))
	{
		// check MD5
		char md5_original[256];
		char md5_current[256];
		strcpy(md5_original, md5_asciistr(tempMovieData.romChecksum));
		strcpy(md5_current, md5_asciistr(GameInfo->MD5));
		if (strcmp(md5_original, md5_current))
		{
			// checksums mismatch, check if they both aren't zero
			unsigned int k, count1 = 0, count2 = 0;
			for(k = 0; k < strlen(md5_original); k++) count1 += md5_original[k] - '0';
			for(k = 0; k < strlen(md5_current); k++) count2 += md5_current[k] - '0';
			if (count1 && count2)
			{
				// ask user if he really wants to load the project
				char message[2048] = {0};
				strcpy(message, "This project was made using different ROM!\n\n");
				strcat(message, "Original ROM:\n");
				strncat(message, tempMovieData.romFilename.c_str(), 2047 - strlen(message));
				strncat(message, "\nMD5: ", 2047 - strlen(message));
				strncat(message, md5_original, 2047 - strlen(message));
				strncat(message, "\n\nCurrent ROM:\n", 2047 - strlen(message));
				strncat(message, GameInfo->filename, 2047 - strlen(message));
				strncat(message, "\nMD5: ", 2047 - strlen(message));
				strncat(message, md5_current, 2047 - strlen(message));
				strncat(message, "\n\nLoad the project anyway?", 2047 - strlen(message));
				int answer = MessageBox(taseditor_window.hwndTasEditor, message, "ROM Checksum Mismatch", MB_YESNO);
				if (answer == IDNO)
					return false;
			}
		}
		taseditor_data_offset = ifs.ftell();
		// load fm3 version from header and check it
		unsigned int file_version;
		if (read32le(&file_version, &ifs))
		{
			if (file_version != PROJECT_FILE_CURRENT_VERSION)
			{
				char message[2048] = {0};
				strcpy(message, "This project was saved using different version of TAS Editor!\n\n");
				strcat(message, "Original version: ");
				char version_num[11];
				_itoa(file_version, version_num, 10);
				strncat(message, version_num, 2047 - strlen(message));
				strncat(message, "\nCurrent version: ", 2047 - strlen(message));
				_itoa(PROJECT_FILE_CURRENT_VERSION, version_num, 10);
				strncat(message, version_num, 2047 - strlen(message));
				strncat(message, "\n\nClick Yes to try loading all data from the file (may crash).\n", 2047 - strlen(message));
				strncat(message, "Click No to only load movie data.\n", 2047 - strlen(message));
				strncat(message, "Click Cancel to abort loading.", 2047 - strlen(message));
				int answer = MessageBox(taseditor_window.hwndTasEditor, message, "FM3 Version Mismatch", MB_YESNOCANCEL);
				if (answer == IDCANCEL)
					return false;
				else if (answer == IDNO)
					load_all = false;
			}
		} else
		{
			// couldn't even load header, this seems like an FM2
			load_all = false;
			char message[2048];
			strcpy(message, "This file doesn't seem to be an FM3 project.\nIt only contains FM2 movie data. Load it anyway?");
			int answer = MessageBox(taseditor_window.hwndTasEditor, message, "Opening FM2 file", MB_YESNO);
			if (answer == IDNO)
				return false;
		}
		// save data to currMovieData and continue loading
		FCEU_printf("\nLoading TAS Editor project %s...\n", fullname);
		currMovieData = tempMovieData;
		LoadSubtitles(currMovieData);
		// ensure that movie has correct set of ports/fourscore
		SetInputType(currMovieData, GetInputType(currMovieData));
	} else
	{
		FCEU_PrintError("Error loading movie data from %s!", fullname);
		// do not alter the project
		return false;
	}

	unsigned int saved_stuff = 0;
	unsigned int number_of_pointers = 0;
	unsigned int data_offset = 0;
	unsigned int pointer_offset = taseditor_data_offset + PROJECT_FILE_OFFSET_OF_POINTERS_DATA;
	if (load_all)
	{
		read32le(&saved_stuff, &ifs);
		read32le(&number_of_pointers, &ifs);
		// load modules
		if (number_of_pointers-- && !(ifs.fseek(pointer_offset, SEEK_SET)) && read32le(&data_offset, &ifs))
			pointer_offset += sizeof(unsigned int);
		else
			data_offset = 0;
		markers_manager.load(&ifs, data_offset);

		if (number_of_pointers-- && !(ifs.fseek(pointer_offset, SEEK_SET)) && read32le(&data_offset, &ifs))
			pointer_offset += sizeof(unsigned int);
		else
			data_offset = 0;
		bookmarks.load(&ifs, data_offset);

		if (number_of_pointers-- && !(ifs.fseek(pointer_offset, SEEK_SET)) && read32le(&data_offset, &ifs))
			pointer_offset += sizeof(unsigned int);
		else
			data_offset = 0;
		greenzone.load(&ifs, data_offset);

		if (number_of_pointers-- && !(ifs.fseek(pointer_offset, SEEK_SET)) && read32le(&data_offset, &ifs))
			pointer_offset += sizeof(unsigned int);
		else
			data_offset = 0;
		history.load(&ifs, data_offset);

		if (number_of_pointers-- && !(ifs.fseek(pointer_offset, SEEK_SET)) && read32le(&data_offset, &ifs))
			pointer_offset += sizeof(unsigned int);
		else
			data_offset = 0;
		piano_roll.load(&ifs, data_offset);

		if (number_of_pointers-- && !(ifs.fseek(pointer_offset, SEEK_SET)) && read32le(&data_offset, &ifs))
			pointer_offset += sizeof(unsigned int);
		else
			data_offset = 0;
		selection.load(&ifs, data_offset);
	} else
	{
		// reset modules
		markers_manager.load(&ifs, 0);
		bookmarks.load(&ifs, 0);
		greenzone.load(&ifs, 0);
		history.load(&ifs, 0);
		piano_roll.load(&ifs, 0);
		selection.load(&ifs, 0);
	}
	// reset other modules
	playback.reset();
	recorder.reset();
	splicer.reset();
	popup_display.reset();
	reset();
	RenameProject(fullname, load_all);
	// restore mouse cursor shape
	taseditor_window.must_update_mouse_cursor = true;
	return true;
}

void TASEDITOR_PROJECT::RenameProject(const char* new_fullname, bool filename_is_correct)
{
	projectFile = new_fullname;
	char drv[512], dir[512], name[512], ext[512];		// For getting the filename
	splitpath(new_fullname, drv, dir, name, ext);
	projectName = name;
	std::string thisfm2name = name;
	thisfm2name.append(".fm2");
	fm2FileName = thisfm2name;
	// if filename is not correct (for example, user opened a corrupted FM3) clear the filename, so on Ctrl+S user will be forwarded to SaveAs
	if (!filename_is_correct)
		projectFile.clear();
}
// -----------------------------------------------------------------
std::string TASEDITOR_PROJECT::GetProjectFile()
{
	return projectFile;
}
std::string TASEDITOR_PROJECT::GetProjectName()
{
	return projectName;
}
std::string TASEDITOR_PROJECT::GetFM2Name()
{
	return fm2FileName;
}

void TASEDITOR_PROJECT::SetProjectChanged()
{
	if (!changed)
	{
		changed = true;
		taseditor_window.UpdateCaption();
		SheduleNextAutosave();
	}
}
bool TASEDITOR_PROJECT::GetProjectChanged()
{
	return changed;
}

void TASEDITOR_PROJECT::SheduleNextAutosave()
{
	if (taseditor_config.autosave_period)
		next_save_shedule = clock() + taseditor_config.autosave_period * AUTOSAVE_PERIOD_SCALE;
}

