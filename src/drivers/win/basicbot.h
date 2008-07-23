#ifndef _BASICBOT_H_
	#define _BASICBOT_H_
	#define BOT_MAXFRAMES 10000			// qfox: max number of frames to be computed per attempt...
	#define BOT_STATEFILE "botstate"	// the filename to save the current state to
	void UpdateBasicBot();
	void CreateBasicBot();
	extern char *BasicBotDir;
	void UpdateExternalButton();
	int BotFrameSkip();
	int BotFramePause();
#endif // _BASICBOT_H_
