-- unfinished mario bros 2 script
-- shows (proper!) grid for horizontal levels (b0rks in vertical levels for now)
-- shows any non-air grid's tile-id
-- is sloooow :p

-- Super Mario Bros. 2 (U) (PRG0) [!].rom
-- qFox
-- 31 july 2008

local function box(x1,y1,x2,y2,color)
	-- gui.text(50,50,x1..","..y1.." "..x2..","..y2);
	if (x1 > 0 and x1 < 0xFF and x2 > 0 and x2 < 0xFF and y1 > 0 and y1 < 239 and y2 > 0 and y2 < 239) then
		gui.drawbox(x1,y1,x2,y2,color);
	end;
end;
local function text(x,y,str)
	if (x > 0 and x < 0xFF and y > 0 and y < 240) then
		gui.text(x,y,str);
	end;
end;
local function toHexStr(n)
	local meh = "%X";
	return meh:format(n);
end;

while (true) do
	if (memory.readbyte(0x0010) > 0x81) then memory.writebyte(0x0010, 0x6D); end; -- birdo fires eggs constantly :p
	
	-- px = horzizontal page of current level
	-- x = page x (relative to current page)
	-- rx = real x (relative to whole level)
	-- sx = screen x (relative to viewport)
	local playerpx = memory.readbyte(0x0014);
	local playerpy = memory.readbyte(0x001E);
	local playerx = memory.readbyte(0x0028);
	local playery = memory.readbyte(0x0032);
	local playerrx = (playerpx*0xFF)+playerx;
	local playerry = (playerpy*0xFF)+playery;
	local playerstate = memory.readbyte(0x0050);
	local screenoffsetx = memory.readbyte(0x04C0);
	local screenoffsety = memory.readbyte(0x00CB);

	local playersx = (playerx - screenoffsetx);
	if (playersx < 0) then playersx = playersx + 0xFF; end;
	
	local playersy = (playery - screenoffsety);
	if (playersy < 0) then playersy = playersy + 0xFF; end;
	
	if (playerstate ~= 0x07) then
		box(playersx, playersy, playersx+16, playersy+16, "green");
	end;

	text(2,10,"x:"..screenoffsetx);
	text(2,25,"y: "..screenoffsety);
	text(230,10,memory.readbyte(0x04C1));
	text(100,10,"Page: "..playerpx..","..playerpy);
	text(playersx,playersy,playerrx.."\n"..playery);
	
	local startpx = 0x0015;
	local startpy = 0x001F;
	local startx = 0x0029;
	local starty = 0x0033;
	local drawn = 0x0051;
	local type = 0x0090;
	for i=0,9 do
		local estate = memory.readbyte(drawn+i);
		if (estate ~= 0) then
			local ex = memory.readbyte(startx+i);
			local epx = memory.readbyte(startpx+i);
			local ey = memory.readbyte(starty+i);
			local epy = memory.readbyte(startpy+i);
			local erx = (epx*0xFF)+ex;
			local ery = (epy*0xFF)+ey;
			local esx = (ex - screenoffsetx);
			if (esx < 0) then esx = esx + 0xFF; end;
			local esy = (ey - screenoffsety);
			if (esy < 0) then esy = esy + 0xFF; end;

			--text(10, 20+(16*i), i..": "..esx.." "..erx); -- show enemy position list

			-- show enemy information
			if ((erx > playerrx-127) and erx < (playerrx+120)) then
				--text(esx,esy,erx); -- show level x pos above enemy
				local wtf = "%X";
				text(esx,esy,wtf:format(memory.readbyte(type+i))); -- show enemy code
				if (estate == 1 and i < 5) then
					box(esx, esy, esx+16, esy+16, "red");
				else
					box(esx, esy, esx+16, esy+16, "blue");
				end;
			end;
		end; -- enemy info

		-- show environment
		-- i have playerrx, which is my real position in this level
		-- i have the level, which is located at 0x6000 (the SRAM)
		-- each tile (denoted by one byte) is 16x16 pixels
		-- each screen is 15 tiles high and about 16 tiles wide
		-- to get the right column, we add our playerrx/16 to 0x6000
		-- to be exact:
		-- 0x6000 + (math.floor(playerrx/16) * 0xF0) + math.mod(playerx,0x0F)
		
		local levelstart = 0x6000; -- start of level layout in RAM
		local addleftcrap = math.mod(screenoffsetx,16)*-1;
		local leftsplitrx = (memory.readbyte(0x04BE)*0xFF) + (screenoffsetx + addleftcrap); -- column start left. add addleftcrap to iterative stuff to build up
		local columns = leftsplitrx/16; -- column x of the level is on the left side of the screen
		
		for i=0,15 do
			text(addleftcrap+(i*16)-1, 37, toHexStr(columns+i));
			for j=0,17 do
				box(addleftcrap+(i*16), 1+(j*16), addleftcrap+(i*16)+16, 1+(j*16)+16, "green");
			end;
		end;
		
		local columntop = levelstart + ((math.floor(columns/16))*0xF0) + math.mod(columns,16);
		text(10,80,toHexStr(columns).." "..toHexStr(columntop));
		for i=0,14 do
			for j=0,15 do
				local tile = memory.readbyte(columntop+(i*0x10)+j);
				if (tile ~= 0x40) then
					text(-2+addleftcrap+(j*16),21+(i*16),toHexStr(tile));
				end;
			end;
		end;
		
	end;
	FCEU.frameadvance();
end;