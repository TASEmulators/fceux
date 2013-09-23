---------------------------------------------------------------------------
-- Recording Input while Rewinding Playback frame-by-frame
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * joypad.getimmediate()
-- * taseditor.getrecordermode()
-- * taseditor.getsuperimpose()
-- * taseditor.getinput()
-- * taseditor.setinput()
-- * taseditor.clearinputchanges()
-- * taseditor.applyinputchanges()
---------------------------------------------------------------------------
-- Usage:
-- Run the script, unpause emulation (or simply Frame Advance once).
-- Now you can hold some joypad buttons and press "Rewind Frame" hotkey
-- to Record those buttons into PREVIOUS frame.
-- Try using this crazy method alongside with Frame Advance Recording.
-- This script supports multitracking and superimpose. Doesn't support Patterns.
---------------------------------------------------------------------------

-- This function reads joypad input table and converts it to single byte
function GetCurrentInputByte(player)
	input_byte = 0;
	input_table = joypad.getimmediate(player);
	if (input_table ~= nil) then
		-- A B select start up down left right
		if (input_table.A) then input_byte = OR(input_byte, 1) end;
		if (input_table.B) then input_byte = OR(input_byte, 2) end;
		if (input_table.select) then input_byte = OR(input_byte, 4) end;
		if (input_table.start) then input_byte = OR(input_byte, 8) end;
		if (input_table.up) then input_byte = OR(input_byte, 16) end;
		if (input_table.down) then input_byte = OR(input_byte, 32) end;
		if (input_table.left) then input_byte = OR(input_byte, 64) end;
		if (input_table.right) then input_byte = OR(input_byte, 128) end;
	end
	return input_byte;
end

function reversed_recorder()
	if taseditor.engaged() then
		playback_position = movie.framecount();
		if (playback_position == (playback_last_position - 1)) then
			-- Playback cursor moved up 1 frame, probably Rewind was used this frame
			if (not movie.readonly()) then
				-- Recording on
				recording_mode = taseditor.getrecordermode();
				superimpose = taseditor.getsuperimpose();
				taseditor.clearinputchanges();
				name = "Record";
				if (recording_mode == "All") then
					-- Recording all 4 joypads
					for target_player = 1, 4 do
						new_joypad_input = GetCurrentInputByte(target_player);
						old_joypad_input = taseditor.getinput(playback_position, target_player);
						-- Superimpose with old input if needed
						if (superimpose == 1 or (superimpose == 2 and new_joypad_input == 0)) then
							new_joypad_input = OR(new_joypad_input, old_joypad_input);
						end
						-- Add joypad info to name
						if (new_joypad_input ~= old_joypad_input) then
							name = name .. "(" .. target_player .. "P)";
						end
						taseditor.submitinputchange(playback_position, target_player, new_joypad_input);
					end
					-- Write to movie data
					taseditor.applyinputchanges(name);
				else
					-- Recording target_player using 1P keys
					new_joypad_input = GetCurrentInputByte(1);
					target_player = 1;
					if (recording_mode == "2P") then target_player = 2 end;
					if (recording_mode == "3P") then target_player = 3 end;
					if (recording_mode == "4P") then target_player = 4 end;
					old_joypad_input = taseditor.getinput(playback_position, target_player);
					-- Superimpose with old input if needed
					if (superimpose == 1 or (superimpose == 2 and new_joypad_input == 0)) then
						new_joypad_input = OR(new_joypad_input, old_joypad_input);
					end
					-- Add joypad info to name
					if (new_joypad_input ~= old_joypad_input) then
						name = name .. "(" .. recording_mode .. ")";
					end
					-- Write to movie data
					taseditor.submitinputchange(playback_position, target_player, new_joypad_input);
					taseditor.applyinputchanges(name);
				end
			end
		end
		playback_last_position = playback_position;
	else
		gui.text(1, 9, "TAS Editor is not engaged.");
	end
end

playback_last_position = movie.framecount();
taseditor.registerauto(reversed_recorder);


