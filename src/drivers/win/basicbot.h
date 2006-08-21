#ifndef _BASICBOT_H_
	#define _BASICBOT_H_
	#include "stdbool.h"
	#define BOT_MAXFRAMES 2048 // qfox: max number of frames to be computed per attempt...
	void UpdateBasicBot();
	void CreateBasicBot();
	extern char *BasicBotDir;
	static int EvaluateFormula(char * formula, char **nextposition, bool ret);
	static int ParseFormula(bool ret);
	static void StopBasicBot();
	static void StartBasicBot();
	static void debug(int n);
	static void error(int n);
	static void FromGUI();\
	void UpdateExternalButton();
#endif // _BASICBOT_H_
