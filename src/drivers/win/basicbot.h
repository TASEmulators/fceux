#ifndef _BASICBOT_H_
	#define _BASICBOT_H_
	#define BOT_MAXFRAMES 10000			// qfox: max number of frames to be computed per attempt...
	#define BOT_STATEFILE "botstate"	// the filename to save the current state to
	void UpdateBasicBot();
	void CreateBasicBot();
	extern char *BasicBotDir;
	static void BotSyntaxError(int errorcode);
	static void StopBasicBot();
	static void StartBasicBot();
	static int debug(int n);
	static char * debugS(char * s);
	static int error(int n);
	static void FromGUI();
	static void UpdateStatics();
	void UpdateExternalButton();
	static bool LoadBasicBotFile(char fn[]);
	static bool LoadBasicBot();
	static bool SaveBasicBotFile(char fn[]);
	static bool SaveBasicBot();
	void CrashWindow();
	void InitCode();
	void ResetStats();
	void UpdateBestGUI();
	static void PlayBest();
	void UpdateLastGUI(int last[]);
	void UpdateAvgGUI();
	void UpdateFullGUI();
	void UpdateCountersGUI();
	void SetNewAttempt();
	void UpdatePrevGUI(int best[]);
	int BotFrameSkip();
	int BotFramePause();
#endif // _BASICBOT_H_
