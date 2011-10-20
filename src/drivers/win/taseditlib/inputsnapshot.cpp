//Implementation file of Input Snapshot class (Undo feature)

#include "movie.h"
#include "inputsnapshot.h"
#include "markers.h"
#include "zlib.h"

const int bytes_per_frame[NUM_SUPPORTED_INPUT_TYPES] = {2, 4};	// 16bits for normal joypads, 32bits for fourscore

extern void FCEU_printf(char *format, ...);

extern MARKERS markers;

INPUT_SNAPSHOT::INPUT_SNAPSHOT()
{

}

void INPUT_SNAPSHOT::init(MovieData& md)
{
	already_compressed = false;
	// retrieve input data from movie data
	size = md.getNumRecords();
	input_type = (md.fourscore)?1:0;
	joysticks.resize(bytes_per_frame[input_type] * size);		// it's much faster to have this format than have [frame][joy] or other structures
	hot_changes.resize(bytes_per_frame[input_type] * size * HOTCHANGE_BYTES_PER_JOY);
	// fill input vector
	int pos = 0;
	switch(input_type)
	{
		case FOURSCORE:
		{
			for (int frame = 0; frame < size; ++frame)
			{
				joysticks[pos++] = md.records[frame].joysticks[0];
				joysticks[pos++] = md.records[frame].joysticks[1];
				joysticks[pos++] = md.records[frame].joysticks[2];
				joysticks[pos++] = md.records[frame].joysticks[3];
			}
			break;
		}
		case NORMAL_2JOYPADS:
		{
			for (int frame = 0; frame < size; ++frame)
			{
				joysticks[pos++] = md.records[frame].joysticks[0];
				joysticks[pos++] = md.records[frame].joysticks[1];
			}
			break;
		}
	}
	// make a copy of markers.markers_array
	markers_array = markers.markers_array;

	coherent = true;
	// save time to description
	time_t raw_time;
	time(&raw_time);
	struct tm * timeinfo = localtime(&raw_time);
	strftime(description, 10, "%H:%M:%S ", timeinfo);
}

void INPUT_SNAPSHOT::toMarkers()
{
	markers.markers_array = markers_array;
}

void INPUT_SNAPSHOT::toMovie(MovieData& md, int start)
{
	// write input data to movie data
	md.records.resize(size);
	switch(input_type)
	{
		case FOURSCORE:
		{
			int pos = start * bytes_per_frame[input_type];
			for (int frame = start; frame < size; ++frame)
			{
				md.records[frame].joysticks[0] = joysticks[pos++];
				md.records[frame].joysticks[1] = joysticks[pos++];
				md.records[frame].joysticks[2] = joysticks[pos++];
				md.records[frame].joysticks[3] = joysticks[pos++];
			}
			break;
		}
		case NORMAL_2JOYPADS:
		{
			int pos = start * bytes_per_frame[input_type];
			for (int frame = start; frame < size; ++frame)
			{
				md.records[frame].joysticks[0] = joysticks[pos++];
				md.records[frame].joysticks[1] = joysticks[pos++];
			}
			break;
		}
	}
}

void INPUT_SNAPSHOT::compress_data()
{
	// compress joysticks
	int len = joysticks.size();
	uLongf comprlen = (len>>9)+12 + len;
	joysticks_compressed.resize(comprlen);
	compress(joysticks_compressed.data(), &comprlen, joysticks.data(), len);
	joysticks_compressed.resize(comprlen);
	// compress hot_changes
	len = hot_changes.size();
	comprlen = (len>>9)+12 + len;
	hot_changes_compressed.resize(comprlen);
	compress(hot_changes_compressed.data(), &comprlen, hot_changes.data(), len);
	hot_changes_compressed.resize(comprlen);
	// compress markers
	len = markers_array.size();
	comprlen = (len>>9)+12 + len;
	markers_array_compressed.resize(comprlen);
	compress(markers_array_compressed.data(), &comprlen, markers_array.data(), len);
	markers_array_compressed.resize(comprlen);
	// don't compress anymore
	already_compressed = true;
}

void INPUT_SNAPSHOT::save(EMUFILE *os)
{
	// write vars
	write32le(size, os);
	write8le(input_type, os);
	if (coherent) write8le(1, os); else write8le((uint8)0, os);
	write32le(jump_frame, os);
	// write description
	int len = strlen(description);
	write8le(len, os);
	os->fwrite(&description[0], len);
	// write data
	if (!already_compressed)
		compress_data();
	// save joysticks data
	write32le(joysticks_compressed.size(), os);
	os->fwrite(joysticks_compressed.data(), joysticks_compressed.size());
	// save hot_changes data
	write32le(hot_changes_compressed.size(), os);
	os->fwrite(hot_changes_compressed.data(), hot_changes_compressed.size());
	// save markers data
	write32le(markers_array_compressed.size(), os);
	os->fwrite(markers_array_compressed.data(), markers_array_compressed.size());
}
bool INPUT_SNAPSHOT::load(EMUFILE *is)
{
	int len;
	uint8 tmp;
	// read vars
	if (!read32le(&size, is)) return false;
	if (!read8le(&tmp, is)) return false;
	input_type = tmp;
	if (!read8le(&tmp, is)) return false;
	coherent = (tmp != 0);
	if (!read32le(&jump_frame, is)) return false;
	// read description
	if (!read8le(&tmp, is)) return false;
	if (tmp < 0 || tmp >= SNAPSHOT_DESC_MAX_LENGTH) return false;
	if (is->fread(&description[0], tmp) != tmp) return false;
	description[tmp] = 0;		// add '0' because it wasn't saved
	// read data
	already_compressed = true;
	int comprlen;
	uLongf destlen;
	// read and uncompress joysticks data
	destlen = size * bytes_per_frame[input_type];
	joysticks.resize(destlen);
	// read size
	if (!read32le(&comprlen, is)) return false;
	if (comprlen <= 0) return false;
	joysticks_compressed.resize(comprlen);
	if (is->fread(joysticks_compressed.data(), comprlen) != comprlen) return false;
	int e = uncompress(joysticks.data(), &destlen, joysticks_compressed.data(), comprlen);
	if (e != Z_OK && e != Z_BUF_ERROR) return false;
	// read and uncompress hot_changes data
	destlen = size * bytes_per_frame[input_type] * HOTCHANGE_BYTES_PER_JOY;
	hot_changes.resize(destlen);
	// read size
	if (!read32le(&comprlen, is)) return false;
	if (comprlen <= 0) return false;
	hot_changes_compressed.resize(comprlen);
	if (is->fread(hot_changes_compressed.data(), comprlen) != comprlen) return false;
	e = uncompress(hot_changes.data(), &destlen, hot_changes_compressed.data(), comprlen);
	if (e != Z_OK && e != Z_BUF_ERROR) return false;
	// read and uncompress markers data
	destlen = size;
	markers_array.resize(destlen);
	// read size
	if (!read32le(&comprlen, is)) return false;
	if (comprlen <= 0) return false;
	markers_array_compressed.resize(comprlen);
	if (is->fread(markers_array_compressed.data(), comprlen) != comprlen) return false;
	e = uncompress(markers_array.data(), &destlen, markers_array_compressed.data(), comprlen);
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
	if (tmp1 < 0 || tmp1 >= SNAPSHOT_DESC_MAX_LENGTH) return false;
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
	switch(input_type)
	{
		case FOURSCORE:
		case NORMAL_2JOYPADS:
		{
			int pos = frame * bytes_per_frame[input_type];
			if (pos < (inp.size * bytes_per_frame[input_type]))
			{
				if (joysticks[pos+joy] != inp.joysticks[pos+joy]) return true;
			} else
			{
				if (joysticks[pos+joy]) return true;
			}
			break;
		}
	}
	return false;
}

// return true if any difference in markers_array is found
bool INPUT_SNAPSHOT::checkMarkersDiff(INPUT_SNAPSHOT& inp)
{
	if (size != inp.size) return true;
	for (int i = size-1; i >= 0; i--)
		if ((markers_array[i] - inp.markers_array[i]) & MARKER_FLAG_BIT) return true;
	return false;
}
// return true if any difference in markers_array is found, comparing to markers.markers_array
bool INPUT_SNAPSHOT::checkMarkersDiff()
{
	if (markers_array.size() != markers.markers_array.size()) return true;
	for (int i = markers_array.size()-1; i >= 0; i--)
		if ((markers_array[i] - markers.markers_array[i]) & MARKER_FLAG_BIT) return true;
	return false;
}

// return number of first frame of difference
int INPUT_SNAPSHOT::findFirstChange(INPUT_SNAPSHOT& inp, int start, int end)
{
	// search for differences to the specified end (or to size)
	if (end < 0 || end >= size) end = size-1;

	switch(input_type)
	{
		case FOURSCORE:
		case NORMAL_2JOYPADS:
		{
			int inp_end = inp.size * bytes_per_frame[input_type];
			end = ((end + 1) * bytes_per_frame[input_type]) - 1;
			for (int pos = start * bytes_per_frame[input_type]; pos <= end; ++pos)
			{
				// if found different byte, or found emptiness in inp when there's non-zero value here
				if (pos < inp_end)
				{
					if (joysticks[pos] != inp.joysticks[pos]) return (pos / bytes_per_frame[input_type]);
				} else
				{
					if (joysticks[pos]) return (pos / bytes_per_frame[input_type]);
				}
			}
			break;
		}
	}
	// if current size is less then previous return size-1 as the frame of difference
	if (size < inp.size) return size-1;
	// no changes were found
	return -1;
}
int INPUT_SNAPSHOT::findFirstChange(MovieData& md)
{
	// search for differences from the beginning to the end of movie (or to size)
	int end = md.getNumRecords()-1;
	if (end >= size) end = size-1;

	switch(input_type)
	{
		case FOURSCORE:
		{
			for (int frame = 0, pos = 0; frame <= end; ++frame)
			{
				if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[1]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[2]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[3]) return frame;
			}
			break;
		}
		case NORMAL_2JOYPADS:
		{
			for (int frame = 0, pos = 0; frame <= end; ++frame)
			{
				if (joysticks[pos++] != md.records[frame].joysticks[0]) return frame;
				if (joysticks[pos++] != md.records[frame].joysticks[1]) return frame;
			}
			break;
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
	switch(input_type)
	{
		case FOURSCORE:
		{
			// 32 buttons = 16bytes
			if (absolute_button & 1)
				// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
				hot_changes[(frame << 4) | (absolute_button >> 1)] &= 0xF0;
			else
				// even buttons (A, S, U, L) - set lower 4 bits of the byte 
				hot_changes[(frame << 4) | (absolute_button >> 1)] &= 0x0F;
			break;
		}
		case NORMAL_2JOYPADS:
		{
			// 16 buttons = 8bytes
			if (absolute_button & 1)
				// odd buttons (B, T, D, R) - set upper 4 bits of the byte 
				hot_changes[(frame << 3) | (absolute_button >> 1)] &= 0xF0;
			else
				// even buttons (A, S, U, L) - set lower 4 bits of the byte 
				hot_changes[(frame << 3) | (absolute_button >> 1)] &= 0x0F;
			break;
		}
	}
}
int INPUT_SNAPSHOT::GetHotChangeInfo(int frame, int absolute_button)
{
	if (frame < 0 || frame >= size) return 0;
	if (absolute_button < 0 || absolute_button > 31) return 0;

	uint8 val;
	switch(input_type)
	{
		case FOURSCORE:
		{
			// 32 buttons, 16bytes
			val = hot_changes[(frame << 4) + (absolute_button >> 1)];
			break;
		}
		case NORMAL_2JOYPADS:
		{
			// 16 buttons, 8bytes
			val = hot_changes[(frame << 3) + (absolute_button >> 1)];
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

