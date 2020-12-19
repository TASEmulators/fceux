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
