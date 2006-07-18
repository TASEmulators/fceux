#ifndef __MOVIE_H_
#define __MOVIE_H_

void FCEUMOV_AddJoy(uint8 *, int SkipFlush);
void FCEUMOV_CheckMovies(void);
void FCEUMOV_AddCommand(int cmd);
void FCEU_DrawMovies(uint8 *);
int FCEUMOV_IsPlaying(void);
int FCEUMOV_IsRecording(void);
int FCEUMOV_ShouldPause(void);
int FCEUMOV_GetFrame(void);

int FCEUMOV_WriteState(FILE* st);
int FCEUMOV_ReadState(FILE* st, uint32 size);
void FCEUMOV_PreLoad(void);
int FCEUMOV_PostLoad(void);
void MovieFlushHeader(void);

#endif /* __MOVIE_H_ */
