int FCEU_InitVirtualVideo(void);
void FCEU_KillVirtualVideo(void);
int SaveSnapshot(void);
extern uint8 *XBuf;
extern uint8 *XBackBuf;

extern struct GUIMESSAGE
{
	//countdown for gui messages
	int howlong;

	//the current gui message
	char errmsg[65];

	//indicates that the movie should be drawn even on top of movies
	bool isMovieMessage;

} guiMessage;


void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur);
