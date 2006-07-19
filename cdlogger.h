#include <windows.h>

void DoCDLogger();
void UpdateCDLogger();
void LogPCM(int romaddress);

extern HWND hCDLogger;
extern volatile int codecount, datacount, undefinedcount;
extern volatile int loggingcodedata;
extern unsigned char *cdloggerdata;
