-- Simple sprite visualizer
-- Draws a box around all sprites on screen.
-- rainwarrior 8/23/2016

sprite_color = "#FF00FF" -- change to customize color

sprite_height_last = 8
sprite_height = 8

function oam_dma(a,s,v)
	local oam = v * 256
	for i=1,64 do
		local is = (i-1) * 4
		local x0 = memory.readbyte(oam + is + 3)
		local x1 = x0 + 7
		local y0 = memory.readbyte(oam + is + 0) + 1
		local y1 = y0 + (sprite_height_last - 1)
		gui.box(x0,y0,x1,y1,"",sprite_color)
	end
end

function ppu_ctrl(a,s,v)
	if AND(v,0x20) == 0 then
		sprite_height = 8
	else
		sprite_height = 16
	end
end

function frame_end()
	-- information about sprite height will be behind by 1 frame
	-- (or potentially wrong if changed before the end of the frame)
	-- in most games this doesn't change from frame to frame, though
	sprite_height_last = sprite_height
end

-- main

memory.registerwrite(0x4014,1,oam_dma)
memory.registerwrite(0x2000,1,ppu_ctrl)
emu.registerafter(frame_end)

while (true) do
	emu.frameadvance()
end
