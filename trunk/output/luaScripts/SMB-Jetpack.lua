--Super Mario Bros. - Jetpack
--Written by XKeeper
--A fun script that gives mario a "Jetback"

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



function doballs()

	count	= 0;
	for k, v in pairs(balls) do
		
		v['x']		= v['x'] + v['xs'];
		v['y']		= v['y'] + v['ys'];
		v['ys']		= v['ys'] - 0.1;
		v['life']	= v['life'] - 1;

		if v['x'] < 0 or v['x'] > 254 or v['y'] < 0 or v['y'] > 243 or v['life'] < 0 then
			balls[k]	= nil;
		else
			balls[k]	= v;
--			pixel(v['x'], v['y'], "#FFFFFF");
			colkey		= math.ceil(255 * (5 - math.max(math.min(5, (v['life'] / 15)), 0)) / 5);

			if v['c'] >= 0 then
				color	= string.format("#%02X%02X%02X", v['c'], v['c'], v['c']);
--				color	= string.format("#%02X%02X%02X", v['c'] * .8, v['c'] * .5, v['c'] * 0);
			else
				color	= string.format("#%02X0000", v['c'] * -1 , 0, 0);
			end;

			if v['life'] > 45 then 
				box(v['x'], v['y'], v['x'] + 1, v['y'] + 1, color);
			else
				pixel(v['x'], v['y'], color);
			end;
			count		= count + 1;
		end;
	end;

--	lifebar( 2, 140, 249, 10, spower, 400, "#ffffff", "#000044", "#bbbbff");
	return count;

end;



function nojumping()
	memory.writebyte(0x000a, AND(memory.readbyte(0x000a), 0x7F));

	return true;
end;

function nomoving()
	joyput	= joypad.read(1);
	if (joyput['left'] or joyput['right']) then
		memory.writebyte(0x000c, 0);
	end;

	return true;
end;


function areascrambler() 
	memory.writebyte(0x00e7, math.random(0, 0xFF));
	memory.writebyte(0x00e8, math.random(0, 0xFF));
end;

function enemyscrambler() 
	memory.writebyte(0x00e9, math.random(0, 0xFF));
	memory.writebyte(0x00ea, math.random(0, 0xFF));
end;


--[[
memory.register(0x00e8, areascrambler);
memory.register(0x00ea, enemyscrambler);
--]]
	
-- gui.register(gameloop);
memory.register(0x000a, nojumping);
memory.register(0x000c, nomoving);

testcount	= 0;
balls		= {};
z			= 0;
timer		= 0;
jmax		= 500;
jlife		= 500; -- jmax;
refillrate	= 0.020;
msgdisp		= 300;
rechargerate	= 0;
while (true) do 
	timer	= timer + 1;


	if timer < msgdisp then
		yo	= (((math.max(0, (timer + 60) - msgdisp))) ^ 2) / 50;
		text(20, 50 - yo, "2009 Xkeeper - http://jul.rustedlogic.net/");
		text(49, 64 - yo, "A: Jetpack  Left/Right: Move");
		text(53, 72 - yo, "  B button: Turbo boost!  ");
	end;

	invincible			= false;
	if memory.readbyte(0x079F) >= 1 then
		cyclepos		= math.abs(15 - math.fmod(timer, 30)) / 15;
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0xFF));
		barcolor		= "#" .. warningboxcolor .. warningboxcolor .."ff";
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0x80) + 0x7F);
		barcolor2		= "#0000" .. warningboxcolor;
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0x40));
		barcolor3		= "#0000" .. warningboxcolor;
--		barcolor3		= "#000000";

		jlife			= math.min(jmax, jlife + (jmax - jlife) / 25 + 0.25);
		rechargerate	= -0.025;
		invincible		= true;
	elseif jlife <= jmax * 0.25 then
		cyclepos		= math.abs(15 - math.fmod(timer, 30)) / 15;
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0xFF));
		barcolor		= "#ff" .. warningboxcolor .. warningboxcolor;
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0x80) + 0x7F);
		barcolor2		= "#" .. warningboxcolor .. "0000";
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0x40));
		barcolor3		= "#" .. warningboxcolor .. "0000";
--		barcolor3		= "#000000";
	else
		barcolor	= "#ffffff";
		barcolor2	= "#ff4444";
		barcolor3	= "#000000";
	end;
	lifebar(5, 8, 240, 2, jlife, jmax, barcolor, barcolor3, "#000000", barcolor2);
	textx	= math.max(4, math.min(math.floor(jlife/ jmax * 240) - 4, 229));
	if jlife == jmax then
		textx	= 223;
	end;
	text(textx, 13, string.format("%2.1d%%", math.min(jlife/ jmax * 100)));
	
--[[
	bxp		= math.sin(timer / 120) * 64 + 127;
	byp		= math.cos(timer / 120) * 64 + 127;
	for i = 0, 0 do
		balls[z]	= {x = bxp + math.random(-100, 200) / 100, y = byp + math.random(-4, 4), xs = math.random(-100, 100) / 50, ys = math.random(-100, 100) / 100, life = math.random(60, 120), c = math.random(128, 255)};
		z			= z + 1;
	end;
]]

	
	doballs();

	
	marioxspeed			= memory.readbytesigned(0x0057);
	marioyspeed			= memory.readbytesigned(0x009F);
	marioxpos			= memory.readbyte(0x4AC);
	marioypos			= memory.readbyte(0x4AD);

--	lifebar(5, 2, 240, 2, marioxspeed + 0x40, 0x80, "#8888ff", "#000000");

	joyput	= joypad.read(1);
	if joyput['up'] then
		memory.writebyte(0x07F8, 3);
	end;
	if joyput['A'] and jlife >= 3 then

		rechargerate	= 0;
		memory.writebyte(0x009F, math.max(-3, marioyspeed - 1));
--		memory.writebyte(0x009F, -3);
		if not invincible then jlife	= math.max(0, jlife - 3); end;
		for i = 0, 10 do
			balls[z]	= {x = marioxpos + 5, y = marioypos + 7, xs = math.random(-70, 70) / 100, ys =  math.random(100, 300) / 100, life = math.random(30, 60), c = math.random(128, 255)};
			z			= z + 1;
		end;
	end;
	if (joyput['left'] or joyput['right']) then
		
		speedchange		= 1;
		speedmax		= 0x28;
		if joyput['B'] and jlife > 0 then
			rechargerate	= 0;
			if not invincible then jlife	= math.max(0, jlife - 1); end;
			speedchange		= 5;
			speedmax		= 0x40;
		end;

		if joyput['left'] then
			memory.writebyte(0x0033, 2);
			memory.writebyte(0x0045, 2);
			if marioxspeed > (speedmax * -1) then
				memory.writebyte(0x0057, math.max(-0x40, marioxspeed - speedchange));
			end;
			for i = 0, 10 do
				balls[z]	= {x = marioxpos + 7, y = marioypos + 7, xs = math.random(300, 400) / 100, ys =  math.random(-10, 20) / 100, life = math.random(5, 5 + speedchange * 10), c = math.random(128, 255)};
				z			= z + 1;
			end;
		else

			memory.writebyte(0x0033, 1);
			memory.writebyte(0x0045, 1);
			if marioxspeed < speedmax then
				memory.writebyte(0x0057, math.min(0x40, marioxspeed + speedchange));
			end;
			for i = 0, 10 do
				balls[z]	= {x = marioxpos + 7, y = marioypos + 7, xs = math.random(-400, -300) / 100, ys =  math.random(-10, 20) / 100, life = math.random(5, 5 + speedchange * 10), c = math.random(128, 255)};
				z			= z + 1;
			end;
		end;
	end;
	if not ((joyput['B'] and (joyput['left'] or joyput['right'])) or joyput['A']) then

		rechargerate	= rechargerate + refillrate;
		jlife			= math.min(jmax, jlife + rechargerate);

	end;





	screenpage		= memory.readbyte(0x071a);
	screenxpos		= memory.readbyte(0x071c);

	arealow			= memory.readbyte(0x00e7);
	areahigh		= memory.readbyte(0x00e8);

	enemylow		= memory.readbyte(0x00e9);
	enemyhigh		= memory.readbyte(0x00ea);


--	text( 10,  24, string.format("Screen position: %02X.%02X", screenpage, screenxpos));
--	text(169,  24, string.format("Area: %02X %02X", areahigh, arealow));
--	text(163,  32, string.format("Enemy: %02X %02X", enemyhigh, enemylow));


--[[
	text( 4,  217, string.format("Memory writes: %04d", testcount));
	if testcount > 1500 or testcount < 200 then
		cyclepos		= math.abs(15 - math.fmod(timer, 30)) / 15;
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0xFF));
		barcolor		= "#ff" .. warningboxcolor .. warningboxcolor;
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0x80) + 0x7F);
		barcolor2		= "#" .. warningboxcolor .. "0000";
		warningboxcolor	= string.format("%02X", math.floor(cyclepos * 0x40));
		barcolor3		= "#" .. warningboxcolor .. "0000";
--		barcolor3		= "#000000";
	else
		barcolor	= "#ffffff";
		barcolor2	= "#ff4444";
		barcolor3	= "#000000";
	end;
	lifebar(5, 226, 240, 2, testcount, 2500, barcolor, barcolor3, "#000000", barcolor2);

--]]

--	memory.writebyte(0x00e7, math.random(0, 0xFF));
--	memory.writebyte(0x00e8, math.random(0, 0xFF));
--	memory.writebyte(0x00e9, math.random(0, 0xFF));
--	memory.writebyte(0x00ea, math.random(0, 0xFF));
	
	testcount		= 0;

--[[
	marioxspeed2		= math.abs(marioxspeed);

	box(2, 217, 0x40 * 3 + 5, 226, "#000000");
	box(2, 218, 0x40 * 3 + 5, 225, "#000000");
	box(2, 219, 0x40 * 3 + 5, 224, "#000000");
	box(2, 220, 0x40 * 3 + 5, 223, "#000000");
	box(2, 221, 0x40 * 3 + 5, 222, "#000000");

	if marioxspeed2 > 0 then 
		for bl = 0, marioxspeed2 do
		
			if bl < 0x20 then
				segcolor	= string.format("#%02XFF00", math.floor(bl / 0x20 * 0xFF));
			else
				segcolor	= string.format("#FF%02X00", math.max(0, math.floor((0x40 - bl) / 0x20 * 0xFF)));
			end;

			box(bl * 3 + 3, 218, bl * 3 + 4, 225, segcolor);
--			line(bl * 3 + 4, 218, bl * 3 + 4, 225, segcolor);
		end;
	end;

	--]]

	maxspeed			= 0x40;
	marioxspeed			= memory.readbytesigned(0x0057);
	marioxspeed2		= math.abs(marioxspeed);


	text(maxspeed * 3 + 2, 221, string.format(" %2d", marioxspeed2));

	box(5, 221, maxspeed * 3 + 5, 230, "#000000");
	box(5, 222, maxspeed * 3 + 5, 229, "#000000");
	box(5, 223, maxspeed * 3 + 5, 228, "#000000");
	box(5, 224, maxspeed * 3 + 5, 227, "#000000");
	box(5, 225, maxspeed * 3 + 5, 226, "#000000");

	if marioxspeed2 > 0 then 
		for bl = 1, marioxspeed2 do
		
			pct		= bl / maxspeed;
			if pct < 0.50 then
				val		= math.floor(pct * 2 * 0xFF);
				segcolor	= string.format("#%02XFF00", val);

			elseif pct < 0.90 then
				val		= math.floor(0xFF - (pct - 0.5) * 100/40 * 0xFF);
				segcolor	= string.format("#FF%02X00", val);

			elseif bl < maxspeed then
				val	= math.floor((pct - 0.90) * 10 * 0xFF);
				segcolor	= string.format("#FF%02X%02X", val, val);

			else
				segcolor	= "#ffffff";
			end;

			yb			= math.max(math.min(3, (bl - 0x28)), 0);
			box(bl * 3 + 3, 225 - yb, bl * 3 + 4, 229, segcolor);
--			box(bl * 3 + 3, 218, bl * 3 + 4, 225, segcolor);
--			line(bl * 3 + 4, 218, bl * 3 + 4, 225, segcolor);
		end;
	end;

	FCEU.frameadvance();
end;

