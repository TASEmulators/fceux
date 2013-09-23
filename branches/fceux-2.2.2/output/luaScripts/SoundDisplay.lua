---------------------------------------------------------------------------
-- Display Sound Channels data
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * sound.get()
---------------------------------------------------------------------------
-- Usage:
-- The script allows you to observe data of 5 sound channels.
-- Analysing such info you can notice some regularities in how
-- the data oscillates/changes when ingame music plays.
-- Then you can write scripts similar to \taseditor\TrackNoise.lua
-- to facilitate synchronous dances in TAS movies.
---------------------------------------------------------------------------

square1 = {x=1, y=8, on=true};
square2 = {x=129, y=8, on=true};
triangle = {x=1, y=67, on=true};
noise = {x=129, y=67, on=true};
dpcm = {x=1, y=118, on=true};

semitones = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
duty_ratio = {"12.5%", "25%", "50%", "75%"};
duty_color = {"purple", "magenta", "orange", "yellow"};
key_color = {"#000000FF", "#202020FF", "#205050FF", "#008080FF", "#00C0C0FF", "#00FFFFFF", "#FFFFFFFF", "#FFFF00FF", "#FFFFFFFF", "#FFFF00FF", "#FFFFFFFF", "#FFFF00FF", "#FFFFFFFF", "#FFFF00FF", "#FFFFFFFF"};
noise_color = {long="orange", short = "yellow"};

sample_names = {
	-- Here you can specify custom names for DMC samples (to display name instead of just address)
	-- For example, here are two samples for Contra(U) ROM
	Sample_FC00 = {name="Kick", color="#C0C0C0FF"},
	Sample_FCC0 = {name="Snare", color="#80C0FFFF"},
}

function everyframe()
	snd = sound.get();

	if (square1.on) then
		gui.text(square1.x, square1.y, "SQUARE 1:");

		Volume = snd.rp2a03.square1.volume;
		gui.text(square1.x, square1.y + 8, "Volume:");
		gui.box(square1.x + 32, square1.y + 7, square1.x + 32+15, square1.y + 15, "#006000FF", "black");
		gui.box(square1.x + 32, square1.y + 7, square1.x + 32 + Volume * 15, square1.y + 15, "green", "clear");
		gui.text(square1.x + 48, square1.y + 8, string.format("%g", Volume));

		Frequency = snd.rp2a03.square1.frequency;
		gui.text(square1.x, square1.y + 16, "Frequency: " .. string.format("%g", Frequency) .. "Hz");

		Midikey = snd.rp2a03.square1.midikey;
		gui.box(square1.x-1, square1.y + 23, square1.x + 127, square1.y + 31, "#008080FF", "black")
		gui.box(square1.x-1, square1.y + 23, square1.x-1 + Midikey, square1.y + 31, "#00C0C0FF", "black")
		Keycolor = key_color[math.floor(Volume * 15) + 1];
		gui.box(square1.x-1 + Midikey-1, square1.y + 23, square1.x-1 + Midikey+1, square1.y + 31, "white", Keycolor)
		Note = math.floor((Midikey - 21) % 12);
		Octave = math.floor((Midikey - 12) / 12);
		Semitone = tostring(semitones[Note + 1]) .. Octave;
		gui.text(square1.x, square1.y + 32, "Note: " .. Semitone .. "  ");
		gui.text(square1.x + 50, square1.y + 32, "(" .. string.format("%g", Midikey) .. ")");

		Period = snd.rp2a03.square1.regs.frequency;
		IndicatorColor = "#" .. string.format("%x", Period % 256) .. "0000FF";
		gui.text(square1.x, square1.y + 40, "Period:");
		gui.box(square1.x + 31, square1.y + 39, square1.x + 39, square1.y + 47, IndicatorColor, "black");
		gui.text(square1.x + 40, square1.y + 40, string.format("%g", Period));

		Duty = snd.rp2a03.square1.duty;
		gui.text(square1.x, square1.y + 48, "Duty #" .. Duty .. " (" .. duty_ratio[Duty + 1] .. ")", duty_color[Duty + 1]);
	end

	if (square2.on) then
		gui.text(square2.x, square2.y, "SQUARE 2:");

		Volume = snd.rp2a03.square2.volume;
		gui.text(square2.x, square2.y + 8, "Volume:");
		gui.box(square2.x + 32, square2.y + 7, square2.x + 32+15, square2.y + 15, "#006000FF", "black");
		gui.box(square2.x + 32, square2.y + 7, square2.x + 32 + Volume * 15, square2.y + 15, "green", "clear");
		gui.text(square2.x + 48, square2.y + 8, string.format("%g", Volume));

		Frequency = snd.rp2a03.square2.frequency;
		gui.text(square2.x, square2.y + 16, "Frequency: " .. string.format("%g", Frequency) .. "Hz");

		Midikey = snd.rp2a03.square2.midikey;
		gui.box(square2.x-1, square2.y + 23, square2.x + 127, square2.y + 31, "#008080FF", "black")
		gui.box(square2.x-1, square2.y + 23, square2.x-1 + Midikey, square2.y + 31, "#00C0C0FF", "black")
		Keycolor = key_color[math.floor(Volume * 15) + 1];
		gui.box(square2.x-1 + Midikey-1, square2.y + 23, square2.x-1 + Midikey+1, square2.y + 31, "white", Keycolor)
		Note = math.floor((Midikey - 21) % 12);
		Octave = math.floor((Midikey - 12) / 12);
		Semitone = tostring(semitones[Note + 1]) .. Octave;
		gui.text(square2.x, square2.y + 32, "Note: " .. Semitone .. "  ");
		gui.text(square2.x + 50, square2.y + 32, "(" .. string.format("%g", Midikey) .. ")");

		Period = snd.rp2a03.square2.regs.frequency;
		IndicatorColor = "#" .. string.format("%x", Period % 256) .. "0000FF";
		gui.text(square2.x, square2.y + 40, "Period:");
		gui.box(square2.x + 31, square2.y + 39, square2.x + 39, square2.y + 47, IndicatorColor, "black");
		gui.text(square2.x + 40, square2.y + 40, string.format("%g", Period));

		Duty = snd.rp2a03.square2.duty;
		gui.text(square2.x, square2.y + 48, "Duty #" .. Duty .. " (" .. duty_ratio[Duty + 1] .. ")", duty_color[Duty + 1]);
	end

	if (triangle.on) then
		gui.text(triangle.x, triangle.y, "TRIANGLE:");

		Volume = snd.rp2a03.triangle.volume;
		gui.text(triangle.x, triangle.y + 8, "Volume:");
		gui.box(triangle.x + 32, triangle.y + 7, triangle.x + 32+15, triangle.y + 15, "#006000FF", "black");
		gui.box(triangle.x + 32, triangle.y + 7, triangle.x + 32 + Volume * 15, triangle.y + 15, "#00A000FF", "clear");
		gui.text(triangle.x + 48, triangle.y + 8, string.format("%g", Volume));

		Frequency = snd.rp2a03.triangle.frequency;
		gui.text(triangle.x, triangle.y + 16, "Frequency: " .. string.format("%g", Frequency) .. "Hz");

		Midikey = snd.rp2a03.triangle.midikey;
		gui.box(triangle.x-1, triangle.y + 23, triangle.x + 127, triangle.y + 31, "#008080FF", "black")
		gui.box(triangle.x-1, triangle.y + 23, triangle.x-1 + Midikey, triangle.y + 31, "#00C0C0FF", "black")
		if (Volume ~= 0) then gui.box(triangle.x-1 + Midikey-1, triangle.y + 23, triangle.x-1 + Midikey+1, triangle.y + 31, "white", "black") end;
		Note = math.floor((Midikey - 21) % 12);
		Octave = math.floor((Midikey - 12) / 12);
		Semitone = tostring(semitones[Note + 1]) .. Octave;
		gui.text(triangle.x, triangle.y + 32, "Note: " .. Semitone .. "  ");
		gui.text(triangle.x + 50, triangle.y + 32, "(" .. string.format("%g", Midikey) .. ")");

		Period = snd.rp2a03.triangle.regs.frequency;
		IndicatorColor = "#" .. string.format("%x", Period % 256) .. "0000FF";
		gui.text(triangle.x, triangle.y + 40, "Period:");
		gui.box(triangle.x + 31, triangle.y + 39, triangle.x + 39, triangle.y + 47, IndicatorColor, "black");
		gui.text(triangle.x + 40, triangle.y + 40, string.format("%g", Period));
	end

	if (noise.on) then
		gui.text(noise.x, noise.y, "NOISE:");

		Volume = snd.rp2a03.noise.volume;
		gui.text(noise.x, noise.y + 8, "Volume:");
		gui.box(noise.x + 32, noise.y + 7, noise.x + 32+15, noise.y + 15, "#006000FF", "black");
		gui.box(noise.x + 32, noise.y + 7, noise.x + 32 + Volume * 15, noise.y + 15, "green", "clear");
		gui.text(noise.x + 48, noise.y + 8, string.format("%g", Volume));

		Frequency = snd.rp2a03.noise.frequency;
		gui.text(noise.x, noise.y + 16, "Frequency: " .. string.format("%g", Frequency) .. "Hz");

		Midikey = snd.rp2a03.noise.midikey;
		gui.box(noise.x-1, noise.y + 23, noise.x + 127, noise.y + 31, "#008080FF", "black")
		gui.box(noise.x-1, noise.y + 23, noise.x-1 + (Midikey/2), noise.y + 31, "#00C0C0FF", "black")
		Keycolor = key_color[math.floor(Volume * 15) + 1];
		gui.box(noise.x-1 + (Midikey/2)-1, noise.y + 23, noise.x-1 + (Midikey/2)+1, noise.y + 31, "white", Keycolor)
		Note = math.floor((Midikey - 21) % 12);
		Octave = math.floor((Midikey - 12) / 12);
		Semitone = tostring(semitones[Note + 1]) .. Octave;
		gui.text(noise.x, noise.y + 32, "Note: " .. Semitone .. "   ");
		gui.text(noise.x + 58, noise.y + 32, "(" .. string.format("%g", Midikey) .. ")");

		Period = snd.rp2a03.noise.regs.frequency;
		IndicatorColor = "#" .. string.format("%X", Period % 256) .. "0000FF";
		gui.text(noise.x, noise.y + 40, "Period:");
		gui.box(noise.x + 31, noise.y + 39, noise.x + 39, noise.y + 47, IndicatorColor, "black");
		gui.text(noise.x + 40, noise.y + 40, string.format("%g", Period));

		if (snd.rp2a03.noise.short) then
			gui.text(noise.x, noise.y + 48, "Mode: short", noise_color.short);
		else
			gui.text(noise.x, noise.y + 48, "Mode: long", noise_color.long);
		end
	end

	if (dpcm.on) then
		gui.text(dpcm.x, dpcm.y, "DPCM:");

		Volume = snd.rp2a03.dpcm.volume;
		gui.text(dpcm.x, dpcm.y + 8, "Volume:");
		gui.box(dpcm.x + 32, dpcm.y + 7, dpcm.x + 32+15, dpcm.y + 15, "#006000FF", "black");
		gui.box(dpcm.x + 32, dpcm.y + 7, dpcm.x + 32 + Volume * 15, dpcm.y + 15, "#00A000FF", "clear");
		gui.text(dpcm.x + 48, dpcm.y + 8, string.format("%g", Volume));

		Frequency = snd.rp2a03.dpcm.frequency;
		gui.text(dpcm.x, dpcm.y + 16, "Frequency: " .. string.format("%g", Frequency) .. "Hz");

		Midikey = snd.rp2a03.dpcm.midikey;
		gui.box(dpcm.x-1, dpcm.y + 23, dpcm.x + 127, dpcm.y + 31, "#008080FF", "black")
		gui.box(dpcm.x-1, dpcm.y + 23, dpcm.x-1 + (Midikey/2), dpcm.y + 31, "#00C0C0FF", "black")
		if (Volume ~= 0) then gui.box(dpcm.x-1 + (Midikey/2)-1, dpcm.y + 23, dpcm.x-1 + (Midikey/2)+1, dpcm.y + 31, "white", "black") end;
		Note = math.floor((Midikey - 21) % 12);
		Octave = math.floor((Midikey - 12) / 12);
		Semitone = tostring(semitones[Note + 1]) .. Octave;
		gui.text(dpcm.x, dpcm.y + 32, "Note: " .. Semitone .. "  ");
		gui.text(dpcm.x + 50, dpcm.y + 32, "(" .. string.format("%g", Midikey) .. ")");

		Period = snd.rp2a03.dpcm.regs.frequency;
		IndicatorColor = "#" .. string.format("%x", Period % 256) .. "0000FF";
		gui.text(dpcm.x, dpcm.y + 40, "Period:");
		gui.box(dpcm.x + 31, dpcm.y + 39, dpcm.x + 39, dpcm.y + 47, IndicatorColor, "black");
		gui.text(dpcm.x + 40, dpcm.y + 40, string.format("%g", Period));

		if (snd.rp2a03.dpcm.dmcloop) then
			gui.text(dpcm.x, dpcm.y + 48, "Sample: $" .. string.format("%X", snd.rp2a03.dpcm.dmcaddress) .. " (" .. snd.rp2a03.dpcm.dmcsize .. " bytes looped)", "orange");
		else
			gui.text(dpcm.x, dpcm.y + 48, "Sample: $" .. string.format("%X", snd.rp2a03.dpcm.dmcaddress) .. " (" .. snd.rp2a03.dpcm.dmcsize .. " bytes)");
		end

		Sample_id = "Sample_" .. string.format("%X", snd.rp2a03.dpcm.dmcaddress);
		if (sample_names[Sample_id] ~= nil) then
			gui.text(dpcm.x + 170, dpcm.y + 48, sample_names[Sample_id].name, sample_names[Sample_id].color);
		end

		gui.text(dpcm.x, dpcm.y + 56, "DMC seed: " .. string.format("%X", snd.rp2a03.dpcm.dmcseed));
	end
end

emu.registerafter(everyframe);

