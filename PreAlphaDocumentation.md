Contributers: Artur Bekker, Steven Phang, Quinlan Daily


Architectural Elements
External Interface
Realistic Loading of ROMs
	Displaying ROM files as Cartridges
	Users will be able to go from the Famicom’s original loading screen to the Virtual Famicom Cartridge loading screen. This was shown visually in the prior design mockup
	Before loading any of the Cartridges though, the user will be greeted by a browse for file dialog box which will allow the user to choose a folder on their computer that contains their ROM files. This path will be used later on
	If there are ROM files in the file path on the computer, then cartridges will display on the Virtual loading sequence
Clicking and dragging Cartridges
	Previously the user was able to specify the ROMs that would be used to display the Cartridges. Now the user will be able to interface with the displayed and loaded cartridges. This means that more functionality needs to be implemented into the project to allow users to interact with these objects
	When not loaded into the Virtual Famicom, Cartridges need to be able to be clicked and then dragged, this will be described in more detail later, but essentially functions to register clicks from a mouse or gamepad need to be recognized and then while recognizing this input, when the cursor of the user moves, the cartridges should move too
	In addition, when cartridges are inside of the virtual Famicom console, functionality to click and drag the “loaded” cartridge from the Famicom will need to be present in order to unload a cartridge. Afterwards, when let go, the cartridge should join the rest of the cartridges if they were loaded
PowerOn and PowerOff functions for Famicom
	Switches need to be present on the Famicom. A Power switch needs to be present on the virtual console in order to emulate the realistic powering sequence of the Famicom. The user should be able to click the Power switch to ‘turn on’ the Famicom and ‘turn off’ the Famicom
	This means functionality to recognize a click similar to the cartridge dragging needs to be implemented. When the power button is pressed it will act like it is pressed in and when power off it will need to be clicked again, exactly like the Famicom
Empty Screen Interface
	When no cartridge is inside of the virtual Famicom, and the Famicom is powered on, the empty screen interface needs to be displayed. It will seem like a game is loaded but instead this is just the defaulted screen the original Famicom uses when no cartridge is present.
	Functionality in Internal Systems will need to be set up to consider the case that no ROMs are loaded when realistically loading ROMs is enabled.

Persistent State
	For displaying the ROM files as Cartridges, a couple of functions need to be called. One of the functions that needs to be called will be our own function called SearchForROMS(string filepath). This function is used when the user interacts with the loading of the ROMs and the realistic option setting is enabled. 
	Once a ROM is ejected from the Famicom on the GUI in the External System, a function ejectROM() will execute. This will call the LUA script in the Internal System which plays the “Set Disk Card” screen until a ROM is inserted back into the Famicom.
	Once a ROM is inserted into the Famicom on the GUI in the External System, a function insertROM(string filepath) containing the file path to the ROM will execute. This will call the LUA script in the Internal System which plays the loading screen for a short period of time until the LUA script begins proper emulation.

Internal Systems
For use case of having a Set Disk Card Screen and Loading Screen
A LUA script using built-in emulator functions, along with additional created functions will be used. The basic architecture is as shown below. 
   if (emu.emulating()) 	//check to make sure that the emulator is running before executing 
				//any LUA scripting
	when disk is ejected:
		movie.play() //function loads in video of Set Disk Card screen
		*framecount variable initialized to 0
		while movie.active()
			increment framecount
			emu.frameadvance() //movie continues to play
			if framecount == movie.length()
				framecount = 0
				movie.replay()   //if  user has not inserted disk and movie ends,        						  //movie replays
	when disk inserted:
		movie.stop()
		movie.close() //disk loading screen ends and exits
		gd.createFromPng():gdStr()    //loads in PNG of loading screen and converts to        						// string format
		gui.gdoverlay()		//draws loading screen onto screen
		for 10 seconds / arbitrary time to simulate disk loading:
			emu.frameadvance() //loading screen image stays on screen
		emu.loadrom() 		//actually loads rom into emulator
		*function to be created to skip first few seconds of game running, as it will flash   	                                 
		*the set disk and loading screen very briefly. This is so it is not shown twice, and 
		*will run from where the game actually begins.
		while (emu.emulating())
			emu.frameadvance() //runs game as normal once loading is commenced

Additional LUA scripting will be done for the other use cases, including modified versions of the Set Disk Card screen implementation for powering on the Famicom on the GUI without inserting a ROM, inserting a ROM without powering on the Famicom, and modifications depending on whether a ROM is being ejected to swap out with a different ROM or is just being switched to the other side and inserted back in.

Information Handling

Communication
Truth Values for States
	In order to appropriately consider the states that will be communicating with each other, boolean values that keep track of the states might be used in different functions in order to quickly check where the external interfaces are displaying
Strings for File Paths and ROMs
	String data structures will be used in order to hold the specific file pathways that are used inside of the emulator to realize ROM files and their locations
FCEUX API to Communicate with Scripts and GUI
	The FCEUX API will be used to help communication to flow between the GUI components and the scripts in this project
The overall flow of communication is as follows:
	GUI components ←→ FCEUX API ←→ LUA scripting 

Integrity & Resilience
On the GUI, if a game is inserted into the Famicom but the power button is not pressed, the ROM will be inserted into a buffer and emulation will be paused until the power button is pressed. This prevents errors of the game starting up before it should. In the reverse scenario, if the power button is pressed before a ROM is inserted, a LUA script will just play the Set Disk Card screen until the ROM is placed.

The Internal Systems/LUA Scripting will ensure that FCEUX is actually emulating before running any code, thus ensuring resilience to program crashes by attempting to execute functions that would otherwise be faulty.

There will also be other checks throughout the code to make sure that before important communication between systems happens, all systems that are being changed or accessed are active and no errors or accidental interference occur.
