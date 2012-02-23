// ---------------------------------------------------------------------------------
// Implementation file of Snapshot class
// (C) 2011-2012 AnS
// ---------------------------------------------------------------------------------
/*
Snapshot - Snapshot of all edited data
* stores the data of specific snapshot of the movie: size, input data (commands and joysticks), Markers at the moment of creating the snapshot, keyframe, type and description of the snapshot (including the time of creation)
* also stores info about sequential recording of input
* optionally can store map of Hot Changes
* implements snapshot creation: copying input, copying Hot Changes, copying Markers, setting time of creation
* implements full/partial restoring of data from snapshot: input, Hot Changes, Markers
* implements compression and decompression of stored data
* saves and loads the data from a project file. On error: sends warning to caller
* implements searching of first mismatch comparing two snapshots ot comparing snapshot to a movie
* provides interface for reading certain data: reading input of any certain frame, reading value at any point of Hot Changes map
* implements all operations with Hot Changes maps: copying (full/partial), updating/fading, setting new hot places by comparing input of two snapshots
*/
// ---------------------------------------------------------------------------------

#include "taseditor_project.h"
#include "zlib.h"

int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES] = {1, 2, 4};

extern MARKERS_MANAGER markers_manager;
extern SELECTION selection;

extern int GetInputType(MovieData& md);

SNAPSHOT::SNAPSHOT()
{
}

void SNAPSHOT::init(MovieData& md, bool hotchanges, int force_input_type)
{
	has_hot_changes = hotchanges;
	if (force_input_type < 0)
		input_type = GetInputType(md);
	else
		input_type = force_input_type;
	// retrieve input data from movie data
	size = md.getNumRecords();
	joysticks.resize(BYTES_PER_JOYSTICK * joysticks_per_frame[input_type] * size);		// it's much faster to have this format than have [frame][joy] or other structures
	commands.resize(size);				// commands take 1 byte per frame
	if (has_hot_changes)
		hot_changes.resize(joysticks_per_frame[input_type] * size * HOTCHANGE_BYTES_PER_JOY);

	// fill input vector
	int pos = 0;
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			for (int frame = 0; frame < size; ++frame)
			{
				joysticks[pos++] = md.records[frame].joysticks[0];
				joysticks[pos++] = md.records[frame].joysticks[1];
				joysticks[pos++] = md.records[frame].joysticks[2];
				joysticks[pos++] = md.records[frame].joysticks[3];
				commands[frame] = md.records[frame].commands;
			}
			break;
		}
		case INPUT_TYPE_2P:
		{
			for (int frame = 0; frame < size; ++frame)
			{
				joysticks[pos++] = md.records[frame].joysticks[0];
				joysticks[pos++] = md.records[frame].joysticks[1];
				commands[frame] = md.records[frame].commands;
			}
			break;
		}
		case INPUT_TYPE_1P:
		{
			for (int frame = 0; frame < size; ++frame)
			{
				joysticks[pos++] = md.records[frame].joysticks[0];
				commands[frame] = md.records[frame].commands;
			}
			break;
		}
	}

	// make a copy of markers_manager.markers
	markers_manager.MakeCopyTo(my_markers);
	if ((int)my_markers.markers_array.size() < size)
		my_markers.markers_array.resize(size);

	already_compressed = false;
	// save current time to description
	time_t raw_time;
	time(&raw_time);
	struct tm * timeinfo = localtime(&raw_time);
	strftime(description, 10, "%H:%M:%S", timeinfo);
}

bool SNAPSHOT::MarkersDifferFromCurrent(int end)
{
	return markers_manager.checkMarkersDiff(my_markers, end);
}
void SNAPSHOT::copyToMarkers(int end)
{
	markers_manager.RestoreFromCopy(my_markers, end);
}
MARKERS& SNAPSHOT::GetMarkers()
{
	return my_markers;
}

void SNAPSHOT::toMovie(MovieData& md, int start, int end)
{
	if (end < 0) end = size-1;
	// write input data to movie data
	md.records.resize(size);
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			int pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
			for (int frame = start; frame <= end; ++frame)
			{
				md.records[frame].joysticks[0] = joysticks[pos++];
				md.records[frame].joysticks[1] = joysticks[pos++];
				md.records[frame].joysticks[2] = joysticks[pos++];
				md.records[frame].joysticks[3] = joysticks[pos++];
				md.records[frame].commands = commands[frame];
			}
			break;
		}
		case INPUT_TYPE_2P:
		{
			int pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
			for (int frame = start; frame <= end; ++frame)
			{
				md.records[frame].joysticks[0] = joysticks[pos++];
				md.records[frame].joysticks[1] = joysticks[pos++];
				md.records[frame].commands = commands[frame];
			}
			break;
		}
		case INPUT_TYPE_1P:
		{
			int pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
			for (int frame = start; frame <= end; ++frame)
			{
				md.records[frame].joysticks[0] = joysticks[pos++];
				md.records[frame].commands = commands[frame];
			}
			break;
		}
	}
}

void SNAPSHOT::compress_data()
{
	// compress joysticks
	int len = joysticks.size();
	uLongf comprlen = (len>>9)+12 + len;
	joysticks_compressed.resize(comprlen);
	compress(&joysticks_compressed[0], &comprlen, &joysticks[0], len);
	joysticks_compressed.resize(comprlen);
	// compress commands
	len = commands.size();
	comprlen = (len>>9)+12 + len;
	commands_compressed.resize(comprlen);
	compress(&commands_compressed[0], &comprlen, &commands[0], len);
	commands_compressed.resize(comprlen);
	if (has_hot_changes)
	{
		// compress hot_changes
		len = hot_changes.size();
		comprlen = (len>>9)+12 + len;
		hot_changes_compressed.resize(comprlen);
		compress(&hot_changes_compressed[0], &comprlen, &hot_changes[0], len);
		hot_changes_compressed.resize(comprlen);
	}
	// don't recompress anymore
	already_compressed = true;
}

void SNAPSHOT::save(EMUFILE *os)
{
	// write vars
	write32le(size, os);
	write8le(input_type, os);
	write32le(jump_frame, os);
	write32le(rec_end_frame, os);
	write32le(rec_joypad_diff_bits, os);
	write32le(mod_type, os);
	if (has_hot_changes) write8le((uint8)1, os); else write8le((uint8)0, os);
	// write description
	int len = strlen(description);
	write8le(len, os);
	os->fwrite(&description[0], len);
	// write data
	if (!already_compressed)
		compress_data();
	// save joysticks data
	write32le(joysticks_compressed.size(), os);
	os->fwrite(&joysticks_compressed[0], joysticks_compressed.size());
	// save commands data
	write32le(commands_compressed.size(), os);
	os->fwrite(&commands_compressed[0], commands_compressed.size());
	if (has_hot_changes)
	{
		// save hot_changes data
		write32le(hot_changes_compressed.size(), os);
		os->fwrite(&hot_changes_compressed[0], hot_changes_compressed.size());
	}
	// save markers data
	my_markers.save(os);
}
// returns true if couldn't load
bool SNAPSHOT::load(EMUFILE *is)
{
	uint8 tmp;
	// read vars
	if (!read32le(&size, is)) return true;
	if (!read8le(&tmp, is)) return true;
	input_type = tmp;
	if (!read32le(&jump_frame, is)) return true;
	if (!read32le(&rec_end_frame, is)) return true;
	if (!read32le(&rec_joypad_diff_bits, is)) return true;
	if (!read32le(&mod_type, is)) return true;
	if (!read8le(&tmp, is)) return true;
	has_hot_changes = (tmp != 0);
	// read description
	if (!read8le(&tmp, is)) return true;
	if (tmp >= SNAPSHOT_DESC_MAX_LENGTH) return true;
	if (is->fread(&description[0], tmp) != tmp) return true;
	description[tmp] = 0;		// add '0' because it wasn't saved in the file
	// read data
	already_compressed = true;
	int comprlen;
	uLongf destlen;
	// read and uncompress joysticks data
	destlen = size * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
	joysticks.resize(destlen);
	// read size
	if (!read32le(&comprlen, is)) return true;
	if (comprlen <= 0) return true;
	joysticks_compressed.resize(comprlen);
	if (is->fread(&joysticks_compressed[0], comprlen) != comprlen) return true;
	int e = uncompress(&joysticks[0], &destlen, &joysticks_compressed[0], comprlen);
	if (e != Z_OK && e != Z_BUF_ERROR) return true;
	// read and uncompress commands data
	destlen = size;
	commands.resize(destlen);
	// read size
	if (!read32le(&comprlen, is)) return true;
	if (comprlen <= 0) return true;
	commands_compressed.resize(comprlen);
	if (is->fread(&commands_compressed[0], comprlen) != comprlen) return true;
	e = uncompress(&commands[0], &destlen, &commands_compressed[0], comprlen);
	if (e != Z_OK && e != Z_BUF_ERROR) return true;
	if (has_hot_changes)
	{
		// read and uncompress hot_changes data
		destlen = size * joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		hot_changes.resize(destlen);
		// read size
		if (!read32le(&comprlen, is)) return true;
		if (comprlen <= 0) return true;
		hot_changes_compressed.resize(comprlen);
		if (is->fread(&hot_changes_compressed[0], comprlen) != comprlen) return true;
		e = uncompress(&hot_changes[0], &destlen, &hot_changes_compressed[0], comprlen);
		if (e != Z_OK && e != Z_BUF_ERROR) return true;
	}
	// load markers data
	if (my_markers.load(is)) return true;
	return false;
}
bool SNAPSHOT::skipLoad(EMUFILE *is)
{
	int tmp;
	uint8 tmp1;
	// read vars
	if (is->fseek(sizeof(int) + // size
				sizeof(uint8) + // input_type
				sizeof(int) +	// jump_frame
				sizeof(int) +	// rec_end_frame
				sizeof(int) +	// rec_joypad_diff_bits
				sizeof(int) +	// mod_type
				sizeof(uint8)	// has_hot_changes
				, SEEK_CUR)) return true;
	// read description
	if (!read8le(&tmp1, is)) return true;
	if (tmp1 >= SNAPSHOT_DESC_MAX_LENGTH) return true;
	if (is->fseek(tmp1, SEEK_CUR) != 0) return true;
	// read joysticks data
	if (!read32le(&tmp, is)) return true;
	if (is->fseek(tmp, SEEK_CUR) != 0) return true;
	// read commands data
	if (!read32le(&tmp, is)) return true;
	if (is->fseek(tmp, SEEK_CUR) != 0) return true;
	if (has_hot_changes)
	{
		// read hot_changes data
		if (!read32le(&tmp, is)) return true;
		if (is->fseek(tmp, SEEK_CUR) != 0) return true;
	}
	// read markers data
	if (my_markers.skipLoad(is)) return true;
	return false;
}

// return true if any difference is found
bool SNAPSHOT::checkDiff(SNAPSHOT& inp)
{
	if (size != inp.size) return true;
	if (findFirstChange(inp) >= 0)
		return true;
	else
		return false;
}

// fills map of bits judging on which joypads differ (this function is only used by "Record" modtype)
void SNAPSHOT::fillJoypadsDiff(SNAPSHOT& inp, int frame)
{
	rec_joypad_diff_bits = 0;
	uint32 current_mask = 1;
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		case INPUT_TYPE_2P:
		case INPUT_TYPE_1P:
		{
			int pos = frame * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
			for (int i = 0; i < BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; ++i)
			{
				if (pos < (inp.size * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]))
				{
					if (joysticks[pos+i] != inp.joysticks[pos+i]) rec_joypad_diff_bits |= current_mask;
				} else
				{
					if (joysticks[pos+i]) rec_joypad_diff_bits |= current_mask;
				}
				current_mask <<= 1;
			}
			break;
		}
	}
}

// return number of first frame of difference between two snapshots
int SNAPSHOT::findFirstChange(SNAPSHOT& inp, int start, int end)
{
	// if these two snapshots have different input_type (abnormal situation) then refuse to search and return the beginning
	if (inp.input_type != input_type)
		return start;

	// search for differences to the specified end (or to the end of this snapshot)
	if (end < 0 || end >= size) end = size-1;
	int inp_end = inp.size;
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				// return the frame if found different byte, or found emptiness in inp when there's non-zero value here
				if (frame < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (commands[frame] != inp.commands[frame]) return frame;
				} else
				{
					if (joysticks[pos++]) return frame;
					if (joysticks[pos++]) return frame;
					if (joysticks[pos++]) return frame;
					if (joysticks[pos++]) return frame;
					if (commands[frame]) return frame;
				}
			}
			break;
		}
		case INPUT_TYPE_2P:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				// return the frame if found different byte, or found emptiness in inp when there's non-zero value here
				if (frame < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (commands[frame] != inp.commands[frame]) return frame;
				} else
				{
					if (joysticks[pos++]) return frame;
					if (joysticks[pos++]) return frame;
					if (commands[frame]) return frame;
				}
			}
			break;
		}
		case INPUT_TYPE_1P:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				// return the frame if found different byte, or found emptiness in inp when there's non-zero value here
				if (frame < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos]) return frame;
					pos++;
					if (commands[frame] != inp.commands[frame]) return frame;
				} else
				{
					if (joysticks[pos++]) return frame;
					if (commands[frame]) return frame;
				}
			}
			break;
		}
	}
	// if current size is less then previous, return size-1 as the frame of difference
	if (size < inp_end) return size-1;
	// no changes were found
	return -1;
}
// return number of first frame of difference between this snapshot and MovieData
int SNAPSHOT::findFirstChange(MovieData& md, int start, int end)
{
	// search for differences to the specified end (or to the end of this snapshot / to the end of the movie)
	if (end < 0 || end >= size) end = size-1;
	if (end >= md.getNumRecords()) end = md.getNumRecords()-1;

	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[1]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[2]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[3]) return frame;
				if (commands[frame] != md.records[frame].commands) return frame;
			}
			break;
		}
		case INPUT_TYPE_2P:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[1]) return frame;
				if (commands[frame] != md.records[frame].commands) return frame;
			}
			break;
		}
		case INPUT_TYPE_1P:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
				if (commands[frame] != md.records[frame].commands) return frame;
			}
			break;
		}
	}
	// if sizes differ, return last frame from the lesser of them
	if (size != md.getNumRecords()) return end;

	return -1;	// no changes were found
}

int SNAPSHOT::GetJoystickInfo(int frame, int joy)
{
	if (frame < 0 || frame >= size) return 0;
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		case INPUT_TYPE_2P:
		case INPUT_TYPE_1P:
		{
			return joysticks[frame * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type] + joy];
		}
	}
	return 0;
}
int SNAPSHOT::GetCommandsInfo(int frame)
{
	if (frame < 0 || frame >= size) return 0;
	return commands[frame];
}

void SNAPSHOT::insertFrames(int at, int frames)
{
	size += frames;
	if(at == -1) 
	{
		// append frames to the end
		commands.resize(size);
		joysticks.resize(BYTES_PER_JOYSTICK * joysticks_per_frame[input_type] * size);
		if (has_hot_changes)
		{
			hot_changes.resize(joysticks_per_frame[input_type] * size * HOTCHANGE_BYTES_PER_JOY);
			// fill new hotchanges with max value
			int lower_limit = joysticks_per_frame[input_type] * (size - frames) * HOTCHANGE_BYTES_PER_JOY;
			for (int i = hot_changes.size() - 1; i >= lower_limit; i--)
				hot_changes[i] = 0xFF;
		}
	} else
	{
		// insert frames
		// insert 1 byte of commands
		commands.insert(commands.begin() + at, frames, 0);
		// insert X bytes of joystics
		int bytes = BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
		joysticks.insert(joysticks.begin() + (at * bytes), frames * bytes, 0);
		if (has_hot_changes)
		{
			// insert X bytes of hot_changes
			bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
			hot_changes.insert(hot_changes.begin() + (at * bytes), frames * bytes, 0xFF);
		}
	}
	// data was changed
	already_compressed = false;
}
void SNAPSHOT::eraseFrame(int frame)
{
	// erase 1 byte of commands
	commands.erase(commands.begin() + frame);
	// erase X bytes of joystics
	int bytes = BYTES_PER_JOYSTICK * joysticks_per_frame[input_type];
	joysticks.erase(joysticks.begin() + (frame * bytes), joysticks.begin() + ((frame + 1) * bytes));
	if (has_hot_changes)
	{
		// erase X bytes of hot_changes
		bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		hot_changes.erase(hot_changes.begin() + (frame * bytes), hot_changes.begin() + ((frame + 1) * bytes));
	}
	size--;
	// data was changed
	already_compressed = false;
}
// --------------------------------------------------------
void SNAPSHOT::copyHotChanges(SNAPSHOT* source_of_hotchanges, int limit_frame_of_source)
{
	// copy hot changes from source snapshot
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int min = hot_changes.size();
		if (min > (int)source_of_hotchanges->hot_changes.size())
			min = source_of_hotchanges->hot_changes.size();

		// special case for Branches: if limit_frame if specified, then copy only hotchanges from 0 to limit_frame
		if (limit_frame_of_source >= 0)
		{
			if (min > limit_frame_of_source * joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY)
				min = limit_frame_of_source * joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		}

		memcpy(&hot_changes[0], &source_of_hotchanges->hot_changes[0], min);
	}
} 
void SNAPSHOT::inheritHotChanges(SNAPSHOT* source_of_hotchanges)
{
	// copy hot changes from source snapshot and fade them
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int min = hot_changes.size();
		if (min > (int)source_of_hotchanges->hot_changes.size())
			min = source_of_hotchanges->hot_changes.size();

		memcpy(&hot_changes[0], &source_of_hotchanges->hot_changes[0], min);
		FadeHotChanges();
	}
} 
void SNAPSHOT::inheritHotChanges_DeleteSelection(SNAPSHOT* source_of_hotchanges)
{
	// copy hot changes from source snapshot, but omit deleted frames (which are represented by current selection)
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, pos = 0, source_pos = 0;
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		SelectionFrames::iterator it(selection.GetStrobedSelection().begin());
		while (pos < this_size && source_pos < source_size)
		{
			if (it != selection.GetStrobedSelection().end() && frame == *it)
			{
				// this frame is selected
				it++;
				// omit the frame
				source_pos += bytes;
			} else
			{
				// copy hotchanges of this frame
				memcpy(&hot_changes[pos], &source_of_hotchanges->hot_changes[source_pos], bytes);
				pos += bytes;
				source_pos += bytes;
			}
			frame++;
		}
		FadeHotChanges();
	}
} 
void SNAPSHOT::inheritHotChanges_InsertSelection(SNAPSHOT* source_of_hotchanges)
{
	// copy hot changes from source snapshot, but insert filled lines for inserted frames (which are represented by current selection)
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, region_len = 0, pos = 0, source_pos = 0;
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		SelectionFrames::iterator it(selection.GetStrobedSelection().begin());
		SelectionFrames::iterator current_selection_end(selection.GetStrobedSelection().end());
		while (pos < this_size)
		{
			if (it != current_selection_end && frame == *it)
			{
				// this frame is selected
				it++;
				region_len++;
				// set filled line to the frame
				memset(&hot_changes[pos], 0xFF, bytes);
			} else if (source_pos < source_size)
			{
				// this frame is not selected
				frame -= region_len;
				region_len = 0;
				// copy hotchanges of this frame
				memcpy(&hot_changes[pos], &source_of_hotchanges->hot_changes[source_pos], bytes);
				source_pos += bytes;
			}
			pos += bytes;
			frame++;
		}
		FadeHotChanges();
	} else
	{
		// no old data, just fill selected lines
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, region_len = 0, pos = 0;
		int this_size = hot_changes.size();
		SelectionFrames::iterator it(selection.GetStrobedSelection().begin());
		SelectionFrames::iterator current_selection_end(selection.GetStrobedSelection().end());
		while (pos < this_size)
		{
			if (it != current_selection_end && frame == *it)
			{
				// this frame is selected
				it++;
				region_len++;
				// set filled line to the frame
				memset(&hot_changes[pos], 0xFF, bytes);
				// exit loop when all selection frames are handled
				if (it == current_selection_end) break;
			} else
			{
				// this frame is not selected
				frame -= region_len;
				region_len = 0;
				// leave zeros in this frame
			}
			pos += bytes;
			frame++;
		}
	}
} 
void SNAPSHOT::inheritHotChanges_PasteInsert(SNAPSHOT* source_of_hotchanges, SelectionFrames& inserted_set)
{
	// copy hot changes from source snapshot and insert filled lines for inserted frames (which are represented by inserted_set)
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, pos = 0, source_pos = 0;
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		SelectionFrames::iterator it(inserted_set.begin());
		SelectionFrames::iterator inserted_set_end(inserted_set.end());
		while (pos < this_size)
		{
			if (it != inserted_set_end && frame == *it)
			{
				// this frame was inserted
				it++;
				// set filled line to the frame
				memset(&hot_changes[pos], 0xFF, bytes);
			} else if (source_pos < source_size)
			{
				// copy hotchanges of this frame
				memcpy(&hot_changes[pos], &source_of_hotchanges->hot_changes[source_pos], bytes);
				source_pos += bytes;
			}
			pos += bytes;
			frame++;
		}
		FadeHotChanges();
	} else
	{
		// no old data, just fill selected lines
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, pos = 0;
		int this_size = hot_changes.size();
		SelectionFrames::iterator it(inserted_set.begin());
		SelectionFrames::iterator inserted_set_end(inserted_set.end());
		while (pos < this_size)
		{
			if (it != inserted_set_end && frame == *it)
			{
				// this frame was inserted
				it++;
				// set filled line to the frame
				memset(&hot_changes[pos], 0xFF, bytes);
				pos += bytes;
				// exit loop when all inserted_set frames are handled
				if (it == inserted_set_end) break;
			} else
			{
				// leave zeros in this frame
				pos += bytes;
			}
			frame++;
		}
	}
} 
void SNAPSHOT::fillHotChanges(SNAPSHOT& inp, int start, int end)
{
	// if these two snapshots have different input_type (abnormal situation) then refuse to compare
	if (inp.input_type != input_type)
		return;

	// compare snapshots to the specified end (or to the end of this snapshot)
	if (end < 0 || end >= size) end = size-1;
	int inp_end = inp.size;
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				// set changed if found different byte, or found emptiness in inp when there's non-zero value here
				if (frame < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 0, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 1, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 2, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 3, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
				} else
				{
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 0, joysticks[pos]);
					pos++;
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 1, joysticks[pos]);
					pos++;
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 2, joysticks[pos]);
					pos++;
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 3, joysticks[pos]);
					pos++;
				}
			}
			break;
		}
		case INPUT_TYPE_2P:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				// set changed if found different byte, or found emptiness in inp when there's non-zero value here
				if (frame < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 0, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 1, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
				} else
				{
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 0, joysticks[pos]);
					pos++;
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 1, joysticks[pos]);
					pos++;
				}
			}
			break;
		}
		case INPUT_TYPE_1P:
		{
			for (int frame = start, pos = start * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type]; frame <= end; ++frame)
			{
				// set changed if found different byte, or found emptiness in inp when there's non-zero value here
				if (frame < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos])
						SetMaxHotChange_Bits(frame, 0, joysticks[pos] ^ inp.joysticks[pos]);
					pos++;
				} else
				{
					if (joysticks[pos])
						SetMaxHotChange_Bits(frame, 0, joysticks[pos]);
					pos++;
				}
			}
			break;
		}
	}
}

void SNAPSHOT::SetMaxHotChange_Bits(int frame, int joypad, uint8 joy_bits)
{
	uint8 mask = 1;
	// check all 8 buttons and set max hot_changes for bits that are set
	for (int i = 0; i < 8; ++i)
	{
		if (joy_bits & mask)
			SetMaxHotChange(frame, joypad * 8 + i);
		mask <<= 1;
	}
}
void SNAPSHOT::SetMaxHotChange(int frame, int absolute_button)
{
	if (frame < 0 || frame >= size || !has_hot_changes) return;
	// set max value (15) to the button hotness
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			// 32 buttons = 16bytes
			if (absolute_button & 1)
				// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
				hot_changes[(frame << 4) | (absolute_button >> 1)] |= 0xF0;
			else
				// even buttons (A, S, U, L) - set lower 4 bits of the byte 
				hot_changes[(frame << 4) | (absolute_button >> 1)] |= 0x0F;
			break;
		}
		case INPUT_TYPE_2P:
		{
			// 16 buttons = 8bytes
			if (absolute_button & 1)
				// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
				hot_changes[(frame << 3) | (absolute_button >> 1)] |= 0xF0;
			else
				// even buttons (A, S, U, L) - set lower 4 bits of the byte 
				hot_changes[(frame << 3) | (absolute_button >> 1)] |= 0x0F;
			break;
		}
		case INPUT_TYPE_1P:
		{
			// 8 buttons = 4bytes
			if (absolute_button & 1)
				// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
				hot_changes[(frame << 2) | (absolute_button >> 1)] |= 0xF0;
			else
				// even buttons (A, S, U, L) - set lower 4 bits of the byte 
				hot_changes[(frame << 2) | (absolute_button >> 1)] |= 0x0F;
			break;
		}
	}
}

void SNAPSHOT::FadeHotChanges()
{
	uint8 hi_half, low_half;
	for (int i = hot_changes.size() - 1; i >= 0; i--)
	{
		if (hot_changes[i])
		{
			hi_half = hot_changes[i] >> 4;
			low_half = hot_changes[i] & 15;
			if (hi_half) hi_half--;
			if (low_half) low_half--;
			hot_changes[i] = (hi_half << 4) | low_half;
		}
	}
}

int SNAPSHOT::GetHotChangeInfo(int frame, int absolute_button)
{
	if (!has_hot_changes || frame < 0 || frame >= size || absolute_button < 0 || absolute_button >= NUM_JOYPAD_BUTTONS * joysticks_per_frame[input_type])
		return 0;

	uint8 val;
	switch(input_type)
	{
		case INPUT_TYPE_FOURSCORE:
		{
			// 32 buttons, 16bytes
			val = hot_changes[(frame << 4) + (absolute_button >> 1)];
			break;
		}
		case INPUT_TYPE_2P:
		{
			// 16 buttons, 8bytes
			val = hot_changes[(frame << 3) + (absolute_button >> 1)];
			break;
		}
		case INPUT_TYPE_1P:
		{
			// 8 buttons, 4bytes
			val = hot_changes[(frame << 2) + (absolute_button >> 1)];
			break;
		}
	}

	if (absolute_button & 1)
		// odd buttons (B, T, D, R) - upper 4 bits of the byte 
		return val >> 4;
	else
		// even buttons (A, S, U, L) - lower 4 bits of the byte 
		return val & 15;
}

