---------------------------------------------------------------------------
-- Tracking Noise channel volume peaks
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * sound.get()
-- * taseditor.markedframe()
-- * taseditor.getmarker()
-- * taseditor.setmarker()
-- * taseditor.getnote()
-- * taseditor.setnote()
---------------------------------------------------------------------------
-- Usage:
-- Run the script, unpause emulation.
-- When the "Auto function" checkbox is checked, the script will
-- automatically create Markers on various frames according to ingame music.
-- These Markers should help you see music patterns,
-- so you can adjust input precisely creating nice Tool-Assisted dances.
-- 
-- This script uses Noise channel volume as an indicator for setting Markers,
-- but it's possible to use any other property of sound channels
-- as a factor to decide whether to set Marker on current frame.
-- 
-- The script was tested on Super Mario Bros, Megaman and Duck Tales.
-- You may want to see it in slow-mo. Use -/= hotkeys to change emulation speed.
-- 
-- To create customized script for your game, first use SoundDisplay.lua
-- to collect and analyse statistics about ingame music. Then choose
-- which channel you want to sync your input to. Finally, change this script
-- so that Markers will be created using new set of rules.
---------------------------------------------------------------------------

NOISE_VOL_THRESHOLD = 0.35;

function track_changes()
	if (taseditor.engaged()) then
		current_frame = movie.framecount();
		if (last_frame ~= current_frame) then
			-- Playback has moved
			-- Get current value of indicator for current_frame
			snd = sound.get();
			indicator = snd.rp2a03.noise.volume;
			-- If Playback moved 1 frame forward, this was probably Frame Advance
			if (last_frame == current_frame - 1) then
				-- Looks like we advanced one frame from the last time
				-- Decide whether to set Marker
				if (indicator > NOISE_VOL_THRESHOLD and last_frame_indicator_value == 0) then
					-- this was a peak in volume! ____/\____
					-- Set Marker and show frequency of noise+triangle in its Note
					SetSoundMarker(current_frame - 1, "Sound: " .. (snd.rp2a03.noise.regs.frequency + snd.rp2a03.triangle.regs.frequency));
				end
			end
			last_frame = current_frame;
			last_frame_indicator_value = indicator;
		end
	else
		gui.text(1, 9, "TAS Editor is not engaged.");
	end
end

function SetSoundMarker(frame, new_note)
	if (taseditor.markedframe(frame)) then
		-- this frame is already marked
		old_note = taseditor.getnote(taseditor.getmarker(frame));
		-- check if the Note of the Marker already contains new_note text
		if (string.find(old_note, new_note) == nil) then
			-- append new_note text to the Marker's Note
			taseditor.setnote(taseditor.getmarker(frame), old_note .. " " .. new_note);
		end
	else
		-- this frame isn't marked
		-- create new Marker here
		new_marker = taseditor.setmarker(frame);
		taseditor.setnote(new_marker, new_note);
	end
end

last_frame = 0;
last_frame_indicator_value = 0;

taseditor.registerauto(track_changes);

