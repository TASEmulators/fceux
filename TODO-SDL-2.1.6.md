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

PROJECT STUFF
=============
	* Contact debian package management
	* Create a sane release procedure script that generates a release ready tarball
	* Create markdown file of README

