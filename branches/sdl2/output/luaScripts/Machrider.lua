--Machrider - Speedometer
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


function savereplay(filename, replaydata) 

	stringout	= "";

	f	= io.open(filename, "w");
	for k, v in pairs(replaydata) do
		stringout		= string.format("%d:%d:%d\n", k, v['speed'], v['gear']);
		f:write(stringout);
	end;

	f:close();

	return true;

end;



olddist			= 0;
lastcount		= 0;
counter			= 0;
altunittype		= true;
waittimer		= 0;
autoshift		= true;		-- auto-handle shifting? for the lazy people
speedgraph		= {};
timer			= 0;
graphlen		= 240;
graphheight		= 100;
maxspeed		= 500;
dorecording		= false;

while true do
	
	joydata		= joypad.read(1);
	if joydata['select'] and not joydatas then
		altunittype	= not altunittype;
		joydatas	= true;

	elseif joydata['up'] and joydata['B'] and not joydatas and dorecording then
		savereplay("machriderspeed.xrp", speedgraph);
		joydatas	= true;
		saved		= true;

	elseif not joydata['select'] then
		joydatas	= false;
	end;

	if saved and dorecording then
		text(100, 140, "Saved data!");
	end;

	speedlow	= memory.readbyte(0x0040);
	speedhigh	= memory.readbyte(0x0041);
	speed		= speedhigh * 0x100 + speedlow;
	rpmlow		= memory.readbyte(0x0042);
	rpmhigh		= memory.readbyte(0x0043);
	rpm			= rpmhigh * 0x100 + rpmlow;
	gear		= memory.readbyte(0x0032);

	
	if autoshift and waittimer <= 0 then
		if gear < 2 then
			memory.writebyte(0x0032, 2);
--			waittimer	= 6;
		elseif speed <= 250 and gear > 2 then
			memory.writebyte(0x0032, math.max(gear - 1, 2));
--			waittimer	= 6;
		elseif speed >= 255 and gear < 3 then
			memory.writebyte(0x0032, math.min(gear + 1, 3));
--			waittimer	= 6;
		end;
	end;
	waittimer	= waittimer - 1;

	if dorecording then
		timer				= timer + 1;
		speedgraph[timer]	= {speed = speed, gear = gear};
		if timer > graphlen then
			temp				= timer - graphlen - 1;
	--		speedgraph[temp]	= nil;
		end;


		for i = timer - graphlen, timer do
			if speedgraph[i] then
				xp		= ((i + 3) - timer) + graphlen;
				yp		= graphheight - (speedgraph[i]['speed'] / maxspeed) * graphheight;

				if (speedgraph[i]['gear'] == 0) then
					c	= "blue";
				elseif (speedgraph[i]['gear'] == 1) then
					c	= "green";
				elseif (speedgraph[i]['gear'] == 2) then
					c	= "#cccc00";
				elseif (speedgraph[i]['gear'] == 3) then
					c	= "red";
				else
					c	= "gray";
				end;

	--			pixel(((i + 3) - timer) + 60, 50 - speedgraph[i], "#ffffff");
				line(xp, 10, xp, graphheight + 10, "#000000");
				line(xp, yp + 10, xp, graphheight + 10, c);
				pixel(xp, yp + 10, "#ffffff");

	--			pixel(((i + 3) - timer) + 60, 50 - speedgraph[i], "#ffffff");
			end;
		end;
	end;

--[[
	dist		= math.fmod(memory.readbyte(0x0062), 0x20);
	text( 8, 15, string.format("%02X   %4dfr/mv", dist, lastcount));
	if dist > olddist then
		lastcount	= counter;
		counter		= 0;
	end;
	olddist			= dist;
	counter		= counter + 1;

	lifebar( 8,  8, 0x1F * 5, 3, dist, 0x1F, "#ffffff", "#0000FF", "#000000", "#ffffff");
--]]

	barwidth			= 100;
	segmentwidth		= 2;
	pct					= speed / maxspeed * 100;

	if altunittype then
		speedadjust		= (65535 / 60 / 60 / 60) * speed;	-- rough approximation of km/h
		text(barwidth * segmentwidth - 2, 221, string.format(" %3.1dkm/h", speedadjust));
	else
		text(barwidth * segmentwidth - 2, 221, string.format(" %3d", speed));
	end;

	box(2, 221, barwidth * segmentwidth + 2, 230, "#000000");
	box(2, 222, barwidth * segmentwidth + 2, 229, "#000000");
	box(2, 223, barwidth * segmentwidth + 2, 228, "#000000");
	box(2, 224, barwidth * segmentwidth + 2, 227, "#000000");
	box(2, 225, barwidth * segmentwidth + 2, 226, "#000000");
	lastseg		= false;

	if pct > 0 then 
		for bl = 1, math.min(maxspeed - 1, speed) do
		
			pct		= bl / maxspeed;
			segment	= math.floor(pct * barwidth);
			if segment ~= lastseg then

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


				yb			= math.max(math.min(3, (pct * 100 - 50)), 0);
	--			box(segment * segmentwidth + 3, 225 - yb, segment * segmentwidth + 3 + (segmentwidth - 2), 229, segcolor);
				box(segment * segmentwidth + 3, 225 - yb, segment * segmentwidth + 3, 229, segcolor);
	--			box(bl * 3 + 3, 218, bl * 3 + 4, 225, segcolor);
	--			line(bl * 3 + 4, 218, bl * 3 + 4, 225, segcolor);
			end;
			lastseg	= segment;
		end;
	end;



	maxrpm				= 0x7F;
	barwidth			= 104;
	segmentwidth		= 1;
	pct					= rpmhigh / maxrpm * 100;

	line(2, 220, (barwidth + 2) * segmentwidth + 2, 220, "#000000");

	if autoshift then
		text( 2, 203, string.format("AUTO %1d", gear + 1));
	else
		text( 2, 203, string.format("Manual %1d", gear + 1));
	end;
	if altunittype then
		text( 2, 211, string.format("%5dRPM", math.min(9999, rpm * .25)));
	else
		text( 2, 211, string.format("%5dRPM", rpm));
	end;

	rpmhigh				= math.min(maxrpm + 10, rpmhigh);
	lastseg		= false;

	if pct > 0 then 
		for bl = 1, rpmhigh do
		
			pct		= bl / maxrpm;
			segment	= math.floor(pct * barwidth);
			if segment ~= lastseg then
				if pct < 0.70 then
					val		= math.floor(pct * (100/70) * 0xFF);
					segcolor	= string.format("#%02XFF00", val);

				elseif pct < 0.90 then
					val		= math.floor(0xFF - (pct - 0.7) * 100/20 * 0xFF);
					segcolor	= string.format("#FF%02X00", val);

				elseif bl < maxrpm then
					val	= math.floor((pct - 0.90) * 10 * 0xFF);
					segcolor	= string.format("#FF%02X%02X", val, val);

				else
					segcolor	= "#ffffff";
				end;

				yb			= math.floor(math.max(math.min(4, segment - 99), 0) / 2);
	--			box(segment * segmentwidth + 3, 225 - yb, segment * segmentwidth + 3 + (segmentwidth - 2), 229, segcolor);
				segment			= math.min(segment, barwidth);
				box(segment * segmentwidth + 3, 221, segment * segmentwidth + 3, 223 - yb, segcolor);
	--			box(bl * 3 + 3, 218, bl * 3 + 4, 225, segcolor);
	--			line(bl * 3 + 4, 218, bl * 3 + 4, 225, segcolor);
			end;
			lastseg	= seg;
		end;
	end;




	FCEU.frameadvance();
end;