-- Teenage Mutant Ninja Turtles (U)[!].rom
-- Written by QFox
-- 31 july 2008
-- Displays Hitboxes, Enemy HP, and various stats on screen

local function box(x1,y1,x2,y2,color)
	-- gui.text(50,50,x1..","..y1.." "..x2..","..y2);
	if (x1 > 0 and x1 < 255 and x2 > 0 and x2 < 255 and y1 > 0 and y1 < 241 and y2 > 0 and y2 < 241) then
		gui.drawbox(x1,y1,x2,y2,color);
	end;
end;
local function text(x,y,str)
	if (x > 0 and x < 255 and y > 0 and y < 240) then
		gui.text(x,y,str);
	end;
end;
local function pixel(x,y,color)
	if (x > 0 and x < 255 and y > 0 and y < 240) then
		gui.drawpixel(x,y,color);
	end;
end;

while (true) do
	local stuff = 0x023C; -- start of tile data, 4 bytes each, y, ?, ?, x. every tile appears to be 10x10px
	-- print boxes for all the tiles
	-- invalid tiles are automatically hidden because their x/y coords are out of range, i guess
	for i=0,0x30 do
		x = memory.readbyte(stuff+3+(i*4));
		y = memory.readbyte(stuff+(i*4));
		box(x,y+1,x+7,y+16,"red");
	end;
	
	-- print player's health
	local x = memory.readbyte(0x0480);
	local y = memory.readbyte(0x0460);
	local hp = memory.readbyte(0x0077+memory.readbyte(0x0067)); -- get health of current char, there are 4 chars and 4 healths
	text(x-10,y-20,hp);

	-- print enemy hp
	local startx = 0x0484;
	local starty = 0x0464;
	local starthp = 0x0564;
	for i=0,11 do
		x = memory.readbyte(startx+i);
		y = memory.readbyte(starty+i);
		hp = memory.readbyte(starthp+i);
		--box(x-5,y-5,x+5,y+5,"green"); -- their 'center', or whatever it is.
		text(x-5,y-20,hp);
	end;
	
	FCEU.frameadvance();
end;