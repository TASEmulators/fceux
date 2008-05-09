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

#include "common.h"
#include "dinput.h"

#include "input.h"
#include "keyboard.h"

static HRESULT  ddrval; //mbg merge 7/17/06 made static

static LPDIRECTINPUTDEVICE7 lpdid=0;

void KeyboardClose(void)
{
 if(lpdid) IDirectInputDevice7_Unacquire(lpdid);
 lpdid=0;
}

static unsigned int keys[256] = {0,}; // with repeat
static unsigned int keys_nr[256] = {0,}; // non-repeating
static unsigned int keys_jd[256] = {0,}; // just-down
static unsigned int keys_jd_lock[256] = {0,}; // just-down released lock
int autoHoldKey = 0, autoHoldClearKey = 0;
int ctr=0;
void KeyboardUpdateState(void)
{
 unsigned char tk[256];
 printf("%d!!\n",ctr++);

 ddrval=IDirectInputDevice7_GetDeviceState(lpdid,256,tk);

 // HACK because DirectInput is totally wacky about recognizing the PAUSE/BREAK key
 if(GetAsyncKeyState(VK_PAUSE)) // normally this should have & 0x8000, but apparently this key is too special for that to work
	 tk[0xC5] = 0x80;

 DWORD time = timeGetTime();

 switch(ddrval)
 {
	case DI_OK: //memcpy(keys,tk,256);break;
	{
		extern int soundoptions;
		#define SO_OLDUP      32

		extern int soundo;
		extern int32 fps_scale;
		int notAlternateThrottle = !(soundoptions&SO_OLDUP) && soundo && ((NoWaiting&1)?(256*16):fps_scale) >= 64;
		#define KEY_REPEAT_INITIAL_DELAY ((!notAlternateThrottle) ? (16) : (64)) // must be >= 0 and <= 255
		#define KEY_REPEAT_REPEATING_DELAY (6) // must be >= 1 and <= 255
		#define KEY_JUST_DOWN_DURATION (4) // must be >= 1 and <= 255

		int i;
		
		for(i = 0 ; i < 256 ; i++)
			if(tk[i]) {
				if(keys_nr[i] == 0)
					keys_nr[i] = time;
			}
			else
				keys_nr[i] = 0;

		memcpy(keys,keys_nr,256*4);

		// key-down detection
		for(i = 0 ; i < 256 ; i++)
			if(!keys_nr[i])
			{
				keys_jd[i] = 0;
				keys_jd_lock[i] = 0;
			}
			else if(keys_jd_lock[i])
			{}
			else if(keys_jd[i]
			/*&& (i != 0x2A && i != 0x36 && i != 0x1D && i != 0x38)*/)
			{
				if(time - keys_jd[i] > KEY_JUST_DOWN_DURATION)
				{
					keys_jd[i] = 0;
					keys_jd_lock[i] = 1;
				}
			}
			else
				keys_jd[i] = time;

		// key repeat
		for(i = 0 ; i < 256 ; i++)
			if(time - keys[i] >= KEY_REPEAT_INITIAL_DELAY && !((time-keys[i])%KEY_REPEAT_REPEATING_DELAY))
				keys[i] = 0;
	}	   
	break;


   case DIERR_INPUTLOST:
   case DIERR_NOTACQUIRED:
                         memset(keys,0,256*4);
                         IDirectInputDevice7_Acquire(lpdid);
                         break;
  }

  extern uint8 autoHoldOn, autoHoldReset;
  autoHoldOn = autoHoldKey && keys[autoHoldKey] != 0;
  autoHoldReset = autoHoldClearKey && keys[autoHoldClearKey] != 0;
}

unsigned int *GetKeyboard(void)
{
 return(keys);
}
unsigned int *GetKeyboard_nr(void)
{
 return(keys_nr);
}
unsigned int *GetKeyboard_jd(void)
{
 return(keys_jd);
}

int KeyboardInitialize(void)
{
 if(lpdid)
  return(1);

 //mbg merge 7/17/06 changed:
 ddrval=IDirectInput7_CreateDeviceEx(lpDI, GUID_SysKeyboard,IID_IDirectInputDevice7, (LPVOID *)&lpdid,0);
 //ddrval=IDirectInput7_CreateDeviceEx(lpDI, &GUID_SysKeyboard,&IID_IDirectInputDevice7, (LPVOID *)&lpdid,0);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error creating keyboard device.");
  return 0;
 }

 ddrval=IDirectInputDevice7_SetCooperativeLevel(lpdid, hAppWnd,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error setting keyboard cooperative level.");
  return 0;
 }

 ddrval=IDirectInputDevice7_SetDataFormat(lpdid,&c_dfDIKeyboard);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error setting keyboard data format.");
  return 0;
 }

 ddrval=IDirectInputDevice7_Acquire(lpdid);
 /* Not really a fatal error. */
 //if(ddrval != DI_OK)
 //{
 // FCEUD_PrintError("DirectInput: Error acquiring keyboard.");
 // return 0;
 //}
 return 1;
}

int KeyboardSetBackgroundAccess(int on)
{
 if(!lpdid)
  return(0);

 ddrval=IDirectInputDevice7_Unacquire(lpdid);

 if(on)
  ddrval=IDirectInputDevice7_SetCooperativeLevel(lpdid, hAppWnd,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
 else
  ddrval=IDirectInputDevice7_SetCooperativeLevel(lpdid, hAppWnd,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error setting keyboard cooperative level.");
  return 0;
 }

 ddrval=IDirectInputDevice7_Acquire(lpdid);
 return 1;
}
