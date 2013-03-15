---------------------------------------------------------------------------
-- Invert Selection
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * taseditor.getselection()
-- * taseditor.setselection()
---------------------------------------------------------------------------
-- Usage:
-- Run the script, unpause emulation (or simply Frame Advance once).
-- Now you can press "Invert Selection" button to invert current Selection.
-- Previously selected frames become deselected, all other frames become selected.
---------------------------------------------------------------------------

function display_selection()
	if (taseditor.engaged()) then
		selection_table = taseditor.getselection();
		if (selection_table ~= nil) then
			selection_size = #selection_table;
			gui.text(0, 10, "Selection: " .. selection_size .. " rows");
		else
			gui.text(0, 10, "Selection: no");
		end
	else
		gui.text(1, 9, "TAS Editor is not engaged.");
	end
end

function invert_selection()
	old_sel = taseditor.getselection();
	new_sel = {};

	-- Select all
	movie_size = movie.length();
	for i = 0, movie_size do
		new_sel[i + 1] = i;
	end

	if (selection_table ~= nil) then
		-- Substract old selection from new selection set
		for i = #old_sel, 1, -1 do
			selected_frame = old_sel[i];
			-- we're taking advantage of the fact that "old_sel" is sorted in ascending order
			-- so we can safely use table.remove to remove indexes from the end to the beginning
			table.remove(new_sel, selected_frame + 1);
		end
	end

	-- Apply new set to TAS Editor selection
	taseditor.setselection(new_sel);
end

taseditor.registerauto(display_selection);
taseditor.registermanual(invert_selection, "Invert Selection");

