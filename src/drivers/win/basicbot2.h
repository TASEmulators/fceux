#ifndef _BASICBOT2_H_
	#define _BASICBOT2_H_
	// statics are only used in this file
	void BotCreateBasicBot();
	static BOOL CALLBACK WindowCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void BotCloseWindow();
	void BasicBotGetInput();
	static unsigned int FieldLength(HWND winhandle,int controlid);
	static char * GetText(int controlid);
	static UINT GetInt(int controlid);
	static void GetAllInputs();
#endif // _BASICBOT2_H_