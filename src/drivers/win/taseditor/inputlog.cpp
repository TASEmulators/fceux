/* ---------------------------------------------------------------------------------
Implementation file of InputLog class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
InputLog - Log of Input

* stores the data about Input state: size, type of Input, Input Log data (commands and joysticks)
* optionally can store map of Hot Changes
* implements InputLog creation: copying Input, copying Hot Changes
* implements full/partial restoring of data from InputLog: Input, Hot Changes
* implements compression and decompression of stored data
* saves and loads the data from a project file. On error: sends warning to caller
* implements searching of first mismatch comparing two InputLogs or comparing this InputLog to a movie
* provides interface for reading specific data: reading Input of any given frame, reading value at any point of Hot Changes map
* implements all operations with Hot Changes maps: copying (full/partial), updating/fading, setting new hot places by comparing two InputLogs
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "zlib.h"

extern SELECTION selection;
extern int GetInputType(MovieData& md);

int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES] = {1, 2, 4};

INPUTLOG::INPUTLOG()
{
}

void INPUTLOG::init(MovieData& md, bool hotchanges, int force_input_type)
{
	has_hot_changes = hotchanges;
	if (force_input_type < 0)
		input_type = GetInputType(md);
	else
		input_type = force_input_type;
	int num_joys = joysticks_per_frame[input_type];
	// retrieve Input data from movie data
	size = md.getNumRecords();
	joysticks.resize(BYTES_PER_JOYSTICK * num_joys * size);		// it's much faster to have this format than have [frame][joy] or other structures
	commands.resize(size);				// commands take 1 byte per frame
	if (has_hot_changes)
		Init_HotChanges();

	// fill Input vector
	int joy;
	for (int frame = 0; frame < size; ++frame)
	{
		for (joy = num_joys - 1; joy >= 0; joy--)
			joysticks[frame * num_joys * BYTES_PER_JOYSTICK + joy * BYTES_PER_JOYSTICK] = md.records[frame].joysticks[joy];
		commands[frame] = md.records[frame].commands;
	}
	already_compressed = false;
}

void INPUTLOG::toMovie(MovieData& md, int start, int end)
{
	if (end < 0 || end >= size) end = size - 1;
	// write Input data to movie data
	md.records.resize(end + 1);
	int num_joys = joysticks_per_frame[input_type];
	int joy;
	for (int frame = start; frame <= end; ++frame)
	{
		for (joy = num_joys - 1; joy >= 0; joy--)
			md.records[frame].joysticks[joy] = joysticks[frame * num_joys * BYTES_PER_JOYSTICK + joy * BYTES_PER_JOYSTICK];
		md.records[frame].commands = commands[frame];
	}
}

void INPUTLOG::compress_data()
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
bool INPUTLOG::Get_already_compressed()
{
	return already_compressed;
}

void INPUTLOG::save(EMUFILE *os)
{
	// write vars
	write32le(size, os);
	write8le(input_type, os);
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
		write8le((uint8)1, os);
		// save hot_changes data
		write32le(hot_changes_compressed.size(), os);
		os->fwrite(&hot_changes_compressed[0], hot_changes_compressed.size());
	} else
	{
		write8le((uint8)0, os);
	}
}
// returns true if couldn't load
bool INPUTLOG::load(EMUFILE *is)
{
	uint8 tmp;
	// read vars
	if (!read32le(&size, is)) return true;
	if (!read8le(&tmp, is)) return true;
	input_type = tmp;
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
	// read hotchanges
	if (!read8le(&tmp, is)) return true;
	has_hot_changes = (tmp != 0);
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
	return false;
}
bool INPUTLOG::skipLoad(EMUFILE *is)
{
	int tmp;
	uint8 tmp1;
	// skip vars
	if (is->fseek(sizeof(int) +	// size
				sizeof(uint8)	// input_type
				, SEEK_CUR)) return true;
	// skip joysticks data
	if (!read32le(&tmp, is)) return true;
	if (is->fseek(tmp, SEEK_CUR) != 0) return true;
	// skip commands data
	if (!read32le(&tmp, is)) return true;
	if (is->fseek(tmp, SEEK_CUR) != 0) return true;
	// skip hot_changes data
	if (!read8le(&tmp1, is)) return true;
	if (tmp1)
	{
		if (!read32le(&tmp, is)) return true;
		if (is->fseek(tmp, SEEK_CUR) != 0) return true;
	}
	return false;
}
// --------------------------------------------------------------------------------------------
// fills map of bits judging on which joypads differ (this function is only used by "Record" modtype)
uint32 INPUTLOG::fillJoypadsDiff(INPUTLOG& their_log, int frame)
{
	uint32 joypad_diff_bits = 0;
	uint32 current_mask = 1;
	if (frame < their_log.size)
	{
		for (int joy = 0; joy < joysticks_per_frame[input_type]; ++joy)
		{
			if (GetJoystickInfo(frame, joy) != their_log.GetJoystickInfo(frame, joy))
				joypad_diff_bits |= current_mask;
			current_mask <<= 1;
		}
	}
	return joypad_diff_bits;
}
// return number of first frame of difference between two InputLogs
int INPUTLOG::findFirstChange(INPUTLOG& their_log, int start, int end)
{
	// search for differences to the specified end (or to the end of this InputLog)
	if (end < 0 || end >= size) end = size-1;
	int their_log_end = their_log.size;

	int joy;
	int num_joys = joysticks_per_frame[input_type];
	for (int frame = start; frame <= end; ++frame)
	{
		for (joy = num_joys - 1; joy >= 0; joy--)
			if (GetJoystickInfo(frame, joy) != their_log.GetJoystickInfo(frame, joy)) return frame;
		if (GetCommandsInfo(frame) != their_log.GetCommandsInfo(frame)) return frame;
	}
	// no difference was found

	// if my_size is less then their_size, return last frame + 1 (= size) as the frame of difference
	if (size < their_log_end) return size;
	// no changes were found
	return -1;
}
// return number of first frame of difference between this InputLog and MovieData
int INPUTLOG::findFirstChange(MovieData& md, int start, int end)
{
	// search for differences to the specified end (or to the end of this InputLog / to the end of the movie data)
	if (end < 0 || end >= size) end = size - 1;
	if (end >= md.getNumRecords()) end = md.getNumRecords() - 1;

	int joy;
	int num_joys = joysticks_per_frame[input_type];
	for (int frame = start; frame <= end; ++frame)
	{
		for (joy = num_joys - 1; joy >= 0; joy--)
			if (GetJoystickInfo(frame, joy) != md.records[frame].joysticks[joy]) return frame;
		if (GetCommandsInfo(frame) != md.records[frame].commands) return frame;
	}
	// no difference was found

	// if sizes differ, return last frame + 1 from the lesser of them
	if (size < md.getNumRecords() && end >= size - 1)
		return size;
	else if (size > md.getNumRecords() && end >= md.getNumRecords() - 1)
		return md.getNumRecords();

	return -1;
}

int INPUTLOG::GetJoystickInfo(int frame, int joy)
{
	if (frame < 0 || frame >= size)
		return 0;
	if (joy > joysticks_per_frame[input_type])
		return 0;
	return joysticks[frame * BYTES_PER_JOYSTICK * joysticks_per_frame[input_type] + joy];
}
int INPUTLOG::GetCommandsInfo(int frame)
{
	if (frame < 0 || frame >= size)
		return 0;
	return commands[frame];
}

void INPUTLOG::insertFrames(int at, int frames)
{
	size += frames;
	if (at == -1) 
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
				hot_changes[i] = BYTE_VALUE_CONTAINING_MAX_HOTCHANGES;
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
			hot_changes.insert(hot_changes.begin() + (at * bytes), frames * bytes, BYTE_VALUE_CONTAINING_MAX_HOTCHANGES);
		}
	}
	// data was changed
	already_compressed = false;
}
void INPUTLOG::eraseFrame(int frame)
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
// -----------------------------------------------------------------------------------------------
void INPUTLOG::Init_HotChanges()
{
	hot_changes.resize(joysticks_per_frame[input_type] * size * HOTCHANGE_BYTES_PER_JOY);
}

void INPUTLOG::copyHotChanges(INPUTLOG* source_of_hotchanges, int limit_frame_of_source)
{
	// copy hot changes from source InputLog
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int frames_to_copy = source_of_hotchanges->size;
		if (frames_to_copy > size)
			frames_to_copy = size;
		// special case for Branches: if limit_frame if specified, then copy only hotchanges from 0 to limit_frame
		if (limit_frame_of_source >= 0 && frames_to_copy > limit_frame_of_source)
			frames_to_copy = limit_frame_of_source;

		int bytes_to_copy = frames_to_copy * joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		memcpy(&hot_changes[0], &source_of_hotchanges->hot_changes[0], bytes_to_copy);
	}
} 
void INPUTLOG::inheritHotChanges(INPUTLOG* source_of_hotchanges)
{
	// copy hot changes from source InputLog and fade them
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int frames_to_copy = source_of_hotchanges->size;
		if (frames_to_copy > size)
			frames_to_copy = size;

		int bytes_to_copy = frames_to_copy * joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		memcpy(&hot_changes[0], &source_of_hotchanges->hot_changes[0], bytes_to_copy);
		FadeHotChanges();
	}
} 
void INPUTLOG::inheritHotChanges_DeleteSelection(INPUTLOG* source_of_hotchanges, SelectionFrames* frameset)
{
	// copy hot changes from source InputLog, but omit deleted frames (which are represented by the "frameset")
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, pos = 0, source_pos = 0;
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		SelectionFrames::iterator it(frameset->begin());
		SelectionFrames::iterator frameset_end(frameset->end());
		while (pos < this_size && source_pos < source_size)
		{
			if (it != frameset_end && frame == *it)
			{
				// omit the frame
				it++;
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
void INPUTLOG::inheritHotChanges_InsertSelection(INPUTLOG* source_of_hotchanges, SelectionFrames* frameset)
{
	// copy hot changes from source InputLog, but insert filled lines for inserted frames (which are represented by the "frameset")
	SelectionFrames::iterator it(frameset->begin());
	SelectionFrames::iterator frameset_end(frameset->end());
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, region_len = 0, pos = 0, source_pos = 0;
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		while (pos < this_size)
		{
			if (it != frameset_end && frame == *it)
			{
				// omit the frame
				it++;
				region_len++;
				// set filled line to the frame
				memset(&hot_changes[pos], BYTE_VALUE_CONTAINING_MAX_HOTCHANGES, bytes);
			} else if (source_pos < source_size)
			{
				// this frame should be copied
				frame -= region_len;
				region_len = 0;
				// copy hotchanges of this frame
				memcpy(&hot_changes[pos], &source_of_hotchanges->hot_changes[source_pos], bytes);
				FadeHotChanges(pos, pos + bytes);
				source_pos += bytes;
			}
			pos += bytes;
			frame++;
		}
	} else
	{
		// no old data, just fill "frameset" lines
		int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
		int frame = 0, region_len = 0, pos = 0;
		int this_size = hot_changes.size();
		while (pos < this_size)
		{
			if (it != frameset_end && frame == *it)
			{
				// this frame is selected
				it++;
				region_len++;
				// set filled line to the frame
				memset(&hot_changes[pos], BYTE_VALUE_CONTAINING_MAX_HOTCHANGES, bytes);
				// exit loop when all frames in the Selection are handled
				if (it == frameset_end) break;
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
void INPUTLOG::inheritHotChanges_DeleteNum(INPUTLOG* source_of_hotchanges, int start, int frames, bool fade_old)
{
	int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
	// copy hot changes from source InputLog up to "start" and from "start+frames" to end
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		int bytes_to_copy = bytes * start;
		int dest_pos = 0, source_pos = 0;
		if (bytes_to_copy > source_size)
			bytes_to_copy = source_size;
		memcpy(&hot_changes[dest_pos], &source_of_hotchanges->hot_changes[source_pos], bytes_to_copy);
		dest_pos += bytes_to_copy;
		source_pos += bytes_to_copy + bytes * frames;
		bytes_to_copy = this_size - dest_pos;
		if (bytes_to_copy > source_size - source_pos)
			bytes_to_copy = source_size - source_pos;
		memcpy(&hot_changes[dest_pos], &source_of_hotchanges->hot_changes[source_pos], bytes_to_copy);
		if (fade_old)
			FadeHotChanges();
	}
} 
void INPUTLOG::inheritHotChanges_InsertNum(INPUTLOG* source_of_hotchanges, int start, int frames, bool fade_old)
{
	int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
	// copy hot changes from source InputLog up to "start", then make a gap, then copy from "start+frames" to end
	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int this_size = hot_changes.size(), source_size = source_of_hotchanges->hot_changes.size();
		int bytes_to_copy = bytes * start;
		int dest_pos = 0, source_pos = 0;
		if (bytes_to_copy > source_size)
			bytes_to_copy = source_size;
		memcpy(&hot_changes[dest_pos], &source_of_hotchanges->hot_changes[source_pos], bytes_to_copy);
		dest_pos += bytes_to_copy + bytes * frames;
		source_pos += bytes_to_copy;
		bytes_to_copy = this_size - dest_pos;
		if (bytes_to_copy > source_size - source_pos)
			bytes_to_copy = source_size - source_pos;
		memcpy(&hot_changes[dest_pos], &source_of_hotchanges->hot_changes[source_pos], bytes_to_copy);
		if (fade_old)
			FadeHotChanges();
	}
	// fill the gap with max_hot lines on frames from "start" to "start+frames"
	memset(&hot_changes[bytes * start], BYTE_VALUE_CONTAINING_MAX_HOTCHANGES, bytes * frames);
}
void INPUTLOG::inheritHotChanges_PasteInsert(INPUTLOG* source_of_hotchanges, SelectionFrames* inserted_set)
{
	// copy hot changes from source InputLog and insert filled lines for inserted frames (which are represented by "inserted_set")
	int bytes = joysticks_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
	int frame = 0, pos = 0;
	int this_size = hot_changes.size();
	SelectionFrames::iterator it(inserted_set->begin());
	SelectionFrames::iterator inserted_set_end(inserted_set->end());

	if (source_of_hotchanges && source_of_hotchanges->has_hot_changes && source_of_hotchanges->input_type == input_type)
	{
		int source_pos = 0;
		int source_size = source_of_hotchanges->hot_changes.size();
		while (pos < this_size)
		{
			if (it != inserted_set_end && frame == *it)
			{
				// this frame was inserted
				it++;
				// set filled line to the frame
				memset(&hot_changes[pos], BYTE_VALUE_CONTAINING_MAX_HOTCHANGES, bytes);
			} else if (source_pos < source_size)
			{
				// copy hotchanges of this frame
				memcpy(&hot_changes[pos], &source_of_hotchanges->hot_changes[source_pos], bytes);
				FadeHotChanges(pos, pos + bytes);
				source_pos += bytes;
			}
			pos += bytes;
			frame++;
		}
	} else
	{
		// no old data, just fill selected lines
		while (pos < this_size)
		{
			if (it != inserted_set_end && frame == *it)
			{
				// this frame was inserted
				it++;
				// set filled line to the frame
				memset(&hot_changes[pos], BYTE_VALUE_CONTAINING_MAX_HOTCHANGES, bytes);
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
void INPUTLOG::fillHotChanges(INPUTLOG& their_log, int start, int end)
{
	// compare InputLogs to the specified end (or to the end of this InputLog)
	if (end < 0 || end >= size) end = size-1;
	uint8 my_joy, their_joy;
	for (int joy = joysticks_per_frame[input_type] - 1; joy >= 0; joy--)
	{
		for (int frame = start; frame <= end; ++frame)
		{
			my_joy = GetJoystickInfo(frame, joy);
			their_joy = their_log.GetJoystickInfo(frame, joy);
			if (my_joy != their_joy)
				SetMaxHotChange_Bits(frame, joy, my_joy ^ their_joy);
		}						
	}
}

void INPUTLOG::SetMaxHotChange_Bits(int frame, int joypad, uint8 joy_bits)
{
	uint8 mask = 1;
	// check all 8 buttons and set max hot_changes for bits that are set
	for (int i = 0; i < BUTTONS_PER_JOYSTICK; ++i)
	{
		if (joy_bits & mask)
			SetMaxHotChange(frame, joypad * BUTTONS_PER_JOYSTICK + i);
		mask <<= 1;
	}
}
void INPUTLOG::SetMaxHotChange(int frame, int absolute_button)
{
	if (frame < 0 || frame >= size || !has_hot_changes) return;
	// set max value to the button hotness
	if (absolute_button & 1)
		hot_changes[frame * (HOTCHANGE_BYTES_PER_JOY * joysticks_per_frame[input_type]) + (absolute_button >> 1)] |= BYTE_VALUE_CONTAINING_MAX_HOTCHANGE_HI;
	else
		hot_changes[frame * (HOTCHANGE_BYTES_PER_JOY * joysticks_per_frame[input_type]) + (absolute_button >> 1)] |= BYTE_VALUE_CONTAINING_MAX_HOTCHANGE_LO;
}

void INPUTLOG::FadeHotChanges(int start_byte, int end_byte)
{
	uint8 hi_half, low_half;
	if (end_byte < 0)
		end_byte = hot_changes.size();
	for (int i = end_byte - 1; i >= start_byte; i--)
	{
		if (hot_changes[i])
		{
			hi_half = hot_changes[i] >> HOTCHANGE_BITS_PER_VALUE;
			low_half = hot_changes[i] & HOTCHANGE_BITMASK;
			if (hi_half) hi_half--;
			if (low_half) low_half--;
			hot_changes[i] = (hi_half << HOTCHANGE_BITS_PER_VALUE) | low_half;
		}
	}
}

int INPUTLOG::GetHotChangeInfo(int frame, int absolute_button)
{
	if (!has_hot_changes || frame < 0 || frame >= size || absolute_button < 0 || absolute_button >= NUM_JOYPAD_BUTTONS * joysticks_per_frame[input_type])
		return 0;

	uint8 val = hot_changes[frame * (HOTCHANGE_BYTES_PER_JOY * joysticks_per_frame[input_type]) + (absolute_button >> 1)];

	if (absolute_button & 1)
		// odd buttons (B, T, D, R) take upper 4 bits of the byte 
		return val >> HOTCHANGE_BITS_PER_VALUE;
	else
		// even buttons (A, S, U, L) take lower 4 bits of the byte 
		return val & HOTCHANGE_BITMASK;
}

