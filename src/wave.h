#include "types.h"

bool FCEUI_BeginWaveRecord(const char *fn);
bool FCEUI_WaveRecordRunning(void);
void FCEU_WriteWaveData(int32 *Buffer, int Count);
int FCEUI_EndWaveRecord(void);
