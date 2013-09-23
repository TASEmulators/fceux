--shapedefs
--A Lua script with defined functions for shapes such as hearts.
--Needed for SM-Lives&HPDisplay-4Matsy.lua

local function box(x1,y1,x2,y2,color)
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

function drawshape (x,y,str,color)
	if str == "heart_5x5" then
		box(x+1,y+0,x+1,y+3,color);
		box(x+3,y+0,x+3,y+3,color);
		box(x+2,y+3,x+2,y+4,color);
		box(x+0,y+1,x+4,y+2,color);
	end;
	if str == "heart_7x7" then
		box(x+1,y+0,x+2,y+4,color);
		box(x+4,y+0,x+5,y+4,color);
		box(x+0,y+1,x+6,y+3,color);
		box(x+3,y+2,x+3,y+6,color);
		box(x+2,y+5,x+4,y+5,color);
	end;
	if str == "z2magicjar" then
		box(x+0,y+5,x+4,y+6,color);
		box(x+1,y+4,x+3,y+7,color);
		box(x+1,y+0,x+3,y+0,color);
		box(x+2,y+1,x+2,y+3,color);
		box(x+3,y+2,x+3,y+2,color);
		box(x+4,y+3,x+4,y+3,color);
	end;
end;