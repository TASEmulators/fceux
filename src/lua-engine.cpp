#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <zlib.h>

#ifdef __linux
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif


extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


#include "types.h"
#include "fceu.h"
#include "video.h"
#include "drawing.h"
#include "state.h"
#include "movie.h"
#include "driver.h"
#include "cheat.h"
#include "utils/xstring.h"
#include "utils/memory.h"
#include "fceulua.h"

#ifdef WIN32
#include "drivers/win/common.h"
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef WIN32
extern void AddRecentLuaFile(const char *filename);
#endif

struct LuaSaveState {
	std::string filename;
	memorystream *data;
	bool anonymous, persisted;
	LuaSaveState()
		: data(0) 
		, anonymous(false)
		, persisted(false)
	{}
	~LuaSaveState() {
		if(data) delete data;
	}
	void persist() {
		persisted = true;
		FILE* outf = fopen(filename.c_str(),"wb");
		fwrite(data->buf(),1,data->size(),outf);
		fclose(outf);
	}
	void ensureLoad() {
		if(data) return;
		persisted = true;
		FILE* inf = fopen(filename.c_str(),"rb");
		fseek(inf,0,SEEK_END);
		int len = ftell(inf);
		fseek(inf,0,SEEK_SET);
		data = new memorystream(len);
		fread(data->buf(),1,len,inf);
		fclose(inf);
	}
};

static lua_State *L;

// Are we running any code right now?
static char *luaScriptName = NULL;

// Are we running any code right now?
int luaRunning = FALSE;

// True at the frame boundary, false otherwise.
static int frameBoundary = FALSE;


// The execution speed we're running at.
static enum {SPEED_NORMAL, SPEED_NOTHROTTLE, SPEED_TURBO, SPEED_MAXIMUM} speedmode = SPEED_NORMAL;

// Rerecord count skip mode
static int skipRerecords = FALSE;

// Used by the registry to find our functions
static const char *frameAdvanceThread = "FCEU.FrameAdvance";
static const char *memoryWatchTable = "FCEU.Memory";
static const char *memoryValueTable = "FCEU.MemValues";
static const char *guiCallbackTable = "FCEU.GUI";

// True if there's a thread waiting to run after a run of frame-advance.
static int frameAdvanceWaiting = FALSE;

// We save our pause status in the case of a natural death.
static int wasPaused = FALSE;

// Transparency strength. 0=opaque, 4=so transparent it's invisible
// TODO: intermediate values would be nice...
static int transparency;

// Our joypads.
static uint8 luajoypads1[4]= { 0xFF, 0xFF, 0xFF, 0xFF }; //x1
static uint8 luajoypads2[4]= { 0x00, 0x00, 0x00, 0x00 }; //0x
/* Crazy logic stuff.
	11 - true		01 - pass-through (default)
	00 - false		10 - invert					*/

static enum { GUI_USED_SINCE_LAST_DISPLAY, GUI_USED_SINCE_LAST_FRAME, GUI_CLEAR } gui_used = GUI_CLEAR;
static uint8 *gui_data = NULL;
static int gui_saw_current_palette = FALSE;

// See drawing.h for comments about FCEU's palette. We interpret zero as transparent.
enum
{
	GUI_COLOUR_CLEAR, GUI_COLOUR_WHITE,
	GUI_COLOUR_BLACK, GUI_COLOUR_GREY,
	GUI_COLOUR_RED,   GUI_COLOUR_GREEN,
	GUI_COLOUR_BLUE
};


// Protects Lua calls from going nuts.
// We set this to a big number like 1000 and decrement it
// over time. The script gets knifed once this reaches zero.
static int numTries;


// Look in fceu.h for macros named like JOY_UP to determine the order.
static const char *button_mappings[] = {
	"A", "B", "select", "start", "up", "down", "left", "right"
};

/**
 * Resets emulator speed / pause states after script exit.
 */
static void FCEU_LuaOnStop() {
	luaRunning = FALSE;
	for (int i = 0 ; i < 4 ; i++ ){
		luajoypads1[i]= 0xFF;	// Set these back to pass-through
		luajoypads2[i]= 0x00;
	}
	gui_used = GUI_CLEAR;
	if (wasPaused && !FCEUI_EmulationPaused())
		FCEUI_ToggleEmulationPause();
	FCEUD_SetEmulationSpeed(EMUSPEED_NORMAL);		//TODO: Ideally lua returns the speed to the speed the user set before running the script
													//rather than returning it to normal, and turbo off.  Perhaps some flags and a FCEUD_GetEmulationSpeed function
	FCEUD_TurboOff();	//Turn off turbo
}


/**
 * Asks Lua if it wants control of the emulator's speed.
 * Returns 0 if no, 1 if yes. If yes, caller should also
 * consult FCEU_LuaFrameSkip().
 */
int FCEU_LuaSpeed() {
	if (!L || !luaRunning)
		return 0;

	//printf("%d\n", speedmode);

	switch (speedmode) {
	case SPEED_NOTHROTTLE:
	case SPEED_TURBO:
	case SPEED_MAXIMUM:
		return 1;
	case SPEED_NORMAL:
	default:
		return 0;
	}
}

/**
 * Asks Lua if it wants control whether this frame is skipped.
 * Returns 0 if no, 1 if frame should be skipped, -1 if it should not be.
 */
int FCEU_LuaFrameSkip() {
	if (!L || !luaRunning)
		return 0;

	switch (speedmode) {
	case SPEED_NORMAL:
		return 0;
	case SPEED_NOTHROTTLE:
		return -1;
	case SPEED_TURBO:
		return 0;
	case SPEED_MAXIMUM:
		return 1;
	}
	return 0;
}

/**
 * When code determines that a write has occurred
 * (not necessarily worth informing Lua), call this.
 *
 */
void FCEU_LuaWriteInform() {
	if (!L || !luaRunning) return;
	// Nuke the stack, just in case.
	lua_settop(L,0);

	lua_getfield(L, LUA_REGISTRYINDEX, memoryWatchTable);
	lua_pushnil(L);
	while (lua_next(L, 1) != 0)
	{
		unsigned int addr = luaL_checkinteger(L, 2);
		lua_Integer value;
		lua_getfield(L, LUA_REGISTRYINDEX, memoryValueTable);
		lua_pushvalue(L, 2);
		lua_gettable(L, 4);
		value = luaL_checkinteger(L, 5);
		if (FCEU_CheatGetByte(addr) != value)
		{
			// Value changed; update & invoke the Lua callback
			lua_pushinteger(L, addr);
			lua_pushinteger(L, FCEU_CheatGetByte(addr));
			lua_settable(L, 4);
			lua_pop(L, 2);

			numTries = 1000;
			int res = lua_pcall(L, 0, 0, 0);
			if (res) {
				const char *err = lua_tostring(L, -1);
				
#ifdef WIN32
				//StopSound(); //mbg merge 7/23/08
				MessageBox(hAppWnd, err, "Lua Engine", MB_OK);
#else
				fprintf(stderr, "Lua error: %s\n", err);
#endif
			}
		}
		lua_settop(L, 2);
	}
	lua_settop(L, 0);
}


/**
 * Toggle certain rendering planes
 * FCEU.setrenderingplanes(sprites, background)
 * Accepts two (lua) boolean values and acts accordingly
*/
static int fceu_setrenderplanes(lua_State *L) {
	bool sprites = (lua_toboolean( L, 1 ) == 1);
	bool background = (lua_toboolean( L, 2 ) == 1);
	FCEUI_SetRenderPlanes(sprites, background);
	return 0;
}

///////////////////////////



// FCEU.speedmode(string mode)
//
//   Takes control of the emulation speed
//   of the system. Normal is normal speed (60fps, 50 for PAL),
//   nothrottle disables speed control but renders every frame,
//   turbo renders only a few frames in order to speed up emulation,
//   maximum renders no frames
//   TODO: better enforcement, done in the same way as basicbot...
static int fceu_speedmode(lua_State *L) {
	const char *mode = luaL_checkstring(L,1);
	
	if (strcasecmp(mode, "normal")==0) {
		speedmode = SPEED_NORMAL;
	} else if (strcasecmp(mode, "nothrottle")==0) {
		speedmode = SPEED_NOTHROTTLE;
	} else if (strcasecmp(mode, "turbo")==0) {
		speedmode = SPEED_TURBO;
	} else if (strcasecmp(mode, "maximum")==0) {
		speedmode = SPEED_MAXIMUM;
	} else
		luaL_error(L, "Invalid mode %s to FCEU.speedmode",mode);
	
	//printf("new speed mode:  %d\n", speedmode);
        if (speedmode == SPEED_NORMAL) 
			FCEUD_SetEmulationSpeed(EMUSPEED_NORMAL);
        else if (speedmode == SPEED_TURBO)				//adelikat: Making turbo actually use turbo.
			FCEUD_TurboOn();							//Turbo and max speed are two different results. Turbo employs frame skipping and sound bypassing if mute turbo option is enabled.
												//This makes it faster but with frame skipping. Therefore, maximum is still a useful feature, in case the user is recording an avi or making screenshots (or something else that needs all frames)
		else											
			FCEUD_SetEmulationSpeed(EMUSPEED_FASTEST);  //TODO: Make nothrottle turn off throttle, or remove the option
	return 0;
}

// FCEU.poweron()
//
// Executes a power cycle
static int fceu_poweron(lua_State *L) {
	if (GameInfo)
		FCEUI_PowerNES();
	
	return 0;
}

// FCEU.softreset()
//
// Executes a power cycle
static int fceu_softreset(lua_State *L) {
	if (GameInfo)
		FCEUI_ResetNES();
	
	return 0;
}

// FCEU.frameadvance()
//
//  Executes a frame advance. Occurs by yielding the coroutine, then re-running
//  when we break out.
static int fceu_frameadvance(lua_State *L) {
	// We're going to sleep for a frame-advance. Take notes.

	if (frameAdvanceWaiting) 
		return luaL_error(L, "can't call FCEU.frameadvance() from here");

	frameAdvanceWaiting = TRUE;

	// Now we can yield to the main 
	return lua_yield(L, 0);


	// It's actually rather disappointing...
}


// FCEU.pause()
//
//  Pauses the emulator, function "waits" until the user unpauses.
//  This function MAY be called from a non-frame boundary, but the frame
//  finishes executing anwyays. In this case, the function returns immediately.
static int fceu_pause(lua_State *L) {
	
	if (!FCEUI_EmulationPaused())
		FCEUI_ToggleEmulationPause();
	speedmode = SPEED_NORMAL;

	// Return control if we're midway through a frame. We can't pause here.
	if (frameAdvanceWaiting) {
		return 0;
	}

	// If it's on a frame boundary, we also yield.	
	frameAdvanceWaiting = TRUE;
	return lua_yield(L, 0);
	
}

//FCEU.unpause()
//
//adelikat:  Why wasn't this added sooner?
//Gives the user a way to unpause the emulator via lua
static int fceu_unpause(lua_State *L) {
	
	if (FCEUI_EmulationPaused())
		FCEUI_ToggleEmulationPause();
	speedmode = SPEED_NORMAL;

	// Return control if we're midway through a frame. We can't pause here.
	if (frameAdvanceWaiting) {
		return 0;
	}

	// If it's on a frame boundary, we also yield.	
	frameAdvanceWaiting = TRUE;
	return lua_yield(L, 0);
	
}


// FCEU.message(string msg)
//
//  Displays the given message on the screen.
static int fceu_message(lua_State *L) {

	const char *msg = luaL_checkstring(L,1);
	FCEU_DispMessage("%s", msg);
	
	return 0;

}


static int rom_readbyte(lua_State *L) {   lua_pushinteger(L, FCEU_ReadRomByte(luaL_checkinteger(L,1))); return 1; }
static int rom_readbytesigned(lua_State *L) {   lua_pushinteger(L, (signed char)FCEU_ReadRomByte(luaL_checkinteger(L,1))); return 1; }
static int memory_readbyte(lua_State *L) {   lua_pushinteger(L, FCEU_CheatGetByte(luaL_checkinteger(L,1))); return 1; }
static int memory_writebyte(lua_State *L) {   FCEU_CheatSetByte(luaL_checkinteger(L,1), luaL_checkinteger(L,2)); return 0; }
static int memory_readbyterange(lua_State *L) {

	int range_start = luaL_checkinteger(L,1);
	int range_size = luaL_checkinteger(L,2);
	if(range_size < 0)
		return 0;

	char* buf = (char*)alloca(range_size);
	for(int i=0;i<range_size;i++) {
		buf[i] = FCEU_CheatGetByte(range_start+i);
	}

	lua_pushlstring(L,buf,range_size);
	
	return 1;
}


// Not for the signed versions though
static int memory_readbytesigned(lua_State *L) {
	signed char c = (signed char) FCEU_CheatGetByte(luaL_checkinteger(L,1));
	lua_pushinteger(L, c);
	return 1;
}

// memory.register(int address, function func)
//
//  Calls the given function when the indicated memory address is
//  written to. No args are given to the function. The write has already
//  occurred, so the new address is readable.
static int memory_register(lua_State *L) {

	// Check args
	unsigned int addr = luaL_checkinteger(L, 1);
	if (lua_type(L,2) != LUA_TNIL && lua_type(L,2) != LUA_TFUNCTION)
		luaL_error(L, "function or nil expected in arg 2 to memory.register");
	
	
	// Check the address range
	if (addr > 0xffff)
		luaL_error(L, "arg 1 should be between 0x0000 and 0x0fff");

	// Commit it to the registery
	lua_getfield(L, LUA_REGISTRYINDEX, memoryWatchTable);
	lua_pushvalue(L,1);
	lua_pushvalue(L,2);
	lua_settable(L, -3);
	lua_getfield(L, LUA_REGISTRYINDEX, memoryValueTable);
	lua_pushvalue(L,1);
	if (lua_isnil(L,2)) lua_pushnil(L);
	else lua_pushinteger(L, FCEU_CheatGetByte(addr));
	lua_settable(L, -3);
	
	return 0;
}

//adelikat: table pulled from GENS.  credz nitsuja!

#ifdef _WIN32
const char* s_keyToName[256] =
{
	NULL,
	"leftclick",
	"rightclick",
	NULL,
	"middleclick",
	NULL,
	NULL,
	NULL,
	"backspace",
	"tab",
	NULL,
	NULL,
	NULL,
	"enter",
	NULL,
	NULL,
	"shift", // 0x10
	"control",
	"alt",
	"pause",
	"capslock",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"escape",
	NULL,
	NULL,
	NULL,
	NULL,
	"space", // 0x20
	"pageup",
	"pagedown",
	"end",
	"home",
	"left",
	"up",
	"right",
	"down",
	NULL,
	NULL,
	NULL,
	NULL,
	"insert",
	"delete",
	NULL,
	"0","1","2","3","4","5","6","7","8","9",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"A","B","C","D","E","F","G","H","I","J",
	"K","L","M","N","O","P","Q","R","S","T",
	"U","V","W","X","Y","Z",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"numpad0","numpad1","numpad2","numpad3","numpad4","numpad5","numpad6","numpad7","numpad8","numpad9",
	"numpad*","numpad+",
	NULL,
	"numpad-","numpad.","numpad/",
	"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
	"F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23","F24",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"numlock",
	"scrolllock",
	NULL, // 0x92
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, // 0xB9
	"semicolon",
	"plus",
	"comma",
	"minus",
	"period",
	"slash",
	"tilde",
	NULL, // 0xC1
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, // 0xDA
	"leftbracket",
	"backslash",
	"rightbracket",
	"quote",
};
#endif

//adelikat - the code for the keys is copied directly from GENS.  Props to nitsuja
//			 the code for the mouse is simply the same code from zapper.get
// input.get()
// takes no input, returns a lua table of entries representing the current input state,
// independent of the joypad buttons the emulated game thinks are pressed
// for example:
//   if the user is holding the W key and the left mouse button
//   and has the mouse at the bottom-right corner of the game screen,
//   then this would return {W=true, leftclick=true, xmouse=319, ymouse=223}
static int input_get(lua_State *L) {
	lua_newtable(L);

#ifdef _WIN32
	// keyboard and mouse button status
	{
		extern int EnableBackgroundInput;
		unsigned char keys [256];
		if(!EnableBackgroundInput)
		{
			if(GetKeyboardState(keys))
			{
				for(int i = 1; i < 255; i++)
				{
					int mask = (i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL) ? 0x01 : 0x80;
					if(keys[i] & mask)
					{
						const char* name = s_keyToName[i];
						if(name)
						{
							lua_pushboolean(L, true);
							lua_setfield(L, -2, name);
						}
					}
				}
			}
		}
		else // use a slightly different method that will detect background input:
		{
			for(int i = 1; i < 255; i++)
			{
				const char* name = s_keyToName[i];
				if(name)
				{
					int active;
					if(i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL)
						active = GetKeyState(i) & 0x01;
					else
						active = GetAsyncKeyState(i) & 0x8000;
					if(active)
					{
						lua_pushboolean(L, true);
						lua_setfield(L, -2, name);
					}
				}
			}
		}
	}
	// mouse position in game screen pixel coordinates
		
	extern void GetMouseData(uint32 (&md)[3]);

	uint32 MouseData[3];
	GetMouseData (MouseData);
	int x = MouseData[0];
	int y = MouseData[1];
	int click = MouseData[2];		///adelikat TODO: remove the ability to store the value 2?  Since 2 is right-clicking and not part of zapper input and is used for context menus

	lua_pushinteger(L, x);
	lua_setfield(L, -2, "xmouse");
	lua_pushinteger(L, y);
	lua_setfield(L, -2, "ymouse");
	lua_pushinteger(L, click);
	lua_setfield(L, -2, "click");		


#else
	// NYI (well, return an empty table)
#endif

	return 1;
}

// table zapper.read 
//int which unecessary because zapper is always controller 2
//Reads the zapper coordinates and a click value (1 if clicked, 0 if not, 2 if right click (but this is not used for zapper input)
static int zapper_read(lua_State *L){
	
	lua_newtable(L);
	
	extern void GetMouseData(uint32 (&md)[3]);

	uint32 MouseData[3];
	GetMouseData (MouseData);
	int x = MouseData[0];
	int y = MouseData[1];
	int click = MouseData[2];		///adelikat TODO: remove the ability to store the value 2?  Since 2 is right-clicking and not part of zapper input and is used for context menus

	lua_pushinteger(L, x);
	lua_setfield(L, -2, "xmouse");
	lua_pushinteger(L, y);
	lua_setfield(L, -2, "ymouse");
	lua_pushinteger(L, click);
	lua_setfield(L, -2, "click");	

	return 1;
}



// table joypad.read(int which = 1)
//
//  Reads the joypads as inputted by the user.
//  This is really the only way to get input to the system.
//  TODO: Don't read in *everything*...
static int joypad_read(lua_State *L) {

	// Reads the joypads as inputted by the user
	int which = luaL_checkinteger(L,1);
	
	if (which < 1 || which > 4) {
		luaL_error(L,"Invalid input port (valid range 1-4, specified %d)", which);
	}
	
	// Use the OS-specific code to do the reading.
	extern void FCEUD_UpdateInput(void);
	FCEUD_UpdateInput();
	extern SFORMAT FCEUCTRL_STATEINFO[];
	uint8 buttons = ((uint8 *) FCEUCTRL_STATEINFO[1].v)[which - 1];
	
	lua_newtable(L);
	
	int i;
	for (i = 0; i < 8; i++) {
		if (buttons & (1<<i)) {
			lua_pushinteger(L,1);
			lua_setfield(L, -2, button_mappings[i]);
		}
	}
	
	return 1;
}


// joypad.set(int which, table buttons)
//
//   Sets the given buttons to be pressed during the next
//   frame advance. The table should have the right 
//   keys (no pun intended) set.
/*FatRatKnight: I changed some of the logic.
  Now with 4 options!*/
static int joypad_set(lua_State *L) {

	// Which joypad we're tampering with
	int which = luaL_checkinteger(L,1);
	if (which < 1 || which > 4) {
		luaL_error(L,"Invalid output port (valid range 1-4, specified %d)", which);

	}

	// And the table of buttons.
	luaL_checktype(L,2,LUA_TTABLE);

	// Set up for taking control of the indicated controller
	luajoypads1[which-1] = 0xFF; // .1  Reset right bit
	luajoypads2[which-1] = 0x00; // 0.  Reset left bit

	int i;
	for (i=0; i < 8; i++) {
		lua_getfield(L, 2, button_mappings[i]);
		
		//Button is not nil, so find out if it is true/false
		if (!lua_isnil(L,-1))	
		{
			if (lua_toboolean(L,-1))							//True or string
				luajoypads2[which-1] |= 1 << i;
			if (lua_toboolean(L,-1) == 0 || lua_isstring(L,-1))	//False or string
				luajoypads1[which-1] &= ~(1 << i);
		}

		else
		{

		}
		lua_pop(L,1);
	}
	
	return 0;
}



//
//// Helper function to convert a savestate object to the filename it represents.
//static char *savestateobj2filename(lua_State *L, int offset) {
//	
//	// First we get the metatable of the indicated object
//	int result = lua_getmetatable(L, offset);
//
//	if (!result)
//		luaL_error(L, "object not a savestate object");
//	
//	// Also check that the type entry is set
//	lua_getfield(L, -1, "__metatable");
//	if (strcmp(lua_tostring(L,-1), "FCEU Savestate") != 0)
//		luaL_error(L, "object not a savestate object");
//	lua_pop(L,1);
//	
//	// Now, get the field we want
//	lua_getfield(L, -1, "filename");
//	
//	// Return it
//	return (char *) lua_tostring(L, -1);
//}
//

// Helper function for garbage collection.
static int savestate_gc(lua_State *L) {

	LuaSaveState *ss = (LuaSaveState *)lua_touserdata(L, 1);
	if(ss->persisted && ss->anonymous)
		remove(ss->filename.c_str());
	ss->~LuaSaveState();

	//// The object we're collecting is on top of the stack
	//lua_getmetatable(L,1);
	//
	//// Get the filename
	//const char *filename;
	//lua_getfield(L, -1, "filename");
	//filename = lua_tostring(L,-1);

	//// Delete the file
	//remove(filename);
	//
	
	// We exit, and the garbage collector takes care of the rest.
	return 0;
}

// object savestate.create(int which = nil)
//
//  Creates an object used for savestates.
//  The object can be associated with a player-accessible savestate
//  ("which" between 1 and 10) or not (which == nil).
static int savestate_create(lua_State *L) {
	int which = -1;
	if (lua_gettop(L) >= 1) {
		which = luaL_checkinteger(L, 1);
		if (which < 1 || which > 10) {
			luaL_error(L, "invalid player's savestate %d", which);
		}
	}

	//lets use lua to allocate the memory, since it is effectively a memory pool.
	LuaSaveState *ss = new(lua_newuserdata(L,sizeof(LuaSaveState))) LuaSaveState();

	if (which > 0) {
		// Find an appropriate filename. This is OS specific, unfortunately.
		// So I turned the filename selection code into my bitch. :)
		// Numbers are 0 through 9 though.
		ss->filename = FCEU_MakeFName(FCEUMKF_STATE, which - 1, 0);
	}
	else {
		//char tempbuf[100] = "snluaXXXXXX";
		//filename = mktemp(tempbuf);
		//doesnt work -^
		
		//mbg 8/13/08 - this needs to be this way. we'll make a better system later:
		ss->filename = tempnam(NULL, "snlua");
		ss->anonymous = true;
	}
	

	// The metatable we use, protected from Lua and contains garbage collection info and stuff.
	lua_newtable(L);
	
	//// First, we must protect it
	lua_pushstring(L, "FCEU Savestate");
	lua_setfield(L, -2, "__metatable");
	//
	//
	//// Now we need to save the file itself.
	//lua_pushstring(L, filename.c_str());
	//lua_setfield(L, -2, "filename");
	
	// If it's an anonymous savestate, we must delete the file from disk should it be gargage collected
	//if (which < 0) {
		lua_pushcfunction(L, savestate_gc);
		lua_setfield(L, -2, "__gc");
	//}
	
	// Set the metatable
	lua_setmetatable(L, -2);

	// Awesome. Return the object
	return 1;
}


// savestate.save(object state)
//
//   Saves a state to the given object.
static int savestate_save(lua_State *L) {

	//char *filename = savestateobj2filename(L,1);

	LuaSaveState *ss = (LuaSaveState *)lua_touserdata(L, 1);
	if(ss->data) delete ss->data;
	ss->data = new memorystream();

//	printf("saving %s\n", filename);

	// Save states are very expensive. They take time.
	numTries--;

	FCEUSS_SaveMS(ss->data,Z_NO_COMPRESSION);
	ss->data->sync();
	return 0;
}

static int savestate_persist(lua_State *L) {

	LuaSaveState *ss = (LuaSaveState *)lua_touserdata(L, 1);
	ss->persist();
	return 0;
}

// savestate.load(object state)
//
//   Loads the given state
static int savestate_load(lua_State *L) {

	//char *filename = savestateobj2filename(L,1);

	LuaSaveState *ss = (LuaSaveState *)lua_touserdata(L, 1);

	numTries--;

	FCEUSS_LoadFP(ss->data,SSLOADPARAM_NOBACKUP);
	ss->data->seekg(0);
	return 0;

}


// int movie.framecount()
//
//   Gets the frame counter for the movie, or nil if no movie running.
int movie_framecount(lua_State *L) {
		
	lua_pushinteger(L, FCEUMOV_GetFrame());
	return 1;
}

//int fceu.lagcount()
//
// Gets the current lag count
int fceu_lagcount(lua_State *L) {

	lua_pushinteger(L, FCEUI_GetLagCount());
	return 1;
}

//fceu_lagged()
//
//Returns true if the game is currently on a lag frame
int fceu_lagged (lua_State *L) {

	bool Lag_Frame = FCEUI_GetLagged();
	lua_pushboolean(L, Lag_Frame);
	return 1;
}

// string movie.mode()
//
//   "record", "playback" or nil
int movie_mode(lua_State *L) {
	if (FCEUMOV_IsRecording())
		lua_pushstring(L, "record");
	else if (FCEUMOV_IsPlaying())
		lua_pushstring(L, "playback");
	else
		lua_pushnil(L);
	return 1;
}


static int movie_rerecordcounting(lua_State *L) {
	if (lua_gettop(L) == 0)
		luaL_error(L, "no parameters specified");

	skipRerecords = lua_toboolean(L,1);
	return 0;
}

// movie.stop()
//
//   Stops movie playback/recording. Bombs out if movie is not running.
static int movie_stop(lua_State *L) {
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying())
		luaL_error(L, "no movie");
	
	FCEUI_StopMovie();
	return 0;

}

// movie.active()
//
//returns a bool value is there is a movie currently open
int movie_active (lua_State *L) {

	bool movieactive = (FCEUMOV_IsRecording() || FCEUMOV_IsPlaying());
	lua_pushboolean(L, movieactive);
	return 1;
}

//movie.rerecordcount()
//
//returns the rerecord count of the current movie
static int movie_rerecordcount (lua_State *L) {
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying())
		luaL_error(L, "No movie loaded.");

	lua_pushinteger(L, FCEUI_GetMovieRerecordCount());
	
	return 1;
}

//movie.length()
//
//returns an int value representing the total length of the current movie loaded

static int movie_length (lua_State *L) {
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying())
		luaL_error(L, "No movie loaded.");

	lua_pushinteger(L, FCEUI_GetMovieLength());

	return 1;
}

//FCEU.getreadonly
//
//returns true is emulator is in read-only mode, false if it is in read+wrte
static int fceu_getreadonly (lua_State *L) {
	lua_pushboolean(L, FCEUI_GetMovieToggleReadOnly());

	return 1;
}

//FCEU.setreadonly
//
//Sets readonly / read+write status
static int fceu_setreadonly (lua_State *L) {
	bool which = (lua_toboolean( L, 1 ) == 1);
	FCEUI_SetMovieToggleReadOnly(which);
	
	return 0;
}

//movie.getname
//
//returns the filename of the movie loaded
static int movie_getname (lua_State *L) {

	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying())
		luaL_error(L, "No movie loaded.");
	
	std::string name = FCEUI_GetMovieName();
	lua_pushstring(L, name.c_str());
	return 1;
}

//movie.playbeginning
//
//calls the play movie from beginning function
static int movie_playbeginning (lua_State *L) {

	FCEUI_MoviePlayFromBeginning();

	return 0;
}

// Common code by the gui library: make sure the screen array is ready
static void gui_prepare() {
	if (!gui_data)
		gui_data = (uint8 *) FCEU_malloc(256 * 256 + 8);
	if (gui_used != GUI_USED_SINCE_LAST_DISPLAY)
		memset(gui_data,GUI_COLOUR_CLEAR,256*256);
	gui_used = GUI_USED_SINCE_LAST_DISPLAY;
}


// Helper for a simple hex parser
static int hex2int(lua_State *L, char c) {
	if (c >= '0' && c <= '9')
		return c-'0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return luaL_error(L, "invalid hex in colour");
}

/**
 * Returns an index approximating an RGB colour.
 * TODO: This is easily improvable in terms of speed and probably
 * quality of matches. (gd overlay & transparency code call it a lot.)
 * With effort we could also cheat and map indices 0x08 .. 0x3F
 * ourselves.
 */
static uint8 gui_colour_rgb(uint8 r, uint8 g, uint8 b) {
	static uint8 index_lookup[1 << (3+3+3)];
        int k;
	if (!gui_saw_current_palette)
	{
		memset(index_lookup, GUI_COLOUR_CLEAR, sizeof(index_lookup));
		gui_saw_current_palette = TRUE;
	}

	k = ((r & 0xE0) << 1) | ((g & 0xE0) >> 2) | ((b & 0xE0) >> 5);
	uint16 test, best = GUI_COLOUR_CLEAR;
	uint32 best_score = 0xffffffffu, test_score;
	if (index_lookup[k] != GUI_COLOUR_CLEAR) return index_lookup[k];
	for (test = 0; test < 0xff; test++)
	{
		uint8 tr, tg, tb;
		if (test == GUI_COLOUR_CLEAR) continue;
		FCEUD_GetPalette(test, &tr, &tg, &tb);
		test_score = abs(r - tr) *  66 +
		             abs(g - tg) * 129 +
		             abs(b - tb) *  25;
		if (test_score < best_score) best_score = test_score, best = test;
	}
	index_lookup[k] = best;
	return best;
}

void FCEU_LuaUpdatePalette()
{
    gui_saw_current_palette = FALSE;
}

/**
 * Converts an integer or a string on the stack at the given
 * offset to a native colour. Several encodings are supported.
 * The user may pass their own palette index, a simple colour name,
 * or an HTML-style "#09abcd" colour, which is approximated.
 */
static uint16 gui_getcolour(lua_State *L, int offset) {
	switch (lua_type(L,offset)) {
	case LUA_TSTRING:
		{
			const char *str = lua_tostring(L,offset);
			if (strcmp(str,"red")==0) {
				return GUI_COLOUR_RED;
			} else if (strcmp(str, "green")==0) {
				return GUI_COLOUR_GREEN;
			} else if (strcmp(str, "blue")==0) {
				return GUI_COLOUR_BLUE;
			} else if (strcmp(str, "black")==0) {
				return GUI_COLOUR_BLACK;
			} else if (strcmp(str, "white")==0) {
				return GUI_COLOUR_WHITE;
			} else if (strcmp(str, "clear")==0) {
				return GUI_COLOUR_CLEAR;
			} else if (str[0] == '#' && strlen(str) == 7) {
				int red, green, blue;
				red = (hex2int(L, str[1]) << 4) | hex2int(L, str[2]);
				green = (hex2int(L, str[3]) << 4) | hex2int(L, str[4]);
				blue = (hex2int(L, str[5]) << 4) | hex2int(L, str[6]);

				return gui_colour_rgb(red, green, blue);
			} else
				return luaL_error(L, "unknown colour %s", str);

		}
	case LUA_TNUMBER:
		return (uint8) lua_tointeger(L,offset);
	default:
		return luaL_error(L, "invalid colour");
	}

}

// I'm going to use this a lot in here
#define swap(T, one, two) { \
	T temp = one; \
	one = two;    \
	two = temp;   \
}

// gui.drawpixel(x,y,colour)
static int gui_drawpixel(lua_State *L) {

	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L,2);

	uint8 colour = gui_getcolour(L,3);

	if (x < 0 || x >= 256 || y < 0 || y >= 256)
		luaL_error(L,"bad coordinates");

	gui_prepare();

	gui_data[y*256 + x] = colour;

	return 0;
}

// gui.drawline(x1,y1,x2,y2,type colour)
static int gui_drawline(lua_State *L) {

	int x1,y1,x2,y2;
	uint8 colour;
	x1 = luaL_checkinteger(L,1);
	y1 = luaL_checkinteger(L,2);
	x2 = luaL_checkinteger(L,3);
	y2 = luaL_checkinteger(L,4);
	colour = gui_getcolour(L,5);

	if (x1 < 0 || x1 >= 256 || y1 < 0 || y1 >= 256)
		luaL_error(L,"bad coordinates");

	if (x2 < 0 || x2 >= 256 || y2 < 0 || y2 >= 256)
		luaL_error(L,"bad coordinates");

	gui_prepare();


	// Horizontal line?
	if (y1 == y2) {
		if (x1 > x2) 
			swap(int, x1, x2);
		int i;
		for (i=x1; i <= x2; i++)
			gui_data[y1*256+i] = colour;
	} else if (x1 == x2) { // Vertical line?
		if (y1 > y2)
			swap(int, y1, y2);
		int i;
		for (i=y1; i < y2; i++)
			gui_data[i*256+x1] = colour;
	} else {
		// Some very real slope. We want to increase along the x value, so we swap for that.
		if (x1 > x2) {
			swap(int, x1, x2);
			swap(int, y1, y2);
		}


		double slope = ((double)y2-(double)y1) / ((double)x2-(double)x1);
		int myX = x1, myY = y1;
		double accum = 0;

		while (myX <= x2) {
			// Draw the current pixel
			gui_data[myY*256 + myX] = colour;

			// If it's above 1, we knock 1 off it and go up 1 pixel
			if (accum >= 1.0) {
				myY += 1;
				accum -= 1.0;
			} else if (accum <= -1.0) {
				myY -= 1;
				accum += 1.0;
			} else {
				myX += 1;
				accum += slope; // Step up

			}
		}


	}

	return 0;
}

// gui.drawbox(x1, y1, x2, y2, colour)
static int gui_drawbox(lua_State *L) {

	int x1,y1,x2,y2;
	uint8 colour;
	int i;

	x1 = luaL_checkinteger(L,1);
	y1 = luaL_checkinteger(L,2);
	x2 = luaL_checkinteger(L,3);
	y2 = luaL_checkinteger(L,4);
	colour = gui_getcolour(L,5);

	if (x1 < 0 || x1 >= 256 || y1 < 0 || y1 >= 256)
		luaL_error(L,"bad coordinates");

	if (x2 < 0 || x2 >= 256 || y2 < 0 || y2 >= 256)
		luaL_error(L,"bad coordinates");


	gui_prepare();

	// For simplicity, we mandate that x1,y1 be the upper-left corner
	if (x1 > x2)
		swap(int, x1, x2);
	if (y1 > y2)
		swap(int, y1, y2);

	// top surface
	for (i=x1; i <= x2; i++)
		gui_data[y1*256 + i] = colour;

	// bottom surface
	for (i=x1; i <= x2; i++)
		gui_data[y2*256 + i] = colour;

	// left surface
	for (i=y1; i <= y2; i++)
		gui_data[i*256+x1] = colour;

	// right surface
	for (i=y1; i <= y2; i++)
		gui_data[i*256+x2] = colour;


	return 0;
}


// gui.gdscreenshot()
//
//  Returns a screen shot as a string in gd's v1 file format.
//  This allows us to make screen shots available without gd installed locally.
//  Users can also just grab pixels via substring selection.
//
//  I think...  Does lua support grabbing byte values from a string?
//  Well, either way, just install gd and do what you like with it.
//  It really is easier that way.
static int gui_gdscreenshot(lua_State *L) {

	// Eat the stack
	lua_settop(L,0);
	
	// This is QUITE nasty...
	
	const int width=256, height=256;
	
	// Stack allocation
	unsigned char *buffer = (unsigned char*)alloca(2+2+2+1+4 + (width*height*4));
	unsigned char *pixels = (buffer + 2+2+2+1+4);

	// Truecolour image
	buffer[0] = 255;
	buffer[1] = 254;
	
	// Width
	buffer[2] = width >> 8;
	buffer[3] = width & 0xFF;
	
	// height
	buffer[4] = height >> 8;
	buffer[5] = height & 0xFF;
	
	// Make it truecolour... AGAIN?
	buffer[6] = 1;
	
	// No transparency
	buffer[7] = buffer[8] = buffer[9] = buffer[10] = 255;
	
	// Now we can actually save the image data
	int i = 0;
	int x,y;
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			uint8 index = XBuf[y*256 + x];

			// Write A,R,G,B (alpha=0 for us):
			pixels[i] = 0;
			FCEUD_GetPalette(index, &pixels[i+1],&pixels[i+2], &pixels[i+3]); 
			i += 4;
		}
	}
	
	// Ugh, ugh, ugh. Don't call this more than once a frame, for god's sake!
	
	lua_pushlstring(L, (char*)buffer, 2+2+2+1+4 + (width*height*4));
	
	// Buffers allocated with alloca are freed by the function's exit, since they're on the stack.
	
	return 1;
}


// gui.transparency(int strength)
//
//  0 = solid, 
static int gui_transparency(lua_State *L) {
	int trans = luaL_checkinteger(L,1);
	if (trans < 0 || trans > 4) {
		luaL_error(L, "transparency out of range");
	}
	
	transparency = trans;
	return 0;
}


// gui.text(int x, int y, string msg)
//
//  Displays the given text on the screen, using the same font and techniques as the
//  main HUD.
static int gui_text(lua_State *L) {
	const char *msg;
	int x, y;

	x = luaL_checkinteger(L,1);
	y = luaL_checkinteger(L,2);
	msg = luaL_checkstring(L,3);

	if (x < 0 || x >= 256 || y < 0 || y >= 256)
		luaL_error(L,"bad coordinates");

	gui_prepare();

	DrawTextTransWH(gui_data+y*256+x, 256, (uint8 *)msg, 0x20+0x80, 256 - x, 256 - y, 1);

	return 0;

}


// gui.gdoverlay(string str)
//
//  Overlays the given image on the screen.
static int gui_gdoverlay(lua_State *L) {

	int baseX, baseY;
	int width, height;
	size_t size;

	baseX = luaL_checkinteger(L,1);
	baseY = luaL_checkinteger(L,2);
	const uint8 *data = (const uint8*) luaL_checklstring(L, 3, &size);
	
	if (size < 11)
		luaL_error(L, "bad image data");
	
	if (data[0] != 255 || data[1] != 254)
		luaL_error(L, "bad image data or not truecolour");
		
	width = data[2] << 8 | data[3];
	height = data[4] << 8 | data[5];
	
	if (!data[6])
		luaL_error(L, "bad image data or not truecolour");
	
	// Don't care about transparent colour
	if ((int)size < (11+ width*height*4))
		luaL_error(L, "bad image data");
	
	const uint8* pixels = data + 11;
	
	// Run image

	gui_prepare();

	// These coordinates are relative to the image itself.
	int x,y;
	
	// These coordinates are relative to the screen
	int sx, sy;
	
	if (baseY < 0) {
		// Above the top of the screen
		sy = 0;
		y = -baseY;
	} else {
		// It starts on the screen itself
		sy = baseY;
		y = 0; 
	}	
	
	for (; y < height && sy < 256; y++, sy++) {
	
		if (baseX < 0) {
			x = -baseX;
			sx = 0;
		} else {
			x = 0;
			sx = baseX;
		}

		for (; x < width && sx < 256; x++, sx++) {
			if (pixels[4 * (y*height+x)] == 127)
				continue;

			uint8 r = pixels[4 * (y*width+x)+1];
			uint8 g = pixels[4 * (y*width+x)+2];
			uint8 b = pixels[4 * (y*width+x)+3];
			gui_data[256*(sy)+sx] = gui_colour_rgb(r, g, b);
		}
	
	}

	return 0;
}


// function gui.register(function f)
//
//  This function will be called just before a graphical update.
//  More complicated, but doesn't suffer any frame delays.
//  Nil will be accepted in place of a function to erase
//  a previously registered function, and the previous function
//  (if any) is returned, or nil if none.
static int gui_register(lua_State *L) {

	// We'll do this straight up.


	// First set up the stack.
	lua_settop(L,1);
	
	// Verify the validity of the entry
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);

	// Get the old value
	lua_getfield(L, LUA_REGISTRYINDEX, guiCallbackTable);
	
	// Save the new value
	lua_pushvalue(L,1);
	lua_setfield(L, LUA_REGISTRYINDEX, guiCallbackTable);
	
	// The old value is on top of the stack. Return it.
	return 1;

}

// string gui.popup(string message, [string type = "ok"])
//
//  Popup dialog!
int gui_popup(lua_State *L) {
	const char *message = luaL_checkstring(L, 1);
	const char *type = luaL_optstring(L, 2, "ok");
	
#ifdef WIN32
	int t;
	if (strcmp(type, "ok") == 0)
		t = MB_OK;
	else if (strcmp(type, "yesno") == 0)
		t = MB_YESNO;
	else if (strcmp(type, "yesnocancel") == 0)
		t = MB_YESNOCANCEL;
	else
		return luaL_error(L, "invalid popup type \"%s\"", type);

    //StopSound(); //mbg merge 7/27/08
	int result = MessageBox(hAppWnd, message, "Lua Script Pop-up", t);
	
	lua_settop(L,1);

	if (t != MB_OK) {
		if (result == IDYES)
			lua_pushstring(L, "yes");
		else if (result == IDNO)
			lua_pushstring(L, "no");
		else if (result == IDCANCEL)
			lua_pushstring(L, "cancel");
		else
			luaL_error(L, "win32 unrecognized return value %d", result);
		return 1;
	}

	// else, we don't care.
	return 0;
#else

	char *t;
#ifdef __linux

	int pid; // appease compiler

	// Before doing any work, verify the correctness of the parameters.
	if (strcmp(type, "ok") == 0)
		t = "OK:100";
	else if (strcmp(type, "yesno") == 0)
		t = "Yes:100,No:101";
	else if (strcmp(type, "yesnocancel") == 0)
		t = "Yes:100,No:101,Cancel:102";
	else
		return luaL_error(L, "invalid popup type \"%s\"", type);

	// Can we find a copy of xmessage? Search the path.
	
	char *path = strdup(getenv("PATH"));

	char *current = path;
	
	char *colon;

	int found = 0;

	while (current) {
		colon = strchr(current, ':');
		
		// Clip off the colon.
		*colon++ = 0;
		
		int len = strlen(current);
		char *filename = (char*)malloc(len + 12); // always give excess
		snprintf(filename, len+12, "%s/xmessage", current);
		
		if (access(filename, X_OK) == 0) {
			free(filename);
			found = 1;
			break;
		}
		
		// Failed, move on.
		current = colon;
		free(filename);
		
	}

	free(path);

	// We've found it?
	if (!found)
		goto use_console;

	pid = fork();
	if (pid == 0) {// I'm the virgin sacrifice
	
		// I'm gonna be dead in a matter of microseconds anyways, so wasted memory doesn't matter to me.
		// Go ahead and abuse strdup.
		char * parameters[] = {"xmessage", "-buttons", t, strdup(message), NULL};

		execvp("xmessage", parameters);
		
		// Aw shitty
		perror("exec xmessage");
		exit(1);
	}
	else if (pid < 0) // something went wrong!!! Oh hell... use the console
		goto use_console;
	else {
		// We're the parent. Watch for the child.
		int r;
		int res = waitpid(pid, &r, 0);
		if (res < 0) // wtf?
			goto use_console;
		
		// The return value gets copmlicated...
		if (!WIFEXITED(r)) {
			luaL_error(L, "don't screw with my xmessage process!");
		}
		r = WEXITSTATUS(r);
		
		// We assume it's worked.
		if (r == 0)
		{
			return 0; // no parameters for an OK
		}
		if (r == 100) {
			lua_pushstring(L, "yes");
			return 1;
		}
		if (r == 101) {
			lua_pushstring(L, "no");
			return 1;
		}
		if (r == 102) {
			lua_pushstring(L, "cancel");
			return 1;
		}
		
		// Wtf?
		return luaL_error(L, "popup failed due to unknown results involving xmessage (%d)", r);
	}

use_console:
#endif

	// All else has failed

	if (strcmp(type, "ok") == 0)
		t = "";
	else if (strcmp(type, "yesno") == 0)
		t = "yn";
	else if (strcmp(type, "yesnocancel") == 0)
		t = "ync";
	else
		return luaL_error(L, "invalid popup type \"%s\"", type);

	fprintf(stderr, "Lua Message: %s\n", message);

	while (TRUE) {
		char buffer[64];

		// We don't want parameters
		if (!t[0]) {
			fprintf(stderr, "[Press Enter]");
			fgets(buffer, sizeof(buffer), stdin);
			// We're done
			return 0;

		}
		fprintf(stderr, "(%s): ", t);
		fgets(buffer, sizeof(buffer), stdin);
		
		// Check if the option is in the list
		if (strchr(t, tolower(buffer[0]))) {
			switch (tolower(buffer[0])) {
			case 'y':
				lua_pushstring(L, "yes");
				return 1;
			case 'n':
				lua_pushstring(L, "no");
				return 1;
			case 'c':
				lua_pushstring(L, "cancel");
				return 1;
			default:
				luaL_error(L, "internal logic error in console based prompts for gui.popup");
			
			}
		}
		
		// We fell through, so we assume the user answered wrong and prompt again.
	
	}

	// Nothing here, since the only way out is in the loop.
#endif

}

// the following bit operations are ported from LuaBitOp 1.0.1,
// because it can handle the sign bit (bit 31) correctly.

/*
** Lua BitOp -- a bit operations library for Lua 5.1.
** http://bitop.luajit.org/
**
** Copyright (C) 2008-2009 Mike Pall. All rights reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
** [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#ifdef _MSC_VER
/* MSVC is stuck in the last century and doesn't have C99's stdint.h. */
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

typedef int32_t SBits;
typedef uint32_t UBits;

typedef union {
  lua_Number n;
#ifdef LUA_NUMBER_DOUBLE
  uint64_t b;
#else
  UBits b;
#endif
} BitNum;

/* Convert argument to bit type. */
static UBits barg(lua_State *L, int idx)
{
  BitNum bn;
  UBits b;
  bn.n = lua_tonumber(L, idx);
#if defined(LUA_NUMBER_DOUBLE)
  bn.n += 6755399441055744.0;  /* 2^52+2^51 */
#ifdef SWAPPED_DOUBLE
  b = (UBits)(bn.b >> 32);
#else
  b = (UBits)bn.b;
#endif
#elif defined(LUA_NUMBER_INT) || defined(LUA_NUMBER_LONG) || \
      defined(LUA_NUMBER_LONGLONG) || defined(LUA_NUMBER_LONG_LONG) || \
      defined(LUA_NUMBER_LLONG)
  if (sizeof(UBits) == sizeof(lua_Number))
    b = bn.b;
  else
    b = (UBits)(SBits)bn.n;
#elif defined(LUA_NUMBER_FLOAT)
#error "A 'float' lua_Number type is incompatible with this library"
#else
#error "Unknown number type, check LUA_NUMBER_* in luaconf.h"
#endif
  if (b == 0 && !lua_isnumber(L, idx))
    luaL_typerror(L, idx, "number");
  return b;
}

/* Return bit type. */
#define BRET(b)  lua_pushnumber(L, (lua_Number)(SBits)(b)); return 1;

static int bit_tobit(lua_State *L) { BRET(barg(L, 1)) }
static int bit_bnot(lua_State *L) { BRET(~barg(L, 1)) }

#define BIT_OP(func, opr) \
  static int func(lua_State *L) { int i; UBits b = barg(L, 1); \
    for (i = lua_gettop(L); i > 1; i--) b opr barg(L, i); BRET(b) }
BIT_OP(bit_band, &=)
BIT_OP(bit_bor, |=)
BIT_OP(bit_bxor, ^=)

#define bshl(b, n)  (b << n)
#define bshr(b, n)  (b >> n)
#define bsar(b, n)  ((SBits)b >> n)
#define brol(b, n)  ((b << n) | (b >> (32-n)))
#define bror(b, n)  ((b << (32-n)) | (b >> n))
#define BIT_SH(func, fn) \
  static int func(lua_State *L) { \
    UBits b = barg(L, 1); UBits n = barg(L, 2) & 31; BRET(fn(b, n)) }
BIT_SH(bit_lshift, bshl)
BIT_SH(bit_rshift, bshr)
BIT_SH(bit_arshift, bsar)
BIT_SH(bit_rol, brol)
BIT_SH(bit_ror, bror)

static int bit_bswap(lua_State *L)
{
  UBits b = barg(L, 1);
  b = (b >> 24) | ((b >> 8) & 0xff00) | ((b & 0xff00) << 8) | (b << 24);
  BRET(b)
}

static int bit_tohex(lua_State *L)
{
  UBits b = barg(L, 1);
  SBits n = lua_isnone(L, 2) ? 8 : (SBits)barg(L, 2);
  const char *hexdigits = "0123456789abcdef";
  char buf[8];
  int i;
  if (n < 0) { n = -n; hexdigits = "0123456789ABCDEF"; }
  if (n > 8) n = 8;
  for (i = (int)n; --i >= 0; ) { buf[i] = hexdigits[b & 15]; b >>= 4; }
  lua_pushlstring(L, buf, (size_t)n);
  return 1;
}

static const struct luaL_Reg bit_funcs[] = {
  { "tobit",	bit_tobit },
  { "bnot",	bit_bnot },
  { "band",	bit_band },
  { "bor",	bit_bor },
  { "bxor",	bit_bxor },
  { "lshift",	bit_lshift },
  { "rshift",	bit_rshift },
  { "arshift",	bit_arshift },
  { "rol",	bit_rol },
  { "ror",	bit_ror },
  { "bswap",	bit_bswap },
  { "tohex",	bit_tohex },
  { NULL, NULL }
};

/* Signed right-shifts are implementation-defined per C89/C99.
** But the de facto standard are arithmetic right-shifts on two's
** complement CPUs. This behaviour is required here, so test for it.
*/
#define BAD_SAR		(bsar(-8, 2) != (SBits)-2)

bool luabitop_validate(lua_State *L) // originally named as luaopen_bit
{
  UBits b;
  lua_pushnumber(L, (lua_Number)1437217655L);
  b = barg(L, -1);
  if (b != (UBits)1437217655L || BAD_SAR) {  /* Perform a simple self-test. */
    const char *msg = "compiled with incompatible luaconf.h";
#ifdef LUA_NUMBER_DOUBLE
#ifdef _WIN32
    if (b == (UBits)1610612736L)
      msg = "use D3DCREATE_FPU_PRESERVE with DirectX";
#endif
    if (b == (UBits)1127743488L)
      msg = "not compiled with SWAPPED_DOUBLE";
#endif
    if (BAD_SAR)
      msg = "arithmetic right-shift broken";
    luaL_error(L, "bit library self-test failed (%s)", msg);
    return false;
  }
  return true;
}

// LuaBitOp ends here

static int bit_bshift_emulua(lua_State *L)
{
	int shift = luaL_checkinteger(L,2);
	if (shift < 0) {
		lua_pushinteger(L, -shift);
		lua_replace(L, 2);
		return bit_lshift(L);
	}
	else
		return bit_rshift(L);
}

static int bitbit(lua_State *L)
{
	int rv = 0;
	int numArgs = lua_gettop(L);
	for(int i = 1; i <= numArgs; i++) {
		int where = luaL_checkinteger(L,i);
		if (where >= 0 && where < 32)
			rv |= (1 << where);
	}
	lua_settop(L,0);
	BRET(rv);
}

// The function called periodically to ensure Lua doesn't run amok.
static void FCEU_LuaHookFunction(lua_State *L, lua_Debug *dbg) {
	
	if (numTries-- == 0) {

		int kill = 0;

#ifdef WIN32
		// Uh oh
                //StopSound(); //mbg merge 7/23/08
		int ret = MessageBox(hAppWnd, "The Lua script running has been running a long time. It may have gone crazy. Kill it?\n\n(No = don't check anymore either)", "Lua Script Gone Nuts?", MB_YESNO);
		
		if (ret == IDYES) {
			kill = 1;
		}

#else
		fprintf(stderr, "The Lua script running has been running a long time.\nIt may have gone crazy. Kill it? (I won't ask again if you say No)\n");
		char buffer[64];
		while (TRUE) {
			fprintf(stderr, "(y/n): ");
			fgets(buffer, sizeof(buffer), stdin);
			if (buffer[0] == 'y' || buffer[0] == 'Y') {
				kill = 1;
				break;
			}
			
			if (buffer[0] == 'n' || buffer[0] == 'N')
				break;
		}
#endif

		if (kill) {
			luaL_error(L, "Killed by user request.");
			FCEU_LuaOnStop();
		}

		// else, kill the debug hook.
		lua_sethook(L, NULL, 0, 0);
	}
	

}

static void fceu_exec_count_hook(lua_State *L, lua_Debug *dbg) {
	luaL_error(L, "exec_count timeout");
}

static int fceu_exec_count(lua_State *L) {
	int count = (int)luaL_checkinteger(L,1);
	lua_pushvalue(L, 2);
	lua_sethook(L, fceu_exec_count_hook, LUA_MASKCOUNT, count);
	int ret = lua_pcall(L, 0, 0, 0);
	lua_sethook(L, NULL, 0, 0);
	lua_settop(L,0);
	lua_pushinteger(L, ret);
	return 1;
}

#ifdef WIN32
static HANDLE readyEvent, goEvent;
DWORD WINAPI fceu_exec_time_proc(LPVOID lpParameter)
{
	SetEvent(readyEvent);
	WaitForSingleObject(goEvent,INFINITE);
	lua_State *L = (lua_State *)lpParameter;
	lua_pushvalue(L, 2);
	int ret = lua_pcall(L, 0, 0, 0);
	lua_settop(L,0);
	lua_pushinteger(L, ret);
	SetEvent(readyEvent);
	return 0;
}

static void fceu_exec_time_hook(lua_State *L, lua_Debug *dbg) {
	luaL_error(L, "exec_time timeout");
}

static int fceu_exec_time(lua_State *L)
{
	int count = (int)luaL_checkinteger(L,1);

	readyEvent = CreateEvent(0,true,false,0);
	goEvent = CreateEvent(0,true,false,0);
	DWORD threadid;
	HANDLE thread = CreateThread(0,0,fceu_exec_time_proc,(LPVOID)L,0,&threadid);
	SetThreadAffinityMask(thread,1);
	//wait for the lua thread to start
	WaitForSingleObject(readyEvent,INFINITE);
	ResetEvent(readyEvent);
	//tell the lua thread to proceed
	SetEvent(goEvent);
	//wait for the lua thread to finish, but no more than the specified amount of time
	WaitForSingleObject(readyEvent,count);
	
	//kill lua (if it hasnt already been killed)
	lua_sethook(L, fceu_exec_time_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);

	//keep on waiting for the lua thread to come back
	WaitForSingleObject(readyEvent,count);

	//clear the lua thread-killer
	lua_sethook(L, NULL, 0, 0);
	
	CloseHandle(readyEvent);
	CloseHandle(goEvent);
	CloseHandle(thread);

	return 1;
}

#else
static int fceu_exec_time(lua_State *L) { return 0; }
#endif
  
static const struct luaL_reg fceulib [] = {

	{"poweron", fceu_poweron},
	{"softreset", fceu_softreset},
	{"speedmode", fceu_speedmode},
	{"frameadvance", fceu_frameadvance},
	{"pause", fceu_pause},
	{"unpause", fceu_unpause},
	{"exec_count", fceu_exec_count},
	{"exec_time", fceu_exec_time},
	{"setrenderplanes", fceu_setrenderplanes},
	{"message", fceu_message},
	{"lagcount", fceu_lagcount},
	{"lagged", fceu_lagged},
	{"getreadonly", fceu_getreadonly},
	{"setreadonly", fceu_setreadonly},
	{NULL,NULL}
};

static const struct luaL_reg romlib [] = {
	{"readbyte", rom_readbyte},
	{"readbytesigned", rom_readbytesigned},
	{NULL,NULL}
};


static const struct luaL_reg memorylib [] = {

	{"readbyte", memory_readbyte},
	{"readbyterange", memory_readbyterange},
	{"readbytesigned", memory_readbytesigned},
	{"writebyte", memory_writebyte},

	{"register", memory_register},

	{NULL,NULL}
};

static const struct luaL_reg joypadlib[] = {
	{"read", joypad_read},
	{"get", joypad_read},
	{"write", joypad_set},
	{"set", joypad_set},

	{NULL,NULL}
};

static const struct luaL_reg zapperlib[] = {
	{"read", zapper_read},
	{NULL,NULL}
};

static const struct luaL_reg inputlib[] = {
	{"get", input_get},
	{NULL,NULL}
};

static const struct luaL_reg savestatelib[] = {
	{"create", savestate_create},
	{"save", savestate_save},
	{"persist", savestate_persist},
	{"load", savestate_load},

	{NULL,NULL}
};

static const struct luaL_reg movielib[] = {

	{"framecount", movie_framecount},
	{"mode", movie_mode},
	{"rerecordcounting", movie_rerecordcounting},
	{"stop", movie_stop},
	{"active", movie_active},
	{"length", movie_length},
	{"rerecordcount", movie_rerecordcount},
	{"getname", movie_getname},
	{"playbeginning", movie_playbeginning},
//	{"record", movie_record},
//	{"playback", movie_playback},

	{NULL,NULL}

};


static const struct luaL_reg guilib[] = {
	
	{"drawpixel", gui_drawpixel},
	{"drawline", gui_drawline},
	{"drawbox", gui_drawbox},
	{"text", gui_text},

	{"gdscreenshot", gui_gdscreenshot},
	{"gdoverlay", gui_gdoverlay},
	{"transparency", gui_transparency},

	{"register", gui_register},
	
	{"popup", gui_popup},
	{NULL,NULL}

};


void FCEU_LuaFrameBoundary() {

//	printf("Lua Frame\n");

	// HA!
	if (!L || !luaRunning)
		return;

	// Our function needs calling
	lua_settop(L,0);
	lua_getfield(L, LUA_REGISTRYINDEX, frameAdvanceThread);
	lua_State *thread = lua_tothread(L,1);	

	// Lua calling C must know that we're busy inside a frame boundary
	frameBoundary = TRUE;
	frameAdvanceWaiting = FALSE;

	numTries = 1000;
	int result = lua_resume(thread, 0);
	
	if (result == LUA_YIELD) {
		// Okay, we're fine with that.
	} else if (result != 0) {
		// Done execution by bad causes
		FCEU_LuaOnStop();
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, frameAdvanceThread);
		
		// Error?
#ifdef WIN32
                //StopSound();//StopSound(); //mbg merge 7/23/08
		MessageBox( hAppWnd, lua_tostring(thread,-1), "Lua run error", MB_OK | MB_ICONSTOP);
#else
		fprintf(stderr, "Lua thread bombed out: %s\n", lua_tostring(thread,-1));
#endif

	} else {
		FCEU_LuaStop();
		FCEU_DispMessage("Script died of natural causes.\n");
	}

	// Past here, the nes actually runs, so any Lua code is called mid-frame. We must
	// not do anything too stupid, so let ourselves know.
	frameBoundary = FALSE;

	if (!frameAdvanceWaiting) {
		FCEU_LuaOnStop();
	}

}

/**
 * Loads and runs the given Lua script.
 * The emulator MUST be paused for this function to be
 * called. Otherwise, all frame boundary assumptions go out the window.
 *
 * Returns true on success, false on failure.
 */
int FCEU_LoadLuaCode(const char *filename) {
	if (filename != luaScriptName)
	{
		if (luaScriptName) free(luaScriptName);
		luaScriptName = strdup(filename);
	}

	//stop any lua we might already have had running
	FCEU_LuaStop();

	if (!L) {
		
		#ifdef WIN32
			HMODULE test = LoadLibrary("lua5.1.dll");
			if(!test)
			{
				FCEUD_PrintError("Couldn't initialize lua system due to failure loading lua5.1.dll");
				return 0;
			}
			FreeLibrary(test);
		#endif

		L = lua_open();
		luaL_openlibs(L);

		luaL_register(L, "FCEU", fceulib);
		luaL_register(L, "memory", memorylib);
		luaL_register(L, "rom", romlib);
		luaL_register(L, "joypad", joypadlib);
		luaL_register(L, "zapper", zapperlib);
		luaL_register(L, "input", inputlib);
		luaL_register(L, "savestate", savestatelib);
		luaL_register(L, "movie", movielib);
		luaL_register(L, "gui", guilib);
		luaL_register(L, "bit", bit_funcs); // LuaBitOp library
		lua_settop(L, 0);		// clean the stack, because each call to luaL_register leaves a table on top

		// old bit operation functions
		lua_register(L, "AND", bit_band);
		lua_register(L, "OR", bit_bor);
		lua_register(L, "XOR", bit_bxor);
		lua_register(L, "SHIFT", bit_bshift_emulua);
		lua_register(L, "BIT", bitbit);

		luabitop_validate(L);

		lua_newtable(L);
		lua_setglobal(L,"emu");
		lua_getglobal(L,"emu");
		lua_newtable(L);
		lua_setfield(L,-2,"OnClose");

		
		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, memoryWatchTable);
		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, memoryValueTable);
	}

	// We make our thread NOW because we want it at the bottom of the stack.
	// If all goes wrong, we let the garbage collector remove it.
	lua_State *thread = lua_newthread(L);
	
	// Load the data	
	int result = luaL_loadfile(L,filename);

	if (result) {
#ifdef WIN32
		// Doing this here caused nasty problems; reverting to MessageBox-from-dialog behavior.
                //StopSound();//StopSound(); //mbg merge 7/23/08
		MessageBox(NULL, lua_tostring(L,-1), "Lua load error", MB_OK | MB_ICONSTOP);
#else
		fprintf(stderr, "Failed to compile file: %s\n", lua_tostring(L,-1));
#endif

		// Wipe the stack. Our thread
		lua_settop(L,0);
		return 0; // Oh shit.
	}
#ifdef WIN32
	AddRecentLuaFile(filename); //Add the filename to our recent lua menu
#endif
	
	// Get our function into it
	lua_xmove(L, thread, 1);
	
	// Save the thread to the registry. This is why I make the thread FIRST.
	lua_setfield(L, LUA_REGISTRYINDEX, frameAdvanceThread);
	

	// Initialize settings
	luaRunning = TRUE;
	skipRerecords = FALSE;

	wasPaused = FCEUI_EmulationPaused();
	if (wasPaused) FCEUI_ToggleEmulationPause();

	// And run it right now. :)
	//FCEU_LuaFrameBoundary();

	// Set up our protection hook to be executed once every 10,000 bytecode instructions.
	//lua_sethook(thread, FCEU_LuaHookFunction, LUA_MASKCOUNT, 10000);

	// We're done.
	return 1;
}

/**
 * Equivalent to repeating the last FCEU_LoadLuaCode() call.
 */
void FCEU_ReloadLuaCode()
{
	if (!luaScriptName)
		FCEU_DispMessage("There's no script to reload.");
	else
		FCEU_LoadLuaCode(luaScriptName);
}


/**
 * Terminates a running Lua script by killing the whole Lua engine.
 *
 * Always safe to call, except from within a lua call itself (duh).
 *
 */
void FCEU_LuaStop() {

	//already killed
	if (!L) return;

	//execute the user's shutdown callbacks
	//onCloseCallback
	lua_getglobal(L, "emu");
	lua_getfield(L, -1, "OnClose");
	lua_pushnil(L);
	while (lua_next(L, -2) != 0)
	{
		lua_call(L,0,0);
	}

	//sometimes iup uninitializes com
	//MBG TODO - test whether this is really necessary. i dont think it is
	#ifdef WIN32
	CoInitialize(0);
	#endif

	//lua_gc(L,LUA_GCCOLLECT,0);


	lua_close(L); // this invokes our garbage collectors for us
	L = NULL;
	FCEU_LuaOnStop();
}

/**
 * Returns true if there is a Lua script running.
 *
 */
int FCEU_LuaRunning() {
	return L && luaRunning;
}


/**
 * Returns true if Lua would like to steal the given joypad control.
 */
//int FCEU_LuaUsingJoypad(int which) {
//	return lua_joypads_used & (1 << which);
//}

//adelikat: TODO: should I have a FCEU_LuaUsingJoypadFalse?

/**
 * Reads the buttons Lua is feeding for the given joypad, in the same
 * format as the OS-specific code.
 *
 * It may force set or force clear the buttons. It may also simply
 * pass the input along or invert it. The act of calling this
 * function will reset everything back to pass-through, though.
 * Generally means don't call it more than once per frame!
 */
uint8 FCEU_LuaReadJoypad(int which, uint8 joyl) {
	joyl = (joyl & luajoypads1[which]) | (~joyl & luajoypads2[which]);
	luajoypads1[which] = 0xFF;
	luajoypads2[which] = 0x00;
	return joyl;
}

//adelikat: Returns the buttons that will be specifically set to false (as opposed to on or nil)
//This will be used in input.cpp to &(and) against the input to override a button with a false value.  This is a work around to allow 3 conditions to be sent be lua, true, false, nil
//uint8 FCEU_LuaReadJoypadFalse(int which) {
//	lua_joypads_used_false &= ~(1 << which);
//	return lua_joypads_false[which];
//}

/**
 * If this function returns true, the movie code should NOT increment
 * the rerecord count for a load-state.
 *
 * This function will not return true if a script is not running.
 */
int FCEU_LuaRerecordCountSkip() {
	return L && luaRunning && skipRerecords;
}


/**
 * Given an 8-bit screen with the indicated resolution,
 * draw the current GUI onto it.
 *
 * Currently we only support 256x* resolutions.
 */
void FCEU_LuaGui(uint8 *XBuf) {

	if (!L || !luaRunning)
		return;

	// First, check if we're being called by anybody
	lua_getfield(L, LUA_REGISTRYINDEX, guiCallbackTable);
	
	if (lua_isfunction(L, -1)) {
		// We call it now
		numTries = 1000;
		int ret = lua_pcall(L, 0, 0, 0);
		if (ret != 0) {
#ifdef WIN32
			//StopSound();//StopSound(); //mbg merge 7/23/08
			MessageBox(hAppWnd, lua_tostring(L, -1), "Lua Error in GUI function", MB_OK);
#else
			fprintf(stderr, "Lua error in gui.register function: %s\n", lua_tostring(L, -1));
#endif
			// This is grounds for trashing the function
			lua_pushnil(L);
			lua_setfield(L, LUA_REGISTRYINDEX, guiCallbackTable);
		
		}
	}

	// And wreak the stack
	lua_settop(L, 0);

	if (gui_used == GUI_CLEAR)
		return;

	gui_used = GUI_USED_SINCE_LAST_FRAME;

	int x,y;

	if (transparency == 4) // wtf?
		return;
	
	// direct copy
	if (transparency == 0) {
		for (y = 0; y < 256; y++) {
			for (x=0; x < 256; x++) {
				if (gui_data[y*256+x] != GUI_COLOUR_CLEAR)
					XBuf[y*256 + x] =  gui_data[y*256+x];
			}
		}
	} else {
		for (y = 0; y < 256; y++) {
			for (x=0; x < 256; x++) {
				if (gui_data[y*256+x] != GUI_COLOUR_CLEAR) {
					uint8 rg, gg, bg,  rx, gx, bx,  r, g, b;
					FCEUD_GetPalette(gui_data[y*256+x], &rg, &gg, &bg);
					FCEUD_GetPalette(    XBuf[y*256+x], &rx, &gx, &bx);
					r = (rg * (4 - transparency) + rx * transparency) / 4;
					g = (gg * (4 - transparency) + gx * transparency) / 4;
					b = (bg * (4 - transparency) + bx * transparency) / 4;
					XBuf[y*256+x] = gui_colour_rgb(r, g, b);
				}
			}
		}
	}

	return;
}

