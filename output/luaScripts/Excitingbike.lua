--Exicitebike
--Written by XKeeper
--Shows various stats including a RAM map & speed

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



function gameloop()




end;


gui.register(gameloop);


barcolors	= {};
barcolors[0]	= "#000000";
barcolors[1]	= "#880000";
barcolors[2]	= "#ff0000";
barcolors[3]	= "#eeee00";
barcolors[4]	= "#00ff00";
barcolors[5]	= "#00ffff";
barcolors[6]	= "#0000ff";
barcolors[7]	= "#ff00ff";
barcolors[8]	= "#ffffff";
barcolors[9]	= "#123456";

lastvalue		= {};
justblinked		= {};
lastzero		= {};
timer			= 0;
speed			= 0;

while (true) do 

	timer		= timer + 1;
	lastvalue['speed']	= speed;


	speed		= memory.readbyte(0x0094) * 0x100 + memory.readbyte(0x0090);
	positionx	= memory.readbyte(0x0050) * 0x100 + memory.readbyte(0x0394);
	timerspeed	= 3 - memory.readbyte(0x004c);
	timerslant	= math.max(0, memory.readbyte(0x0026) - 1);

	if memory.readbyte(0x0303) ~= 0x8E then
		text(255, 181, "Didn't advance this frame");
	end;
	
	speedadj1	= math.fmod(speed, 0x100);
	speedadj2	= math.fmod(math.floor(speed / 0x100), #barcolors + 1);
	speedadj3	= math.fmod(speedadj2 + 1, #barcolors + 1);

	lifebar(  61, 11, 100, 4, speedadj1, 0x100, barcolors[speedadj3], barcolors[speedadj2], "#000000", "#ffffff");

	text(198,  9, string.format("Speed %2d", timerspeed));
	text(196, 17, string.format(" Slant %2d", timerslant));
	box(      186, 10, 198, 18, "#000000");
	box(      186, 18, 198, 26, "#000000");

	if timerspeed == 0 then
		filledbox(187, 11, 197, 17, "#00ff00");
		if memory.readbyte(0x00B0) ~= 0 then
			temp	= "->";
		else
			temp	= "(B)";
		end;
		text( 0, 50, string.format("Hold %s to maintain speed", temp));
	else
		filledbox(187, 11, 197, 17, "#880000");
	end;
	if timerslant <= 1 then
		filledbox(187, 19, 197, 25, "#00ff00");
		text( 0, 58, string.format("Use < + > to modify angle"));
	else
		filledbox(187, 19, 197, 25, "#880000");
	end;



	text(  0,   4 +  6, string.format("Speed: %4X", speed));
	text(  1,   4 + 14, string.format("S.Chg: %4d", speed - lastvalue['speed']));



	value	= memory.readbyte(0x0064);
	lifebar(  1,   1, 240, 6, value, 0xFF, "#ffffff", "#111144", false, "#000000");
	tp		= math.floor(value / 255 * 240) + 4;
	text(tp,   1, string.format("%02X", value));


	drawerpos	= memory.readbyte(0x00e8);
	screenpos	= memory.readbyte(0x00eb);

	for x = 0, 0x3F do
		for y = 0, 5 do
			offset	= y * 0x40 + x + 0x400;
			value	= memory.readbyte(offset);
			color	= string.format("#%02x%02x%02x", value, value, value);
			x		= math.fmod(x - screenpos, 0x40);
			while (x < 0) do
				x	= x + 0x40;
			end;
			box(x * 3 + 8, y * 3 + 28, x * 3 + 9, y * 3 + 30, color);
			box(x * 3 + 9, y * 3 + 28, x * 3 +10, y * 3 + 30, color);
--			pixel(x * 3 + 9, y * 3 + 28, color);
		end;
	end;

	drawerpos	= drawerpos - screenpos;
	while (drawerpos < 0) do
		drawerpos	= drawerpos + 0x40;
	end;

	box(math.fmod(drawerpos, 0x40) * 3 + 7, 27, math.fmod(drawerpos, 0x40) * 3 + 11, 5 * 3 + 31, "#dd0000");
--	box(math.fmod(screenpos, 0x40) * 3 + 7, 25, math.fmod(screenpos, 0x40) * 3 + 11, 5 * 3 + 31, "#00ff00");
	box(math.fmod(0, 0x40)         * 3 + 7, 27, math.fmod(0, 0x40)         * 3 + 11, 5 * 3 + 31, "#00ff00");

	for i = 0, 5 do
		offset	= 0x00e8 + i;
		if not lastzero[offset] then
			lastzero[offset]	= {};
		end;

		
		value	= memory.readbyte(offset);

		if lastvalue[offset] and lastvalue[offset] ~= value then
			if not justblinked[offset] then
				color		= "#ffffff";
			else
				color	= "#dd0000";
			end;
			justblinked[offset]	= true;
		else
			color	= "#dd0000";
			justblinked[offset]	= false;
		end;

		lifebar(  3, 190 + i * 8, 240, 6, value, 240, color, "#111144", false, "#000000");
		tp		= math.floor(value / 240 * 240) + 4;
		text(tp, 190 + i * 8, string.format("%02X", value));

		if lastzero[offset]['time'] then
			text(165, 190 + i * 8, string.format("%7sa %4df", string.format("%.2f", (lastzero[offset]['total'] / lastzero[offset]['samples'])), lastzero[offset]['time']));
		end;
		if value == 0 and not lastzero[offset]['uhoh'] then
			if lastzero[offset]['fr'] then
				lastzero[offset]['time']	= timer - lastzero[offset]['fr'];
				if lastzero[offset]['total'] then
					lastzero[offset]['total']	= lastzero[offset]['total'] + lastzero[offset]['time'];
					lastzero[offset]['samples']	= lastzero[offset]['samples'] + 1;
				else
					lastzero[offset]['total']	= lastzero[offset]['time'];
					lastzero[offset]['samples']	= 1;
				end;
			end;
			lastzero[offset]['fr']		= timer;
			lastzero[offset]['uhoh']	= true;
		elseif value ~= 0 then
			lastzero[offset]['uhoh']	= false;
		end;

		lastvalue[offset]	= value;

	end;



--[[
	startoffset	= 0x5E0;
	s			= 0x120;
	xs			= 0x40;
	ys			= math.floor(s / xs);
	for x = 0, xs - 1 do
		for y = 0, ys - 1 do
			offset	= y * xs + x + startoffset;
			value	= memory.readbyte(offset);
			if value == 0x40 then
				color	= "clear";
			else
				value	= math.fmod(value * 0x10, 0x100);
				color	= string.format("#%02x%02x%02x", value, value, value);
			end;

			box(x * 3 + 8, y * 3 + 64, x * 3 + 10, y * 3 + 66, color);
			pixel(x * 3 + 9, y * 3 + 65, color);
		end;
	end;

]]

	FCEU.frameadvance();
end;