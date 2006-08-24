/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

static int mute=0;				/* TODO:  add to config? add to GUI. */


int silencer=0;

#undef min
#undef max
#include "oakra.h"

OAKRA_Module_OutputDS *dsout;

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

	template <class ST>
	ST getElementAtIndex(int addr) {
		addr *= sizeof(ST); //shorts are 2bytes
		int buffer = (addr+offset)>>BufferSizeBits;
		int ofs = (addr+offset) & BufferSizeBitmask;
		return *(ST*)((char*)liveBuffers[buffer]->data+ofs);
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
	int lock(int length, void **ptr) {
		int remain = BufferSize-offset;
		*ptr = (char*)liveBuffers[0]->data + offset;
		if(length<remain)
			return length;
		else
			return remain;
	}

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


class PlayerBase : public OAKRA_Module {
public:

	BufferSet buffers;
	int scale;

	PlayerBase() {
		scale = 256;
	}

	void receive(int bytes, void *buf) {
		dsout->lock();
		buffers.enqueue(bytes,buf);
		buffers.decay(60);
		dsout->unlock();
	}

	virtual int getSamplesInBuffer() = 0;

	void throttle() {
		//wait for the buffer to be satisfactorily low before continuing
		for(;;) {
			dsout->lock();
			int remain = getSamplesInBuffer();
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
};

template <class ST,int SHIFT,int ZERO>
class Player : public PlayerBase {
public:

	int cursor;

	int getSamplesInBuffer() { return buffers.length>>SHIFT; }

	//not interpolating! someday it will! 
	int generate(int samples, void *buf) {

		int incr = 256;
		int bufferSamples = buffers.length>>SHIFT;

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
		ST *sbuf = (ST*)buf;
		for(int i=0;i<todo;i++) {
			sbuf[i] = buffers.getElementAtIndex<ST>(cursor>>8);
			cursor += incr;
		}
		buffers.dequeue((cursor>>8)<<SHIFT);
		cursor &= 255;
		memset(sbuf+todo,ZERO,(samples-todo)<<SHIFT);
		return samples;
	}

	Player() {
		cursor = 0;
	}

};

class ShortPlayer : public Player<short,1,0> {};
class BytePlayer : public Player<unsigned char,0,0x80> {};
static ShortPlayer *shortPlayer;
static BytePlayer *bytePlayer;
static PlayerBase *player;

void win_Throttle() {
	player->throttle();
}

void StopSound() {
	//dont think this needs to do anything anymore
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
	
	if(bits == 8)
		player = bytePlayer = new BytePlayer();
	else player = shortPlayer = new ShortPlayer();

	dsout->lock();
	voice->setSource(player);
	dsout->unlock();
}

void TrashSound() {
	delete dsout;
	if(player) delete player;
	player = 0;
}

//HACK - aviout is expecting this
WAVEFORMATEX wf;
int bittage; //1 -> 8bit
static int bits;


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

	switch(bits) {
	case 16: {
		void *tempbuf = alloca(2*count);
		short *sbuf = (short *)tempbuf;
		for(int i=0;i<count;i++)
			sbuf[i] = buffer[i];
		player->receive(count*2,tempbuf);
		break;
		}
	case 8: {
		void *tempbuf = alloca(count);
		unsigned char *bbuf = (unsigned char *)tempbuf;
		for(int i=0;i<count;i++)
			bbuf[i] = (buffer[i]>>8)^128;
		player->receive(count,tempbuf);
		}
	}
}




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
   case WM_NCLBUTTONDOWN:StopSound();break;

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


void ConfigSound(void)
{
 if(!uug)
  uug=CreateDialog(fceu_hInstance,"SOUNDCONFIG",0,SoundConCallB);
 else
  SetFocus(uug);
}

void FCEUD_SoundToggle(void)
{
	if(mute)
	{
		mute=0;
		FCEU_DispMessage("Sound mute off.");
	}
	else
	{
		mute=1;
		StopSound();
		FCEU_DispMessage("Sound mute on.");
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
	mute=0;
	FCEUI_SetSoundVolume(soundvolume);
	FCEU_DispMessage("Sound volume %d.", soundvolume);
}

//-----------


#include "wave.cpp"
