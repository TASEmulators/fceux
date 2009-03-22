require "x_functions";

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
	x_requires(5);
end;

function drawmouse(x, y, click)

	if click then
		fill	= "#cccccc";
	else
		fill	= "#ffffff";
	end;

	y	= y + 1;

	for i = 0, 6 do
		if i ~= 6 then 
			line(x + i, y + i, x + i, y + 8 - math.floor(i / 2), fill);
			pixel(x + i, y + 8 - math.floor(i / 2), "#000000");
		end;
		pixel(x + i, y + i - 1, "#000000");
	end;
	pixel(x + 1, y +  0, "#000000");

	line(x    , y     , x     , y +  9, "#000000");
--	line(x + 1, y +  1, x + 6 , y +  6, "#000000");
--	line(x    , y + 11, x + 7 , y +  7, "#000000");

end;



function hitbox(b1x1, b1y1, b1x2, b1y2, b2x1, b2y1, b2x2, b2y2, con, coff)

	if con == nil then
		con	= "#dd0000";
	end;
	if coff == nil then
		coff = "#00ff00"
	end;

	boxes	= {
		{
			x	= {b1x1, b1x2},
			y	= {b1y1, b1y2},
			},
		{
			x	= {b2x1, b2x2},
			y	= {b2y1, b2y2},
			},
	};

	hit	= false;

	for xc = 1, 2 do
		for yc = 1, 2 do

			if	(boxes[1]['x'][xc] >= boxes[2]['x'][1]) and
				(boxes[1]['y'][yc] >= boxes[2]['y'][1]) and
				(boxes[1]['x'][xc] <= boxes[2]['x'][2]) and
				(boxes[1]['y'][yc] <= boxes[2]['y'][2]) then

				hit	= true;
			end;
		end;
	end;

	if hit == true then
		box(b2x1, b2y1, b2x2, b2y2, con);
		return true;
	else
		box(b2x1, b2y1, b2x2, b2y2, coff);
		return false;
	end;

	return true;

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

function smbmoveenemy(n, x, y, ax, ay)

	x1	= math.fmod(x, 0x100);
	x2	= math.floor(x / 0x100);
	y1	= math.fmod(y, 0x100);
	y2	= math.floor(y / 0x100);

	memory.writebyte(0x006D + n, x2);
	memory.writebyte(0x0086 + n, x1);
	memory.writebyte(0x00B5 + n, y2);
	memory.writebyte(0x00CE + n, y1);
	memory.writebyte(0x0057 + n, ax);
	memory.writebyte(0x009F + n, ay);

end;





smbdata		= {screenpos = 0};
mode		= 1;
last		= {};
inpt		= {};
enemyhold	= {};
while (true) do 


	smbdata['screenposold']	= smbdata['screenpos'];
	smbdata['screenpos']	= memory.readbyte(0x071a) * 0x100 + memory.readbyte(0x071c);
	smbdata['screenposchg']	= smbdata['screenpos'] - smbdata['screenposold'];
	smbdata['rendercol']	= memory.readbyte(0x06A0);
	if smbdata['screenposchg'] < 0 then
		smbdata['screenposchg'] = 0;
	end;
	timer	= timer + 1;


	last	= table.clone(inpt);
	inpt	= input.get();

	c	= "#bbbbbb";
	if inpt['leftclick']then 
		c	= "#dd0000";
	end;

--	line(0, inpt['y'], 255, inpt['y'], c);
--	line(inpt['x'], 0, inpt['x'], 244, c);

--	text(inpt['x'] - 10, 8, string.format("%3d", inpt['x']));
--	text(0, inpt['y'] - 5, string.format("%3d", inpt['y']));

	i	= 2;
	for k,v in pairs(inpt) do
	--	text(8, 8 * i, string.format("%s > %s", k, tostring(v)));
		i	= i + 1;
	end;


	c	= "#ffffff";
	if math.fmod(timer, 4) < 2 then
		c	= "#cccccc";
	end;
	if mode == 0 then
		c1	= c;
		c2	= c;
		c3	= "#ffffff";
		c4	= "#0000ff";
	else
		c1	= "#ffffff";
		c2	= "#0000ff";
		c3	= c;
		c4	= c;
	end;

	text(72, 13, "Tiles");
	text(99, 13, "Enemies");
	box(  73, 14,  94, 22, "#000000");
	box( 100, 14, 135, 22, "#000000");
	if hitbox(inpt['x'], inpt['y'], inpt['x'], inpt['y'],  72, 13,  95, 23, c1, c2) and inpt['leftclick'] then
		mode	= 0;
	end;
	if hitbox(inpt['x'], inpt['y'], inpt['x'], inpt['y'],  99, 13, 136, 23, c3, c4) and inpt['leftclick'] then
		mode	= 1;
	end;

	if mode == 0 then
		ramval		= smbpx2ram(inpt['x'], inpt['y']);
		if ramval then
			ret	= smbram2px(ramval);
			c	= "#ffffff";
			if math.fmod(timer, 4) < 2 then
				c	= "#cccccc";
			end;
			if ret then
				tx1	= math.max(0, ret['x'] - 1);
				tx2	= math.min(0xFF, ret['x'] + 0x10);
				ty1	= math.max(ret['y'] * 0x10 + 0x1F, 0);
				ty2	= math.min(244, ret['y'] * 0x10 + 0x30);
				box(tx1, ty1, tx2, ty2, c);
			end;

			textx	= inpt['x'] + 10;
			texty	= inpt['y'] -  4;
			
			if textx > 229 then
				textx	= textx - 42;
			end;
			texty	= math.min(214, texty);

			text(textx, texty, string.format("%04X", ramval));
			text(textx, texty + 8, string.format(" %02X ", memory.readbyte(ramval)));

		end;

	else

		for i=1,6 do  
			if (memory.readbyte(0x000E+i) ~= 0) then --and memory.readbyte(0x04AC+(i*4)) ~= 0xFF) and (memory.readbyte(0x0015 + i) ~= 0x30 and memory.readbyte(0x0015 + i) ~= 0x31) then 
				if not enemyhold[i] or not inpt['leftclick'] then
					enemyhold[i]	= nil;
--					text(8, 50 + i * 8, "-");
				elseif enemyhold[i] then
--					text(8, 50 + i * 8, string.format("HOLD  %04X %04X", smbdata['screenpos'] + inpt['x'] - enemyhold[i]['x'], inpt['y'] + 0x100 - enemyhold[i]['y']));
					smbmoveenemy(i, smbdata['screenpos'] + inpt['x'] - enemyhold[i]['x'], inpt['y'] + 0x100 - enemyhold[i]['y'], (inpt['x'] - last['x']) * 8, inpt['y'] - last['y']);
				end;

				e2x1 = memory.readbyte(0x04AC+(i*4));
				e2y1 = memory.readbyte(0x04AC+(i*4)+1);
				e2x2 = memory.readbyte(0x04AC+(i*4)+2);
				e2y2 = memory.readbyte(0x04AC+(i*4)+3);
--				text(e2x1 - 5, e2y1 - 13, string.format("%02X", memory.readbyte(0x001E + i)));

				
				
				enemyxpos	= memory.readbytesigned(0x006D + i) * 0x0100 + memory.readbyte(0x0086 + i);
				enemyypos	= memory.readbytesigned(0x00B5 + i) * 0x100 + memory.readbyte(0x00CE + i);
				enemyxacc	= memory.readbytesigned(0x0057 + i);
				enemyyacc	= memory.readbytesigned(0x009f + i);
				enemyxposa	= enemyxpos - smbdata['screenpos'];
				enemyyposa	= enemyypos - 0x100;
				enemyyposa2	= math.fmod(enemyyposa + 0x10000, 0x100);

				line(enemyxposa, enemyyposa2, enemyxposa + 16, enemyyposa2, "#ffffff");

				dead		= "";
				if enemyyposa <= -72 then
					dead	= "DEAD";
				end;
				text(8, 24 + 8 * i, string.format("E%X - %04d %04d %04d %04d %3d %3d %s", i, enemyxpos, enemyypos, enemyxposa, enemyyposa, enemyxacc, enemyyacc, dead));


				if hitbox(inpt['x'], inpt['y'], inpt['x'], inpt['y'], e2x1, e2y1, e2x2, e2y2) then
--				if hitbox(inpt['x'], inpt['y'], inpt['x'], inpt['y'], enemyxposa, enemyyposa, enemyxposa + 0xF, enemyyposa + 0x17) then
					
--					text(e2x1 - 5, e2y1 - 13, string.format("#%d %02X", i, memory.readbyte(0x0015 + i)));


					if inpt['leftclick'] then

						if not enemyhold[i] then 
							enemyhold[i]	= { x = inpt['x'] - enemyxposa, y = inpt['y'] - enemyyposa };
						end;


--						memory.writebyte(0x001F + i, 0xFF);
--						memory.writebyte(0x04AC + i, 0xFF);
--						memory.writebyte(0x000E + i, 0x00);
					end;  
				end;
			else
				enemyhold[i]	= nil;

			end;  
		end;  
	end;

	--[[
	zap		= zapper.read();

	box(zap['x'] - 5, zap['y'] - 5, zap['x'] + 5, zap['y'] + 5, "#ffffff");
	if zap['click'] == 1 then 
		box(zap['x'] - 7, zap['y'] - 7, zap['x'] + 7, zap['y'] + 7, "#ff0000");
	end;
	line(zap['x'] - 0, zap['y'] - 9, zap['x'] + 0, zap['y'] + 9, "#ffffff");
	line(zap['x'] - 9, zap['y'] - 0, zap['x'] + 9, zap['y'] + 0, "#ffffff");
	]]


	drawmouse(inpt['x'], inpt['y'], inpt['leftclick']);

	FCEU.frameadvance();

end