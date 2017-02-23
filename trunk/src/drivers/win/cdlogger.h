//#include <windows.h>
//
bool DoCDLogger();
void UpdateCDLogger();
//void LogPCM(int romaddress); //No longer used

bool LoadCDLog(const char* nameo);
void LoadCDLogFile();
void SaveCDLogFileAs();
void SaveCDLogFile();
void RenameCDLog(const char* newName);
void SaveStrippedROM(int invert);
bool PauseCDLogging();
void StartCDLogging();
void FreeCDLog();
void InitCDLog();
void ResetCDLog();

//
extern HWND hCDLogger;
//extern volatile int codecount, datacount, undefinedcount;
//extern volatile int loggingcodedata;
//extern unsigned char *cdloggerdata;
