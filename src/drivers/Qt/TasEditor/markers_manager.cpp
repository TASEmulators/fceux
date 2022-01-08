/* ---------------------------------------------------------------------------------
Implementation file of Markers_manager class
Copyright (c) 2011-2013 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Markers_manager - Manager of Markers
[Single instance]

* stores one snapshot of Markers, representing current state of Markers in the project
* saves and loads the data from a project file. On error: clears the data
* regularly ensures that the size of current Markers array is not less than the number of frames in current Input
* implements all operations with Markers: setting Marker to a frame, removing Marker, inserting/deleting frames between Markers, truncating Markers array, changing Notes, finding frame for any given Marker, access to the data of Snapshot of Markers state
* implements full/partial copying of data between two Snapshots of Markers state, and searching for first difference between two Snapshots of Markers state
* also here's the code of searching for "similar" Notes
* also here's the code of editing Marker Notes
* also here's the code of Find Note dialog 
* stores resources: save id, properties of searching for similar Notes
------------------------------------------------------------------------------------ */

#include "fceu.h"
#include "Qt/TasEditor/taseditor_project.h"
#include "Qt/TasEditor/TasEditorWindow.h"

// resources
static char markers_save_id[MARKERS_ID_LEN] = "MARKERS";
static char markers_skipsave_id[MARKERS_ID_LEN] = "MARKERX";
//static char keywordDelimiters[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

MARKERS_MANAGER::MARKERS_MANAGER()
{
	memset(findNoteString, 0, MAX_NOTE_LEN);
}

void MARKERS_MANAGER::init()
{
	reset();
}
void MARKERS_MANAGER::free()
{
	markers.markersArray.resize(0);
	markers.notes.resize(0);
}
void MARKERS_MANAGER::reset()
{
	free();
	markerNoteEditMode = MARKER_NOTE_EDIT_NONE;
	currentIterationOfFindSimilar = 0;
	markers.notes.resize(1);
	markers.notes[0] = "Power on";
	update();
}
void MARKERS_MANAGER::update()
{
	// the size of current markers_array must be no less then the size of Input
	if ((int)markers.markersArray.size() < currMovieData.getNumRecords())
		markers.markersArray.resize(currMovieData.getNumRecords());
}

void MARKERS_MANAGER::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		setTasProjectProgressBarText("Saving Markers...");
		setTasProjectProgressBar( 0, 100 );

		// write "MARKERS" string
		os->fwrite(markers_save_id, MARKERS_ID_LEN);
		markers.resetCompressedStatus();		// must recompress data, because most likely it has changed since last compression
		markers.save(os);
		setTasProjectProgressBar( 100, 100 );
	}
       	else
	{
		// write "MARKERX" string, meaning that Markers are not saved
		os->fwrite(markers_skipsave_id, MARKERS_ID_LEN);
	}
}
// returns true if couldn't load
bool MARKERS_MANAGER::load(EMUFILE *is, unsigned int offset)
{
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		return false;
	}
	// read "MARKERS" string
	char save_id[MARKERS_ID_LEN];
	if ((int)is->fread(save_id, MARKERS_ID_LEN) < MARKERS_ID_LEN) goto error;
	if (!strcmp(markers_skipsave_id, save_id))
	{
		// string says to skip loading Markers
		FCEU_printf("No Markers in the file\n");
		reset();
		return false;
	}
	if (strcmp(markers_save_id, save_id)) goto error;		// string is not valid
	if (markers.load(is)) goto error;
	// all ok
	return false;
error:
	FCEU_printf("Error loading Markers\n");
	reset();
	return true;
}
// -----------------------------------------------------------------------------------------
int MARKERS_MANAGER::getMarkersArraySize()
{
	return markers.markersArray.size();
}
bool MARKERS_MANAGER::setMarkersArraySize(int newSize)
{
	// if we are truncating, clear Markers that are gonna be erased (so that obsolete notes will be erased too)
	bool markers_changed = false;
	for (int i = markers.markersArray.size() - 1; i >= newSize; i--)
	{
		if (markers.markersArray[i])
		{
			removeMarkerFromFrame(i);
			markers_changed = true;
		}
	}
	markers.markersArray.resize(newSize);
	return markers_changed;
}

int MARKERS_MANAGER::getMarkerAtFrame(int frame)
{
	if (frame >= 0 && frame < (int)markers.markersArray.size())
		return markers.markersArray[frame];
	else
		return 0;
}
// finds and returns # of Marker starting from start_frame and searching up
int MARKERS_MANAGER::getMarkerAboveFrame(int startFrame)
{
	if (startFrame >= (int)markers.markersArray.size())
		startFrame = markers.markersArray.size() - 1;
	for (; startFrame >= 0; startFrame--)
		if (markers.markersArray[startFrame]) return markers.markersArray[startFrame];
	return 0;
}
// special version of the function
int MARKERS_MANAGER::getMarkerAboveFrame(MARKERS& targetMarkers, int startFrame)
{
	if (startFrame >= (int)targetMarkers.markersArray.size())
		startFrame = targetMarkers.markersArray.size() - 1;
	for (; startFrame >= 0; startFrame--)
		if (targetMarkers.markersArray[startFrame]) return targetMarkers.markersArray[startFrame];
	return 0;
}
// finds frame where the Marker is set
int MARKERS_MANAGER::getMarkerFrameNumber(int marker_id)
{
	for (int i = markers.markersArray.size() - 1; i >= 0; i--)
		if (markers.markersArray[i] == marker_id) return i;
	// didn't find
	return -1;
}
// returns number of new Marker
int MARKERS_MANAGER::setMarkerAtFrame(int frame)
{
	if (frame < 0)
		return 0;
	else if (frame >= (int)markers.markersArray.size())
		markers.markersArray.resize(frame + 1);
	else if (markers.markersArray[frame])
		return markers.markersArray[frame];

	int marker_num = getMarkerAboveFrame(frame) + 1;
	markers.markersArray[frame] = marker_num;
	if (taseditorConfig->emptyNewMarkerNotes)
		markers.notes.insert(markers.notes.begin() + marker_num, 1, "");
	else
		// copy previous Marker note
		markers.notes.insert(markers.notes.begin() + marker_num, 1, markers.notes[marker_num - 1]);
	// increase following Markers' ids
	int size = markers.markersArray.size();
	for (frame++; frame < size; ++frame)
		if (markers.markersArray[frame])
			markers.markersArray[frame]++;
	return marker_num;
}
void MARKERS_MANAGER::removeMarkerFromFrame(int frame)
{
	if (markers.markersArray[frame])
	{
		// erase corresponding note
		markers.notes.erase(markers.notes.begin() + markers.markersArray[frame]);
		// clear Marker
		markers.markersArray[frame] = 0;
		// decrease following Markers' ids
		int size = markers.markersArray.size();
		for (frame++; frame < size; ++frame)
			if (markers.markersArray[frame])
				markers.markersArray[frame]--;
	}
}
void MARKERS_MANAGER::toggleMarkerAtFrame(int frame)
{
	if (frame >= 0 && frame < (int)markers.markersArray.size())
	{
		if (markers.markersArray[frame])
			removeMarkerFromFrame(frame);
		else
			setMarkerAtFrame(frame);
	}
}

bool MARKERS_MANAGER::eraseMarker(int frame, int numFrames)
{
	bool markers_changed = false;
	if (frame < (int)markers.markersArray.size())
	{
		if (numFrames == 1)
		{
			// erase 1 frame
			// if there's a Marker, first clear it
			if (markers.markersArray[frame])
			{
				removeMarkerFromFrame(frame);
				markers_changed = true;
			}
			// erase 1 frame
			markers.markersArray.erase(markers.markersArray.begin() + frame);
		} else
		{
			// erase many frames
			if (frame + numFrames > (int)markers.markersArray.size())
				numFrames = (int)markers.markersArray.size() - frame;
			// if there are Markers at those frames, first clear them
			for (int i = frame; i < (frame + numFrames); ++i)
			{
				if (markers.markersArray[i])
				{
					removeMarkerFromFrame(i);
					markers_changed = true;
				}
			}
			// erase frames
			markers.markersArray.erase(markers.markersArray.begin() + frame, markers.markersArray.begin() + (frame + numFrames));
		}
		// check if there were some Markers after this frame
		// since these Markers were shifted, markers_changed should be set to true
		if (!markers_changed)
		{
			for (int i = markers.markersArray.size() - 1; i >= frame; i--)
			{
				if (markers.markersArray[i])
				{
					markers_changed = true;		// Markers moved
					break;
				}
			}
		}
	}
	return markers_changed;
}
bool MARKERS_MANAGER::insertEmpty(int at, int numFrames)
{
	if (at == -1) 
	{
		// append blank frames
		markers.markersArray.resize(markers.markersArray.size() + numFrames);
		return false;
	} else
	{
		bool markers_changed = false;
		// first check if there are Markers after the frame
		for (int i = markers.markersArray.size() - 1; i >= at; i--)
		{
			if (markers.markersArray[i])
			{
				markers_changed = true;		// Markers moved
				break;
			}
		}
		markers.markersArray.insert(markers.markersArray.begin() + at, numFrames, 0);
		return markers_changed;
	}
}

int MARKERS_MANAGER::getNotesSize()
{
	return markers.notes.size();
}
std::string MARKERS_MANAGER::getNoteCopy(int index)
{
	if (index >= 0 && index < (int)markers.notes.size())
		return markers.notes[index];
	else
		return markers.notes[0];
}
// special version of the function
std::string MARKERS_MANAGER::getNoteCopy(MARKERS& targetMarkers, int index)
{
	if (index >= 0 && index < (int)targetMarkers.notes.size())
		return targetMarkers.notes[index];
	else
		return targetMarkers.notes[0];
}
void MARKERS_MANAGER::setNote(int index, const char* newText)
{
	if (index >= 0 && index < (int)markers.notes.size())
		markers.notes[index] = newText;
}
// ---------------------------------------------------------------------------------------
void MARKERS_MANAGER::makeCopyOfCurrentMarkersTo(MARKERS& destination)
{
	destination.markersArray = markers.markersArray;
	destination.notes = markers.notes;
	destination.resetCompressedStatus();
}
void MARKERS_MANAGER::restoreMarkersFromCopy(MARKERS& source)
{
	markers.markersArray = source.markersArray;
	markers.notes = source.notes;
}

// return true only when difference is found before end frame (not including end frame)
bool MARKERS_MANAGER::checkMarkersDiff(MARKERS& theirMarkers)
{
	int end_my = getMarkersArraySize() - 1;
	int end_their = theirMarkers.markersArray.size() - 1;
	int min_end = end_my;
	int i;
	// 1 - check if there are any Markers after min_end
	if (end_my < end_their)
	{
		for (i = end_their; i > min_end; i--)
			if (theirMarkers.markersArray[i])
				return true;
	} else if (end_my > end_their)
	{
		min_end = end_their;
		for (i = end_my; i > min_end; i--)
			if (markers.markersArray[i])
				return true;
	}
	// 2 - check if there's any difference before min_end
	for (i = min_end; i >= 0; i--)
	{
		if (markers.markersArray[i] != theirMarkers.markersArray[i])
			return true;
		else if (markers.markersArray[i] &&	// not empty
			markers.notes[markers.markersArray[i]].compare(theirMarkers.notes[theirMarkers.markersArray[i]]))	// notes differ
			return true;
	}
	// 3 - check if there's difference between 0th Notes
	if (markers.notes[0].compare(theirMarkers.notes[0]))
		return true;
	// no difference found
	return false;
}
// ------------------------------------------------------------------------------------
// custom ordering function, used by std::sort
bool ordering(const std::pair<int, double>& d1, const std::pair<int, double>& d2)
{
  return d1.second < d2.second;
}

void MARKERS_MANAGER::findSimilarNote()
{
	currentIterationOfFindSimilar = 0;
	findNextSimilarNote();
}
void MARKERS_MANAGER::findNextSimilarNote()
{
	// This is implemented in TasEditorWindow.cpp
}
// ------------------------------------------------------------------------------------
void MARKERS_MANAGER::updateEditedMarkerNote()
{
	if (!markerNoteEditMode)
	{
		return;
	}
	int len=0;
	char new_text[MAX_NOTE_LEN];
	if (markerNoteEditMode == MARKER_NOTE_EDIT_UPPER)
	{
		len = tasWin->upperMarkerNote->text().size();
		if ( len >= MAX_NOTE_LEN )
		{
			len = MAX_NOTE_LEN-1;
		}
		strncpy( new_text, tasWin->upperMarkerNote->text().toStdString().c_str(), MAX_NOTE_LEN );
		new_text[len] = 0;
		// check changes
		if (strcmp(getNoteCopy(playback->displayedMarkerNumber).c_str(), new_text))
		{
			setNote(playback->displayedMarkerNumber, new_text);
			if (playback->displayedMarkerNumber)
			{
				history->registerMarkersChange(MODTYPE_MARKER_RENAME, getMarkerFrameNumber(playback->displayedMarkerNumber), -1, new_text);
			}
			else
			{
				// zeroth Marker - just assume it's set on frame 0
				history->registerMarkersChange(MODTYPE_MARKER_RENAME, 0, -1, new_text);
			}
			// notify Selection to change text in the lower Marker (in case both are showing same Marker)
			selection->mustFindCurrentMarker = true;
		}
	}
	else if (markerNoteEditMode == MARKER_NOTE_EDIT_LOWER)
	{
		len = tasWin->lowerMarkerNote->text().size();
		if ( len >= MAX_NOTE_LEN )
		{
			len = MAX_NOTE_LEN-1;
		}
		strncpy( new_text, tasWin->lowerMarkerNote->text().toStdString().c_str(), MAX_NOTE_LEN );
		new_text[len] = 0;

		// check changes
		if (strcmp(getNoteCopy(selection->displayedMarkerNumber).c_str(), new_text))
		{
			setNote(selection->displayedMarkerNumber, new_text);
			if (selection->displayedMarkerNumber)
			{
				history->registerMarkersChange(MODTYPE_MARKER_RENAME, getMarkerFrameNumber(selection->displayedMarkerNumber), -1, new_text);
			}
			else
			{
				// zeroth Marker - just assume it's set on frame 0
				history->registerMarkersChange(MODTYPE_MARKER_RENAME, 0, -1, new_text);
			}
			// notify Playback to change text in upper Marker (in case both are showing same Marker)
			playback->mustFindCurrentMarker = true;
		}
	}
}
// ------------------------------------------------------------------------------------
