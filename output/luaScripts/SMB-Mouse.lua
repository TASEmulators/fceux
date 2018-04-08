--Super Mario Bros. - Drag and Drop
--Written by XKeeper
--Allows you to use the mouse to pick up enemies and movie them around!

debugmodes	= {
	enabled			= false;	-- CHANGE THIS AT YOUR PERIL
	showenemydata	= false;
	drawmouse		= false;
	locktimer		= false;
	invincible		= false;
	ver				= "03/29 15:00:00";
	};

control	= {};	-- must come first.
				-- basically, will contain all the "interface" things,
				-- buttons, menus, etc.
				-- Easier to organize, I guess.

require "x_functions";
require "x_interface";
if debugmodes['enabled'] and false then
	require "x_smb1enemylist";		-- used for summoning and other things... not finished
end;

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


-- ****************************************************************************
-- * drawmouse(xpos, ypos, click)
-- * Draws a crude mouse pointer at the location; mostly good for screenshots and debugging.
-- ****************************************************************************
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


-- ****************************************************************************
-- * smbpx2ram( screen-x, screen-y )
-- * Returns the offset that represents the tile under screenx/screeny.
-- ****************************************************************************
function smbpx2ram(px, py)

		local py		= math.floor(py) - 0x20;
		local px		= math.floor(px);
		if px < 0 or px > 400 or py < 0x00 or py > (240 - 0x20) then
			return false;
		end;
		
		local oy		= math.floor(py / 0x10);
		local ox		= math.fmod(math.floor((px + smbdata['screenpos']) / 0x10), 0x20);

		offset	= 0x500 + math.fmod(oy * 0x10 + math.floor(ox / 0x10) * 0xC0 + math.fmod(ox, 0xD0), 0x1A0);
		return offset;

end;


-- ****************************************************************************
-- * smbram2px( memory offset )
-- * Gives the current top-left pixel of the tile the offset represents.
-- ****************************************************************************
function smbram2px(offset)
	
	offset		= offset - 0x500;
	if offset < 0 or offset >= 0x1A0 then
		return false;
	end;

	local px	= (math.fmod(offset, 0x10) + math.floor(offset / 0xD0) * 0x10) * 0x10;
	px			= px - math.fmod(smbdata['screenpos'], 0x200);
	if px < 0 then
		px		= px + 0x200;
	end;

	local py	= math.floor(math.fmod(offset, 0xD0) / 0x10);
	returnval	= {x = px, y = py};
	return returnval;

end;


-- ****************************************************************************
-- * smbmoveenemy( Enemy number, xpos, ypos, x accell, y accell )
-- * moves enemies to given point. auto-sets facing direction, as well
-- ****************************************************************************
function smbmoveenemy(n, x, y, ax, ay)

	local x1	= math.fmod(x, 0x100);
	local x2	= math.floor(x / 0x100);
	local y1	= math.fmod(y, 0x100);
	local y2	= math.floor(y / 0x100);
	local ax	= math.max(-128, math.min(ax, 0x7F));
	local ay	= math.max(-128, math.min(ay, 0x7F));

	memory.writebyte(0x006D + n, x2);
	memory.writebyte(0x0086 + n, x1);
	memory.writebyte(0x00B5 + n, y2);
	memory.writebyte(0x00CE + n, y1);
	memory.writebyte(0x0057 + n, ax);
	memory.writebyte(0x009F + n, ay);
	
	if ax > 0 then
		memory.writebyte(0x0045 + n, 1);
	elseif ax < 0 then
		memory.writebyte(0x0045 + n, 2);
	end;

end;


-- ****************************************************************************
-- * inputaverage()
-- * Mouse movement averages (something unique to this...).
-- ****************************************************************************
function inputaverage()

	local tempx	= 0;
	local tempy	= 0;
	for temp = 1, 2 do
		tempx				= tempx	+ avgmove[temp]['xmouse'];
		tempy				= tempy	+ avgmove[temp]['ymouse'];
		avgmove[temp]		= avgmove[temp + 1];
	end;
	avgmove[3]['xmouse']			= inpt['xmouse'] - last['xmouse'];
	avgmove[3]['ymouse']			= inpt['ymouse'] - last['ymouse'];
	avgmove['calc']['xmouse']	= (tempx + avgmove[3]['xmouse']) / 3;
	avgmove['calc']['ymouse']	= (tempy + avgmove[3]['ymouse']) / 3;

end;


-- ****************************************************************************
-- * tileview()
-- * This does all the "what tile is here" shit for you (me)
-- ****************************************************************************
function tileview ()
	local ramval		= smbpx2ram(inpt['xmouse'], inpt['ymouse']);
	if ramval then
		local ret	= smbram2px(ramval);
		local c	= "#ffffff";
		if math.fmod(timer, 4) < 2 then
			c	= "#cccccc";
		end;
		if ret then
			local tx1	= math.max(0, ret['x'] - 1);
			local tx2	= math.min(0xFF, ret['x'] + 0x10);
			local ty1	= math.max(ret['y'] * 0x10 + 0x1F, 0);
			local ty2	= math.min(244, ret['y'] * 0x10 + 0x30);
			box(tx1, ty1, tx2, ty2, c);
		end;

		local textx	= inpt['xmouse'] + 10;
		local texty	= inpt['ymouse'] -  4;
		
		if textx > 229 then
			textx	= textx - 42;
		end;
		texty	= math.min(214, texty);

		text(textx, texty, string.format("%04X", ramval));
		text(textx, texty + 8, string.format(" %02X ", memory.readbyte(ramval)));
	end;
end;


-- ****************************************************************************
-- * Generic test function, really. Does nothing of use.
-- * Incidentally, most of the times this shows up, it doesn't
-- ****************************************************************************
function test(arg)
	
	text(50, 100, "IT WORKS");

end;


-- ****************************************************************************
-- * spawnsetup(arg)
-- * Prepares spawning of an enemy.
-- * Perhaps should create a dialog box in the middle of the screen?..
-- ****************************************************************************
function spawnsetup(args)
	
	if mode ~= 2 then
		spawndata['lmode']	= mode;
		mode				= 2;
		spawndata['enum']	= 0x00;
		spawndata['ename']	= "Green Koopa";
		spawndata['etype']	= 0;
		spawndata['exs']	= 16;
		spawndata['eys']	= 24;
		-- etype:
		--- 0: normal enemy (one slot)
		--- 1: big enemy (two slots, takes latter but fills both)
		--- 2: powerup (takes slot 6)
	end;
end;


-- ****************************************************************************
-- * spawnsetup(arg)
-- * Prepares spawning of an enemy.
-- * Perhaps should create a dialog box in the middle of the screen?..
-- ****************************************************************************
function spawnenemy(args)
	
	local c	= "#ffffff";
	if math.fmod(timer, 4) < 2 then
		c	= "#888888";
	end;

	local freespace	= 0;

	if debugmodes['showenemydata'] then
		for i = 1, 6 do 
			text(8, 8 + 8 * i, string.format("%d %02X", i, memory.readbyte(0x000E+i)));
		end;
	end;
	for i = 1, 6 do 
		if ((spawndata['etype'] <= 1 and i <= 5) or (spawndata['etype'] == 2 and i == 6)) and memory.readbyte(0x000E+i) == 0 then
			if debugmodes['showenemydata'] then
				text(8, 8 + 8 * i, string.format("%d %02X *", i, memory.readbyte(0x000E+i)));
			end;
			if (spawndata['etype'] == 1 and memory.readbyte(0x000E + i - 1) == 0) or spawndata['etype'] ~= 1 then
				freespace	= i;
				break;
			end;
		end;
	end;

	if freespace > 0 then
		box(inpt['xmouse'] - (spawndata['exs'] / 2), inpt['ymouse'] - (spawndata['eys'] / 2), inpt['xmouse'] + (spawndata['exs'] / 2), inpt['ymouse'] + (spawndata['eys'] / 2),  c);
		text(70, 31, string.format("Summon [%s]", spawndata['ename']));
		if debugmodes['showenemydata'] then
			text(70, 39, string.format("Enemy slot [%X]", freespace));
		end;
		local mx	= smbdata['screenpos'] + inpt['xmouse'];
		local my	= 0x100 + inpt['ymouse'];
		if inpt['leftclick'] and not last['leftclick'] then
			memory.writebyte(0x000E + freespace, 1);
			memory.writebyte(0x0015 + freespace, spawndata['enum']);
			memory.writebyte(0x0499 + freespace, 3);
			smbmoveenemy(freespace, mx - (spawndata['exs'] / 2), my - (spawndata['eys'] / 2), -1, 0);
			mode	= spawndata['lmode'];
		end;

	else
		text(70, 31, string.format("Can't summon (too many enemies)!"));
	
	end;

end;


-- ****************************************************************************
-- * modechange(arg)
-- * changes current mode (used in menu).
-- * also changes menu text to reflect new mode.
-- ****************************************************************************
function modechange(args)
	
	mode	= args[1];
	if args[1] == 0 then
		mainmenu['menu']['m001_mode']['menu']['m001_tiles']['marked']	= 1;
		mainmenu['menu']['m001_mode']['menu']['m002_enemy']['marked']	= 0;
	else
		mainmenu['menu']['m001_mode']['menu']['m001_tiles']['marked']	= 0;
		mainmenu['menu']['m001_mode']['menu']['m002_enemy']['marked']	= 1;
	end;
end;


-- ****************************************************************************
-- * debugmode(arg)
-- * changes debugmode flags
-- * useful for on-the-fly checking, I guess
-- ****************************************************************************
function debugmode(args)
	
	if debugmodes[args[2]] == false then
		debugmodes[args[2]]	= true;
		mainmenu['menu']['m999_debug']['menu'][args[1]]['marked']	= 1;
	else
		debugmodes[args[2]]	= false;
		mainmenu['menu']['m999_debug']['menu'][args[1]]['marked']	= 0;
	end;
end;





mainmenu	= {
	test	= 1;
	life	= 0;
	width	= 54;
	menu	= {
		m001_mode	= {
			label	= "Mode",
			life	= 0;
			width	= 50;
			menu	= {
				m001_tiles	= {
					label	= " Objects",
					action	= modechange,
					args	= {0},
					marked	= 0;
					},
				m002_enemy	= {
					label	= " Sprites",
					action	= modechange,
					args	= {1},
					marked	= 1;
					},
				},
			},
		m002_summon	= {
			label	= "Summon",
			action	= spawnsetup,
			args	= {1},
			},
		},
};

if debugmodes['enabled'] then
	mainmenu['menu']['m999_debug']	= {
			label	= "Debug",
			life	= 0;
			width	= 90;
			menu	= {
				m000_showmouse = {
					label	= " Draw mouse",
					action	= debugmode,
					marked	= debugmodes['drawmouse'] and 1 or 0;
					args	= {"m000_showmouse", "drawmouse"},
					},
				m001_enemydata = {
					label	= " Show enemy data",
					action	= debugmode,
					marked	= debugmodes['showenemydata'] and 1 or 0;
					args	= {"m001_enemydata", "showenemydata"},
					},
				m002_locktimer = {
					label	= " Lock timer",
					action	= debugmode,
					marked	= debugmodes['locktimer'] and 1 or 0;
					args	= {"m002_locktimer", "locktimer"},
					},
				m003_invincible	= {
					label	= " Invincibility",
					action	= debugmode,
					marked	= debugmodes['invincible'] and 1 or 0;
					args	= {"m003_invincible", "invincible"},
					},
				},
			};
end;




smbdata		= {screenpos = 0};
mode		= 1;
enemyhold	= {};
avgmove		= { 
				{	xmouse = 0,	ymouse = 0	}, 
				{	xmouse = 0,	ymouse = 0	}, 
				{	xmouse = 0,	ymouse = 0	}, 
				calc = {} 
			};
spawndata	= {};

while (true) do 


	input.update();			-- updates mouse position
	inputaverage();			-- average movement (for throwing)

	smbdata['screenposold']	= smbdata['screenpos'];
	smbdata['screenpos']	= memory.readbyte(0x071a) * 0x100 + memory.readbyte(0x071c);
	smbdata['screenposchg']	= smbdata['screenpos'] - smbdata['screenposold'];
	smbdata['rendercol']	= memory.readbyte(0x06A0);
	if smbdata['screenposchg'] < 0 then
		smbdata['screenposchg'] = 0;
	end;
	timer	= timer + 1;


	if debugmodes['enabled'] then
		if control.button( 234, 15,  19, 2, "SET\n999") then
			memory.writebyte(0x07F8, 0x09);
			memory.writebyte(0x07F9, 0x09);
			memory.writebyte(0x07FA, 0x09);
		end;

		if debugmodes['locktimer'] then
			memory.writebyte(0x0787, 0x1F);
		end;

		if debugmodes['invincible'] then
			memory.writebyte(0x079E, 0x02);
		end;
	end;

	
	if mode == 0 then
		tileview();
	elseif mode == 2 then
		
		spawnenemy();
		

	else
		if debugmodes['showenemydata'] then
			text(0, 25 + 0, string.format("E# XPOS YPOS XREL YREL XA YA TY HB"));
		end;

		for i=1,6 do  
			if (memory.readbyte(0x000E+i) ~= 0) then --and memory.readbyte(0x04AC+(i*4)) ~= 0xFF) and (memory.readbyte(0x0015 + i) ~= 0x30 and memory.readbyte(0x0015 + i) ~= 0x31) then 
				if not enemyhold[i] or not inpt['leftclick'] then
					enemyhold[i]	= nil;
--					text(8, 50 + i * 8, "-");
				elseif enemyhold[i] then
--					text(8, 50 + i * 8, string.format("HOLD  %04X %04X", smbdata['screenpos'] + inpt['xmouse'] - enemyhold[i]['xmouse'], inpt['ymouse'] + 0x100 - enemyhold[i]['ymouse']));
					smbmoveenemy(i, smbdata['screenpos'] + inpt['xmouse'] - enemyhold[i]['x'], inpt['ymouse'] + 0x100 - enemyhold[i]['y'], (avgmove['calc']['xmouse']) * 8, avgmove['calc']['ymouse'] / 2.5);
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
				enemytype	= memory.readbyte(0x0015 + i);
				enemyhitbox	= memory.readbyte(0x0499 + i);

				dead		= "";
				if enemyyposa <= -72 + 32 and enemyyacc < -3 then
					dead	= "UP";
				end;
				if math.abs(enemyxacc) >= 0x60 then
					dead	= dead .. "SIDE";
				end;
				if dead ~= "" then
					dead	= " ".. dead;
				end;

				if debugmodes['showenemydata'] then
					line(enemyxposa, enemyyposa2, enemyxposa + 16, enemyyposa2, "#ffffff");
					text(enemyxposa, enemyyposa2, memory.readbyte(0x0045 + i));
					text(1, 25 + 8 * i, string.format("E%X %04X %04X %04X %04X %02X %02X %02X %02X %s", i, 
						AND(enemyxpos, 0xFFFF), 
						AND(enemyypos, 0xFFFF), 
						AND(enemyxposa, 0xFFFF), 
						AND(enemyyposa, 0xFFFF), 
						AND(enemyxacc, 0xFF), 
						AND(enemyyacc, 0xFF), 
						AND(enemytype, 0xFF), 
						AND(enemyhitbox, 0xFF), 
						"")); -- dead));
				end;

				if hitbox(inpt['xmouse'], inpt['ymouse'], inpt['xmouse'], inpt['ymouse'], e2x1, e2y1, e2x2, e2y2, "#ffffff", "#0000ff") then
--				if hitbox(inpt['xmouse'], inpt['ymouse'], inpt['xmouse'], inpt['ymouse'], enemyxposa, enemyyposa, enemyxposa + 0xF, enemyyposa + 0x17) then
					
--					text(e2x1 - 5, e2y1 - 13, string.format("#%d %02X", i, memory.readbyte(0x0015 + i)));


					if inpt['leftclick'] then

						if not enemyhold[i] then 
							enemyhold[i]	= { x = inpt['xmouse'] - enemyxposa, y = inpt['ymouse'] - enemyyposa };
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

	box(zap['xmouse'] - 5, zap['ymouse'] - 5, zap['xmouse'] + 5, zap['ymouse'] + 5, "#ffffff");
	if zap['click'] == 1 then 
		box(zap['xmouse'] - 7, zap['ymouse'] - 7, zap['xmouse'] + 7, zap['ymouse'] + 7, "#ff0000");
	end;
	line(zap['xmouse'] - 0, zap['ymouse'] - 9, zap['xmouse'] + 0, zap['ymouse'] + 9, "#ffffff");
	line(zap['xmouse'] - 9, zap['ymouse'] - 0, zap['xmouse'] + 9, zap['ymouse'] + 0, "#ffffff");
	]]



	-- almost always
	if control.button(78, 14, 54, 1, "<Main Menu>") then
		mainmenu['life']	= 70;
	end;
	control.showmenu(78, 25, mainmenu);

	text( 20, 222, " 2009 Xkeeper - http://jul.rustedlogic.net/ ");
	line(  21, 231, 232, 231, "#000000");

	if debugmodes['enabled'] then
		text( 41, 214, " Debug version - ".. debugmodes['ver'] .." ");
	end;
	

	-- always on top
	if debugmodes['drawmouse'] then
		drawmouse(inpt['xmouse'], inpt['ymouse'], inpt['leftclick']);
	end;
	FCEU.frameadvance();

end