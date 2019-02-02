-- feos, r57shell, 2012-2018
-- gui.box frame simulates transparency

print("Hi-hat and keys may glitch if you produce sound effects.")
print("Leftclick over the displays: channel names to hide the volumes, notes to hide the keyboard.")
print(" ")
print("And praise Gocha!")

iterator = 15
kb = {x=9, y=154, on=true}
prev_keys = input.get()
semitones = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"}

volumes = {
	S1V = {0}, S1C = {},
	S2V = {0}, S2C = {},
	TV = {0},
	NV = {0},
	DPCMV = {0}
}

function Draw()
	snd = sound.get()
	keys = input.get()
	
	if iterator == 1 then
		kb.y = 30
	else
		kb.y = 154
	end
	
	-- do only at the first frame
	if #volumes.S1V == 1 then
		channels = {
			Square1  = {x=1,      y=9, vol=volumes.S1V, color=volumes.S1C, duty=0, midi=0, semitone=0, octave=0, prev_semitone=0, float = {}, floaty = {}, floatz = {}},
			Square2  = {x=1+45*1, y=9, vol=volumes.S2V, color=volumes.S2C, duty=0, midi=0, semitone=0, octave=0, prev_semitone=0, float = {}, floaty = {}, floatz = {}},
			Triangle = {x=1+45*2, y=9, vol=volumes.TV, midi=0, semitone=0, octave=0, prev_semitone=0, float = {}, floaty = {}, floatz = {}},
			Noise    = {x=1+45*3, y=9, vol=volumes.NV, midi=0, semitone=0, octave=0},
			DPCM     = {x=1+45*4, y=9, vol=volumes.DPCMV}
		}
	end

	-- update the first indices for volume tables
	-- shift the previous ones farther
	table.insert(channels.Square1.vol,  1, snd.rp2a03.square1.volume*15)
	table.insert(channels.Square2.vol,  1, snd.rp2a03.square2.volume*15)
	table.insert(channels.Triangle.vol, 1, snd.rp2a03.triangle.volume*15)
	table.insert(channels.Noise.vol,    1, snd.rp2a03.noise.volume*15)
	table.insert(channels.DPCM.vol,     1, snd.rp2a03.dpcm.volume*15)

	-- get duty and midikey for proper channels
	channels.Square1.duty = snd.rp2a03.square1.duty
	channels.Square2.duty = snd.rp2a03.square2.duty
	
	channels.Square1.midi  = snd.rp2a03.square1.midikey
	channels.Square2.midi  = snd.rp2a03.square2.midikey
	channels.Triangle.midi = snd.rp2a03.triangle.midikey
	channels.Noise.midi    = snd.rp2a03.noise.midikey

	-- guess notes
	for name, chan in pairs(channels) do
		if name == "Square1" or name == "Square2" or name == "Triangle" or name == "Noise" then
			if chan.vol[1] > 0 then
				chan.octave = math.floor((chan.midi - 12) / 12)
				chan.semitone = tostring(semitones[math.floor((chan.midi - 21) % 12) + 1])
			else chan.semitone = "--"; chan.octave = "-"
			end
		end
	end
	
	-- notes display
	gui.text(kb.x+203, kb.y,    "S1: "..channels.Square1.semitone..channels.Square1.octave,   "#ff0000ff", "#000000ff")
	gui.text(kb.x+203, kb.y+9,  "S2: "..channels.Square2.semitone..channels.Square2.octave,   "#aa00ccff", "#000000ff")
	gui.text(kb.x+204, kb.y+18, "Tr: "..channels.Triangle.semitone..channels.Triangle.octave, "#00aaffff", "#000000ff")
	gui.text(kb.x+204, kb.y+27, "Ns: "..channels.Noise.semitone..channels.Noise.octave,       "#ffffffff", "#000000ff")

-----------------
-- Draw hi-hat --
-----------------

	xhh1 = 227
	yhh1 = 18
	xhh2 = 227
	yhh2 = 18
	
	if channels.Noise.vol[1] > 0 then 
		if channels.Noise.octave >= 9 and channels.Noise.octave <= 12 then
			colorhh = "#ffaa00"
			if channels.Noise.vol[2] - channels.Noise.vol[1] < 4
			and channels.Noise.vol[2] > 0
			then yhh1 = 15
			end
		end
	else colorhh = "#00000000"
	end

	gui.line(xhh1-1,  yhh1,   xhh1+28, yhh1,   "#00000088")
	gui.line(xhh1-1,  yhh1-1, xhh1+28, yhh1-1, "#00000088")
	gui.line(xhh1-1,  yhh1-2, xhh1+28, yhh1-2, "#00000088")
	gui.line(xhh1+3,  yhh1-3, xhh1+24, yhh1-3, "#00000088")
	gui.line(xhh1+8,  yhh1-4, xhh1+19, yhh1-4, "#00000088")
	gui.line(xhh1+11, yhh1-5, xhh1+16, yhh1-5, "#00000088")
	gui.line(xhh1+12, yhh1-6, xhh1+15, yhh1-6, "#00000088")	
	gui.line(xhh2-1,  yhh2,   xhh2+28, yhh2,   "#00000088")
	gui.line(xhh2-1,  yhh2+1, xhh2+28, yhh2+1, "#00000088")
	gui.line(xhh2-1,  yhh2+2, xhh2+28, yhh2+2, "#00000088")
	gui.line(xhh2+3,  yhh2+3, xhh2+24, yhh2+3, "#00000088")
	gui.line(xhh2+8,  yhh2+4, xhh2+19, yhh2+4, "#00000088")
	gui.line(xhh2+11, yhh2+5, xhh2+16, yhh2+5, "#00000088")
	gui.line(xhh2+12, yhh2+6, xhh2+15, yhh2+6, "#00000088")

	gui.line(xhh1,    yhh1-1, xhh1+27, yhh1-1, colorhh)
	gui.line(xhh1+4,  yhh1-2, xhh1+23, yhh1-2, colorhh)
	gui.line(xhh1+9,  yhh1-3, xhh1+18, yhh1-3, colorhh)
	gui.line(xhh1+12, yhh1-4, xhh1+15, yhh1-4, colorhh)
	gui.line(xhh1+13, yhh1-5, xhh1+14, yhh1-5, colorhh)	
	gui.line(xhh2,    yhh2+1, xhh2+27, yhh2+1, colorhh)
	gui.line(xhh2+4,  yhh2+2, xhh2+23, yhh2+2, colorhh)
	gui.line(xhh2+9,  yhh2+3, xhh2+18, yhh2+3, colorhh)
	gui.line(xhh2+12, yhh2+4, xhh2+15, yhh2+4, colorhh)
	gui.line(xhh2+13, yhh2+5, xhh2+14, yhh2+5, colorhh)
	
--------------------
-- Keyboard stuff --
--------------------

	if (kb.on) then
		-- capture leftclicks
		if keys.xmouse <= 256 and keys.xmouse >= 205 and keys.ymouse >= kb.y and keys.ymouse <= kb.y+27 then
			if keys["leftclick"] and not prev_keys["leftclick"] then kb.on = false end
		end		
		-- draw the kayboard
		gui.box(kb.x-8, kb.y, kb.x+200, kb.y+16, "#ffffffff") -- white solid box
		for a = -2, 49 do gui.box(kb.x+4*a, kb.y, kb.x+4*a, kb.y+16, "#00000000") end -- black lines
		-- draw colored boxes as clean notes
		for name, chan in pairs(channels) do
			if name == "Square1" or name == "Square2" or name == "Triangle" then
				if name == "Triangle" then color = "#00aaffff"
				elseif name == "Square1" then color = "#ff0000ff"
				else color = "#aa00ccff"
				end
				
				if     chan.semitone == "C" then gui.box(kb.x+1 +28*(chan.octave-1), kb.y, kb.x+3 +28*(chan.octave-1), kb.y+16, color)
				elseif chan.semitone == "D" then gui.box(kb.x+5 +28*(chan.octave-1), kb.y, kb.x+7 +28*(chan.octave-1), kb.y+16, color)
				elseif chan.semitone == "E" then gui.box(kb.x+9 +28*(chan.octave-1), kb.y, kb.x+11+28*(chan.octave-1), kb.y+16, color)
				elseif chan.semitone == "F" then gui.box(kb.x+13+28*(chan.octave-1), kb.y, kb.x+15+28*(chan.octave-1), kb.y+16, color)
				elseif chan.semitone == "G" then gui.box(kb.x+17+28*(chan.octave-1), kb.y, kb.x+19+28*(chan.octave-1), kb.y+16, color)
				elseif chan.semitone == "A" then gui.box(kb.x+21+28*(chan.octave-1), kb.y, kb.x+23+28*(chan.octave-1), kb.y+16, color)
				elseif chan.semitone == "B" then gui.box(kb.x+25+28*(chan.octave-1), kb.y, kb.x+27+28*(chan.octave-1), kb.y+16, color)
				end
			end
		end		
		-- draw accidental keys
		gui.box(kb.x-3, kb.y, kb.x-5, kb.y+10, "#00000000")		
		for oct = 0, 6 do
			gui.box(kb.x+3+28*oct,  kb.y, kb.x+5+28*oct,  kb.y+10, "#00000000")
			gui.box(kb.x+7+28*oct,  kb.y, kb.x+9+28*oct,  kb.y+10, "#00000000")
			gui.box(kb.x+15+28*oct, kb.y, kb.x+17+28*oct, kb.y+10, "#00000000")
			gui.box(kb.x+19+28*oct, kb.y, kb.x+21+28*oct, kb.y+10, "#00000000")
			gui.box(kb.x+23+28*oct, kb.y, kb.x+25+28*oct, kb.y+10, "#00000000")			
		end
		-- draw colored boxes over accidental keys
		for name, chan in pairs(channels) do			
			if name == "Square1" or name == "Square2" or name == "Triangle" then
				if name == "Triangle" then color = "#00aaffff"
				elseif name == "Square1" then color = "#ff0000ff"
				else color = "#aa00ccff"
				end
				
				if     chan.semitone == "C#" then gui.box(kb.x+3 +28*(chan.octave-1), kb.y, kb.x+5 +28*(chan.octave-1), kb.y+10, color)
				elseif chan.semitone == "D#" then gui.box(kb.x+7 +28*(chan.octave-1), kb.y, kb.x+9 +28*(chan.octave-1), kb.y+10, color)
				elseif chan.semitone == "F#" then gui.box(kb.x+15+28*(chan.octave-1), kb.y, kb.x+17+28*(chan.octave-1), kb.y+10, color)
				elseif chan.semitone == "G#" then gui.box(kb.x+19+28*(chan.octave-1), kb.y, kb.x+21+28*(chan.octave-1), kb.y+10, color)
				elseif chan.semitone == "A#" then gui.box(kb.x+23+28*(chan.octave-1), kb.y, kb.x+25+28*(chan.octave-1), kb.y+10, color)
				end
			end
		end
		
		grey = "#aaaaaaaa"	
		for oct = 0, 6 do					
			gui.line(kb.x+3+28*oct,  kb.y+10, kb.x+5+28*oct,  kb.y+10, grey)
			gui.line(kb.x+7+28*oct,  kb.y+10, kb.x+9+28*oct,  kb.y+10, grey)
			gui.line(kb.x+15+28*oct, kb.y+10, kb.x+17+28*oct, kb.y+10, grey)
			gui.line(kb.x+19+28*oct, kb.y+10, kb.x+21+28*oct, kb.y+10, grey)
			gui.line(kb.x+23+28*oct, kb.y+10, kb.x+25+28*oct, kb.y+10, grey)
		end
		gui.line(kb.x-3,   kb.y+10, kb.x-5,   kb.y+10, grey)
		gui.line(kb.x-8,   kb.y,    kb.x+200, kb.y,    "#00000088")
		gui.line(kb.x-8,   kb.y+16, kb.x+200, kb.y+16, "#00000088")
		gui.line(kb.x-8,   kb.y,    kb.x-8,   kb.y+16, "#00000088")
		gui.line(kb.x+200, kb.y,    kb.x+200, kb.y+16, "#00000088")
	else
		-- capture leftclicks
		if keys.xmouse <= 256 and keys.xmouse >= 205 and keys.ymouse >= kb.y and keys.ymouse <= kb.y+27 then
			if keys["leftclick"] and not prev_keys["leftclick"] then kb.on = true end
		end
	end

--------------------
-- Floating notes --
--------------------
	
	if (kb.on) then
		for name, chan in pairs(channels) do
			if name == "Square1" or name == "Square2" or name == "Triangle" then
			
				if chan.prev_semitone ~= chan.semitone then
					local len_prev = #chan.float
					if     chan.semitone == "C"  then table.insert(chan.float, 1, kb.x+1 +28*(chan.octave-1))
					elseif chan.semitone == "D"  then table.insert(chan.float, 1, kb.x+5 +28*(chan.octave-1))
					elseif chan.semitone == "E"  then table.insert(chan.float, 1, kb.x+9 +28*(chan.octave-1))
					elseif chan.semitone == "F"  then table.insert(chan.float, 1, kb.x+13+28*(chan.octave-1))
					elseif chan.semitone == "G"  then table.insert(chan.float, 1, kb.x+17+28*(chan.octave-1))
					elseif chan.semitone == "A"  then table.insert(chan.float, 1, kb.x+21+28*(chan.octave-1))
					elseif chan.semitone == "B"  then table.insert(chan.float, 1, kb.x+25+28*(chan.octave-1))
					elseif chan.semitone == "C#" then table.insert(chan.float, 1, kb.x+3 +28*(chan.octave-1))
					elseif chan.semitone == "D#" then table.insert(chan.float, 1, kb.x+7 +28*(chan.octave-1))
					elseif chan.semitone == "F#" then table.insert(chan.float, 1, kb.x+15+28*(chan.octave-1))
					elseif chan.semitone == "G#" then table.insert(chan.float, 1, kb.x+19+28*(chan.octave-1))
					elseif chan.semitone == "A#" then table.insert(chan.float, 1, kb.x+23+28*(chan.octave-1))
					end
					if len_prev ~= #chan.float then table.insert(chan.floaty, 1, 0) table.insert(chan.floatz, 1, 0) end
				elseif chan.semitone ~= '--' then
					chan.floaty[1] = 0
				end
				
				if name == "Triangle" then color = "#00aaffff"
				elseif name == "Square1" then color = "#ff0000ff"
				else color = "#aa00ccff"
				end
			
				for i = 1, #chan.float do
					local y = kb.y+chan.floaty[i]
					local z = kb.y+chan.floatz[i]
					chan.floaty[i] = chan.floaty[i] + 1
					chan.floatz[i] = chan.floatz[i] + 1
					if y+17<=z+15 then
						if movie.framecount()%2 == 0 then gui.box(chan.float[i]-1, y+16, chan.float[i]+3, z+16, "#eedd2200") end
						gui.box(chan.float[i], y+17, chan.float[i]+2, z+15, color)
					end
				end
				while #chan.float ~= 0 and chan.floaty[#chan.float] > 224 do
					table.remove(chan.float,  #chan.float)
					table.remove(chan.floaty, #chan.floaty)
					table.remove(chan.floatz, #chan.floatz)
				end
			end
		end
	end

---------------------
-- Volumes display --
---------------------

	for name, chan in pairs(channels) do
		if name == "Square1" or name == "Square2" then
			-- set color for each duty value
			if chan.duty == 0 then table.insert(chan.color,1,"#aaff00ff")
			elseif chan.duty == 1 then table.insert(chan.color,1,"#00ff00ff")
			elseif chan.duty == 2 then table.insert(chan.color,1,"#00bb00ff")
			else table.insert(chan.color,1,"#008800ff")
			end
		end
		-- capture leftclicks
		if iterator == 15 then
			if keys.ymouse <= 24 and keys.ymouse >= 0 then
				if keys["leftclick"] and not prev_keys["leftclick"] then iterator = 1 end
			end
		else
			if keys.ymouse <= 24 and keys.ymouse >= 0 then
				if keys["leftclick"] and not prev_keys["leftclick"] then iterator = 15 end
			end
		end
		-- draw volumes
		gui.text(chan.x, 9, name, "#ffffffff", "#000000ff")	
		if iterator <=14 then
			-- draw just first volume values
			gui.text(chan.x, chan.y+9+1, chan.vol[1])
			if tonumber(chan.vol[1]) > 0 then
				for j = 0, chan.vol[1]-1 do
					gui.box(chan.x+13+j*2, chan.y+9, chan.x+15+j*2, chan.y+8+9, "#000000ff")
					gui.line(chan.x+14+j*2, chan.y+1+9, chan.x+14+j*2, chan.y+7+9, "#00ff00ff")
					if name == "Square1" or name == "Square2" then
						-- color comes from duty
						gui.text(chan.x+38, chan.y, chan.duty, chan.color[1], "#000000ff")
						gui.line(chan.x+14+j*2, chan.y+1+9, chan.x+14+j*2, chan.y+7+9, chan.color[1])
					end
				end
			end
		else
			-- draw all 15 volume values
			for i = 1, #chan.vol do
				gui.text(chan.x, chan.y+i*9+1, chan.vol[i])
				if tonumber(chan.vol[i]) > 0 then
					for j = 0, chan.vol[i]-1 do
						gui.box(chan.x+13+j*2, chan.y+i*9, chan.x+15+j*2, chan.y+8+i*9, "#000000ff")
						gui.line(chan.x+14+j*2, chan.y+1+i*9, chan.x+14+j*2, chan.y+7+i*9, "#00ff00ff")
						if name == "Square1" or name == "Square2" then
							-- color comes from duty
							gui.text(chan.x+38, chan.y, chan.duty, chan.color[1], "#000000ff")
							gui.line(chan.x+14+j*2, chan.y+1+i*9, chan.x+14+j*2, chan.y+7+i*9, chan.color[i])
						end
					end
				end
			end
		end
		-- keep the table limited
		table.remove(chan.vol, 15)
		
		-- highlight the first values
		-- 30 Hz blinking, works properly if your monitor is set to 60 Hz
		if chan.vol[1] > 0 and movie.framecount()%2 == 0 then
			gui.box(chan.x+12, chan.y+8, chan.x+14+chan.vol[1]*2, chan.y+18, "#ffcc0000")
		end
	end	
	
	for name, chan in pairs(channels) do
		if name == "Square1" or name == "Square2" or name == "Triangle" then
			chan.prev_semitone = chan.semitone
		end
	end
	
	prev_keys = keys
end
emu.registerafter(Draw);