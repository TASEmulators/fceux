--Super Mario Bros. - It's Snowing!
--Written by XKeeper


require("x_functions");

if not x_requires then
	-- Sanity check. If they require a newer version, let them know.
	timer	= 1;
	while (true) do
		timer = timer + 1;
		for i = 0, 32 do
			gui.drawbox( 6, 28 + i, 250, 92 - i, "#000000");
		end;
		gui.text( 10, 32, string.format("This Lua script requires the x_functions library."));
		gui.text( 53, 42, string.format("It appears you do not have it."));
		gui.text( 39, 58, "Please get the x_functions library at");
		gui.text( 14, 69, "http://xkeeper.shacknet.nu/");
		gui.text(114, 78, "emu/nes/lua/x_functions.lua");

		warningboxcolor	= string.format("%02X", math.floor(math.abs(30 - math.fmod(timer, 60)) / 30 * 0xFF));
		gui.drawbox(7, 29, 249, 91, "#ff" .. warningboxcolor .. warningboxcolor);

		FCEU.frameadvance();
	end;

else
	x_requires(4);
end;


function smbpx2ram(px, py)

		py		= math.floor(py) - 0x20;
		px		= math.floor(px);

--		text(90, 16, string.format("PX[%4d] PY[%4d]", px, py));
		if px < 0 or px > 400 or py < 0x00 or py > (240 - 0x20) then
			return false;
		end;
		
		oy		= math.floor(py / 0x10);
		ox		= math.fmod(math.floor((px + smbdata['screenpos']) / 0x10), 0x20);

--		text(90, 16, string.format("CX[%4X] CY[%4X]", ox, oy));

		offset	= 0x500 + math.fmod(oy * 0x10 + math.floor(ox / 0x10) * 0xC0 + math.fmod(ox, 0xD0), 0x1A0);
		return offset;

end;

function smbram2px(offset)
	
	offset		= offset - 0x500;
	if offset < 0 or offset >= 0x1A0 then
		return false;
	end;


	px			= (math.fmod(offset, 0x10) + math.floor(offset / 0xD0) * 0x10) * 0x10;
	px			= px - math.fmod(smbdata['screenpos'], 0x200);
--	text(8,  8, string.format("PX[%4d] OF[%4X]", px, offset));
	if px < 0 then
		px		= px + 0x200;
	end;

	py			= math.floor(math.fmod(offset, 0xD0) / 0x10);
	returnval	= {x = px, y = py};
	return returnval;

end;


function doballs()

	count	= 0;
	for k, v in pairs(balls) do
		
		v['x']		= v['x'] + v['xs'] - smbdata['screenposchg'];
		v['y']		= v['y'] + v['ys'];
--		v['ys']		= v['ys'] - 0.1;
		v['life']	= v['life'] - 1;

		offset	= smbpx2ram(v['x'], v['y']);
		temp	= 0;
		if offset then
			temp	= memory.readbyte(offset);
		end;


		-- 354 so we can spawn them offscreen
		if v['x'] < 0 or v['x'] > 512 or v['y'] < 0 or v['y'] > 243 or v['life'] < 0 or (temp > 0) then
			balls[k]	= nil;
		else
			balls[k]	= v;
			colkey		= math.ceil(255 * (5 - math.max(math.min(5, (v['life'] / 15)), 0)) / 5);

			if v['c'] >= 0 then
				color	= string.format("#%02X%02X%02X", v['c'], v['c'], 255);
--				color	= string.format("#%02X%02X%02X", v['c'] * .8, v['c'] * .5, v['c'] * 0);
			else
				color	= string.format("#%02X0000", v['c'] * -1 , 0, 0);
			end;

			if v['life'] > 400 then 
				box(v['x'] - 1, v['y'] - 1, v['x'] + 1, v['y'] + 1, color);
				pixel(v['x'], v['y'], color);
			elseif v['life'] > 200 then 
				box(v['x'], v['y'], v['x'] + 1, v['y'] + 1, color);
			else
				pixel(v['x'], v['y'], color);
			end;
			count		= count + 1;
		end;
	end;

	return count;

end;


balls		= {};
z			= 0;
timer		= 0;
smbdata		= {screenpos = 0};
while (true) do 

	if 
		memory.readbyte(0x0301) == 0x3F and
		memory.readbyte(0x0302) == 0x10 and
		memory.readbyte(0x0303) == 0x04 and
		memory.readbyte(0x0304) == 0x22
	then
		memory.writebyte(0x0304, 0x0F);
	end;

	smbdata['screenposold']	= smbdata['screenpos'];
	smbdata['screenpos']	= memory.readbyte(0x071a) * 0x100 + memory.readbyte(0x071c);
	smbdata['screenposchg']	= smbdata['screenpos'] - smbdata['screenposold'];
	smbdata['rendercol']	= memory.readbyte(0x06A0);
	if smbdata['screenposchg'] < 0 then
		smbdata['screenposchg'] = 0;
	end;
	timer	= timer + 1;

	
	ballcount	= doballs();


	for i = 0, 2 do
		balls[z]	= {x = math.random(0, 512), y = 0, xs = math.random(-50, 00) / 100, ys =  math.random(100, 150) / 100, life = math.random(400, 700), c = math.random(200, 255)};
		z			= z + 1;
	end;

--	lifebar(8, 8, 240, 2, ballcount, 1000, "#ffffff", "clear");
--]]

--[[
	for x = 0x00, 0x0F do
		for y = 0x00, 0x0C do
			box(x * 0x10 +  0, y * 0x10 + 0x20, x * 0x10 + 0x01, y * 0x10 +  0x21, "#FFFFFF");
			value	= memory.readbyte(smbpx2ram(x * 0x10, y * 0x10 + 0x20));
			if value > 0 then
				text(x * 0x10 +  0, y * 0x10 + 0x20, string.format("%02X", value));
			end;
		end;
	end;
]]

--	text(8, 8, string.format("0x06A0 [%4X]", smbdata['rendercol']));
--[[
	for i = 0, 5 do
		ret	= smbram2px(0x500 + i * 0x10 + i);
		if ret then
--			text(8, 16, string.format("PX[%4d] PY[%4d]", ret['x'], ret['y']));
			box(ret['x'] +  0, ret['y'] * 0x10 + 0x20, ret['x'] + 0x0F, ret['y'] * 0x10 + 0x2F, "#FFFFFF");
		end;
	end;
--]]
--[[
	box(19, 19, 0x20 * 2 + 20, 46, "#0000ff");
	box(18, 18, 0x20 * 2 + 21, 47, "#0000ff");
	for x = 0, 0x1F do
		for y = 0, 0x0C do
			offset	= 0x500 + y * 0x10 + math.floor(x / 0x10) * 0xD0 + math.fmod(x, 0x10);
			c		= memory.readbyte(offset);
			box(x * 2 + 20, y * 2 + 20, x * 2 + 21, y * 2 + 21, string.format("#%02X%02X%02X", c, c, c));
		end;
	end;

	if math.fmod(timer, 2) < 1 then
		temp	= math.floor(math.fmod(smbdata['screenpos'], 0x200) / 8);
		if temp < 0x20 then
			box(temp + 20, 19, temp + 0x10 * 2 + 20, 46, "#ffffff");
		else
			box(temp + 20, 19, 0x20 * 2 + 20, 46, "#ffffff");
			box(19, 19, (temp - 0x20) + 20, 46, "#ffffff");
		end;

		line(smbdata['rendercol'] * 2 + 20, 19, smbdata['rendercol'] * 2 + 20, 46, "#00ff00");
	end;

--]]
--[[
	x		= 0;
	y		= 0;
	px		= math.sin(timer / 60) * 100 + 127;
	py		= math.cos(timer / 60) * 90 + 100 + 0x20;
	offset	= smbpx2ram(px, py);
	if offset then
		offset	= offset - 0x500;
		x		= math.floor(offset / 0xD0) * 0x10 + math.fmod(offset, 0x10);
		y		= math.floor(math.fmod(offset, 0xD0) / 0x10);

		x2		= math.fmod(smbdata['screenpos'] + x * 0x10, 0x100);
		box( x2,  y * 0x10 + 0x20,  x2 + 0x0F, y * 0x10 + 0x2F, "#ffffff");
--		line(  0,  y * 0x10,  255, y * 0x10, "#ffffff");
		
		box (px - 3, py - 3, px + 3, py + 3, "#ffffff");
		line(px - 6, py    , px + 6, py    , "#ffffff");
		line(px    , py - 6, px    , py + 6, "#ffffff");
		text(90, 24, string.format("Offset[%04X]", offset + 0x500));
		text(90, 32, string.format("OX[%4X] OY[%4X]", x, y));
		text(90, 40, string.format("SX[%4X]", x2));
	else
		text(90, 24, "Offset failed");
	end;
	
	if math.fmod(timer, 2) < 1 then
		temp	= math.floor(math.fmod(smbdata['screenpos'], 0x200) / 8) + 20;
		line(temp, 18, temp, 47, "#ffffff");
		box(20 + x * 2, 20 + y * 2, 21 + x * 2, 21 + y * 2, "#ffffff");
		
	end;

--]]


	text( 20, 222, " 2009 Xkeeper - http://jul.rustedlogic.net/ ");
	line(  21, 231, 232, 231, "#000000");

	FCEU.frameadvance();

end;

