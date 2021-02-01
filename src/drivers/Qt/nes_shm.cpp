/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "Qt/nes_shm.h"

nes_shm_t *nes_shm = NULL;

//************************************************************************
nes_shm_t *open_nes_shm(void)
{
	nes_shm_t *vaddr;

	vaddr = (nes_shm_t*)malloc( sizeof(struct nes_shm_t) );

	//int shmId;
	//struct shmid_ds ds;

	//shmId = shmget( IPC_PRIVATE, sizeof(struct nes_shm_t), IPC_CREAT | S_IRWXU | S_IRWXG );

	//if ( shmId == -1 )
	//{
	//   perror("Error: GL shmget Failed:");
	//	return NULL;
	//}
	//printf("Created ShmID: %i \n", shmId );

	//vaddr = (nes_shm_t*)shmat( shmId, NULL, 0);

	//if ( vaddr == (nes_shm_t*)-1 )
	//{
	//   perror("Error: NES shmat Failed:");
	//	return NULL;
	//}
	//memset( vaddr, 0, sizeof(struct nes_shm_t));

	//if ( shmctl( shmId, IPC_RMID, &ds ) != 0 )
	//{
	//   perror("Error: GLX shmctl IPC_RMID Failed:");
	//}

	//sem_init( &vaddr->sem, 1, 1 );

	vaddr->video.ncol      = 256;
	vaddr->video.nrow      = 256;
	vaddr->video.pitch     = 256 * 4;
	vaddr->video.scale     = 1;
	vaddr->video.xyRatio   = 1;
	vaddr->video.preScaler = 0;

	return vaddr;
}
//************************************************************************
