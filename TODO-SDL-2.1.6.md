BUGS
====
	Save/Load State Console Print Bug
	---------------------------------
	* ie:

	`	 -select state-
		State -777722344 saved `
	
	* If the root of the bug cannot be determined by release time, just #ifdef the print statement for win32
	
	Clip sides bug in BOTH WINDOWED AND FULLSCREEN mode
	---------------------------------------------------
	* Go to game genie with clip sides enabled or save a state to see
	* Ensure no clip sides is only being enabled during fullscreen

	DONE! Gamepad dialog and others segfault on Ubuntu 10.04 (and older GTK versions)
	---------------------------------------------------------------------------------
	* Not planning on using legacy code here (especially with Ubuntu 12.04 on its way) but perhaps 
	a messagebox can be displayed recommending the user to upgrade their GTK version to prevent
	the frustrating segfault.  Segfaults need to be avoided like the plague.

	* This is fixed in r2447.  FceuX will print a warning to the console that the GTK version is old, and 
	provide instructions on how to configure the gamepad.
		* Maybe we should also provide a GTK MessageBox with this info?

FEATURES
========
	XDG Standardization of Config
	-----------------------------
	* fceux currently stores stuff to ~/.fceux by default
	* FD.O/XDG recommends default to be ~/.config/fceux (and to check $XDG_whatever)
	* If the XDG folder exists, use it.  If NO folder exists, use the XDG folder.
	* If .fceux exists, display a warning message but use .fceux
	* Don't move around people's config files without asking (or at all in this case)

input.cpp
=========
	* This code is a fscking mess!  However, now is not the time to clean it.  Perhaps set a goal to clean this file up for
	2.1.7, but cleaning this for 2.1.6 is not realistic (and would break too many things and require more testing time)

TESTING
=======
	Distro Test
	-----------
	* Arch Linux 64 bit - compiles; runs
	* Debian 6
	* Ubuntu 10.04 / 11.04
	* ???

	Arch Linux 64 bit
	-----------------
	* Flawless installation from fceux-svn in [aur].  No issues other than the ones already 
	noted.
	* Dpendencies: sdl, gtk2, scons libz

	Ubuntu 10.04
	------------
	* Compiles/builds without issue.
	* Build deps: libsdl1.2-dev, scons, libgtk2.0-dev, zlib-dev, build-essentials
	* No issues found in gameplay.
	* DONE Some dialogs with segfault FCEUX due to an older version of GTK.  Perhaps we can detect the old version of GTK and just prevent the dialog from being opened so the segfault doesnt occur? (bug added).
		* DONE Wrote CheckGTKVersion(), which will be used like CheckGTKVersion(2, 24) to check the GTK version before segfaulting on dialogs
		* DONE Implement a check for the dialogs that would bomb -- (a hook in init; makes all items under Options inacessible besides fullscreen if under 2.24)

	Ubuntu 12.04 Beta
	-----------------
	* Compiles/builds without issue.
	* GTK3 preferred for Unity
	* Build deps: libsdl1.2-dev, scons, libgtk-3-dev, zlib-dev, build-essentials
	* No issues AFAICS

	Fedora Core 16
	--------------
	* Dependencies: subversion, scons, SDL-devel, gtk2-devel/gtk3-devel
	* BUG Scons issues?

	openSUSE 12.1
	-------------
	* scons bombs out because can't find libgtk; I can't find gtk headers package

PROJECT STUFF
=============
	* Contact debian package management
	* Create a sane release procedure script that generates a release ready tarball
	* DONE - Create markdown file of README
	* Locate important features and summarize them.

