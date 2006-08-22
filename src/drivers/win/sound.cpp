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

LPDIRECTSOUND ppDS=0;           /* DirectSound object. */
LPDIRECTSOUNDBUFFER ppbuf=0;    /* Primary buffer object. */
LPDIRECTSOUNDBUFFER ppbufsec=0; /* Secondary buffer object. */
LPDIRECTSOUNDBUFFER ppbufw;     /* Buffer to actually write to. */

long DSBufferSize;              /* The size of the buffer that we can write to, in bytes. */

long BufHowMuch;                /* How many bytes we should try to buffer. */
DWORD ToWritePos;               /* Position which the next write to the buffer
                                   should write to.
                                */



DSBUFFERDESC DSBufferDesc;
WAVEFORMATEX wfa;
WAVEFORMATEX wf;

//bittag=1 -> 8bit output
int bittage;
static int mute=0;				/* TODO:  add to config? add to GUI. */

void TrashSound(void)
{
 FCEUI_Sound(0);
 if(ppbufsec)
 {
  IDirectSoundBuffer_Stop(ppbufsec);
  IDirectSoundBuffer_Release(ppbufsec);
  ppbufsec=0;
 }
 if(ppbuf)
 {
  IDirectSoundBuffer_Stop(ppbuf);
  IDirectSoundBuffer_Release(ppbuf);
  ppbuf=0;
 }
 if(ppDS)
 {
  IDirectSound_Release(ppDS);
  ppDS=0;
 }
}

void CheckDStatus(void)
{
  DWORD status;
  status=0;
  IDirectSoundBuffer_GetStatus(ppbufw, &status);

  if(status&DSBSTATUS_BUFFERLOST)
  {
   IDirectSoundBuffer_Restore(ppbufw);
  }

  if(!(status&DSBSTATUS_PLAYING))
  {
   ToWritePos=0;
   IDirectSoundBuffer_SetFormat(ppbufw,&wf);
   IDirectSoundBuffer_Play(ppbufw,0,0,DSBPLAY_LOOPING);
  }
}

static uint32 RawCanWrite(void)
{
 DWORD CurWritePos,CurPlayPos=0;

 CheckDStatus();

 CurWritePos=0;

 if(IDirectSoundBuffer_GetCurrentPosition(ppbufw,&CurPlayPos,&CurWritePos)==DS_OK)
 {
//   FCEU_DispMessage("%8d",(CurWritePos-CurPlayPos));
 }
 CurWritePos=(CurPlayPos+BufHowMuch)%DSBufferSize;

 /*  If the current write pos is >= half the buffer size less than the to write pos,
     assume DirectSound has wrapped around.
 */

 if(((int32)ToWritePos-(int32)CurWritePos) >= (DSBufferSize/2))
 {
  CurWritePos+=DSBufferSize;
  //FCEU_printf("Fixit: %d,%d,%d\n",ToWritePos,CurWritePos,CurWritePos-DSBufferSize);
 }
 if(ToWritePos<CurWritePos)
 {
  int32 howmuch=(int32)CurWritePos-(int32)ToWritePos;
  if(howmuch > BufHowMuch)      /* Oopsie.  Severe buffer overflow... */
  {
   //FCEU_printf("Ack");
   ToWritePos=CurWritePos%DSBufferSize;
  }
  return(CurWritePos-ToWritePos);
 }
 else
  return(0);
}

int32 GetWriteSound(void)
{
 if(!soundo)
  return 0;
 return(RawCanWrite() >> bittage);
}

int32 GetMaxSound(void)
{
 return( BufHowMuch >> bittage);
}

///enqueues the given samples for playback
static void EnqueueSamples(void *data, uint32 len) {
}

static int RawWrite(void *data, uint32 len)
{
 //uint32 cw; //mbg merge 7/17/06 removed

 //printf("Pre: %d\n",SexyALI_DSound_RawCanWrite(device));
 //fflush(stdout);

 if(!soundo)
  return 0;

 CheckDStatus();
 /* In this block, we write as much data as we can, then we write
    the rest of it in >=1ms chunks.
 */
 while(len)
 {
  VOID *LockPtr[2]={0,0};
  DWORD LockLen[2]={0,0};
  
  //mbg merge 7/17/06 changed to uint
  //mbg merge 7/18/06 was causing a crash when fastforwarding unless this was initialized to len
  uint32 curlen=len; 


  // THIS LIMITS THE EMULATION SPEED
  //if((!NoWaiting) || (soundoptions&SO_OLDUP))
  //printf("RawCanWrite(): %d\n",RawCanWrite());
  //while(!(curlen=RawCanWrite()))
  //{  
  // Sleep(1);
  //}

  if(curlen>len) curlen=len;

  if(DS_OK == IDirectSoundBuffer_Lock(ppbufw,ToWritePos,curlen,&LockPtr[0],&LockLen[0],&LockPtr[1],&LockLen[1],0))
  {
  }

  if(LockPtr[1] != 0 && LockPtr[1] != LockPtr[0])
  {
   if(mute)
   {
    if(bittage)
	{
     memset(LockPtr[0], 0, LockLen[0]);
     memset(LockPtr[1], 0, len-LockLen[0]);
	}
    else
	{
     memset(LockPtr[0], 0x80, LockLen[0]);
     memset(LockPtr[1], 0x80, len-LockLen[0]);
	}
   }
   else
   {
	/* not mute */
    memcpy(LockPtr[0],data,LockLen[0]);
    memcpy(LockPtr[1],(char*)data+LockLen[0],len-LockLen[0]); //mbg merge 7/17/06 added cast
   }
  }
  else if(LockPtr[0])
  {
   if(mute)
   {
    if(bittage)
     memset(LockPtr[0], 0, curlen);
    else
     memset(LockPtr[0], 0x80, curlen);
   }
   else
   {
	/* not mute */
    memcpy(LockPtr[0],data,curlen);
   }
  }
  IDirectSoundBuffer_Unlock(ppbufw,LockPtr[0],LockLen[0],LockPtr[1],LockLen[1]);
  ToWritePos=(ToWritePos+curlen)%DSBufferSize;

  len-=curlen;
  data=(uint8 *)data+curlen; //mbg merge 7/17/06 reworked to be type proper

  //if(len && !NoWaiting && (fps_scale <= 256 || (soundoptions&SO_OLDUP)))
  // Sleep(1); // do some extra sleeping if we think there's time and we're not scaling up the FPS or in turbo mode

 } // end while(len) loop


 return(1);
}

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

	BufferSet buffers;
	int cursor;
	
	bool turbo;
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
		memset(sbuf+todo,0,(samples-todo)<<1);
		return samples;
	}

	Player() {
		scale = 256;
		cursor = 0;
		turbo = false;
	}

	///receives and enqueues s16 stereo samples
	void receive(int samples, short *buf, bool turbo, int scale) {

		dsout->lock();
		buffers.enqueue(samples*2,buf);
		buffers.decay(60);
		this->scale = scale;
		dsout->unlock();

		//throttle
//		//wait for the buffer to be satisfactorily low before continuing
		if(!turbo) {
			for(;;) {
				dsout->lock();
				int remain = buffers.length>>1;
				dsout->unlock();
				if(remain<44100/60) break;
				Sleep(1);
			}
		}
	}

};
Player *player;


void sound_init_new() {
	dsout = new OAKRA_Module_OutputDS();
	dsout->start(0);
	dsout->beginThread();
	OAKRA_Format fmt;
	fmt.format = OAKRA_S16;
	fmt.channels = 1;
	fmt.rate = 44100;
	fmt.size = 2;
	OAKRA_Voice *voice = dsout->getVoice(fmt);
	player = new Player();
	dsout->lock();
	voice->setSource(player);
	dsout->unlock();
}

void FCEUD_WriteSoundData_new(int32 *Buffer, int scale, int Count, bool turbo) {
	#define WSD_BUFSIZE (2 * 96000 / 50)
	static int16 MBuffer[WSD_BUFSIZE*2];

	for(int i=0;i<Count;i++)
		MBuffer[i] = Buffer[i];
	player->receive(Count,MBuffer,turbo,scale);
}

int FCEUD_WriteSoundData(int32 *Buffer, int scale, int Count)
{
#define WSD_BUFSIZE (2 * 96000 / 50)

	int P;
	int iCount=0;
	static int16 MBuffer[WSD_BUFSIZE*2];

	//if(!(soundoptions&SO_OLDUP))
	//{
	//	if(FCEUI_EmulationPaused())
	//		memset(MBuffer, 0, WSD_BUFSIZE); // slow and/or unnecessary

	//	if(FCEUI_EmulationPaused()) scale >>= 1;

	//	// limit frequency change to between 50% and 200%
	//	if(scale > 512) scale = 512;
	//	if(scale < 128) scale = 128;
	//}

//	for(;Count>0;Count-=WSD_BUFSIZE)
	{
		//int amt = (soundoptions&SO_OLDUP) ? Count : (Count > WSD_BUFSIZE ? WSD_BUFSIZE : Count);
		int amt = Count;

		int cando = RawCanWrite();
		printf("Count/cando %d/%d\n",amt,cando);

		if(!bittage)
		{
			if(silencer)
				for(P=0;P<amt;P++)
					*(((uint8*)MBuffer)+P)=((int8)(Buffer[0]>>8))^128;
			else if(scale == 256) // exactly 100% speed
				for(P=0;P<amt;P++)
					*(((uint8*)MBuffer)+P)=((int8)(Buffer[P]>>8))^128;
			else // change sound frequency
				for(P=0;P<amt;P++)
					*(((uint8*)MBuffer)+P)=((int8)(Buffer[P*scale/256]>>8))^128;

			RawWrite(MBuffer,cando);
		}
		else // force 8-bit sound is off:
		{
			if(silencer)
				for(P=0;P<amt;P++)
					MBuffer[P]=Buffer[0];
			else if(scale == 256) // exactly 100% speed
				for(P=0;P<amt;P++)
					MBuffer[P]=Buffer[P];
			else // change sound frequency
				for(P=0;P<amt;P++)
					MBuffer[P]=Buffer[P*scale/256];

			RawWrite(MBuffer,cando * 2);
		}

		iCount+=amt;
	}

	// FCEUI_AviSoundUpdate((void*)MBuffer, Count);
	return iCount;
}

/*
//mbg merge 7/19/06 an experiment in understanding the sound update code
int FCEUD_WriteSoundData(int32 *Buffer, int scale, int Count)
{
#define WSD_BUFSIZE (2 * 96000 / 50)

	int P;
	int iCount=0;
	static int16 MBuffer[WSD_BUFSIZE*2];

	//if(!(soundoptions&SO_OLDUP))
	//{
		//shouldnt the sound be stopped?
		//if(FCEUI_EmulationPaused())
		//	memset(MBuffer, 0, WSD_BUFSIZE); // slow and/or unnecessary

		//why?
		//if(FCEUI_EmulationPaused()) scale >>= 1;
		// limit frequency change to between 50% and 200%
		//if(scale > 512) scale = 512;
		//if(scale < 128) scale = 128;
	//}

	int silence = FCEUI_EmulationPaused() | silencer;

	for(;Count>0;Count-=WSD_BUFSIZE)
	{
		//int amt = (soundoptions&SO_OLDUP) ? Count : (Count > WSD_BUFSIZE ? WSD_BUFSIZE : Count);
		int amt = (Count > WSD_BUFSIZE ? WSD_BUFSIZE : Count);

		if(!bittage)
		{
			if(silence)
				for(P=0;P<amt;P++)
					*(((uint8*)MBuffer)+P)=((int8)(Buffer[0]>>8))^128;
			else if(scale == 256) // exactly 100% speed
				for(P=0;P<amt;P++)
					*(((uint8*)MBuffer)+P)=((int8)(Buffer[P]>>8))^128;
			else // change sound frequency
				for(P=0;P<amt;P++)
					*(((uint8*)MBuffer)+P)=((int8)(Buffer[P*scale/256]>>8))^128;

			RawWrite(MBuffer,amt);
		}
		else // force 8-bit sound is off:
		{
			if(silence)
				for(P=0;P<amt;P++)
					MBuffer[P]=Buffer[0];
			else if(scale == 256) // exactly 100% speed
				for(P=0;P<amt;P++)
					MBuffer[P]=Buffer[P];
			else // change sound frequency
				for(int chan=0;chan<2;chan++) {
					int16 *dest = MBuffer+chan;
					int32 *src = Buffer+chan;
					int work = amt>>1;
					for(P=0;P<work;P++) {
						//interpolate
						int loc = P*scale;
						int mix = src[(loc>>8)<<1];
						mix += ((src[((loc>>8)+1)<<1]-mix)*(loc&255))>>8;
						dest[P<<1] = mix;
					}
				}

			RawWrite(MBuffer,amt * 2);
		}

		iCount+=amt;
	}

	// FCEUI_AviSoundUpdate((void*)MBuffer, Count);
	return iCount;
}
*/

int InitSound()
{
 DSCAPS dscaps;
 DSBCAPS dsbcaps;

 sound_init_new();

 memset(&wf,0x00,sizeof(wf));
 wf.wFormatTag = WAVE_FORMAT_PCM;
 wf.nChannels = 1;
 wf.nSamplesPerSec = soundrate;

 ddrval=DirectSoundCreate(0,&ppDS,0);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error creating DirectSound object.");
  return 0;
 }

 if(soundoptions&SO_SECONDARY)
 {
  trysecondary:
  ddrval=IDirectSound_SetCooperativeLevel(ppDS,hAppWnd,DSSCL_PRIORITY);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error setting cooperative level to DDSCL_PRIORITY.");
   TrashSound();
   return 0;
  }
 }
 else
 {
  ddrval=IDirectSound_SetCooperativeLevel(ppDS,hAppWnd,DSSCL_WRITEPRIMARY);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error setting cooperative level to DDSCL_WRITEPRIMARY.  Forcing use of secondary sound buffer and trying again...");
   soundoptions|=SO_SECONDARY;
   goto trysecondary;
  }
 }
 memset(&dscaps,0x00,sizeof(dscaps));
 dscaps.dwSize=sizeof(dscaps);
 ddrval=IDirectSound_GetCaps(ppDS,&dscaps);
 if(ddrval!=DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error getting capabilities.");
  return 0;
 }

 if(dscaps.dwFlags&DSCAPS_EMULDRIVER)
  FCEUD_PrintError("DirectSound: Sound device is being emulated through waveform-audio functions.  Sound quality will most likely be awful.  Try to update your sound device's sound drivers.");

 IDirectSound_Compact(ppDS);

 memset(&DSBufferDesc,0x00,sizeof(DSBUFFERDESC));
 DSBufferDesc.dwSize=sizeof(DSBufferDesc);
 if(soundoptions&SO_SECONDARY)
  DSBufferDesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
 else
  DSBufferDesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_GETCURRENTPOSITION2;

 ddrval=IDirectSound_CreateSoundBuffer(ppDS,&DSBufferDesc,&ppbuf,0);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error creating primary buffer.");
  TrashSound();
  return 0;
 } 

 memset(&wfa,0x00,sizeof(wfa));

 if(soundoptions&SO_FORCE8BIT)
  bittage=0;
 else
 {
  bittage=1;
  if( (!(dscaps.dwFlags&DSCAPS_PRIMARY16BIT) && !(soundoptions&SO_SECONDARY)) ||
      (!(dscaps.dwFlags&DSCAPS_SECONDARY16BIT) && (soundoptions&SO_SECONDARY)))
  {
   FCEUD_PrintError("DirectSound: 16-bit sound is not supported.  Forcing 8-bit sound.");
   bittage=0;
   soundoptions|=SO_FORCE8BIT;
  }
 }

 wf.wBitsPerSample=8<<bittage;
 wf.nBlockAlign = bittage+1;
 wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
 
 ddrval=IDirectSoundBuffer_SetFormat(ppbuf,&wf);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error setting primary buffer format.");
  TrashSound();
  return 0;
 }

 IDirectSoundBuffer_GetFormat(ppbuf,&wfa,sizeof(wfa),0);

 if(soundoptions&SO_SECONDARY)
 {
  memset(&DSBufferDesc,0x00,sizeof(DSBUFFERDESC));  
  DSBufferDesc.dwSize=sizeof(DSBufferDesc);
  DSBufferDesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
  if(soundoptions&SO_GFOCUS)
   DSBufferDesc.dwFlags|=DSBCAPS_GLOBALFOCUS;
  DSBufferDesc.dwBufferBytes=65536;
  DSBufferDesc.lpwfxFormat=&wfa;  
  ddrval=IDirectSound_CreateSoundBuffer(ppDS, &DSBufferDesc, &ppbufsec, 0);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error creating secondary buffer.");
   TrashSound();
   return 0;
  }
 }

 //sprintf(TempArray,"%d\n",wfa.nSamplesPerSec);
 //FCEUD_PrintError(TempArray);

 if(soundoptions&SO_SECONDARY)
 {
  DSBufferSize=65536;
  IDirectSoundBuffer_SetCurrentPosition(ppbufsec,0);
  ppbufw=ppbufsec;
 }
 else
 {
  memset(&dsbcaps,0,sizeof(dsbcaps));
  dsbcaps.dwSize=sizeof(dsbcaps);
  ddrval=IDirectSoundBuffer_GetCaps(ppbuf,&dsbcaps);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error getting buffer capabilities.");
   TrashSound();
   return 0;
  }

  DSBufferSize=dsbcaps.dwBufferBytes;

  if(DSBufferSize<8192)
  {
   FCEUD_PrintError("DirectSound: Primary buffer size is too small!");
   TrashSound();
   return 0;
  }
  ppbufw=ppbuf;
 }

 BufHowMuch=(soundbuftime*soundrate/1000)<<bittage;
 FCEUI_Sound(soundrate);
 return 1;
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
                 BufHowMuch=(soundbuftime*soundrate/1000)<<bittage;
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


void StopSound(void)
{
 if(soundo)
 {
  VOID *LockPtr=0;
  DWORD LockLen=0;
  if(DS_OK == IDirectSoundBuffer_Lock(ppbufw,0,DSBufferSize,&LockPtr,&LockLen,0,0,0))
  {
   //FCEUD_PrintError("K");
   if(bittage)
    memset(LockPtr, 0, LockLen);
   else
    memset(LockPtr, 0x80, LockLen);
   IDirectSoundBuffer_Unlock(ppbufw,LockPtr,LockLen,0,0);
  }

  //IDirectSoundBuffer_Stop(ppbufw);
 }
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
