#ifndef _BASICBOT_H_
	#define _BASICBOT_H_
	#define BOT_MAXFRAMES 10000			// qfox: max number of frames to be computed per attempt...
	#define BOT_STATEFILE "botstate"	// the filename to save the current state to
	void UpdateBasicBot();
	void CreateBasicBot();
	extern char *BasicBotDir;
	void UpdateExternalButton();
	void CrashWindow();
	void InitCode();
	void ResetStats();
	void UpdateBestGUI();
	void UpdateLastGUI(int last[]);
	void UpdateAvgGUI();
	void UpdateFullGUI();
	void UpdateCountersGUI();
	void UpdateTitles();
	void SetNewAttempt();
	void SetNewPart();
	void UpdatePrevGUI(int best[]);
	int BotFrameSkip();
	int BotFramePause();
	bool LoggingEnabled();
	void LogAttempt(int *scores, bool better);
	void ShowCounters();
#endif // _BASICBOT_H_
