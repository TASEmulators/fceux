--SMB area scrambler
--Randomly changes the level contents.  Doesn't garuantee a winnable level (nor does it guarantee it won't crash the game)
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





function areascrambler() 
end;


function gameloop()

	joyin	= joypad.read(1);
	if joyin['select'] then
		memory.writebyte(0x00e7, math.random(0, 0xFF));
		memory.writebyte(0x00e8, math.random(0, 0xFF));
		memory.writebyte(0x00e9, math.random(0, 0xFF));
		memory.writebyte(0x00ea, math.random(0, 0xFF));
		memory.writebyte(0x0750, math.random(0, 0xFF));
	end;
	
	if joyin['up'] then
		memory.writebyte(0x009F, -5);
		memory.writebyte(0x07F8, 3);
		memory.writebyte(0x0722, 0xFF)
	end;

	screenpage		= memory.readbyte(0x071a);
	screenxpos		= memory.readbyte(0x071c);

	arealow			= memory.readbyte(0x00e7);
	areahigh		= memory.readbyte(0x00e8);

	enemylow		= memory.readbyte(0x00e9);
	enemyhigh		= memory.readbyte(0x00ea);

	unknown			= memory.readbyte(0x0750);


	text(  6,  30, string.format("Position: %02X %02X", screenpage, screenxpos));
	text( 19,  38, string.format("Area: %02X %02X", areahigh, arealow));
	text( 13,  46, string.format("Enemy: %02X %02X", enemyhigh, enemylow));
	text( 13,  54, string.format("?: %02X", unknown));
end;


function areascramble()
		memory.writebyte(0x00e7, math.random(0, 0xFF));
		memory.writebyte(0x00e8, math.random(0, 0xFF));
end;

function enemyscramble()
		memory.writebyte(0x00e9, math.random(0, 0xFF));
		memory.writebyte(0x00ea, math.random(0, 0xFF));
end;

	
gui.register(gameloop);
memory.register(0x00e8, areascramble);
memory.register(0x00ea, enemyscramble);



while (true) do 

	memory.writebyte(0x079F, 2);
	FCEU.frameadvance();
end;

