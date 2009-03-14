#ifdef _S9XLUA_H


// Just forward function declarations 

//void FCEU_LuaWrite(uint32 addr);
void FCEU_LuaFrameBoundary();
int FCEU_LoadLuaCode(const char *filename);
void FCEU_ReloadLuaCode();
void FCEU_LuaStop();
int FCEU_LuaRunning();

int FCEU_LuaUsingJoypad(int);
uint8 FCEU_LuaReadJoypad(int);
uint8 FCEU_LuaReadJoypadFalse(int which);	//adelikat - will be used to generate a 3rd button condition - false (in addition to true & nil)
int FCEU_LuaSpeed();
int FCEU_LuaFrameskip();
int FCEU_LuaRerecordCountSkip();

void FCEU_LuaGui(uint8 *XBuf);
void FCEU_LuaUpdatePalette();

// And some interesting REVERSE declarations!
char *FCEU_GetFreezeFilename(int slot);

// Call this before writing into a buffer passed to FCEU_CheatAddRAM().
// (That way, Lua-based memwatch will work as expected for somebody
// used to FCEU's memwatch.)
void FCEU_LuaWriteInform();


#endif
