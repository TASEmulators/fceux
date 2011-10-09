//Implementation file of Input History and Input Snapshot classes (for Undo feature)

#include "movie.h"
#include "inputhistory.h"
#include "zlib.h"

extern void FCEU_printf(char *format, ...);
extern void RedrawHistoryList();
extern void UpdateHistoryList();
extern void UpdateProgressbar(int a, int b);

char modCaptions[23][12] = {"Init",
							"Change",
							"Set",
							"Unset",
							"Insert",
							"Delete",
							"Truncate",
							"Clear",
							"Cut",
							"Paste",
							"PasteInsert",
							"Clone",
							"Record",
							"Branch0",
							"Branch1",
							"Branch2",
							"Branch3",
							"Branch4",
							"Branch5",
							"Branch6",
							"Branch7",
							"Branch8",
							"Branch9"};
char joypadCaptions[4][5] = {"(1P)", "(2P)", "(3P)", "(4P)"};

INPUT_SNAPSHOT::INPUT_SNAPSHOT()
{

}

void INPUT_SNAPSHOT::init(MovieData& md)
{
	// retrieve input data from movie data
	size = md.getNumRecords();
	fourscore = md.fourscore;
	int num = (fourscore)?4:2;
	joysticks.resize(num*size);		// it's much faster to have this format [joy + frame << JOY_POWER] than have [frame][joy]
	hot_changes.resize(num*size * HOTCHANGE_BYTES_PER_JOY);
	int pos = 0;
	if (fourscore)
	{
		for (int frame = 0; frame < size; ++frame)
		{
			joysticks[pos++] = md.records[frame].joysticks[0];
			joysticks[pos++] = md.records[frame].joysticks[1];
			joysticks[pos++] = md.records[frame].joysticks[2];
			joysticks[pos++] = md.records[frame].joysticks[3];
		}
	} else
	{
		for (int frame = 0; frame < size; ++frame)
		{
			joysticks[pos++] = md.records[frame].joysticks[0];
			joysticks[pos++] = md.records[frame].joysticks[1];
		}
	}
	coherent = true;
	// save time to description
	time_t raw_time;
	time(&raw_time);
	struct tm * timeinfo = localtime(&raw_time);
	strftime(description, 10, "%H:%M:%S ", timeinfo);
}

void INPUT_SNAPSHOT::toMovie(MovieData& md, int start)
{
	// write input data to movie data
	md.records.resize(size);
	md.frames_flags.resize(size);
	if (fourscore)
	{
		int pos = start << 2;
		for (int frame = start; frame < size; ++frame)
		{
			md.records[frame].joysticks[0] = joysticks[pos++];
			md.records[frame].joysticks[1] = joysticks[pos++];
			md.records[frame].joysticks[2] = joysticks[pos++];
			md.records[frame].joysticks[3] = joysticks[pos++];
		}
	} else
	{
		int pos = start << 1;
		for (int frame = start; frame < size; ++frame)
		{
			md.records[frame].joysticks[0] = joysticks[pos++];
			md.records[frame].joysticks[1] = joysticks[pos++];
		}
	}
}

void INPUT_SNAPSHOT::save(EMUFILE *os)
{
	// write vars
	write32le(size, os);
	if (fourscore) write8le(4, os); else write8le((uint8)0, os);
	if (coherent) write8le(1, os); else write8le((uint8)0, os);
	write32le(jump_frame, os);
	// write description
	int len = strlen(description);
	write8le(len, os);
	os->fwrite(&description[0], len);
	// compress and save joysticks data
	len = joysticks.size();
	int comprlen = (len>>9)+12 + len;
	std::vector<uint8> cbuf(comprlen);
	int e = compress(cbuf.data(), (uLongf*)&comprlen,(uint8*)joysticks.data(),len);
	// write size
	write32le(comprlen, os);
	os->fwrite(cbuf.data(), comprlen);
	// compress and save hot_changes data
	len = hot_changes.size();
	comprlen = (len>>9)+12 + len;
	std::vector<uint8> cbuf2(comprlen);
	e = compress(cbuf2.data(),(uLongf*)&comprlen,(uint8*)hot_changes.data(),len);
	// write size
	write32le(comprlen, os);
	os->fwrite(cbuf2.data(), comprlen);
}
bool INPUT_SNAPSHOT::load(EMUFILE *is)
{
	int len;
	uint8 tmp;
	// read vars
	if (!read32le(&size, is)) return false;
	if (!read8le(&tmp, is)) return false;
	fourscore = (tmp != 0);
	if (!read8le(&tmp, is)) return false;
	coherent = (tmp != 0);
	if (!read32le(&jump_frame, is)) return false;
	// read description
	if (!read8le(&tmp, is)) return false;
	if (tmp < 0 || tmp >= SNAPSHOT_DESC_MAX_LENGTH) return false;
	if (is->fread(&description[0], tmp) != tmp) return false;
	description[tmp] = 0;		// add '0' because it wasn't saved
	// read and uncompress joysticks data
	len = (fourscore)?4*size:2*size;
	joysticks.resize(len);
	// read size
	int comprlen;
	if (!read32le(&comprlen, is)) return false;
	if (comprlen <= 0 || comprlen > len) return false;
	std::vector<uint8> cbuf(comprlen);
	if (is->fread(cbuf.data(),comprlen) != comprlen) return false;
	int e = uncompress((uint8*)joysticks.data(),(uLongf*)&len,cbuf.data(),comprlen);
	if (e != Z_OK && e != Z_BUF_ERROR) return false;
	// read and uncompress hot_changes data
	len = (fourscore) ? 4*size*HOTCHANGE_BYTES_PER_JOY : 2*size*HOTCHANGE_BYTES_PER_JOY;
	hot_changes.resize(len);
	// read size
	if (!read32le(&comprlen, is)) return false;
	if (comprlen <= 0 || comprlen > len) return false;
	std::vector<uint8> cbuf2(comprlen);
	if (is->fread(cbuf2.data(),comprlen) != comprlen) return false;
	e = uncompress(hot_changes.data(),(uLongf*)&len,cbuf.data(),comprlen);
	if (e != Z_OK && e != Z_BUF_ERROR) return false;
	return true;
}
bool INPUT_SNAPSHOT::skipLoad(EMUFILE *is)
{
	int tmp;
	uint8 tmp1;
	// read vars
	if (!read32le(&tmp, is)) return false;
	if (!read8le(&tmp1, is)) return false;
	if (!read8le(&tmp1, is)) return false;
	if (!read32le(&tmp, is)) return false;
	// read description
	if (!read8le(&tmp1, is)) return false;
	if (is->fseek(tmp1, SEEK_CUR) != 0) return false;
	// read joysticks data
	if (!read32le(&tmp, is)) return false;
	if (is->fseek(tmp, SEEK_CUR) != 0) return false;
	// read hot_changes data
	if (!read32le(&tmp, is)) return false;
	if (is->fseek(tmp, SEEK_CUR) != 0) return false;
	return true;
}

// return true if any difference is found
bool INPUT_SNAPSHOT::checkDiff(INPUT_SNAPSHOT& inp)
{
	if (size != inp.size) return true;
	if (findFirstChange(inp) >= 0)
		return true;
	else
		return false;
}

// return true if joypads differ
bool INPUT_SNAPSHOT::checkJoypadDiff(INPUT_SNAPSHOT& inp, int frame, int joy)
{
	if (fourscore)
	{
		int pos = frame << 2;
		if (pos < (inp.size << 2))
		{
			if (joysticks[pos+joy] != inp.joysticks[pos+joy]) return true;
		} else
		{
			if (joysticks[pos+joy]) return true;
		}
	} else
	{
		int pos = frame << 1;
		if (pos < (inp.size << 1))
		{
			if (joysticks[pos+joy] != inp.joysticks[pos+joy]) return true;
		} else
		{
			if (joysticks[pos+joy]) return true;
		}
	}
	return false;
}
// return number of first frame of difference
int INPUT_SNAPSHOT::findFirstChange(INPUT_SNAPSHOT& inp, int start, int end)
{
	// search for differences to the specified end (or to size)
	if (end < 0 || end >= size) end = size-1;

	if (fourscore)
	{
		int inp_end = inp.size << 2;
		end = (end << 2) + 3;
		for (int pos = start << 2; pos <= end; ++pos)
		{
			// if found different byte, or found emptiness in inp when there's non-zero value here
			if (pos < inp_end)
			{
				if (joysticks[pos] != inp.joysticks[pos]) return (pos >> 2);
			} else
			{
				if (joysticks[pos]) return (pos >> 2);
			}
		}
	} else
	{
		int inp_end = inp.size << 1;
		end = (end << 1) + 1;
		for (int pos = start << 1; pos <= end; ++pos)
		{
			// if found different byte, or found emptiness in inp when there's non-zero value here
			if (pos < inp_end)
			{
				if (joysticks[pos] != inp.joysticks[pos]) return (pos >> 1);
			} else
			{
				if (joysticks[pos]) return (pos >> 1);
			}
		}
	}
	// if current size is less then previous return size-1 as the frame of difference
	if (size < inp.size) return size-1;

	return -1;	// no changes were found
}
int INPUT_SNAPSHOT::findFirstChange(MovieData& md)
{
	// search for differences from the beginning to the end of movie (or to size)
	int end = md.getNumRecords()-1;
	if (end >= size) end = size-1;

	if (fourscore)
	{
		for (int frame = 0, pos = 0; frame <= end; ++frame)
		{
			if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
			if (joysticks[pos++] != md.records[frame].joysticks[1]) return frame;
			if (joysticks[pos++] != md.records[frame].joysticks[2]) return frame;
			if (joysticks[pos++] != md.records[frame].joysticks[3]) return frame;
		}
	} else
	{
		for (int frame = 0, pos = 0; frame <= end; ++frame)
		{
			if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
			if (joysticks[pos++] != md.records[frame].joysticks[1]) return frame;
		}
	}
	// if sizes differ, return last frame from the lesser of them
	if (size != md.getNumRecords()) return end;

	return -1;	// no changes were found
}

void INPUT_SNAPSHOT::SetMaxHotChange(int frame, int absolute_button)
{
	if (frame < 0 || frame >= size) return;
	// set max value (15) to the button hotness
	if (fourscore)
	{
		// 32 buttons, 16bytes
		if (absolute_button & 1)
			// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
			hot_changes[(frame << 4) | (absolute_button >> 1)] &= 0xF0;
		else
			// even buttons (A, S, U, L) - set lower 4 bits of the byte 
			hot_changes[(frame << 4) | (absolute_button >> 1)] &= 0x0F;
	} else
	{
		// 16 buttons, 8bytes
		if (absolute_button & 1)
			// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
			hot_changes[(frame << 3) | (absolute_button >> 1)] &= 0xF0;
		else
			// even buttons (A, S, U, L) - set lower 4 bits of the byte 
			hot_changes[(frame << 3) | (absolute_button >> 1)] &= 0x0F;
	}
}
int INPUT_SNAPSHOT::GetHotChangeInfo(int frame, int absolute_button)
{
	if (frame < 0 || frame >= size) return 0;
	if (absolute_button < 0 || absolute_button > 31) return 0;

	uint8 val;
	if (fourscore)
		// 32 buttons, 16bytes
		val = hot_changes[(frame << 4) + (absolute_button >> 1)];
	else
		// 16 buttons, 8bytes
		val = hot_changes[(frame << 3) + (absolute_button >> 1)];

	if (absolute_button & 1)
		// odd buttons (B, T, D, R) - upper 4 bits of the byte 
		return val >> 4;
	else
		// even buttons (A, S, U, L) - lower 4 bits of the byte 
		return val & 15;
}
// -----------------------------------------------------------------------------
INPUT_HISTORY::INPUT_HISTORY()
{

}

void INPUT_HISTORY::init(int new_size)
{
	history_size = new_size + 1;
	// clear snapshots history
	history_total_items = 0;
	input_snapshots.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial snapshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData);
	strcat(inp.description, modCaptions[0]);
	inp.jump_frame = -1;
	AddInputSnapshotToHistory(inp);
}
void INPUT_HISTORY::free()
{
	input_snapshots.resize(0);
}

// returns frame of first input change (for greenzone invalidation)
int INPUT_HISTORY::jump(int new_pos)
{
	if (new_pos < 0) new_pos = 0; else if (new_pos >= history_total_items) new_pos = history_total_items-1;
	// if nothing is done, do not invalidate greenzone
	if (new_pos == history_cursor_pos) return -1;

	// make jump
	history_cursor_pos = new_pos;
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;

	int first_change = input_snapshots[real_pos].findFirstChange(currMovieData);
	if (first_change < 0) return -1;	// if somehow there's no changes
	
	// update current movie
	input_snapshots[real_pos].toMovie(currMovieData, first_change);
	RedrawHistoryList();
	return first_change;
}
int INPUT_HISTORY::undo()
{
	return jump(history_cursor_pos - 1);
}
int INPUT_HISTORY::redo()
{
	return jump(history_cursor_pos + 1);
}
// ----------------------------
INPUT_SNAPSHOT& INPUT_HISTORY::GetCurrentSnapshot()
{
	return input_snapshots[(history_start_pos + history_cursor_pos) % history_size];
}
INPUT_SNAPSHOT& INPUT_HISTORY::GetNextToCurrentSnapshot()
{
	if (history_cursor_pos < history_total_items)
		return input_snapshots[(history_start_pos + history_cursor_pos + 1) % history_size];
	else
		return input_snapshots[(history_start_pos + history_cursor_pos) % history_size];
}
int INPUT_HISTORY::GetCursorPos()
{
	return history_cursor_pos;
}
int INPUT_HISTORY::GetTotalItems()
{
	return history_total_items;
}
char* INPUT_HISTORY::GetItemDesc(int pos)
{
	return input_snapshots[(history_start_pos + pos) % history_size].description;
}
bool INPUT_HISTORY::GetItemCoherence(int pos)
{
	return input_snapshots[(history_start_pos + pos) % history_size].coherent;
}
// ----------------------------
void INPUT_HISTORY::AddInputSnapshotToHistory(INPUT_SNAPSHOT &inp)
{
	// history uses conveyor of snapshots (vector with fixed size) to aviod resizing which is awfully expensive with such large objects as INPUT_SNAPSHOT
	int real_pos;
	if (history_cursor_pos+1 >= history_size)
	{
		// reached the end of available history_size - move history_start_pos (thus deleting older snapshot)
		history_cursor_pos = history_size-1;
		history_start_pos = (history_start_pos + 1) % history_size;
	} else
	{
		// didn't reach the end of history yet
		history_cursor_pos++;
		if (history_cursor_pos < history_total_items)
		{
			// overwrite old snapshot
			real_pos = (history_start_pos + history_cursor_pos) % history_size;
			// compare with the snapshot we're going to overwrite, if it's different then break the chain of coherent snapshots
			if (input_snapshots[real_pos].checkDiff(inp))
			{
				for (int i = history_cursor_pos+1; i < history_total_items; ++i)
				{
					real_pos = (history_start_pos + i) % history_size;
					input_snapshots[real_pos].coherent = false;
				}
			}
		} else
		{
			// add new smapshot
			history_total_items = history_cursor_pos+1;
			UpdateHistoryList();
		}
	}
	// write snapshot
	real_pos = (history_start_pos + history_cursor_pos) % history_size;
	input_snapshots[real_pos] = inp;
	RedrawHistoryList();
}

// returns frame of first actual change
int INPUT_HISTORY::RegisterInputChanges(int mod_type, int start, int end)
{
	// create new input shanshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData);
	// check if there are differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(input_snapshots[real_pos], start, end);
	if (first_changes >= 0)
	{
		// differences found
		// fade old hot_changes by 1

		// highlight new hot changes

		// fill description
		strcat(inp.description, modCaptions[mod_type]);
		switch (mod_type)
		{
			case MODTYPE_CHANGE:
			case MODTYPE_SET:
			case MODTYPE_UNSET:
			case MODTYPE_TRUNCATE:
			case MODTYPE_CLEAR:
			case MODTYPE_CUT:
			case MODTYPE_BRANCH_0: case MODTYPE_BRANCH_1:
			case MODTYPE_BRANCH_2: case MODTYPE_BRANCH_3:
			case MODTYPE_BRANCH_4: case MODTYPE_BRANCH_5:
			case MODTYPE_BRANCH_6: case MODTYPE_BRANCH_7:
			case MODTYPE_BRANCH_8: case MODTYPE_BRANCH_9:
			{
				inp.jump_frame = first_changes;
				break;
			}
			case MODTYPE_INSERT:
			case MODTYPE_DELETE:
			case MODTYPE_PASTE:
			case MODTYPE_PASTEINSERT:
			case MODTYPE_CLONE:
			{
				// for these changes user prefers to see frame of attempted change (selection beginning), not frame of actual differences
				inp.jump_frame = start;
				break;
			}
			case MODTYPE_RECORD:
			{
				// add info which joypads were affected
				int num = (inp.fourscore)?4:2;
				for (int i = 0; i < num; ++i)
				{
					if (inp.checkJoypadDiff(input_snapshots[real_pos], first_changes, i))
						strcat(inp.description, joypadCaptions[i]);
				}
				inp.jump_frame = start;
			}
		}
		// add upper and lower frame to description
		char framenum[11];
		_itoa(start, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		if (end > start)
		{
			_itoa(end, framenum, 10);
			strcat(inp.description, "-");
			strcat(inp.description, framenum);
		}
		AddInputSnapshotToHistory(inp);
	}
	return first_changes;
}
// ----------------------------
void INPUT_HISTORY::save(EMUFILE *os)
{
	int real_pos, last_tick = 0;
	// write vars
	write32le(history_cursor_pos, os);
	write32le(history_total_items, os);
	// write snapshots starting from history_start_pos
	for (int i = 0; i < history_total_items; ++i)
	{
		real_pos = (history_start_pos + i) % history_size;
		input_snapshots[real_pos].save(os);
		UpdateProgressbar(i, history_total_items);
	}
}
void INPUT_HISTORY::load(EMUFILE *is)
{
	int i = -1;
	INPUT_SNAPSHOT inp;
	// delete old snapshots
	input_snapshots.resize(history_size);
	// read vars
	if (!read32le((uint32 *)&history_cursor_pos, is)) goto error;
	if (!read32le((uint32 *)&history_total_items, is)) goto error;
	history_start_pos = 0;
	// read snapshots
	int total = history_total_items;
	if (history_total_items > history_size)
	{
		// user can't afford that much undo levels, skip some snapshots
		int num_snapshots_to_skip = history_total_items - history_size;
		// first try to skip snapshots over history_cursor_pos (future snapshots), because "redo" is less important than "undo"
		int num_redo_snapshots = history_total_items-1 - history_cursor_pos;
		if (num_snapshots_to_skip >= num_redo_snapshots)
		{
			// skip all redo snapshots
			history_total_items = history_cursor_pos+1;
			num_snapshots_to_skip -= num_redo_snapshots;
			// and still need to skip some undo snapshots
			for (i = 0; i < num_snapshots_to_skip; ++i)
				if (!inp.skipLoad(is)) goto error;
			total -= num_snapshots_to_skip;
			history_cursor_pos -= num_snapshots_to_skip;
		}
		history_total_items -= num_snapshots_to_skip;
	}
	// load snapshots
	for (i = 0; i < history_total_items; ++i)
	{
		// skip snapshots if current history_size is less then history_total_items
		if (!input_snapshots[i].load(is)) goto error;
		UpdateProgressbar(i, history_total_items);
	}
	// skip redo snapshots if needed
	for (; i < total; ++i)
		if (!inp.skipLoad(is)) goto error;

	return;
error:
	// couldn't load full history - reset it
	FCEU_printf("Error loading history\n");
	init(history_size-1);
}




