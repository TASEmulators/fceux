/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel and zeromus
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <list>
#include "common.h"
#include "main.h"

/// controls whether playback is muted
static bool mute = false;
/// indicates whether we've been coerced into outputting 8bit audio
static int bits;

#undef min
#undef max
#include "oakra.h"

OAKRA_Module_OutputDS *dsout;

//manages a set of small buffers which together work as one large buffer capable of resizing itsself and 
//shrinking when a larger buffer is no longer necessary
class BufferSet {
public:

	static const int BufferSize = 1024;
	static const int BufferSizeBits = 10;
	static const int BufferSizeBitmask = 1023;
	
	class Buffer {
	public:
		int decay, size, length;
		Buffer(int size) { length = 0; this->size = size; data = OAKRA_Module::malloc(size); }
		int getRemaining() { return size-length; }
		void *data;
		~Buffer() { delete data; }
	};

	std::vector<Buffer*> liveBuffers;
	std::vector<Buffer*> freeBuffers;
	int length;
	int offset; //offset of beginning of addressing into current buffer
	int bufferCount;

	BufferSet() {
		offset = length = bufferCount = 0;
	}

	//causes the oldest free buffer to decay one unit. kills it if it gets too old
	void decay(int threshold) {
		if(freeBuffers.empty()) return;
		if(freeBuffers[0]->decay++>=threshold) {
			delete freeBuffers[0];
			freeBuffers.erase(freeBuffers.begin());
		}
	}

	Buffer *getBuffer() {
		//try to get a buffer from the free pool first
		//if theres nothing in the free pool, get a new buffer
		if(!freeBuffers.size()) return getNewBuffer();
		//otherwise, return the last thing in the free pool (most recently freed)
		Buffer *ret = *--freeBuffers.end();
		freeBuffers.erase(--freeBuffers.end());
		return ret;
	}

	//adds the buffer to the free list
	void freeBuffer(Buffer *buf) {
		freeBuffers.push_back(buf);
		buf->decay = 0;
		buf->length = 0;
	}

	Buffer *getNewBuffer() {
		bufferCount++;
		return new Buffer(BufferSize);
	}

	short getShortAtIndex(int addr) {
		addr <<= 1; //shorts are 2bytes
		int buffer = (addr+offset)>>BufferSizeBits;
		int ofs = (addr+offset) & BufferSizeBitmask;
		return *(short*)((char*)liveBuffers[buffer]->data+ofs);
	}

	//dequeues the specified number of bytes
	void dequeue(int length) {
		offset += length;
		while(offset >= BufferSize) {
			Buffer *front = liveBuffers[0];
			freeBuffer(front);
			liveBuffers.erase(liveBuffers.begin());
			offset -= BufferSize;
		}
		this->length -= length;
	}

	//not being used now:

	//tries to lock the specified number of bytes for reading.
	//not all the bytes you asked for will be locked (no more than one buffer-full)
	//try again if you didnt get enough.
	//returns the number of bytes locked.
	//int lock(int length, void **ptr) {
	//	int remain = BufferSize-offset;
	//	*ptr = (char*)liveBuffers[0]->data + offset;
	//	if(length<remain)
	//		return length;
	//	else
	//		return remain;
	//}

	void enqueue(int length, void *data) {
		int todo = length;
		int done = 0;

		//if there are no buffers in the queue, then we definitely need one before we start
		if(liveBuffers.empty()) liveBuffers.push_back(getBuffer());
		
		while(todo) {
			//check the frontmost buffer
			Buffer *end = *--liveBuffers.end();
			int available = std::min(todo,end->getRemaining());
			memcpy((char*)end->data + end->length,(char*)data + done,available);
			end->length += available;
			todo -= available;
			done += available;
			
			//we're going to need another buffer
			if(todo != 0)
				liveBuffers.push_back(getBuffer());
		}

		this->length += length;
	}
};

class Player : public OAKRA_Module {
public:

	int cursor;
	BufferSet buffers;
	int scale;

	//not interpolating! someday it will! 
	int generate(int samples, void *buf) {

		int incr = 256;
		int bufferSamples = buffers.length>>1;

		//if we're we're too far behind, playback faster
		if(bufferSamples > 44100*3/60) {
			int behind = bufferSamples - 44100/60;
			incr = behind*256*60/44100/2;
			//we multiply our playback rate by 1/2 the number of frames we're behind
		}
		if(incr<256) printf("OHNO -- %d -- shouldnt be less than 256!\n",incr); //sanity check: should never be less than 256

		incr = (incr*scale)>>8; //apply scaling factor

		//figure out how many dest samples we can generate at this rate without running out of source samples
		int destSamplesCanGenerate = (bufferSamples<<8) / incr;

		int todo = std::min(destSamplesCanGenerate,samples);
		short *sbuf = (short*)buf;
		for(int i=0;i<todo;i++) {
			sbuf[i] = buffers.getShortAtIndex(cursor>>8);
			cursor += incr;
		}
		buffers.dequeue((cursor>>8)<<1);
		cursor &= 255;
		
		//perhaps mute
		if(mute) memset(sbuf,0,samples<<1);
		else memset(sbuf+todo,0,(samples-todo)<<1);

		return samples;
	}

	void receive(int bytes, void *buf) {
		dsout->lock();
		buffers.enqueue(bytes,buf);
		buffers.decay(60);
		dsout->unlock();
	}

	void throttle() {
		//wait for the buffer to be satisfactorily low before continuing
		for(;;) {
			dsout->lock();
			int remain = buffers.length>>1;
			dsout->unlock();
			if(remain<44100/60) break;
			//if(remain<44100*scale/256/60) break; ??
			Sleep(1);
		}
	}

	void setScale(int scale) {
		dsout->lock();
		this->scale = scale;
		dsout->unlock();
	}

	Player() {
		scale = 256;
		cursor = 0;
	}
};

//this class just converts the primary 16bit audio stream to 8bit
class Player8 : public OAKRA_Module {
public:
	Player *player;
	Player8(Player *player) { this->player = player; }
	int generate(int samples, void *buf) {
		int half = samples>>1;
		//retrieve first half of 16bit samples
		player->generate(half,buf);
		//and convert to 8bit
		unsigned char *dbuf = (unsigned char*)buf;
		short *sbuf = (short*)buf;
		for(int i=0;i<half;i++)
			dbuf[i] = (sbuf[i]>>8)^0x80;
		//now retrieve second half
		int remain = samples-half;
		short *halfbuf = (short*)alloca(remain<<1);
		player->generate(remain,halfbuf);
		dbuf += half;
		for(int i=0;i<remain;i++)
			dbuf[i] = (halfbuf[i]>>8)^0x80;
		return samples;
	}
};


static Player *player;
static Player8 *player8;

void win_Throttle() {
	player->throttle();
}

void win_SoundInit(int bits) {
	dsout = new OAKRA_Module_OutputDS();
	dsout->start(hAppWnd);
	dsout->beginThread();
	OAKRA_Format fmt;
	fmt.format = bits==8?OAKRA_U8:OAKRA_S16;
	fmt.channels = 1;
	fmt.rate = 44100;
	fmt.size = OAKRA_Module::calcSize(fmt);
	OAKRA_Voice *voice = dsout->getVoice(fmt);
	
	player = new Player();
	player8 = new Player8(player);

	dsout->lock();
	if(bits == 8) voice->setSource(player8);
	else voice->setSource(player);
	dsout->unlock();
}

void TrashSound() {
	if(dsout) delete dsout;
	if(player) delete player;
	dsout = 0;
	player = 0;
}



int InitSound() {
	if(soundoptions&SO_FORCE8BIT)
		bits = 8;
	else {
		//no modern system should have this problem, and we dont use primary buffer
		/*if( (!(dscaps.dwFlags&DSCAPS_PRIMARY16BIT) && !(soundoptions&SO_SECONDARY)) ||
		(!(dscaps.dwFlags&DSCAPS_SECONDARY16BIT) && (soundoptions&SO_SECONDARY)))
		{
		FCEUD_PrintError("DirectSound: 16-bit sound is not supported.  Forcing 8-bit sound.");*/
		bits = 16;
	}
	bits = 8;

	win_SoundInit(bits);

	FCEUI_Sound(soundrate);
	return 1;
	//  FCEUD_PrintError("DirectSound: Sound device is being emulated through waveform-audio functions.  Sound quality will most likely be awful.  Try to update your sound device's sound drivers.");
}


void win_SoundSetScale(int scale) {
	player->scale = scale;
}

void win_SoundWriteData(int32 *buffer, int count) {
	
	//todo..
		// FCEUI_AviSoundUpdate((void*)MBuffer, Count);

	void *tempbuf = alloca(2*count);
	short *sbuf = (short *)tempbuf;
	for(int i=0;i<count;i++)
		sbuf[i] = buffer[i];
	player->receive(count*2,tempbuf);
}


//--------
//GUI and control APIs

static HWND uug=0;

static void UpdateSD(HWND hwndDlg)
{
 int t;

 CheckDlgButton(hwndDlg,126,soundo?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,122,(soundoptions&SO_FORCE8BIT)?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,123,(soundoptions&SO_SECONDARY)?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,124,(soundoptions&SO_GFOCUS)?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,130,(soundoptions&SO_MUTEFA)?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,131,(soundoptions&SO_OLDUP)?BST_CHECKED:BST_UNCHECKED);
 SendDlgItemMessage(hwndDlg,129,CB_SETCURSEL,soundquality,(LPARAM)(LPSTR)0);
 t=0;
 if(soundrate==22050) t=1;
 else if(soundrate==44100) t=2;
 else if(soundrate==48000) t=3;
 else if(soundrate==96000) t=4;
 SendDlgItemMessage(hwndDlg,200,CB_SETCURSEL,t,(LPARAM)(LPSTR)0);
}

BOOL CALLBACK SoundConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

  switch(uMsg) {
   case WM_NCRBUTTONDOWN:
   case WM_NCMBUTTONDOWN:
   case WM_NCLBUTTONDOWN:break;

   case WM_INITDIALOG:
                /* Volume Trackbar */
                SendDlgItemMessage(hwndDlg,500,TBM_SETRANGE,1,MAKELONG(0,150));
                SendDlgItemMessage(hwndDlg,500,TBM_SETTICFREQ,25,0);
                SendDlgItemMessage(hwndDlg,500,TBM_SETPOS,1,150-soundvolume);

                /* buffer size time trackbar */
                SendDlgItemMessage(hwndDlg,128,TBM_SETRANGE,1,MAKELONG(15,200));
                SendDlgItemMessage(hwndDlg,128,TBM_SETTICFREQ,1,0);
                SendDlgItemMessage(hwndDlg,128,TBM_SETPOS,1,soundbuftime);

                {
                 char tbuf[8];
                 sprintf(tbuf,"%d",soundbuftime);
                 SetDlgItemText(hwndDlg,666,(LPTSTR)tbuf);
                }

                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Low");
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"High");
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Highest");

                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"11025");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"22050");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"44100");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"48000");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"96000");

                UpdateSD(hwndDlg);
                break;
   case WM_VSCROLL:
                soundvolume=150-SendDlgItemMessage(hwndDlg,500,TBM_GETPOS,0,0);
                FCEUI_SetSoundVolume(soundvolume);
                break;
   case WM_HSCROLL:
                {
                 char tbuf[8];
                 soundbuftime=SendDlgItemMessage(hwndDlg,128,TBM_GETPOS,0,0);
                 sprintf(tbuf,"%d",soundbuftime);
                 SetDlgItemText(hwndDlg,666,(LPTSTR)tbuf);
                 //soundbufsize=(soundbuftime*soundrate/1000);
                }
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(HIWORD(wParam))
                {
                 case CBN_SELENDOK:
			switch(LOWORD(wParam))
                        {
			 case 200:
                         {
                          int tmp;
                          tmp=SendDlgItemMessage(hwndDlg,200,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                          if(tmp==0) tmp=11025;
                          else if(tmp==1) tmp=22050;
                          else if(tmp==2) tmp=44100;
                          else if(tmp==3) tmp=48000;
                          else tmp=96000;
                          if(tmp!=soundrate)
                          {
                           soundrate=tmp;
                           if(soundrate<44100)
                           {
			    soundquality=0;
                            FCEUI_SetSoundQuality(0);
                            UpdateSD(hwndDlg);
                           }
                           if(soundo)
                           {
                            TrashSound();
                            soundo=InitSound();
                            UpdateSD(hwndDlg);
                           }
                          }
                         }
			 break;

			case 129:
			 soundquality=SendDlgItemMessage(hwndDlg,129,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         if(soundrate<44100) soundquality=0;
                         FCEUI_SetSoundQuality(soundquality);
                         UpdateSD(hwndDlg);
			 break;
			}
                        break;

                 case BN_CLICKED:
                        switch(LOWORD(wParam))
                        {
                         case 122:soundoptions^=SO_FORCE8BIT;   
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);                                   
                                  }
                                  break;
                         case 123:soundoptions^=SO_SECONDARY;
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);
                                  }
                                  break;
                         case 124:soundoptions^=SO_GFOCUS;
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);
                                  }
                                  break;
                         case 130:soundoptions^=SO_MUTEFA;
                                  break;
                         case 131:soundoptions^=SO_OLDUP;
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);
                                  }
                                  break;
                         case 126:soundo=!soundo;
                                  if(!soundo) TrashSound();
                                  else soundo=InitSound();
                                  UpdateSD(hwndDlg);
                                  break;
                        }
                }

                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:                         
                        DestroyWindow(hwndDlg);
                        uug=0;
                        break;
               }
              }
  return 0;
}

/**
* Shows the sounds configuration dialog.
**/
void ConfigSound()
{
	if(!uug)
	{
		uug = CreateDialog(fceu_hInstance, "SOUNDCONFIG", 0, SoundConCallB);
	}
	else
	{
		SetFocus(uug);
	}
}

void FCEUD_SoundToggle(void)
{
	if(mute)
	{
		mute = false;
		FCEU_DispMessage("Sound unmuted");
	}
	else
	{
		mute = true;
		FCEU_DispMessage("Sound muted");
	}
}

void FCEUD_SoundVolumeAdjust(int n)
{
	switch(n)
	{
	case -1:	soundvolume-=10; if(soundvolume<0) soundvolume=0; break;
	case 0:		soundvolume=100; break;
	case 1:		soundvolume+=10; if(soundvolume>150) soundvolume=150; break;
	}
	mute = false;
	FCEUI_SetSoundVolume(soundvolume);
	FCEU_DispMessage("Sound volume %d.", soundvolume);
}

//-----------
//throttle stuff
//-----------

static uint64 desiredfps;

static int32 fps_scale_table[]=
{ 3, 3, 4, 8, 16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096, 8192, 16384, 16384};
int32 fps_scale = 256;

void RefreshThrottleFPS()
{
	printf("WTF\n");
	fflush(stdout);
 desiredfps=FCEUI_GetDesiredFPS()>>8;
 desiredfps=(desiredfps*fps_scale)>>8;
 
}

static void IncreaseEmulationSpeed(void)
{
 int i;
 for(i=1; fps_scale_table[i]<fps_scale; i++)
  ;
 fps_scale = fps_scale_table[i+1];
}

static void DecreaseEmulationSpeed(void)
{
 int i;
 for(i=1; fps_scale_table[i]<fps_scale; i++)
  ;
 fps_scale = fps_scale_table[i-1];
}

#define fps_table_size		(sizeof(fps_scale_table)/sizeof(fps_scale_table[0]))

void FCEUD_SetEmulationSpeed(int cmd)
{
	switch(cmd)
	{
	case EMUSPEED_SLOWEST:	fps_scale=fps_scale_table[0];  break;
	case EMUSPEED_SLOWER:	DecreaseEmulationSpeed(); break;
	case EMUSPEED_NORMAL:	fps_scale=256; break;
	case EMUSPEED_FASTER:	IncreaseEmulationSpeed(); break;
	case EMUSPEED_FASTEST:	fps_scale=fps_scale_table[fps_table_size-1]; break;
	default:
		return;
	}

	RefreshThrottleFPS();

	FCEU_DispMessage("emulation speed %d%%",(fps_scale*100)>>8);
}


//#include "wave.cpp"
