//Implementation file of Greenzone class

#include "movie.h"
#include "state.h"
#include "../common.h"
#include "zlib.h"
#include "taseditproj.h"
#include "../tasedit.h"

extern TASEDIT_PROJECT project;
extern PLAYBACK playback;
extern int TASEdit_greenzone_capacity;
extern bool TASEdit_restore_position;

extern void FCEU_printf(char *format, ...);

GREENZONE::GREENZONE()
{

}

void GREENZONE::init()
{
	clearGreenzone();

	reset();
}
void GREENZONE::reset()
{
	lag_history.resize(currMovieData.getNumRecords());

}
void GREENZONE::update()
{


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



	ClearGreenzoneTail();
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

void GREENZONE::ClearGreenzoneTail()
{
	int tail_frame = greenZoneCount-1 - TASEdit_greenzone_capacity;

	if (tail_frame >= currFrameCounter) tail_frame = currFrameCounter - 1;
	for (;tail_frame >= 0; tail_frame--)
	{
		if (savestates[tail_frame].empty()) break;
		ClearSavestate(tail_frame);

    RedrawRow(tail_frame);
	}
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
	// write size
	write32le(greenZoneCount, os);
	write32le(currFrameCounter, os);
	// write savestates
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
		// write lag history
		write8le(lag_history[frame], os);
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
bool GREENZONE::load(EMUFILE *is)
{
	clearGreenzone();
	lag_history.resize(currMovieData.getNumRecords());
	int frame = 0, prev_frame = 0, size = 0;
	int last_tick = 0;
	// read size
	if (read32le((uint32 *)&size, is) && size >= 0 && size <= currMovieData.getNumRecords())
	{
		greenZoneCount = size;
		savestates.resize(greenZoneCount);
		int greenzone_tail_frame = greenZoneCount-1 - TASEdit_greenzone_capacity;

		if (read32le((uint32 *)&frame, is))
		{
			currFrameCounter = frame;
			while(1)
			{
				if (!read32le((uint32 *)&frame, is)) break;
				if (frame == -1) break;
				// update TASEditor progressbar from time to time
				if (frame / PROGRESSBAR_UPDATE_RATE > last_tick)
				{
					playback.SetProgressbar(frame, greenZoneCount);
					last_tick = frame / PROGRESSBAR_UPDATE_RATE;
				}
				// read lag history
				if (!read8le(&lag_history[frame], is)) break;
				// read lua_colorings
				// read monitorings
				// read savestate
				if (!read32le((uint32 *)&size, is)) break;
				if (size < 0) break;
				if (frame > greenzone_tail_frame || frame == currFrameCounter)
				{
					// load savestate
					savestates[frame].resize(size);
					if ((int)is->fread(savestates[frame].data(),size) < size) break;
					prev_frame = frame;			// successfully read one greenzone frame info
				} else
				{
					// skip loading this savestate
					if (is->fseek(size,SEEK_CUR) != 0) break;
				}
			}
			greenZoneCount = prev_frame+1;	// cut greenZoneCount to last good frame
			if (frame == -1)
			{
				// everything went fine - load savestate at cursor position
				if (loadTasSavestate(currFrameCounter))
					return true;
			} else goto error;
		}
	}
error:
	// there was some error while reading greenzone
	FCEU_printf("Error loading greenzone\n");
	return false;
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





