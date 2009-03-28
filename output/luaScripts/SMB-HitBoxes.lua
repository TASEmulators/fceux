-- Super Mario Bros.  hitbox script
-- Super Mario Bros (JU) (PRG0) [!].nes
-- Written by qFox
-- 28 july 2008

-- This script shows hitboxes of anything that has them
-- It also displays the found hitboxes in a output window with iup
-- Includes a toggle to automatically kill all enemies within 50px range of mario :p

-- Include our help script to load iup and take care of exit
require("auxlib");

local running = true;
local restrainingorder = false;
local myoutput;
function createGUI(n)
	local mybutton = iup.button{title="Close (exits the main loop)"};
	mybutton.action = function(self, x) 
											running = false;
											--handles[n]:destroy(); 
											--handles[n] = false; 
										end;
	myoutput = iup.multiline{size="200x200",expand="YES",value="Debug crap should be here"}
	nottooclose = iup.toggle{title="Kill enemies that come too close", value="OFF"};
	nottooclose.action = function(self, v) restrainingorder = (v == 1); end; -- v is 0 or 1
	handles[n] = 
		iup.dialog{
		  iup.frame
		  {                   
				iup.vbox
				{
					mybutton,
					nottooclose,
					myoutput,
					title="Lua tools are izi!"
				}
			}
		};
	handles[n]:showxy(iup.CENTER, iup.CENTER)
end

dialogs = dialogs + 1;
createGUI(dialogs);

local outstr;
local function knifeEnemyIfTooClose(enemynumber) -- enemynumber starts at 1
	-- we add the suffix for x coords because the screen is 255 positions wide but smb has two pages
	-- loaded at any given time. some enemy can be in page 2 while you are on page one. since a byte
	-- can only hold 255 values, this would wrap around and make it seem like the enemy is on the same
	-- page. hence we add the number of pages they are one, if they are equal, it all works out :)
	local mx = memory.readbyte(0x0086)+(255*memory.readbyte(0x006D));
	local my = memory.readbyte(0x00CE);
	local ex = memory.readbyte(0x0086+enemynumber)+(255*memory.readbyte(0x006D+enemynumber));
	local ey = memory.readbyte(0x00CE+enemynumber);
	local d = math.sqrt(((mx-ex)^2)+((my-ey)^2)); -- pythagoras ;)
	outstr = outstr .. d.." < 30.0 ?\n";
	if (d < 50.0 and restrainingorder and memory.readbyte(0x0015+enemynumber) ~= 40) then -- dont kill of horizontal moving platforms. we kinda need them.
			-- KNIFE!
			outstr = outstr .. "Knifing next enemy!\n";
			memory.writebyte(0x001D+enemynumber, 0xFF); -- this address denotes an enemy state. writing FF to it kills them as if hit by a star (so not flatten).
	end;
	return d;
end;

-- draw a box and take care of coordinate checking
local function box(x1,y1,x2,y2,color)
	-- gui.text(50,50,x1..","..y1.." "..x2..","..y2);
	if (x1 > 0 and x1 < 255 and x2 > 0 and x2 < 255 and y1 > 0 and y1 < 224 and y2 > 0 and y2 < 224) then
		gui.drawbox(x1,y1,x2,y2,color);
	end;
end;

-- hitbox coordinate offsets (x1,y1,x2,y2)
local mario_hb = 0x04AC; -- 1x4
local enemy_hb = 0x04B0; -- 5x4
local coin_hb  = 0x04E0; -- 3x4
local fiery_hb = 0x04C8; -- 2x4
local hammer_hb= 0x04D0; -- 9x4
local power_hb = 0x04C4; -- 1x4

-- addresses to check, to see whether the hitboxes should be drawn at all
local mario_ch = 0x000E;
local enemy_ch = 0x000F;
local coin_ch  = 0x0030;
local fiery_ch = 0x0024;
local hammer_ch= 0x002A;
local power_ch = 0x0014;

local a,b,c,d;
while (running) do
		outstr = '';
		-- from 0x04AC are about 0x48 addresse that indicate a hitbox
		-- different items use different addresses, some share
		-- there can for instance only be one powerup on screen at any time (the star in 1.1 gets replaced by the flower, if you get it)
		-- we cycle through the animation addresses for each type of hitbox, draw the corresponding hitbox if they are drawn
		-- we draw: mario (1), enemies (5), coins (3), hammers (9), powerups (1). (bowser and (his) fireball are considered enemies)

		-- mario
		if (memory.readbyte(mario_hb) > 0) then 
			a,b,c,d = memory.readbyte(mario_hb),memory.readbyte(mario_hb+1),memory.readbyte(mario_hb+2),memory.readbyte(mario_hb+3);
			box(a,b,c,d, "green"); 
			outstr = outstr .. "Mario: <"..a..","..b..","..c..","..d..">\n";
		end;
		
		-- enemies
		if (memory.readbyte(enemy_ch  ) > 0) then 
			a,b,c,d = memory.readbyte(enemy_hb),   memory.readbyte(enemy_hb+1), memory.readbyte(enemy_hb+2), memory.readbyte(enemy_hb+3);
			box(a,b,c,d, "green");
			outstr = outstr .. "Enemy 1: <"..memory.readbyte(0x0016).."> <"..a..","..b..","..c..","..d.."> "..knifeEnemyIfTooClose(1).."\n";
		end;
		if (memory.readbyte(enemy_ch+1) > 0) then 
			a,b,c,d = memory.readbyte(enemy_hb+4), memory.readbyte(enemy_hb+5), memory.readbyte(enemy_hb+6), memory.readbyte(enemy_hb+7);
			box(a,b,c,d, "green");
			outstr = outstr .. "Enemy 2: <"..memory.readbyte(0x0017).."> <"..a..","..b..","..c..","..d.."> "..knifeEnemyIfTooClose(2).."\n";
		end;
		if (memory.readbyte(enemy_ch+2) > 0) then 
			a,b,c,d = memory.readbyte(enemy_hb+8), memory.readbyte(enemy_hb+9), memory.readbyte(enemy_hb+10),memory.readbyte(enemy_hb+11);
			box(a,b,c,d, "green");
			outstr = outstr .. "Enemy 3: <"..memory.readbyte(0x0018).."> <"..a..","..b..","..c..","..d.."> "..knifeEnemyIfTooClose(3).."\n";
		end;
		if (memory.readbyte(enemy_ch+3) > 0) then 
			a,b,c,d = memory.readbyte(enemy_hb+12),memory.readbyte(enemy_hb+13),memory.readbyte(enemy_hb+14),memory.readbyte(enemy_hb+15);
			box(a,b,c,d, "green");
			outstr = outstr .. "Enemy 4: <"..memory.readbyte(0x0019).."> <"..a..","..b..","..c..","..d.."> "..knifeEnemyIfTooClose(4).."\n";
		end;
		if (memory.readbyte(enemy_ch+4) > 0) then 
			a,b,c,d = memory.readbyte(enemy_hb+16),memory.readbyte(enemy_hb+17),memory.readbyte(enemy_hb+18),memory.readbyte(enemy_hb+19)
			box(a,b,c,d, "green");
			outstr = outstr .. "Enemy 5: <"..memory.readbyte(0x001A).."> <"..a..","..b..","..c..","..d.."> "..knifeEnemyIfTooClose(5).."\n";
		end;

		-- coins
		if (memory.readbyte(coin_ch  ) > 0) then box(memory.readbyte(coin_hb),   memory.readbyte(coin_hb+1), memory.readbyte(coin_hb+2),  memory.readbyte(coin_hb+3),  "green"); end;
		if (memory.readbyte(coin_ch+1) > 0) then box(memory.readbyte(coin_hb+4), memory.readbyte(coin_hb+5), memory.readbyte(coin_hb+6),  memory.readbyte(coin_hb+7),  "green"); end;
		if (memory.readbyte(coin_ch+2) > 0) then box(memory.readbyte(coin_hb+8), memory.readbyte(coin_hb+9), memory.readbyte(coin_hb+10), memory.readbyte(coin_hb+11), "green"); end;
		
		-- (mario's) fireballs
		if (memory.readbyte(fiery_ch  ) > 0) then box(memory.readbyte(fiery_hb),   memory.readbyte(fiery_hb+1), memory.readbyte(fiery_hb+2), memory.readbyte(fiery_hb+3),  "green"); end;
		if (memory.readbyte(fiery_ch+1) > 0) then box(memory.readbyte(fiery_hb+4), memory.readbyte(fiery_hb+5), memory.readbyte(fiery_hb+6),memory.readbyte(fiery_hb+7), "green"); end;
		
		-- hammers
		if (memory.readbyte(hammer_ch  ) > 0) then box(memory.readbyte(hammer_hb),   memory.readbyte(hammer_hb+1), memory.readbyte(hammer_hb+2), memory.readbyte(hammer_hb+3),  "green"); end;
		if (memory.readbyte(hammer_ch+1) > 0) then box(memory.readbyte(hammer_hb+4), memory.readbyte(hammer_hb+5), memory.readbyte(hammer_hb+6), memory.readbyte(hammer_hb+7),  "green"); end;
		if (memory.readbyte(hammer_ch+2) > 0) then box(memory.readbyte(hammer_hb+8), memory.readbyte(hammer_hb+9), memory.readbyte(hammer_hb+10),memory.readbyte(hammer_hb+11), "green"); end;
		if (memory.readbyte(hammer_ch+3) > 0) then box(memory.readbyte(hammer_hb+12),memory.readbyte(hammer_hb+13),memory.readbyte(hammer_hb+14),memory.readbyte(hammer_hb+15), "green"); end;
		if (memory.readbyte(hammer_ch+4) > 0) then box(memory.readbyte(hammer_hb+16),memory.readbyte(hammer_hb+17),memory.readbyte(hammer_hb+18),memory.readbyte(hammer_hb+19), "green"); end;
		if (memory.readbyte(hammer_ch+5) > 0) then box(memory.readbyte(hammer_hb+20),memory.readbyte(hammer_hb+21),memory.readbyte(hammer_hb+22),memory.readbyte(hammer_hb+23), "green"); end;
		if (memory.readbyte(hammer_ch+6) > 0) then box(memory.readbyte(hammer_hb+24),memory.readbyte(hammer_hb+25),memory.readbyte(hammer_hb+26),memory.readbyte(hammer_hb+27), "green"); end;
		if (memory.readbyte(hammer_ch+7) > 0) then box(memory.readbyte(hammer_hb+28),memory.readbyte(hammer_hb+29),memory.readbyte(hammer_hb+30),memory.readbyte(hammer_hb+31), "green"); end;
		if (memory.readbyte(hammer_ch+8) > 0) then box(memory.readbyte(hammer_hb+32),memory.readbyte(hammer_hb+33),memory.readbyte(hammer_hb+34),memory.readbyte(hammer_hb+35), "green"); end;

		-- powerup
		if (memory.readbyte(power_ch) > 0) then box(memory.readbyte(power_hb),memory.readbyte(power_hb+1),memory.readbyte(power_hb+2),memory.readbyte(power_hb+3), "green"); end;

    gui.text(5,32,"Green rectangles are hitboxes!");

		if (myoutput) then
			myoutput.value = outstr;
		end;

    FCEU.frameadvance()
end

gui.popup("script exited main loop");

