int FCEU_InitVirtualVideo(void);
void FCEU_KillVirtualVideo(void);
int SaveSnapshot(void);
extern uint8 *XBuf;
extern uint8 *XBackBuf;
extern int howlong;
void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur);
