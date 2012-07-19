---------------------------------------------------------------------------
-- Swap 1P and 2P buttons at Selected frames
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * taseditor.getselection()
-- * taseditor.clearinputchanges()
-- * taseditor.getinput()
-- * taseditor.submitinputchange()
-- * taseditor.applyinputchanges()
---------------------------------------------------------------------------
-- Usage:
-- Use the script when you want to exchange recorded input between joypads.
-- Run the script, unpause emulation (or simply Frame Advance once).
-- Now you can select several frames of input data and then
-- press "Swap 1P/2P" button to swap Input of 1P and 2P in selected frames.
-- You can easily modify the script to swap any other joypads.
---------------------------------------------------------------------------

function swap()
	selection_table = taseditor.getselection();
	if (selection_table ~= nil) then
		taseditor.clearinputchanges();
		for i = 1, #selection_table do
			selected_frame = selection_table[i];
			joypad1data = taseditor.getinput(selected_frame, 1);
			joypad2data = taseditor.getinput(selected_frame, 2);
			taseditor.submitinputchange(selected_frame, 1, joypad2data);
			taseditor.submitinputchange(selected_frame, 2, joypad1data);
		end
		taseditor.applyinputchanges("Swap 1P/2P");
	end
end

taseditor.registermanual(swap, "Swap 1P/2P");


