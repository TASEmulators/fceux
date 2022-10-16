#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

#include <pybind11/embed.h> 
namespace py = pybind11;


#include "movie.h"
#include "state.h"
#include "types.h"
#include "fceupython.h"

// Are we running any code right now?
static char* pythonScriptName = NULL; 
bool pythonRunning = false;

// True if there's a thread waiting to run after a run of frame-advance.
static std::atomic_bool frameAdvanceWaiting = false;

// True if there's a thread waiting to run after a run of frame-advance.
static std::atomic_bool inFrameBoundry = false;

std::mutex mtx; 
std::condition_variable cv;


// Look in fceu.h for macros named like JOY_UP to determine the order.
static const char *button_mappings[] = {
	"A", "B", "select", "start", "up", "down", "left", "right"
};

// Our joypads.
static uint8 pythonjoypads1[4]= { 0xFF, 0xFF, 0xFF, 0xFF }; //x1
static uint8 pythonjoypads2[4]= { 0x00, 0x00, 0x00, 0x00 }; //0x
/* Crazy logic stuff.
	11 - true		01 - pass-through (default)
	00 - false		10 - invert					*/


static void emu_frameadvance() 
{
	// Can't call if a frameAdvance is already waiting
	if (frameAdvanceWaiting)
		return;

	frameAdvanceWaiting = true;

	// Notify main thread it can advance the frame
	cv.notify_all();

	// Wait until inFrameBoundry is true to continue python script
	std::unique_lock<std::mutex> lock(mtx);
	inFrameBoundry = false;
	cv.wait(lock, [] { return bool(inFrameBoundry); });
}

static int emu_framecount()
{
	return FCEUMOV_GetFrame(); 
}

// dict joypad.read(int which = 1)
//
// Reads the joypads as inputted by the user.
static py::dict joy_get_internal(int player, bool reportUp, bool reportDown) 
{
	// Use the OS-specific code to do the reading.
	extern SFORMAT FCEUCTRL_STATEINFO[];
	uint8 buttons = ((uint8*) FCEUCTRL_STATEINFO[1].v)[player - 1];

	py::dict input;
	
	for (int i = 0; i < 8; i++) {
		bool pressed = (buttons & (1<<i)) != 0;
		if ((pressed && reportDown) || (!pressed && reportUp)) {
			input[button_mappings[i]] = py::bool_(pressed);
		}
	}

	return input;
}

static py::dict joypad_get(int player) 
{
	return joy_get_internal(player, true, true);
}

// joypad.getdown(player)
// returns a dict of every game button that is currently held
static py::dict joypad_getdown(int player)
{
	return joy_get_internal(player, false, true);
}

// joypad.getup(player)
// returns a dict of every game button that is not currently held
static py::dict joypad_getup(int player) 
{
	return joy_get_internal(player, true, false);
}

static void joypad_set(int player, py::dict input) 
{
	// Set up for taking control of the indicated controller
	pythonjoypads1[player-1] = 0xFF; // .1  Reset right bit
	pythonjoypads2[player-1] = 0x00; // 0.  Reset left bit

	for (int i=0; i < 8; i++) {
		if (input.contains<std::string>(button_mappings[i])) {
			auto buttonIn = input[button_mappings[i]];
			auto buttonInType = py::type::of(buttonIn);

			bool buttonBool = false;
			bool buttonIsString = false;

			if (py::isinstance<py::bool_>(buttonIn)) {
				buttonBool = buttonIn.cast<bool>();
			} else if (py::isinstance<py::str>(buttonIn)) {
				buttonIsString = true;
			} else {
				continue;
			}

			if (buttonBool)
				pythonjoypads2[player-1] |= 1 << i;
			if (!buttonBool || buttonIsString)
				pythonjoypads1[player-1] &= ~(1 << i);
		}
	}
}

PYBIND11_EMBEDDED_MODULE(emu, m) 
{
	m.def("frameadvance", emu_frameadvance);
	m.def("framecount", emu_framecount);
}

PYBIND11_EMBEDDED_MODULE(joypad, m)
{
	m.def("get", joypad_get);
	m.def("read", joypad_get);
	m.def("getdown", joypad_getdown);
	m.def("readdown", joypad_getdown);
	m.def("getup", joypad_getup);
	m.def("readup", joypad_getup);
	m.def("set", joypad_set);
	m.def("write", joypad_set);
}

void FCEU_PythonFrameBoundary() 
{
	if(!pythonRunning)
		return; 
	
	// Notify Python thread the main thread is in the frame boundry
	inFrameBoundry = true;
	cv.notify_all();


	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [] { return bool(frameAdvanceWaiting); });
	frameAdvanceWaiting = false;
}

void pythonStart(std::string filename) 
{
	// Wait until in_frame_boundry is true to start python script
	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [] { return bool(inFrameBoundry); });
	lock.unlock();

	// Start evaluating the python file
	py::gil_scoped_acquire acquire;
	py::eval_file(filename); 
}

/**
 * Loads and runs the given Python script.
 * The emulator MUST be paused for this function to be
 * called. Otherwise, all frame boundary assumptions go out the window.
 *
 * Returns true on success, false on failure.
 */
void FCEU_LoadPythonCode(const char* filename) 
{
	if (filename != pythonScriptName)
	{
		if (pythonScriptName)
			free(pythonScriptName);
		pythonScriptName = strdup(filename); 
	}

	// Start interpreter
	pythonRunning = true;
	py::initialize_interpreter();

	// gil_scoped_release created on heap to not be destroyed on leaving FCEU_LoadPythonCode scope
	py::gil_scoped_release* release = new py::gil_scoped_release;
	(void)release; // supress unused variable warning on release

	std::thread(pythonStart, std::string(filename)).detach();

	FCEU_PythonFrameBoundary();
}

/**
 * Reads the buttons Python is feeding for the given joypad, in the same
 * format as the OS-specific code.
 *
 * It may force set or force clear the buttons. It may also simply
 * pass the input along or invert it. The act of calling this
 * function will reset everything back to pass-through, though.
 * Generally means don't call it more than once per frame!
 */
uint8 FCEU_PythonReadJoypad(int player, uint8 joyl)
{
	joyl = (joyl & pythonjoypads1[player]) | (~joyl & pythonjoypads2[player]);
	pythonjoypads1[player] = 0xFF;
	pythonjoypads2[player] = 0x00;
	return joyl;
}


/** 
 * Terminates a running Python scripts by killing the whole Python Interpretor.
 */
void FCEU_PythonStop() 
{
	if (!pythonRunning)
		return;
	
	// Stop interpretor
	pythonRunning = false;
	py::finalize_interpreter();
}