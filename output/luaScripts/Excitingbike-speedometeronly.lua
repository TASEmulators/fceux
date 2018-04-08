--Exciting Bike - Speedometer
--Written by XKeeper
--Shows the speedometer (obviously)

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

	text(  0,   4 +  6, string.format("Speed: %4X", speed));
	text(  1,   4 + 14, string.format("S.Chg: %4d", speed - lastvalue['speed']));

	text( 20, 222, " 2009 Xkeeper - http://jul.rustedlogic.net/ ");
	line(  21, 231, 232, 231, "#000000");

	FCEU.frameadvance();
end;