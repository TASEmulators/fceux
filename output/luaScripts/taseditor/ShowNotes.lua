---------------------------------------------------------------------------
-- Showing Markers' Notes on screen
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * taseditor.getmarker()
-- * taseditor.getnote()
-- * taseditor.getselection()
---------------------------------------------------------------------------
-- Usage:
-- Run the script, unpause emulation (or simply Frame Advance once).
-- Now you can observe Marker Notes not only in TAS Editor window,
-- but also in FCEUX main screen.
-- This script automatically divides long Notes into several lines of text
---------------------------------------------------------------------------

-- Custom function for word-wrapping long lines of text
function DisplayText(x, y, text, max_num_chars)
	while string.len(text) >= max_num_chars do
		-- Find last spacebar within first max_num_chars of text
		last_spacebar = string.find(string.reverse(string.sub(text, 1, max_num_chars)), " ");
		if (last_spacebar ~= nil) then
			-- Output substring up to the spacebar
			substring_len = max_num_chars - last_spacebar;
		else
			-- No spacebar found within first max_num_chars of the string
			-- output substring up to the first spacebar (this substring will have length > max_num_chars)
			substring_len = string.find(text, " ");
			if (substring_len == nil) then
				-- No spacebars found at all, output whole string
				substring_len = string.len(text);
			else
				-- Don't output the spacebar
				substring_len = substring_len - 1;
			end
		end
		gui.text(x, y, string.sub(text, 1, substring_len));
		text = string.sub(text, substring_len + 2);
		-- Next line
		y = y + 8;
	end
	gui.text(x, y, text);
end

function ShowNotes()
	if taseditor.engaged() then
		-- Take Marker near Playback cursor and display its Note in upper half of the screen
		playback_position = movie.framecount();
		note = taseditor.getnote(taseditor.getmarker(playback_position));
		DisplayText(1, 9, note, 50);
		-- Take Marker near Selection cursor and display its Note in lower half of the screen
		current_selection = taseditor.getselection();
		if (current_selection ~= nil) then
			selection_position = current_selection[1];
			note = taseditor.getnote(taseditor.getmarker(selection_position));
			DisplayText(1, 190, note, 50);
		end
	else
		gui.text(1, 9, "TAS Editor is not engaged.");
	end
end

taseditor.registerauto(ShowNotes)

