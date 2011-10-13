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
extern bool TASEdit_bind_markers;
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

}
void GREENZONE::update()
{


}

void GREENZONE::TryDumpIncremental(bool lagFlag)
{
	// if movie length is less than currFrame, pad it with empty frames
	if((int)currMovieData.records.size() <= currFrameCounter)
		currMovieData.insertEmpty(-1, 1 + currFrameCounter - (int)currMovieData.records.size());

	// if frame chanegd - log savestate
	storeTasSavestate(currFrameCounter);
	// also log frame_flags
	if (currFrameCounter > 0)
	{
		// lagFlag indicates that lag was in previous frame
		if (lagFlag)
			currMovieData.frames_flags[currFrameCounter-1] |= LAG_FLAG_BIT;
		else
			currMovieData.frames_flags[currFrameCounter-1] &= ~LAG_FLAG_BIT;
	}
	// update greenzone upper limit
	if (greenZoneCount <= currFrameCounter)
		greenZoneCount = currFrameCounter+1;

	ClearGreenzoneTail();
}

bool GREENZONE::loadTasSavestate(int frame)
{
	if (frame < 0 || frame >= (int)currMovieData.records.size())
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
	greenZoneCount = 1;
	currMovieData.frames_flags.resize(1);
	// reset lua_colorings
	// reset monitorings
	
}

int GREENZONE::dumpGreenzone(EMUFILE *os)
{
	int start = os->ftell();
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
		// write frames_flags
		os->fwrite(&currMovieData.frames_flags[frame], 1);
		// write lua_colorings
		// write monitorings
		// write savestate
		size = savestates[frame].size();
		write32le(size, os);
		os->fwrite(savestates[frame].data(), size);

	}
	// write -1 as eof for greenzone
	write32le(-1, os);

	int end = os->ftell();
	return end-start;
}

bool GREENZONE::loadGreenzone(EMUFILE *is)
{
	clearGreenzone();
	currMovieData.frames_flags.resize(currMovieData.records.size());
	int frame = 0, prev_frame = 0, size = 0;
	int last_tick = 0;
	// read size
	if (read32le((uint32 *)&size, is) && size >= 0 && size <= (int)currMovieData.records.size())
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
				// read frames_flags
				if ((int)is->fread(&currMovieData.frames_flags[frame],1) != 1) break;
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
	if (after < 0) return;
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
				playback.JumpToFrame(greenZoneCount-1);
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





