--Galaxian
--Written by XKeeper
--Accesses the Music Player Easter Egg and displays the songs & information

require "x_functions";
require "x_interface";

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
		gui.text( 14, 69, "http://xkeeper.shacknet.nu:5/");
		gui.text(114, 78, "emu/nes/lua/x_functions.lua");

		warningboxcolor	= string.format("%02X", math.floor(math.abs(30 - math.fmod(timer, 60)) / 30 * 0xFF));
		gui.drawbox(7, 29, 249, 91, "#ff" .. warningboxcolor .. warningboxcolor);

		FCEU.frameadvance();
	end;

else
	x_requires(6);
end;

function musicplayer()
	resets	= memory.readbyte(0x0115);
	song	= memory.readbyte(0x0002);
	songlua	= math.max(1, math.floor(resets / 45));
	speed	= memory.readbyte(0x0004);
	speedde	= memory.readbyte(0x0104);	-- it's really an AND. But the only two values used are 0F and 07, so this modulous works.
	pos		= memory.readbyte(0x0000);
	note	= memory.readbyte(0x0001);
	offsetb	= song * 0x0100 + 0x4010 - 1;
	offset	= song * 0x0100 + pos - 1 + 0x4010;
	note1	= 0x10 - math.floor(note / 0x10);
	note2	= math.fmod(note, 0x10);
	if note1 == 0x10 then note1 = 0 end;


	text( 35,  42, string.format("Song Position: %02X%02X", song, pos));
	text( 40,  50, string.format("ROM Offset: %04X", offset));
	text( 43,  58, string.format("Song Speed:   %02X", speed));
	text(105,  66, string.format("  %02X", math.fmod(speedde, speed)));
	
	lifebar(186, 21,  64, 4, note1, 15, "#8888ff", "#000066", false, "#ffffff");
	lifebar(186, 29,  64, 4, note2, 15, "#8888ff", "#000066", false, "#ffffff");
	text(178, 20, string.format("%X\n%X", note1, note2));

	lifebar( 44, 67,  64, 4, math.fmod(speedde, speed), speed, "#8888ff", "#000066", false, "#ffffff");
	
	if control.button(75, 90, 84, 1, "Toggle hex viewer") then
		hexmap	= not hexmap;
	end;

	if hexmap then
		songdata	= {};
		songdata2	= {};
		for i = 0x00, 0xFF do
			if i >= songs[songlua]['st'] and i <= songs[songlua]['en'] or true then
				o			= string.format("%02X", rom.readbyte(offsetb + i));
			else
				o			= "";
			end;
			x				= math.fmod(i, 0x10);
			y				= math.floor(i / 0x70);
			if not songdata2[x] then
				songdata2[x]	= {};
			end;
			if not songdata2[x][y] then
				songdata2[x][y]	= o;
	--			songdata2[x][y]	= string.format("%02X", y);
			else
				songdata2[x][y]	= songdata2[x][y] .."\n".. o;
			end;
		end;

		for x = 0, 0x0F do
			text(29 + x * 14, 100 + 8 *  0, songdata2[x][0]);
			text(29 + x * 14, 100 + 8 *  7, songdata2[x][1]);
			text(29 + x * 14, 100 + 8 * 14, songdata2[x][2]);
		end;

		oboxx	= 29 + math.fmod(pos, 0x10) * 14;
		oboxy	= 100 + 8 * math.floor(pos / 0x10);
		c		= "#ff0000";
		if math.fmod(timer, 4) < 2 then
			c = "#FFFFFF";
		end;
		box(oboxx, oboxy + 0, oboxx + 14, oboxy + 10, c);
	end;

	if pos >= songs[songlua]['en'] then --and pos == 0xFE then
		if not songs[songlua]['xx'] then
			memory.writebyte(0x0104, 0xFF);
--			text(50, 50, "LOCK");
		else
			memory.writebyte(0x0115, songs[songs[songlua]['xx']]['rv']);
			memory.writebyte(0x0002, songs[songs[songlua]['xx']]['sv']);
			memory.writebyte(0x0004, songs[songs[songlua]['xx']]['sp']);
			memory.writebyte(0x0000, songs[songs[songlua]['xx']]['st']);
		end;
	end;

	for id, val in pairs(songs) do
		if id == songlua then
			c		= "#8888ff";
			if math.fmod(timer, 4) < 2 then
				c = "#FFFFFF";
			end;
		else
			c	= nil;
		end;
		if control.button(195, 33 + 11 * id, 57, 1, string.format("Play song %d", id), c, true) then
--			resetrequired	= true;
--			memory.register(0x0104, nil);
			memory.writebyte(0x0115, val['rv']);
			memory.writebyte(0x0002, val['sv']);
			memory.writebyte(0x0004, val['sp']);
			memory.writebyte(0x0000, val['st']);
		end;
	end;

	if resetrequired then
		text(50, 85, "Please soft-reset game.");
		if movie.framecount() == 0 then
			resetrequired	= false;
		end;
	end;
end;


songs	= {
	--  resets		song `id`	speed		start		end
	{	rv	= 0x2E,	sv	= 0x1B,	sp	= 0x0F,	st	= 0x00,	en	= 0xC6	},
	{	rv	= 0x5A,	sv	= 0x18,	sp	= 0x07,	st	= 0x00,	en	= 0xC2	},
	{	rv	= 0x87,	sv	= 0x06,	sp	= 0x0F,	st	= 0x00,	en	= 0x7F	},
	{	rv	= 0xB4,	sv	= 0x16,	sp	= 0x0F,	st	= 0x50,	en	= 0xAF	},
	{	rv	= 0xE1,	sv	= 0x1A,	sp	= 0x0F,	st	= 0x80,	en	= 0xF8, xx	= 0x01	},
	};
timer	= 0;
hexmap	= true;

while true do

	timer	= timer + 1;
	input.update();

	if memory.readbyte(0x0101) == 1 then
		musicplayer();

	else
		filledbox(23, 49, 233, 130, "#000000");
		text( 25,  50, "Normally you'd have to do something insane");
		text( 52,  58, "like push RESET 45 times and");
		text( 56,  66, "hold A+B on P2's controller.");

		text( 28,  82, "Lucky for you, though, we borrowed this.");
		text( 73,  90, "Feel free to use it.");

		text( 73, 119, "(Yeah, we've got that)");

		timer2	= math.fmod(timer, 60);
		if timer2 >= 30 then timer2 = 60 - timer2; end;
		timer2	= math.floor((timer2 / 30) * 255);
		c		= string.format("#%02X%02XFF", timer2, timer2);

		if control.button(110, 106, 37, 1, " EASY ", c, "#ffffff") then
			memory.writebyte(0x0101, 0x01);
			memory.writebyte(0x0115, songs[2]['rv']);
			memory.writebyte(0x0002, songs[2]['sv']);
			memory.writebyte(0x0004, songs[2]['sp']);
			memory.writebyte(0x0000, songs[2]['st']);
		FCEU.softreset();
--			text(1, 1, 'woohoo');
		end;
	end;
	FCEU.frameadvance();

end;
