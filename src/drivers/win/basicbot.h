#ifndef _BASICBOT_H_
	#define _BASICBOT_H_
	#define BOT_MAXFRAMES 2048 // qfox: max number of frames to be computed per attempt...
	void UpdateBasicBot();
	void CreateBasicBot();
	extern char *BasicBotDir;
	static void BotSyntaxError(int errorcode);
	static void StopBasicBot();
	static void StartBasicBot();
	static void debug(int n);
	static void error(int n);
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
	void UpdateLastGUI(int last[]);
	void UpdateAvgGUI();
	void UpdateFullGUI();
	void UpdateCountersGUI();
	void SetNewAttempt();
#endif // _BASICBOT_H_
