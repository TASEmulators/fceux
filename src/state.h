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

#include <stdio.h>

enum ENUM_SSLOADPARAMS
{
	SSLOADPARAM_NOBACKUP,
	SSLOADPARAM_BACKUP,
	SSLOADPARAM_DUMMY
};

void FCEUSS_Save(char *);
int FCEUSS_Load(char *);
bool FCEUSS_SaveFP(FILE *, int compressionLevel); //zlib values: 0 (none) through 9 (max) or -1 (default)
bool FCEUSS_LoadFP(FILE *, ENUM_SSLOADPARAMS);

extern int CurrentState;
void FCEUSS_CheckStates(void);

typedef struct {
           void *v;
           uint32 s;
	   char *desc;
} SFORMAT;



void ResetExState(void (*PreSave)(void),void (*PostSave)(void));
void AddExState(void *v, uint32 s, int type, char *desc);

#define FCEUSTATE_RLSB            0x80000000

void FCEU_DrawSaveStates(uint8 *XBuf);

