#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

#include <pybind11/embed.h> 
namespace py = pybind11;

#include "fceupython.h"

#define SetCurrentDir chdir

// Are we running any code right now?
static char* pythonScriptName = NULL; 
bool pythonRunning = false;

// True if there's a thread waiting to run after a run of frame-advance.
static std::atomic_bool frameAdvanceWaiting = false;

// True if there's a thread waiting to run after a run of frame-advance.
static std::atomic_bool inFrameBoundry = false;

std::mutex mtx; 
std::condition_variable cv;

// Python thread object
// std::thread python_thread;


void FCEU_PythonFrameBoundary() 
{
	std::cout << "in FCEU_PythonFrameBoundary" << std::endl;

	if(!pythonRunning)
		return; 
	
	// Notify Python thread the main thread is in the frame boundry
	inFrameBoundry = true;
	cv.notify_all();


	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [] { return bool(frameAdvanceWaiting); });
	frameAdvanceWaiting = false;
}

void emu_frameadvance() 
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

PYBIND11_EMBEDDED_MODULE(emu, m) 
{
	m.def("frameadvance", &emu_frameadvance);
	// m.def("framecount", [] { return emu_framecount; });
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

	std::thread(pythonStart, std::string(filename)).detach();

	FCEU_PythonFrameBoundary();
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