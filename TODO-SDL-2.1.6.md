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

TESTING
=======
	TODO
	----


