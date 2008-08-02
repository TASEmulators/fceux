-- This script shows a simple button and click event

-- Include our help script to load iup and take care of exit
require("auxlib");

function testiup()
	-- Our callback function
	function someAction(self, a) gui.text(10,10,"pressed me!"); end;
	
	-- Create a button
	myButton = iup.button{title="Button Textwtf"};
	
	-- Set the callback
	myButton.action = someAction;
	
	-- Create the dialog
	dialogs = dialogs + 1;
	handles[dialogs] = iup.dialog{ myButton, title="IupDialog Title"; };
	
	-- Show the dialog (the colon notation is equal 
	-- to calling handles[dialogs].show(handles[dialogs]); )
	handles[dialogs]:show();

end

testiup();

while (true) do -- prevent script from exiting
	FCEU.frameadvance();
end;