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

#include "Qt/gl_win.h"

gl_win_shm_t *gl_shm = NULL;

//************************************************************************
gl_win_shm_t *open_video_shm(void)
{
	int shmId;
	gl_win_shm_t *vaddr;
	struct shmid_ds ds;

	shmId = shmget( IPC_PRIVATE, sizeof(struct gl_win_shm_t), IPC_CREAT | S_IRWXU | S_IRWXG );

	if ( shmId == -1 )
	{
	   perror("Error: GL shmget Failed:");
		return NULL;
	}
	printf("Created ShmID: %i \n", shmId );

	vaddr = (gl_win_shm_t*)shmat( shmId, NULL, 0);

	if ( vaddr == (gl_win_shm_t*)-1 )
	{
	   perror("Error: GLX shmat Failed:");
		return NULL;
	}
	memset( vaddr, 0, sizeof(struct gl_win_shm_t));

	if ( shmctl( shmId, IPC_RMID, &ds ) != 0 )
	{
	   perror("Error: GLX shmctl IPC_RMID Failed:");
	}

	//sem_init( &vaddr->sem, 1, 1 );

	vaddr->ncol  = 256;
	vaddr->nrow  = 256;
	vaddr->pitch = 256 * 4;

	return vaddr;
}
//************************************************************************
