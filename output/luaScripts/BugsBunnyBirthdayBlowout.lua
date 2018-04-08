--Bugs Bunny Birthday Blowout
--Written by XKeeper
--Creates Lag and Sprite counters as well as Camera position

-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function box(x1,y1,x2,y2,color)
	if (x1 >= 0 and x1 <= 255 and x2 >= 0 and x2 <= 255 and y1 >= 0 and y1 <= 244 and y2 >= 0 and y2 <= 244) then
		gui.drawbox(x1,y1,x2,y2,color);
	end;
end;



-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function filledbox(x1,y1,x2,y2,color)
	for i = 0, math.abs(y1 - y2) do
		line(x1,y1 + i,x2,y1 + i,color);
	end;
end;




-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function lifebar(x, y, sx, sy, a1, a2, oncolor, offcolor, noborder)
	-- this function will have the effect of drawing an HP bar
	-- keep in mind xs and ys are 2px larger to account for borders
	
	x1	= x;
	x2	= x + sx + 4;
	y1	= y;
	y2	= y + sy + 4;
	w	= math.floor(a1 / math.max(1, a1, a2) * sx);
	if not a2 then w = 0 end;

	if (noborder) then 
		box(x1 + 1, y1 + 1, x2 - 1, y2 - 1, "#000000");
	else
		box(x1 + 1, y1 + 1, x2 - 1, y2 - 1, "#ffffff");
		box(x1    , y1    , x2    , y2    , "#000000");
	end; 

	if (w < sx) then
		filledbox(x1 + w + 2, y1 + 2, x2 - 2, y2 - 2, offcolor);
	end;		

	if (w > 0) then
		filledbox(x1 + 2, y1 + 2, x1 + 2 + w, y2 - 2, oncolor);
	end;		

end;






-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function line(x1,y1,x2,y2,color)
	if (x1 >= 0 and x1 <= 255 and x2 >= 0 and x2 <= 255 and y1 >= 0 and y1 <= 244 and y2 >= 0 and y2 <= 244) then
		local success = pcall(function() gui.drawline(x1,y1,x2,y2,color) end);
		if not success then 
			text(60, 224, "ERROR: ".. x1 ..",".. y1 .." ".. x2 ..",".. y2);
		end;
	end;
end;

function text(x,y,str)
	if (x >= 0 and x <= 255 and y >= 0 and y <= 240) then
		gui.text(x,y,str);
	end;
end;

function pixel(x,y,color)
	if (x >= 0 and x <= 255 and y >= 0 and y <= 240) then
		gui.drawpixel(x,y,color);
	end;
end;



function drawpos(cx, cy, ex, ey, n)
	sx	= ex - cx;
	sy	= ey - cy;
	
	num	= "";
	if n then
		num	= string.format("%02X", n);	
	end;

	if sx >= 0 and sx <= 255 and sy >= 0 and sy <= 244 then
		line(sx, sy, sx + 16, sy +  0, "#ff0000");
		line(sx, sy, sx +  0, sy + 16, "#ff0000");
		text(sx, sy, num);

	elseif sx < 0 and sy >= 0 and sy <= 244 then
		line(0, sy, 16, sy, "#ff0000");
		text(4, sy, num);

	elseif sx > 255 and sy >= 0 and sy <= 244 then
		line(239, sy, 255, sy, "#ff0000");
		text(243, sy, num);

	elseif sy < 0 and sx >= 0 and sx <= 256 then
		line(sx, 8, sx, 24, "#ff0000");
		text(sx, 8, num);

	elseif sy > 244 and sx >= 0 and sx <= 256 then
		line(sx, 212, sx, 244, "#ff0000");
		text(sx, 216, num);
	
	end;


end;


lagdetectorold	= 0;
timer			= 0;
lagframes		= 0;
lastlag			= 0;

while (true) do 

	timer	= timer + 1;

	lagdetector	= memory.readbyte(0x00f5);
--	if lagdetector == lagdetectorold then
	if AND(lagdetector, 0x20) == 0x20 then
--	if lagdetector == 0x0C then
		lagframes	= lagframes + 1;
	else
		if lagframes ~= 0 then 
			lastlag = lagframes;
		end;
		lagframes	= 0;
		lagdetectorold	= lagdetector;
	end;
	memory.writebyte(0x00f5, OR(lagdetector, 0x20));

	playerx	= memory.readbyte(0x0432) + memory.readbyte(0x0433) * 0x100;
	playery	= memory.readbyte(0x0435) + memory.readbyte(0x0436) * 0x100;

	screenx	= memory.readbyte(0x0456) + memory.readbyte(0x0457) * 0x100;
	screeny	= memory.readbyte(0x0458) + memory.readbyte(0x0459) * 0x100;

	text(  8,   8, string.format("%04X, %04X", playerx, playery));
	text(  8,  16, string.format("%04X, %04X", screenx, screeny));

	drawpos(screenx, screeny, playerx, playery);


	tmp		= 0;
	for i = 0, 0xb do
		
		offset	= 0x7680 + i * 0x20;
	
		enemyt	= memory.readbyte(offset);
		enemyx	= memory.readbyte(offset + 2) + memory.readbyte(offset + 3) * 0x100;
		enemyy	= memory.readbyte(offset + 4) + memory.readbyte(offset + 5) * 0x100;

		if enemyt ~= 0xff then
--			text(160, 8 + 8 * tmp, string.format("%02X: %02X <%04X, %04X>", i, enemyt, enemyx, enemyy));
			drawpos(screenx, screeny, enemyx, enemyy, i);
			tmp	= tmp + 1;
		end
	end;


	text(142, 192, string.format("%02d lag frames", lastlag));
	text(142, 216, string.format("%02d active sprites", tmp));
	
--	box(2, 208, 2 + 8 * lastlag, 210, "#ff4444");
--	box(2, 209, 2 + 8 * lastlag, 211, "#ff4444");
--	box(2, 212, 2 + 8 * tmp, 213, "#4444ff");
--	box(2, 214, 2 + 8 * tmp, 215, "#4444ff");

	lifebar(144, 200, 100, 4, lastlag, 8, "#ffcc22", "#000000");
	lifebar(144, 208, 100, 4, tmp, 12,    "#4488ff", "#000000");

	FCEU.frameadvance();

end;















