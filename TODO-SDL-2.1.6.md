BUGS
====
Save/Load State Console Print Bug
---------------------------------
* ie:
	 -select state-
		State -777722344 saved 
	
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

DISTRO TESTING
==============
Summary
-------
* Out of box working distros:
	* Arch Linux
	* Ubuntu 10.04
	* Ubuntu 12.04
* Works, but needs patching:
	* Fedora Core 16
* Couldn't get working
	* openSUSE
* Untested:
	* Linux Mint
	* Debian


Arch Linux 64 bit
-----------------
* Flawless installation from fceux-svn in [aur].  No issues other than the ones already noted.
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
* Dependencies: subversion, scons, SDL-devel, gtk2-devel/gtk3-devel, gcc, [glib-devel?]
* You also need to install the "Development Tools" group:

		yum groupinstall "Development Tools"

* scons bombs out (another missing dependency?

		/usr/bin/ld: src/lua/src/loadlib.o: undefined reference to symbol 'dlclose@@GLIBC_2.2.5'
		/usr/bin/ld: note: 'dlclose@@GLIBC_2.2.5' is defined in DSO /lib64/libdl.so.2 so try adding it to the linker command line
		/lib64/libdl.so.2: could not read symbols: Invalid operation
		collect2: ld returned 1 exit status
		scons: *** [src/fceux] Error 1
		scons: building terminated because of errors.

	* This was fixed by forcing "-ldl" to the linker command line.  This can be done by adding the following
	to line 92 of the SConstruct:

		env.Append(LINKFLAGS = ["-ldl"])
	
	* TODO Answer some questions:
		1. Is -ldl necessary everywhere?
		2.  Should we always use it?  If not what can we test for for when to use it?
	
	* Check the code circa line 92 in SConstruct for a Fedora resolution.

	* Tested with GTK2, runs smoothly in VirtualBox.

openSUSE 12.1
-------------
* scons bombs out because can't find libgtk; I can't find gtk headers package

PROJECT STUFF
=============
* Contact debian package management about inclusion
* Create a sane release procedure script that generates a release ready tarball
* DONE - Create markdown file of README
* Locate important features/changes new to 2.1.6 and summarize them.

