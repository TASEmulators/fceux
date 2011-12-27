//Implementation file of Markers class
#include "taseditproj.h"
#include "zlib.h"
#include <Shlwapi.h>		// for StrStrI

extern bool TASEdit_empty_marker_notes;
extern HWND hwndTasEdit;

extern PLAYBACK playback;
extern TASEDIT_SELECTION selection;

// resources
char markers_save_id[MARKERS_ID_LEN] = "MARKERS";
char markers_skipsave_id[MARKERS_ID_LEN] = "MARKERX";
char keywordDelimiters[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

MARKERS::MARKERS()
{
}

void MARKERS::init()
{
	reset();
}
void MARKERS::free()
{
	markers_array.resize(0);
	notes.resize(0);
}
void MARKERS::reset()
{
	free();
	notes.resize(1);
	notes[0] = "Power on";
	update();
}
void MARKERS::update()
{
	if ((int)markers_array.size() < currMovieData.getNumRecords())
		markers_array.resize(currMovieData.getNumRecords());
}

void MARKERS::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		// write "MARKERS" string
		os->fwrite(markers_save_id, MARKERS_ID_LEN);
		// write size
		int size = markers_array.size();
		write32le(size, os);
		// compress and write array
		int len = markers_array.size() * sizeof(int);
		uLongf comprlen = (len>>9)+12 + len;
		std::vector<uint8> cbuf(comprlen);
		compress(&cbuf[0], &comprlen, (uint8*)&markers_array[0], len);
		write32le(comprlen, os);
		os->fwrite(&cbuf[0], comprlen);
		// write notes
		size = notes.size();
		write32le(size, os);
		for (int i = 0; i < size; ++i)
		{
			len = notes[i].length() + 1;
			if (len > MAX_NOTE_LEN) len = MAX_NOTE_LEN;
			write32le(len, os);
			os->fwrite(notes[i].c_str(), len);
		}
	} else
	{
		// write "MARKERX" string
		os->fwrite(markers_skipsave_id, MARKERS_ID_LEN);
	}
}
// returns true if couldn't load
bool MARKERS::load(EMUFILE *is)
{
	// read "MARKERS" string
	char save_id[MARKERS_ID_LEN];
	if ((int)is->fread(save_id, MARKERS_ID_LEN) < MARKERS_ID_LEN) goto error;
	if (!strcmp(markers_skipsave_id, save_id))
	{
		// string says to skip loading Markers
		FCEU_printf("No markers in the file\n");
		reset();
		return false;
	}
	if (strcmp(markers_save_id, save_id)) goto error;		// string is not valid
	int size;
	if (read32le(&size, is))
	{
		markers_array.resize(size);
		// read and uncompress array
		int comprlen, len;
		uLongf destlen = size * sizeof(int);
		if (!read32le(&comprlen, is)) goto error;
		if (comprlen <= 0) goto error;
		std::vector<uint8> cbuf(comprlen);
		if (is->fread(&cbuf[0], comprlen) != comprlen) goto error;
		int e = uncompress((uint8*)&markers_array[0], &destlen, &cbuf[0], comprlen);
		if (e != Z_OK && e != Z_BUF_ERROR) goto error;
		// read notes
		if (read32le(&size, is) && size >= 0)
		{
			notes.resize(size);
			char temp_str[MAX_NOTE_LEN];
			for (int i = 0; i < size; ++i)
			{
				if (!read32le(&len, is) || len < 0) goto error;
				if ((int)is->fread(temp_str, len) < len) goto error;
				notes[i] = temp_str;
			}
			// all ok
			return false;
		}
	}
error:
	FCEU_printf("Error loading markers\n");
	reset();
	return true;
}
bool MARKERS::skipLoad(EMUFILE *is)
{
	// read "MARKERS" string
	char save_id[MARKERS_ID_LEN];
	if ((int)is->fread(save_id, MARKERS_ID_LEN) < MARKERS_ID_LEN) goto error;
	if (!strcmp(markers_skipsave_id, save_id))
	{
		// string says to skip loading Markers
		reset();
		return false;
	}
	if (strcmp(markers_save_id, save_id)) goto error;		// string is not valid
	int size;
	if (!(is->fseek(sizeof(int), SEEK_CUR)))
	{
		// read array
		int comprlen, len;
		if (!read32le(&comprlen, is)) goto error;
		if (is->fseek(comprlen, SEEK_CUR) != 0) goto error;
		// read notes
		if (read32le(&size, is) && size >= 0)
		{
			for (int i = 0; i < size; ++i)
			{
				if (!read32le(&len, is) || len < 0) goto error;
				if (is->fseek(len, SEEK_CUR) != 0) goto error;
			}
			// all ok
			return false;
		}
	}
error:
	FCEU_printf("Error skiploading markers\n");
	return true;
}
// ----------------------------------------------------------
void MARKERS::MakeCopy(MARKERS& source)
{
	// provide references
	source.CopyMarkersHere(markers_array, notes);
}
void MARKERS::CopyMarkersHere(std::vector<int>& array_for_markers, std::vector<std::string>& for_notes)
{
	// copy data to provided arrays
	array_for_markers = markers_array;
	for_notes = notes;
}
void MARKERS::RestoreFromCopy(MARKERS& source, int until_frame)
{
	if (until_frame >= 0)
	{
		// restore markers up to and including the frame
		if ((int)markers_array.size()-1 <= until_frame)
		{
			// only copy head of source
			source.CopyMarkersHere(markers_array, notes);
			markers_array.resize(until_frame+1);
			// find last marker
			int last_marker = GetMarkerUp(until_frame);
			// delete all notes foolowing the note of the last marker
			notes.resize(last_marker+1);
		} else
		{
			// combine head of source and tail of destination (old markers)
			// 1 - head
			std::vector<int> temp_markers_array;
			std::vector<std::string> temp_notes;
			source.CopyMarkersHere(temp_markers_array, temp_notes);
			temp_markers_array.resize(until_frame+1);
			// find last marker in temp_markers_array
			int last_marker, frame;
			for (frame = until_frame; frame >= 0; frame--)
				if (temp_markers_array[frame]) break;
			if (frame >= 0)
				last_marker = temp_markers_array[frame];
			else
				last_marker = 0;
			// delete all temp_notes foolowing the note of the last marker
			temp_notes.resize(last_marker+1);
			// 2 - tail
			// delete all markers (and their notes) up to and including until_frame
			//for (int i = until_frame; i >= 0; i--)		// actually no need for that
			//	ClearMarker(i);
			// 3 - combine head and tail (if there are actually markers left in the tail)
			int size = markers_array.size();
			temp_markers_array.resize(size);
			for (int i = until_frame+1; i < size; ++i)
			{
				if (markers_array[i])
				{
					last_marker++;
					temp_markers_array[i] = last_marker;
					temp_notes.push_back(notes[markers_array[i]]);
				}
			}
			// 4 - save result
			markers_array = temp_markers_array;
			notes = temp_notes;
		}
	} else
	{
		// frame not specified, consider this as "copy all"
		MakeCopy(source);
	}
}
// ----------------------------------------------------------
int MARKERS::GetMarkersSize()
{
	return markers_array.size();
}
void MARKERS::SetMarkersSize(int new_size)
{
	// if we are truncating, clear markers that are gonna be erased (so that obsolete notes will be erased too)
	for (int i = markers_array.size() - 1; i >= new_size; i--)
		if (markers_array[i])
			ClearMarker(i);
	markers_array.resize(new_size);
}

int MARKERS::GetMarker(int frame)
{
	if (frame >= 0 && frame < (int)markers_array.size())
		return markers_array[frame];
	else
		return 0;
}
// finds and returns # of Marker starting from start_frame and searching up
int MARKERS::GetMarkerUp(int start_frame)
{
	if (start_frame >= (int)markers_array.size())
		start_frame = markers_array.size() - 1;
	for (; start_frame >= 0; start_frame--)
		if (markers_array[start_frame]) return markers_array[start_frame];
	return 0;
}
// finds frame where the Marker is set
int MARKERS::GetMarkerFrame(int marker_id)
{
	for (int i = markers_array.size() - 1; i >= 0; i--)
		if (markers_array[i] == marker_id) return i;
	// didn't find
	return -1;
}
// returns number of new marker
int MARKERS::SetMarker(int frame)
{
	if (frame < 0)
		return 0;
	else if (frame >= (int)markers_array.size())
		markers_array.resize(frame + 1);
	else if (markers_array[frame])
		return markers_array[frame];

	int marker_num = GetMarkerUp(frame) + 1;
	markers_array[frame] = marker_num;
	if (TASEdit_empty_marker_notes)
		notes.insert(notes.begin() + marker_num, 1, "");
	else
		// copy previous marker note
		notes.insert(notes.begin() + marker_num, 1, notes[marker_num - 1]);
	// increase following markers' ids
	int size = markers_array.size();
	for (frame++; frame < size; ++frame)
		if (markers_array[frame])
			markers_array[frame]++;
	return marker_num;
}
void MARKERS::ClearMarker(int frame)
{
	if (markers_array[frame])
	{
		// erase corresponding note
		notes.erase(notes.begin() + markers_array[frame]);
		// erase marker
		markers_array[frame] = 0;
		// decrease following markers' ids
		int size = markers_array.size();
		for (frame++; frame < size; ++frame)
			if (markers_array[frame])
				markers_array[frame]--;
	}
}
void MARKERS::ToggleMarker(int frame)
{
	if (frame >= 0 && frame < (int)markers_array.size())
	{
		if (markers_array[frame])
			ClearMarker(frame);
		else
			SetMarker(frame);
	}
}

void MARKERS::EraseMarker(int frame)
{
	// if there's a marker, first clear it
	if (markers_array[frame])
		ClearMarker(frame);
	markers_array.erase(markers_array.begin() + frame);
}
void MARKERS::insertEmpty(int at, int frames)
{
	if(at == -1) 
	{
		markers_array.resize(markers_array.size() + frames);
	} else
	{
		markers_array.insert(markers_array.begin() + at, frames, 0);
	}
}

int MARKERS::GetNotesSize()
{
	return notes.size();
}
std::string MARKERS::GetNote(int index)
{
	if (index >= 0 && index < (int)notes.size())
		return notes[index];
	else return notes[0];
}
void MARKERS::SetNote(int index, const char* new_text)
{
	if (index >= 0 && index < (int)notes.size())
		notes[index] = new_text;
}

// ----------------------------------------------------------
// return true if any difference in markers_array is found, comparing to markers.markers_array
bool MARKERS::checkMarkersDiff(MARKERS& their_markers)
{
	if (GetMarkersSize() != their_markers.GetMarkersSize()) return true;
	if (GetNotesSize() != their_markers.GetNotesSize()) return true;
	for (int i = markers_array.size()-1; i >= 0; i--)
	{
		if (markers_array[i] != their_markers.GetMarker(i))
			return true;
		else if (markers_array[i] && notes[markers_array[i]].compare(their_markers.GetNote(markers_array[i])))
			return true;
	}
	// also check if there's difference between 0th notes
	if (notes[0].compare(their_markers.GetNote(0)))
		return true;
	return false;
}
// return true only when difference is found before end frame (not including end frame)
bool MARKERS::checkMarkersDiff(MARKERS& their_markers, int end)
{
	if (GetMarkersSize() != their_markers.GetMarkersSize() && (GetMarkersSize()-1 < end || their_markers.GetMarkersSize()-1 < end)) return true;
	for (int i = end-1; i >= 0; i--)
	{
		if (markers_array[i] != their_markers.GetMarker(i))
			return true;
		else if (markers_array[i] && notes[markers_array[i]].compare(their_markers.GetNote(markers_array[i])))
			return true;
	}
	return false;
}
// ------------------------------------------------------------------------------------
bool ordering(const std::pair<int, double>& d1, const std::pair<int, double>& d2)
{
  return d1.second < d2.second;
}
void MARKERS::FindSimilar(int offset)
{
	int i, t;
	int sourceMarker = playback.shown_marker;
	char sourceNote[MAX_NOTE_LEN];
	strcpy(sourceNote, GetNote(sourceMarker).c_str());

	// check if playback_marker_text is empty
	if (!sourceNote[0])
	{
		MessageBox(hwndTasEdit, "Marker Note under Playback cursor is empty!", "Find Similar Note", MB_OK);
		return;
	}

	// 0 - divide source string into keywords
	int totalSourceKeywords = 0;
	char sourceKeywords[MAX_NUM_KEYWORDS][MAX_NOTE_LEN] = {0};
	int current_line_pos = 0;
	char sourceKeywordsLine[MAX_NUM_KEYWORDS] = {0};
	char* pch;
	// divide into tokens
	pch = strtok(sourceNote, keywordDelimiters);
	while (pch != NULL)
	{
		if (strlen(pch) >= KEYWORD_MIN_LEN)
		{
			// check if same keyword already appeared in the string
			for (t = totalSourceKeywords - 1; t >= 0; t--)
				if (!_stricmp(sourceKeywords[t], pch)) break;
			if (t < 0)
			{
				// save new keyword
				strcpy(sourceKeywords[totalSourceKeywords], pch);
				// also set its id into the line
				sourceKeywordsLine[current_line_pos++] = totalSourceKeywords + 1;
				totalSourceKeywords++;
			} else
			{
				// same keyword found
				sourceKeywordsLine[current_line_pos++] = t + 1;
			}
		}
		pch = strtok(NULL, keywordDelimiters);
	}
	// we found the line (sequence) of keywords
	sourceKeywordsLine[current_line_pos] = 0;
	
	if (!totalSourceKeywords)
	{
		MessageBox(hwndTasEdit, "Marker Note under Playback cursor doesn't have keywords!", "Find Similar Note", MB_OK);
		return;
	}

	// 1 - find how frequently each keyword appears in notes
	std::vector<int> keywordFound(totalSourceKeywords);
	char checkedNote[MAX_NOTE_LEN];
	for (i = notes.size() - 1; i > 0; i--)
	{
		if (i != sourceMarker)
		{
			strcpy(checkedNote, notes[i].c_str());
			for (t = totalSourceKeywords - 1; t >= 0; t--)
				if (StrStrI(checkedNote, sourceKeywords[t]))
					keywordFound[t]++;
		}
	}
	// findmax
	int maxFound = 0;
	for (t = totalSourceKeywords - 1; t >= 0; t--)
		if (maxFound < keywordFound[t])
			maxFound = keywordFound[t];
	// and then calculate weight of each keyword: the more often it appears in markers, the less weight it has
	std::vector<double> keywordWeight(totalSourceKeywords);
	for (t = totalSourceKeywords - 1; t >= 0; t--)
		keywordWeight[t] = KEYWORD_WEIGHT_BASE + KEYWORD_WEIGHT_FACTOR * (keywordFound[t] / (double)maxFound);

	// start accumulating priorities
	std::vector<std::pair<int, double>> notePriority(notes.size());

	// 2 - find keywords in notes (including cases when keyword appears inside another word)
	for (i = notePriority.size() - 1; i > 0; i--)
	{
		notePriority[i].first = i;
		if (i != sourceMarker)
		{
			strcpy(checkedNote, notes[i].c_str());
			for (t = totalSourceKeywords - 1; t >= 0; t--)
			{
				if (StrStrI(checkedNote, sourceKeywords[t]))
					notePriority[i].second += KEYWORD_CASEINSENTITIVE_BONUS_PER_CHAR * keywordWeight[t] * strlen(sourceKeywords[t]);
				if (strstr(checkedNote, sourceKeywords[t]))
					notePriority[i].second += KEYWORD_CASESENTITIVE_BONUS_PER_CHAR * keywordWeight[t] * strlen(sourceKeywords[t]);
			}
		}
	}

	// 3 - search sequences of keywords from all other notes
	current_line_pos = 0;
	char checkedKeywordsLine[MAX_NUM_KEYWORDS] = {0};
	int keyword_id;
	for (i = notes.size() - 1; i > 0; i--)
	{
		if (i != sourceMarker)
		{
			strcpy(checkedNote, notes[i].c_str());
			// divide into tokens
			pch = strtok(checkedNote, keywordDelimiters);
			while (pch != NULL)
			{
				if (strlen(pch) >= KEYWORD_MIN_LEN)
				{
					// check if the keyword is one of sourceKeywords
					for (t = totalSourceKeywords - 1; t >= 0; t--)
						if (!_stricmp(sourceKeywords[t], pch)) break;
					if (t >= 0)
					{
						// the keyword is one of sourceKeywords - set its id into the line
						checkedKeywordsLine[current_line_pos++] = t + 1;
					} else
					{
						// found keyword that doesn't appear in sourceNote, give penalty
						notePriority[i].second -= KEYWORD_PENALTY_FOR_STRANGERS * strlen(pch);
						// since the keyword breaks our sequence of coincident keywords, check if that sequence is similar to sourceKeywordsLine
						if (current_line_pos >= KEYWORDS_LINE_MIN_SEQUENCE)
						{
							checkedKeywordsLine[current_line_pos] = 0;
							// search checkedKeywordsLine in sourceKeywordsLine
							if (strstr(sourceKeywordsLine, checkedKeywordsLine))
							{
								// found same sequence of keywords! add priority to this checkedNote
								for (t = current_line_pos - 1; t >= 0; t--)
								{
									// add bonus for every keyword in the sequence
									keyword_id = checkedKeywordsLine[t] - 1;
									notePriority[i].second += current_line_pos * KEYWORD_SEQUENCE_BONUS_PER_CHAR * keywordWeight[keyword_id] * strlen(sourceKeywords[keyword_id]);
								}
							}
						}
						// clear checkedKeywordsLine
						memset(checkedKeywordsLine, 0, MAX_NUM_KEYWORDS);
						current_line_pos = 0;
					}
				}
				pch = strtok(NULL, keywordDelimiters);
			}
			// finished dividing into tokens
			if (current_line_pos >= KEYWORDS_LINE_MIN_SEQUENCE)
			{
				checkedKeywordsLine[current_line_pos] = 0;
				// search checkedKeywordsLine in sourceKeywordsLine
				if (strstr(sourceKeywordsLine, checkedKeywordsLine))
				{
					// found same sequence of keywords! add priority to this checkedNote
					for (t = current_line_pos - 1; t >= 0; t--)
					{
						// add bonus for every keyword in the sequence
						keyword_id = checkedKeywordsLine[t] - 1;
						notePriority[i].second += current_line_pos * KEYWORD_SEQUENCE_BONUS_PER_CHAR * keywordWeight[keyword_id] * strlen(sourceKeywords[keyword_id]);
					}
				}
			}
			// clear checkedKeywordsLine
			memset(checkedKeywordsLine, 0, MAX_NUM_KEYWORDS);
			current_line_pos = 0;
		}
	}

	// 4 - sort notePriority by second member of the pair
	std::sort(notePriority.begin(), notePriority.end(), ordering);

	/*
	// debug trace
	FCEU_printf("\n\n\n\n\n\n\n\n\n\n");
	for (t = totalSourceKeywords - 1; t >= 0; t--)
		FCEU_printf("Keyword: %s, %d, %f\n", sourceKeywords[t], keywordFound[t], keywordWeight[t]);
	for (i = notePriority.size() - 1; i > 0; i--)
	{
		int marker_id = notePriority[i].first;
		FCEU_printf("Result: %s, %d, %f\n", notes[marker_id].c_str(), marker_id, notePriority[i].second);
	}
	*/

	// Send selection to the marker found
	if (notePriority[notePriority.size()-1 - offset].second >= MIN_PRIORITY_TRESHOLD)
	{
		int marker_id = notePriority[notePriority.size()-1 - offset].first;
		int frame = GetMarkerFrame(marker_id);
		if (frame >= 0)
			selection.JumpToFrame(frame);
	} else
	{
		if (offset)
			MessageBox(hwndTasEdit, "Could not find more Notes similar to Marker Note under Playback cursor!", "Find Similar Note", MB_OK);
		else
			MessageBox(hwndTasEdit, "Could not find anything similar to Marker Note under Playback cursor!", "Find Similar Note", MB_OK);
	}
}

