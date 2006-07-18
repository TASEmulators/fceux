int InitJoysticks(HWND wnd);
int KillJoysticks(void);

void BeginJoyWait(HWND hwnd);
int DoJoyWaitTest(GUID *guid, uint8 *devicenum, uint16 *buttonnum);
void EndJoyWait(HWND hwnd);

void JoyClearBC(ButtConfig *bc);

void UpdateJoysticks(void);
int DTestButtonJoy(ButtConfig *bc);
