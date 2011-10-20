//Implementation file of Greenzone class

#include "movie.h"
#include "state.h"
#include "../common.h"
#include "zlib.h"
#include "taseditproj.h"
#include "../tasedit.h"
#include "zlib.h"

extern TASEDIT_PROJECT project;
extern PLAYBACK playback;
extern int TASEdit_greenzone_capacity;
extern bool TASEdit_restore_position;

extern void FCEU_printf(char *format, ...);

char greenzone_save_id[GREENZONE_ID_LEN] = "GREENZONE";

GREENZONE::GREENZONE()
{

}

void GREENZONE::init()
{
	clearGreenzone();
	reset();
	next_cleaning_time = clock() + TIME_BETWEEN_CLEANINGS;
}
void GREENZONE::reset()
{
	lag_history.resize(currMovieData.getNumRecords());

}
void GREENZONE::update()
{
	if (clock() > next_cleaning_time)
		GreenzoneCleaning();

}

void GREENZONE::TryDumpIncremental(bool lagFlag)
{
	// if movie length is less than currFrame, pad it with empty frames
	if(currMovieData.getNumRecords() <= currFrameCounter)
		currMovieData.insertEmpty(-1, 1 + currFrameCounter - currMovieData.getNumRecords());

	// update greenzone upper limit
	if (greenZoneCount <= currFrameCounter)
		greenZoneCount = currFrameCounter+1;
	if ((int)savestates.size() < greenZoneCount)
		savestates.resize(greenZoneCount);
	if ((int)lag_history.size() < greenZoneCount)
		lag_history.resize(greenZoneCount);

	// if frame changed - log savestate
	storeTasSavestate(currFrameCounter);
	// also log frame_flags
	if (currFrameCounter > 0)
	{
		// lagFlag indicates that lag was in previous frame
		if (lagFlag)
			lag_history[currFrameCounter-1] = 1;
		else
			lag_history[currFrameCounter-1] = 0;
	}
}

bool GREENZONE::loadTasSavestate(int frame)
{
	if (frame < 0 || frame >= currMovieData.getNumRecords())
		return false;
	if ((int)savestates.size() <= frame || savestates[frame].empty())
		return false;

	EMUFILE_MEMORY ms(&savestates[frame]);
	return FCEUSS_LoadFP(&ms, SSLOADPARAM_NOBACKUP);
}

void GREENZONE::storeTasSavestate(int frame)
{
	if ((int)savestates.size()<=frame)
		savestates.resize(frame+1);

	EMUFILE_MEMORY ms(&savestates[frame]);
	FCEUSS_SaveMS(&ms, Z_DEFAULT_COMPRESSION);
	ms.trim();
}

void GREENZONE::GreenzoneCleaning()
{
	int i = currFrameCounter - TASEdit_greenzone_capacity;
	if (i < 0) goto none_changed;
	int limit;
	// 2x of 1/2
	limit = i - 2 * TASEdit_greenzone_capacity;
	if (limit < -1) limit = -1;
	for (; i > limit; i--)
	{
		if ((i & 0x1) && !savestates[i].empty())
			ClearSavestate(i);
	}
	if (i < 0) goto finish;
	// 4x of 1/4
	limit = i - 4 * TASEdit_greenzone_capacity;
	if (limit < -1) limit = -1;
	for (; i > limit; i--)
	{
		if ((i & 0x3) && !savestates[i].empty())
			ClearSavestate(i);
	}
	if (i < 0) goto finish;
	// 8x of 1/8
	limit = i - 8 * TASEdit_greenzone_capacity;
	if (limit < -1) limit = -1;
	for (; i > limit; i--)
	{
		if ((i & 0x7) && !savestates[i].empty())
			ClearSavestate(i);
	}
	if (i < 0) goto finish;
	// 16x of 1/16
	limit = i - 16 * TASEdit_greenzone_capacity;
	if (limit < -1) limit = -1;
	for (; i > limit; i--)
	{
		if ((i & 0xF) && !savestates[i].empty())
			ClearSavestate(i);
	}
	// clear all remaining
	for (; i >= 0; i--)
	{
		if (!savestates[i].empty())
			ClearSavestate(i);
	}
finish:
	RedrawList();
none_changed:
	// shedule next cleaning
	next_cleaning_time = clock() + TIME_BETWEEN_CLEANINGS;
}

void GREENZONE::ClearSavestate(int index)
{
    std::vector<uint8> tmp;
    savestates[index].swap(tmp);
}

void GREENZONE::clearGreenzone()
{
	int size = savestates.size();
	for (int i = 0; i < size; ++i)
	{
		ClearSavestate(i);
	}
	savestates.resize(0);
	greenZoneCount = 0;
	lag_history.resize(0);
	// reset lua_colorings
	// reset monitorings
	
}

void GREENZONE::save(EMUFILE *os)
{
	int frame, size;
	int last_tick = 0;
	// write "GREENZONE" string
	os->fwrite(greenzone_save_id, GREENZONE_ID_LEN);
	// write size
	write32le(greenZoneCount, os);
	// compress and write lag history
	int len = lag_history.size();
	uLongf comprlen = (len>>9)+12 + len;
	std::vector<uint8> cbuf(comprlen);
	compress(cbuf.data(), &comprlen, lag_history.data(), len);
	write32le(comprlen, os);
	os->fwrite(cbuf.data(), comprlen);
	// write playback position
	write32le(currFrameCounter, os);
	// write savestates
	GreenzoneCleaning();
	for (frame = 0; frame < greenZoneCount; ++frame)
	{
		// update TASEditor progressbar from time to time
		if (frame / PROGRESSBAR_UPDATE_RATE > last_tick)
		{
			playback.SetProgressbar(frame, greenZoneCount);
			last_tick = frame / PROGRESSBAR_UPDATE_RATE;
		}
		if (savestates[frame].empty()) continue;
		write32le(frame, os);
		// write lua_colorings
		// write monitorings
		// write savestate
		size = savestates[frame].size();
		write32le(size, os);
		os->fwrite(savestates[frame].data(), size);
	}
	// write -1 as eof for greenzone
	write32le(-1, os);
}
// returns true if couldn't load
bool GREENZONE::load(EMUFILE *is)
{
	clearGreenzone();
	int frame = 0, prev_frame = -1, size = 0;
	int last_tick = 0;
	// read "GREENZONE" string
	char save_id[GREENZONE_ID_LEN];
	if ((int)is->fread(save_id, GREENZONE_ID_LEN) < GREENZONE_ID_LEN) goto error;
	if (strcmp(greenzone_save_id, save_id)) goto error;		// string is not valid
	// read size
	if (read32le((uint32 *)&size, is) && size >= 0 && size <= currMovieData.getNumRecords())
	{
		greenZoneCount = size;
		savestates.resize(greenZoneCount);
		// read and uncompress lag history
		lag_history.resize(greenZoneCount);
		int comprlen;
		uLongf destlen = greenZoneCount;
		if (!read32le(&comprlen, is)) goto error;
		if (comprlen <= 0) goto error;
		std::vector<uint8> cbuf(comprlen);
		if (is->fread(cbuf.data(), comprlen) != comprlen) goto error;
		int e = uncompress(lag_history.data(), &destlen, cbuf.data(), comprlen);
		if (e != Z_OK && e != Z_BUF_ERROR) goto error;
		// read playback position
		if (read32le((uint32 *)&frame, is))
		{
			currFrameCounter = frame;
			int greenzone_tail_frame = currFrameCounter - TASEdit_greenzone_capacity;
			int greenzone_tail_frame2 = greenzone_tail_frame - 2 * TASEdit_greenzone_capacity;
			int greenzone_tail_frame4 = greenzone_tail_frame - 4 * TASEdit_greenzone_capacity;
			int greenzone_tail_frame8 = greenzone_tail_frame - 8 * TASEdit_greenzone_capacity;
			int greenzone_tail_frame16 = greenzone_tail_frame - 16 * TASEdit_greenzone_capacity;
			// read savestates
			while(1)
			{
				if (!read32le((uint32 *)&frame, is)) break;
				if (frame < 0) break;		// -1 = eof
				// update TASEditor progressbar from time to time
				if (frame / PROGRESSBAR_UPDATE_RATE > last_tick)
				{
					playback.SetProgressbar(frame, greenZoneCount);
					last_tick = frame / PROGRESSBAR_UPDATE_RATE;
				}
				// read lua_colorings
				// read monitorings
				// read savestate
				if (!read32le((uint32 *)&size, is)) break;
				if (size < 0) break;
				if (frame <= greenzone_tail_frame16
					|| (frame <= greenzone_tail_frame8 && (frame & 0xF))
					|| (frame <= greenzone_tail_frame4 && (frame & 0x7))
					|| (frame <= greenzone_tail_frame2 && (frame & 0x3))
					|| (frame <= greenzone_tail_frame && (frame & 0x1)))
				{
					// skip loading this savestate
					if (is->fseek(size, SEEK_CUR) != 0) break;
				} else
				{
					// load this savestate
					savestates[frame].resize(size);
					if ((int)is->fread(savestates[frame].data(), size) < size) break;
					prev_frame = frame;			// successfully read one greenzone frame info
				}
			}
			if (prev_frame+1 == greenZoneCount)
			{
				// everything went fine - load savestate at cursor position
				if (loadTasSavestate(currFrameCounter))
					return false;
			}
			// uh, okay, but maybe we managed to read at least something useful from the file
			// first see if original position of currFrameCounter was read successfully
			if (loadTasSavestate(currFrameCounter))
			{
				greenZoneCount = prev_frame+1;		// cut greenZoneCount to last good frame
				FCEU_printf("Greenzone loaded partially\n");
				return false;
			}
			// then at least jump to some frame that was read successfully
			for (; prev_frame >= 0; prev_frame--)
			{
				if (loadTasSavestate(prev_frame))
				{
					currFrameCounter = prev_frame;
					greenZoneCount = prev_frame+1;		// cut greenZoneCount to this good frame
					FCEU_printf("Greenzone loaded partially, playback moved to the end of greenzone\n");
					return false;
				}
			}
		}
	}
error:
	return true;
}

void GREENZONE::InvalidateGreenZone(int after)
{
	if (after >= 0)
	{
		project.changed = true;
		if (greenZoneCount > after+1)
		{
			greenZoneCount = after+1;
			currMovieData.rerecordCount++;
			// either set playback cursor to the end of greenzone or run seeking to restore playback position
			if (currFrameCounter >= greenZoneCount)
			{
				if (TASEdit_restore_position)
					playback.restorePosition();
				else
					playback.jump(greenZoneCount-1);
			}
		}
	}
	// redraw list even if greenzone didn't change
	RedrawList();
}

int GREENZONE::FindBeginningOfGreenZone(int starting_index)
{
	for (int i = starting_index; i < greenZoneCount; ++i)
	{
		if (!savestates[i].empty()) return i;
	}
	return starting_index;
}





