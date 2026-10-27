#ifdef _S9XLUA_H
#ifndef _FCEULUA_H
#define _FCEULUA_H

enum LuaCallID
{
	LUACALL_BEFOREEMULATION,
	LUACALL_AFTEREMULATION,
	LUACALL_BEFOREEXIT,
	LUACALL_BEFORESAVE,
	LUACALL_AFTERLOAD,
	LUACALL_TASEDITOR_AUTO,
	LUACALL_TASEDITOR_MANUAL,

	LUACALL_COUNT
};
extern void CallRegisteredLuaFunctions(LuaCallID calltype);

enum LuaMemHookType
{
	LUAMEMHOOK_WRITE,
	LUAMEMHOOK_READ,
	LUAMEMHOOK_EXEC,

	LUAMEMHOOK_COUNT
};
void CallRegisteredLuaMemHook(unsigned int address, int size, unsigned int value, LuaMemHookType hookType);

struct LuaSaveData
{
	LuaSaveData() { recordList = 0; }
	~LuaSaveData() { ClearRecords(); }

	struct Record
	{
		unsigned int key; // crc32
		unsigned int size; // size of data
		unsigned char* data;
		Record* next;
	};

	Record* recordList;

	void SaveRecord(struct lua_State* L, unsigned int key); // saves Lua stack into a record and pops it
	void LoadRecord(struct lua_State* L, unsigned int key, unsigned int itemsToLoad) const; // pushes a record's data onto the Lua stack
	void SaveRecordPartial(struct lua_State* L, unsigned int key, int idx); // saves part of the Lua stack (at the given index) into a record and does NOT pop anything

	void ExportRecords(void* file) const; // writes all records to an already-open file
	void ImportRecords(void* file); // reads records from an already-open file
	void ClearRecords(); // deletes all record data

private:
	// disallowed, it's dangerous to call this
	// (because the memory the destructor deletes isn't refcounted and shouldn't need to be copied)
	// so pass LuaSaveDatas by reference and this should never get called
	LuaSaveData(const LuaSaveData& copy) {}
};

#define LUA_DATARECORDKEY 42

void CallRegisteredLuaSaveFunctions(int savestateNumber, LuaSaveData& saveData);
void CallRegisteredLuaLoadFunctions(int savestateNumber, const LuaSaveData& saveData);

// Just forward function declarations

void FCEU_LuaFrameBoundary();
int FCEU_LoadLuaCode(const char *filename, const char *arg=NULL);
void FCEU_ReloadLuaCode();
void FCEU_LuaStop();
int FCEU_LuaRunning();

void FCEU_LuaReadZapper(const uint32* mouse_in, uint32* mouse_out);
uint8 FCEU_LuaReadJoypad(int,uint8); // HACK - Function needs controller input
int FCEU_LuaSpeed();
int FCEU_LuaFrameskip();
bool FCEU_LuaRerecordCountSkip();

void FCEU_LuaGui(uint8 *XBuf);
void FCEU_LuaUpdatePalette();

struct lua_State* FCEU_GetLuaState();
char* FCEU_GetLuaScriptName();

// And some interesting REVERSE declarations!
char *FCEU_GetFreezeFilename(int slot);

// Lua source debugger interface (used by the Qt GUI).
// Inspection calls are only valid while FCEU_LuaDbgIsPaused() is true.
#include <string>
#include <vector>

struct LuaDbgCallbacks
{
	// Called from the thread that hit a break. Must return once
	// FCEU_LuaDbgResumeRequested() becomes true.
	void (*waitForResume)(void);
};

struct LuaDbgFrame
{
	int level = 0;
	std::string name;
	std::string what;
	std::string source;      // normalized file path, empty if not a file
	std::string shortSource; // display name of the chunk
	int currentLine = -1;
	int lineDefined = -1;
};

enum LuaDbgScope
{
	LUADBG_SCOPE_LOCAL = 0,
	LUADBG_SCOPE_UPVALUE,
	LUADBG_SCOPE_GLOBAL,
	LUADBG_SCOPE_FIELD,
	LUADBG_SCOPE_WATCH
};

struct LuaDbgVar
{
	std::string name;
	std::string type;
	std::string value;
	int ref = 0;       // registry reference when the value is a table, else 0
	int keyRef = 0;    // registry reference of the key (table fields only)
	int parentRef = 0; // registry reference of the owning table (table fields only)
	int scope = LUADBG_SCOPE_LOCAL;
	int level = 0;
	int index = 0;
};

void FCEU_LuaDbgAttach(const LuaDbgCallbacks *cb);
void FCEU_LuaDbgDetach(void);
bool FCEU_LuaDbgIsAttached(void);
std::string FCEU_LuaDbgNormalizePath(const std::string &path);
std::string FCEU_LuaDbgGetScriptName(void);

void FCEU_LuaDbgSetBreakpoint(const std::string &file, int line, bool enable);
void FCEU_LuaDbgClearBreakpoints(void);
void FCEU_LuaDbgSetBreakOnStart(bool enable);

bool FCEU_LuaDbgIsPaused(void);
unsigned int FCEU_LuaDbgPauseSerial(void);
bool FCEU_LuaDbgResumeRequested(void);

void FCEU_LuaDbgRequestPause(void);
void FCEU_LuaDbgContinue(void);
void FCEU_LuaDbgStepInto(void);
void FCEU_LuaDbgStepOver(void);
void FCEU_LuaDbgStepOut(void);
void FCEU_LuaDbgRunToLine(const std::string &file, int line);

bool FCEU_LuaDbgGetStack(std::vector<LuaDbgFrame> &frames);
bool FCEU_LuaDbgGetScope(int level, int scope, std::vector<LuaDbgVar> &vars);
bool FCEU_LuaDbgGetChildren(int ref, std::vector<LuaDbgVar> &vars);
bool FCEU_LuaDbgEvaluate(int level, const std::string &expr, LuaDbgVar &result);
bool FCEU_LuaDbgSetValue(const LuaDbgVar &var, const std::string &expr, std::string &errMsg);


#endif
#endif
