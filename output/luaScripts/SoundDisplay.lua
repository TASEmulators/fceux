---------------------------------------------------------------------------
-- Display Sound Channels data
---------------------------------------------------------------------------
-- Showcases following functions:
-- * sound.get()
---------------------------------------------------------------------------
-- Usage:
-- The script allows you to observe raw data of 5 sound channels.
-- Analysing such info you can notice some regularities in how
-- the data oscillates/changes when ingame music plays.
-- Then you can write scripts similar to \taseditor\TrackNoise.lua
-- to facilitate synchronous dances in TAS movies.
---------------------------------------------------------------------------

function everyframe()
	snd = sound.get();

	gui.text(0, 8, "Square 1:");
	gui.text(0, 16, snd.rp2a03.square1.volume);
	gui.text(0, 24, snd.rp2a03.square1.frequency);
	gui.text(0, 32, snd.rp2a03.square1.midikey);
	gui.text(0, 40, snd.rp2a03.square1.duty);
	gui.text(0, 48, snd.rp2a03.square1.regs.frequency);

	gui.text(128, 8, "Square 2:");
	gui.text(128, 16, snd.rp2a03.square2.volume);
	gui.text(128, 24, snd.rp2a03.square2.frequency);
	gui.text(128, 32, snd.rp2a03.square2.midikey);
	gui.text(128, 40, snd.rp2a03.square2.duty);
	gui.text(128, 48, snd.rp2a03.square2.regs.frequency);

	gui.text(0, 64, "Triangle:");
	gui.text(0, 72, snd.rp2a03.triangle.volume);
	gui.text(0, 80, snd.rp2a03.triangle.frequency);
	gui.text(0, 88, snd.rp2a03.triangle.midikey);
	gui.text(0, 96, snd.rp2a03.triangle.regs.frequency);

	gui.text(128, 64, "Noise:");
	gui.text(128, 72, snd.rp2a03.noise.volume);
	gui.text(128, 80, tostring(snd.rp2a03.noise.short));
	gui.text(128, 88, snd.rp2a03.noise.frequency);
	gui.text(128, 96, snd.rp2a03.noise.midikey);
	gui.text(128, 104, snd.rp2a03.noise.regs.frequency);

	gui.text(0, 120, "DPCM:");
	gui.text(0, 128, snd.rp2a03.dpcm.volume);
	gui.text(0, 136, snd.rp2a03.dpcm.frequency);
	gui.text(0, 144, snd.rp2a03.dpcm.midikey);
	gui.text(0, 152, snd.rp2a03.dpcm.dmcaddress);
	gui.text(0, 160, snd.rp2a03.dpcm.dmcsize);
	gui.text(0, 168, tostring(snd.rp2a03.dpcm.dmcloop));
	gui.text(0, 176, snd.rp2a03.dpcm.dmcseed);
	gui.text(0, 184, snd.rp2a03.dpcm.regs.frequency);
end

emu.registerafter(everyframe);

