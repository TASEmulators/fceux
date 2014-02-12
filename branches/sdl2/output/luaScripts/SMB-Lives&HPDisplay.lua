-- Super Mario Bros. script by 4matsy.
-- 2008, September 11th.
--Displays the # of lives for Mario and a HP meter for Bowswer

require("shapedefs");

function box(x1,y1,x2,y2,color)
	if (x1 > 0 and x1 < 255 and x2 > 0 and x2 < 255 and y1 > 0 and y1 < 241 and y2 > 0 and y2 < 241) then
		gui.drawbox(x1,y1,x2,y2,color);
	end;
end;

function line(x1,y1,x2,y2,color)
	if (x1 > 0 and x1 < 255 and x2 > 0 and x2 < 255 and y1 > 0 and y1 < 241 and y2 > 0 and y2 < 241) then
		gui.drawline(x1,y1,x2,y2,color);
	end;
end;

function text(x,y,str)
	if (x > 0 and x < 255 and y > 0 and y < 240) then
		gui.text(x,y,str);
	end;
end;

function pixel(x,y,color)
	if (x > 0 and x < 255 and y > 0 and y < 240) then
		gui.drawpixel(x,y,color);
	end;
end;



while (true) do
	
	-- print player's lives...I always thought this was a major omission of the status bar :p
	text(63,13,"x"..memory.readbyte(0x075a)+1);

     -- check the enemy identifier buffer for presence of t3h b0ws3r (id #2d). if found, show his hp.
	for i=0,5 do
		if memory.readbyte((0x0016)+i) == 0x2d then -- aha, found you. YOU CANNOT HIDE FROM MY MAD SCRIPTZ0RING SKILLZ BWAHAHA
			local bowsermaxhp = memory.readbyte(0xc56c); -- bowser's starting hp
			local bowsercurhp = memory.readbyte(0x0483); -- bowser's current hp
			local meterx = 104;                          -- x-origin of the meter
			local metery = 228;                          -- y-origin of the meter
			local spacingx = 8;                          -- how much x-space between each shape?
			local spacingy = 0;                          -- how much y-space between each shape?
			text((meterx-2),(metery-11),"Bowser:");
			for a=0,bowsermaxhp-1 do
				drawshape((meterx+(spacingx*a)+0),(metery+(spacingy*a)+1),"heart_7x7","#000000");
				drawshape((meterx+(spacingx*a)+1),(metery+(spacingy*a)+0),"heart_7x7","#000000");
				drawshape((meterx+(spacingx*a)+1),(metery+(spacingy*a)+1),"heart_7x7","#000000");
				drawshape((meterx+(spacingx*a)+0),(metery+(spacingy*a)+0),"heart_7x7","#ffffff");
				if a < bowsercurhp then
					drawshape((meterx+(spacingx*a)+1),(metery+(spacingy*a)+1),"heart_5x5","#ff0000");
				end;
			end;
		end;
	end;
	FCEU.frameadvance();
end;
