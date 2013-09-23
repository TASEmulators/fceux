
-- Valkyrie no Bouken stuffs
-- This game sucks, don't play it
-- Lovingly based off of 4matsy's SMB code, and then mutilated and mamed as required
-- Xkeeper          2008, September 12th


require("x_functions");
x_requires(4);






-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************



function mapdot(x,y,color)
	if (x >= 1 and x <= 254 and y >= 1 and y <= 239) then
		gui.drawline(x - 1, y    , x + 1, y    , color);
		gui.drawline(x    , y - 1, x    , y + 2, color);
	end;
end;


-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function doexp()

	totalexp	= vnbnumber(0x00d5, 5);
	growth		= memory.readbyte(0x0111);
	level		= memory.readbyte(0x00b9);
	nextlv		= memory.readbyte(0x00bb);
	prevexp		= -1;

	if level ~= 0 then

		if nextlv < 0x14 then
			nextexp		= exptable[nextlv + 1];
		else
			prevexp		= math.floor(totalexp / 10000) * 10000;
			nextexp		= math.floor(totalexp / 10000) * 10000 + 10000;
		end;

		if growth < 3 and prevexp == -1 then
--			prevexp		= exptable[leveltable[growth][level] + 1];

		end;

		expval	= {};
		expval["level"]	= level;
		expval["next"]	= nextexp - totalexp;
		expval["prev"]	= prevexp;
		expval["pct"]	= math.floor((totalexp - prevexp) / (nextexp - prevexp) * 100);
		expval["exp"]	= totalexp;

		if prevexp == -1 then
			expval["pct"]	= -1;
		end;

	else

		expval	= {};
		expval["level"]	= 0;
		expval["next"]	= 0;
		expval["prev"]	= 0;
		expval["pct"]	= 0;
		expval["exp"]	= 0;

	end;

	return expval;
end;





-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function vnbnumber(offset, length)
	val		= 0;

	for i = 0, length do
		inp	= memory.readbyte(offset + (i));
		if (inp ~= 0x26) then val = val + inp * (10 ^ i); end;
	end;

	return val;
end;






-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function worldmap()

	herox		= memory.readbyte(0x0080) + memory.readbyte(0x0081) * 256;		-- hero's X position
	heroy		= memory.readbyte(0x0082) + memory.readbyte(0x0083) * 256;		-- hero's current MP

	if mapstyle == 1 then
		mapx		= 8;
		mapy		= 9;
		mapw		= 60;
		maph		= 37;
	elseif mapstyle == 2 or mapstyle == 3 then

		mapx		= 8;
		mapy		= 34;
		mapw		= 240;
		maph		= 147;

	else
		return nil;
	end;

	if gamemode == 0x05 or gamemode == 0x01 or gamemode == 0x06 or gamemode == 0x08 then

		maphx		= math.ceil(herox / 3840 * mapw);
		maphy		= math.ceil(heroy / 2352 * maph);


--		filledbox(mapx - 1, mapy - 1, mapx + mapw + 1, mapy + maph, "#000000");
		box(mapx - 1, mapy - 1, mapx + mapw + 1, mapy + maph, "#ffffff");

		if mapstyle == 3 then
			for i = 0, 0xFF do

				mappx		= math.ceil((3840 / 16) * math.fmod(i, 0x10) / 3840 * mapw);
				mappy		= math.ceil((2352 / 16) * math.floor(i / 0x10) / 2352 * maph);
				mappx2		= math.ceil((3840 / 16) * (math.fmod(i, 0x10) + 1) / 3840 * mapw) - 1;
				mappy2		= math.ceil((2352 / 16) * (math.floor(i / 0x10) + 1) / 2352 * maph) - 1;
				tmp			= memory.readbyte(0x81E5 + i) * 2;
				filledbox(mapx + mappx, mapy + mappy, mapx + mappx2, mapy + mappy2, string.format("#%02x%02x%02x", tmp, tmp, tmp));
				
			end;
		end;

		if math.fmod(timer, 60) >= 30 then
			color	= "#888888";
		else
			color	= "#bbbbbb";
		end;


--		line(mapx, mapy + maphy, mapx + mapw, mapy + maphy, "#cccccc");
--		line(mapx + maphx, mapy, mapx + maphx, mapy + maph, "#cccccc");

		mapdist		= 51;
		for i = 1, mappoints do
			mappx		= math.ceil(mapdots[i]["x"] / 3840 * mapw);
			mappy		= math.ceil(mapdots[i]["y"] / 2352 * maph);
			mapdot(mapx + mappx, mapy + mappy, mapdots[i]["color"]);
			if mapdots[i]["name"] then
				mapdotdist	= math.abs(mapdots[i]["x"] - herox) + math.abs(mapdots[i]["y"] - heroy);
				if mapdotdist < mapdist then
					mapdist		= mapdotdist;
					mapdistn	= mapdots[i]["name"];
				end;
			end;
		end;
			
		if mapdist <= 50 then
			text(90, 17, mapdistn);
		end;
		filledbox(mapx + maphx - 0, mapy + maphy - 0, mapx + maphx + 0, mapy + maphy + 0, "#ffffff");
		box(mapx + maphx - 1, mapy + maphy - 1, mapx + maphx + 1, mapy + maphy + 1, color);

--		text(mapx + 0, mapy + maph + 2, string.format("%04d, %04d", herox, heroy));


	end;
end;



-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function gameloop()

	if gamemode == 0x01 and math.fmod(timer, 60) >= 30 then
		text(105, 180, "< DEMO >");
	end;


	herohp		= memory.readbyte(0x00c0) + memory.readbyte(0x00c1) * 256;		-- hero's current HP
	heromp		= memory.readbyte(0x00c2) + memory.readbyte(0x00c3) * 256;		-- hero's current MP
	heromaxhp	= memory.readbyte(0x00c4) + memory.readbyte(0x00c5) * 256;		-- hero's maximum HP
	heromaxmp	= memory.readbyte(0x00c6) + memory.readbyte(0x00c7) * 256;		-- hero's maximum MP
	money		= vnbnumber(0x00d0, 4);
	gametime	= memory.readbyte(0x0031) * 0x3c + memory.readbyte(0x0030);		-- game-time
	gamehour	= math.floor(gametime / 320);
	gameminute	= math.floor((gametime - 320 * gamehour) / 320 * 60);
	expval		= doexp();

	worldmap();

	filledbox(96, 194, 255, 244, "#000000");

	text(191,   8, string.format("Time:  %02d:%02d", gamehour, gameminute));
--	text(188,  23, string.format("GameMode: %02x", gamemode));

	text( 90, 194, string.format("HP %3d/%3d MP %3d/%3d", herohp, heromaxhp, heromp, heromaxmp));
	if expval["next"] > 0 then
		text( 90, 218, string.format("Lv.%2d: %6d XP  Next %6d", expval["level"], expval["exp"], expval["next"]));
	else
		text( 90, 218, string.format("Lv.%2d: %6d EXP", expval["level"], expval["exp"]));
	end;


	text(217, 202, string.format("ATK %2d", memory.readbyte(0x00e7)));
	text(217, 210, string.format("$%5d", money));


	lifebar(191,  16,  60,  2, gamehour, 24, "#ffffff", "#777777", true);
	lifebar(191,  20,  60,  0, gameminute, 60, "#cccccc", "#555555", true);
	lifebar( 90, 202, 123,  4, herohp, heromaxhp, "#ffcc00", "#880000", true);
	lifebar( 90, 208, 123,  3, heromp, heromaxmp, "#9999ff", "#0000dd", true);

	if expval["pct"] ~= -1 then
		lifebar( 90, 226, 162,  2, expval["pct"], 100, "#ffffff", "#555555", true);
	end;

	box(89, 194, 91, 244, "#000000");
	line(90, 194, 90, 244, "#000000");


	if not enemy then enemy	= {} end;

	if gamemode == 0x05 or gamemode == 0x01 or gamemode == 0x06 then
		for i = 0, 5 do

			offset	= 0x500 + 0x10 * i;

			if not enemy[i] then 
				enemy[i] = {};
			end;

			if (memory.readbyte(offset) > 0) then

				if not enemy[i]["maxhp"] then enemy[i]["maxhp"] = 0 end;

				enemy[i]["t"]		= memory.readbyte(offset);
				enemy[i]["x"]		= memory.readbyte(offset +  5);
				enemy[i]["y"]		= memory.readbyte(offset +  6);
				enemy[i]["hp"]		= memory.readbyte(offset + 15);
				enemy[i]["maxhp"]	= math.max(enemy[i]["maxhp"], enemy[i]["hp"]);

				enemy[i]["item"]	= memory.readbyte(offset + 10);
				
				if enemy[i]["t"] > 1 then
					text(enemy[i]["x"] - 9, enemy[i]["y"] - 16, enemy[i]["hp"] .."/".. enemy[i]["maxhp"]);
					lifebar(enemy[i]["x"] - 5, enemy[i]["y"] - 8, 22,  0, enemy[i]["hp"], enemy[i]["maxhp"], "#ffcc00", "#dd0000", false);
				else
					if (enemy[i]["item"] == 0x1C) then
						if enemy[i]["hp"] == 0 then enemy[i]["hp"] = 10 end;
						if enemy[i]["hp"] == 9 then enemy[i]["hp"] = 99 end;
						text(enemy[i]["x"] - 6 - math.min(math.floor(enemy[i]["hp"] / 10) * 2, 2), enemy[i]["y"] - 8, "$".. enemy[i]["hp"]);
					else 
						box(enemy[i]["x"], enemy[i]["y"], enemy[i]["x"] + 15, enemy[i]["y"] + 15, "#ffffff");
						text(enemy[i]["x"] - string.len(itemlist[enemy[i]["item"]]) * 1.75 + 0, enemy[i]["y"] - 8, itemlist[enemy[i]["item"]]);
					
					end;
				end;
			
			else 
				enemy[i] = {};

			end;
		end;
	end;

	for i = 0, 7 do
		offset	= 0x0160 + 0x02 * i;
		item	= memory.readbyte(offset + 1);
		uses	= memory.readbyte(offset + 1);
		xo		= math.fmod(i, 4);
		yo		= math.floor(i / 4);
		if (item > 0 and (uses > 0 and uses < 255)) then
			text(xo * 12 + 8, 194 + yo * 16, uses);
		end;
	end;




--[[-- auto-regenerate HP
	if (herohp < heromaxhp) and ((math.fmod(timer, 3) == 0) or true) then
		herohp	= herohp + 1;
		memory.writebyte(0x00c0, math.fmod(herohp, 256));
		memory.writebyte(0x00c1, math.floor(herohp / 256));
	end;
--]]--

		

--[[	basically the Big Cheating Section.

	if not expbooster then expbooster = 0 end;
	expbooster	= expbooster + 1;
	if expbooster >= 3 and gamemode == 0x06 then
		expbooster	= 0;
		memory.writebyte(0x0d5, memory.readbyte(0x00d5) + 1);

		for i = 0,5 do
			inp	= memory.readbyte(0x00d5 + i);
			if (inp == 0x0a) then
				memory.writebyte(0x00d5 + (i + 1), memory.readbyte(0x00d5 + (i + 1)) + 1);
				memory.writebyte(0x00d5 + i, 0);

			elseif (inp == 0x27) then
				memory.writebyte(0x00d5 + i, 1);

			end;
		end;
	end;
]]--
end;




-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function charaselect()
	asign	= memory.readbyte(0x0110);
	btype	= memory.readbyte(0x0111);

--	line( 92,  30, 92, 130, "#ffffff");


	filledbox( 13,  48,  96,  64, "#000000");		-- cover JP text
	filledbox( 13,  80,  96,  96, "#000000");		-- cover JP text
	filledbox( 13, 112,  96, 128, "#000000");		-- cover JP text
	filledbox(160,  48, 231,  64, "#000000");		-- cover JP text

	text( 74,  24, " Create your character ");

	text( 15,  51, "Astrological sign:");
	text(158,  51, string.format("%s", asigns[asign]));

	text( 41,  83, "Blood type:");
--	text(164,  83, string.format("%s (%X)", btypes[btype], btype));

	text( 26, 115, "Clothing color:");

	
	asign2	= math.fmod(asign, 4);
	if asign2 == 0 then
		shp	= 64;
		smp	= 32;
	elseif asign2 == 1 then
		shp	= 48;
		smp	= 48;
	elseif asign2 == 2 then
		shp	= 32;
		smp	= 64;
	elseif asign2 == 3 then
		shp	= 33;
		smp	= 63;
	end;

	strength	= 10 + math.floor(shp / 32);

	text(  8, 135, "Starting HP:  ");
	text(  8, 143, "Starting MP: ");
	text(167, 135, " ".. shp);
	text(167, 143, " ".. smp);
	lifebar( 68, 136, 100,  4, shp, 64, "#ffcc00", "#880000");
	lifebar( 68, 144, 100,  4, smp, 64, "#9999ff", "#0000dd");

	text(194, 136, "Strength: ".. strength);
	text(194, 146, "Magic:");
	if smp >= 60 then
		text(204, 162, "Fireball");
	end;
	if smp >= 40 then
		text(204, 154, "Heal");
	end;
	
	

	graphx	= 17;
	graphy	= 160;

	text(graphx - 10, graphy + 63, "Lv.");

	for i = 1, 12 do
		thispointx	= i * 15 + graphx;
		line(thispointx, graphy, thispointx, graphy + 63, "#000088");
		text(thispointx - 4 - (string.len(string.format("%d", i)) - 1) * 3, graphy + 63, string.format("%d", i));
	end;


	for i = 0, 7 do
		lg			= i * 3;
		thispointy	= graphy + 63 - (lg) * 3;
		line(graphx + 15, thispointy, graphx + 180, thispointy, "#000088");
	
		if exptable[lg] then
			temp	= string.format("%d", exptable[lg]);
			text(graphx - 17, thispointy - 4, string.format("%5d", exptable[lg]));
		end;

	end;

	line(graphx + 15, graphy     , graphx +  15, graphy + 63, "#9999ff");
	line(graphx + 15, graphy + 63, graphx + 180, graphy + 63, "#9999ff");

--	filledbox(graphx + 19, graphy + 1, graphx + 73, graphy + 8, "#666666");
	text(graphx + 16, graphy - 1, "EXP Growth");

	if btype <= 2 then

		lastpointx	= graphx;
		lastpointy	= graphy + 180;
		for i = 1, 12 do
			thispointx	= i * 15 + graphx;
			thispointy	= graphy + 63 - (leveltable[btype][i] * 3);
			line(lastpointx, lastpointy, thispointx, thispointy, "#ffffff");
			lastpointx	= thispointx;
			lastpointy	= thispointy;
		end;

	else

--		filledbox(graphx + 75, graphy + 27, graphx + 119, graphy + 35, "#666666");
		text(graphx + 74, graphy + 26, " Random ");


	end;

	line(194, 145, 254, 145, "#000000");
	line( 8, 153, 192, 153, "#000000");

end;





-- ************************************************************************************
-- ************************************************************************************
-- ************************************************************************************

function keyintercept()
	tmp		= memory.readbyte(0x0026);

	if AND(tmp, 0x04) == 0x04 then
		mapstyle		= math.fmod(mapstyle + 1, 3)
	end;

--	memory.writebyte(0x0026, AND(tmp, 0xFB));
end;

memory.register(0x0026, keyintercept);








timer		= 0;
mapstyle	= 1;		-- 0 = hidden, 1 = mini, 2 = bigmap
gamemode	= 0;

itemlist	= {};
itemlist[0x01]	= "Lantrn";
itemlist[0x02]	= "Lantrn2";
itemlist[0x03]	= "Potion";
itemlist[0x04]	= "Potion2";
itemlist[0x05]	= "Antidt";
itemlist[0x06]	= "Antidt2";
itemlist[0x07]	= "Key";
itemlist[0x08]	= "GoldKey";
itemlist[0x09]	= "Ax";
itemlist[0x0a]	= "Ax2";
itemlist[0x0b]	= "Sword";
itemlist[0x0c]	= "Sword2";
itemlist[0x0d]	= "PwSword";
itemlist[0x0e]	= "PwSword2";
itemlist[0x0f]	= "MsSword";
itemlist[0x10]	= "SandraSl";
itemlist[0x11]	= "Mantle";
itemlist[0x12]	= "Mantle2";
itemlist[0x13]	= "Helmet";
itemlist[0x14]	= "Helmet2";
itemlist[0x15]	= "Tent";
itemlist[0x16]	= "Tent2";
itemlist[0x17]	= "Tiara";
itemlist[0x18]	= "Whale";
itemlist[0x19]	= "CureAll";
itemlist[0x1a]	= "TimeKey";
itemlist[0x1b]	= "Ship";
itemlist[0x1c]	= "Cash";

exptable	= {
	     0,
	    20,
	    50,
	    90,
	   150,
	   230,
	   350,
	   510,
	   750,
	  1100,
	  1600,
	  2200,
	  3200,
	  4400,
	  6400,
	  9000,
	 12000,
	 15000,
	 20000,
	 30000,
	 40000,
	 50000,
	};

leveltable		= {};
leveltable[0]	= { 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x13, 0x14};
leveltable[1]	= { 0x00, 0x01, 0x02, 0x03, 0x04, 0x07, 0x0A, 0x0D, 0x10, 0x13, 0x14, 0x15};
leveltable[2]	= { 0x00, 0x03, 0x06, 0x09, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13};

asigns			= {};
asigns[0x00]	= "Aries";
asigns[0x01]	= "Taurus";
asigns[0x02]	= "Gemini";
asigns[0x03]	= "Cancer";
asigns[0x04]	= "Leo";
asigns[0x05]	= "Virgo";
asigns[0x06]	= "Libra";
asigns[0x07]	= "Scorpio";
asigns[0x08]	= "Sagittarius";
asigns[0x09]	= "Capricorn";
asigns[0x0A]	= "Aquarius";
asigns[0x0B]	= "Pisces";

btypes			= {};
btypes[0x00]	= "A";
btypes[0x01]	= "B";
btypes[0x02]	= "O";
btypes[0x03]	= "AB";


mapdots			= {};
mapdots[1]		= {x =  996, y = 1888,	color = "#4444ff"};		-- house
mapdots[2]		= {x =  773, y =  989,	color = "#4444ff"};		-- house

mapdots[3]		= {x =  266, y = 2263,	color = "#00ff00"};		-- warp point
mapdots[4]		= {x = 1800, y =  216,	color = "#00ff00"};		-- warp point
mapdots[5]		= {x =  776, y =  344,	color = "#00ff00"};		-- warp point
mapdots[6]		= {x = 2055, y = 2008,	color = "#00ff00"};		-- warp point

mapdots[7]		= {x = 1863, y = 2045,	color = "#dd0000", name = "South Pyramid"};		-- s.pyramid
mapdots[8]		= {x = 1607, y =  381,	color = "#dd0000", name = "North Pyramid"};		-- n.pyramid



mappoints		= table.maxn(mapdots);

while (true) do


	timer		= timer +1;														-- timer for script events

	gamemode	= memory.readbyte(0x0029);										-- game mode


	if gamemode == 0x05 or gamemode == 0x01 or gamemode == 0x06 or gamemode == 0x08 then
		gameloop();

	elseif gamemode == 0x02 then
		charaselect();
	else

--	gametime	= memory.readbyte(0x0031) * 0x3c + memory.readbyte(0x0030);		-- game-time
--	gamehour	= math.floor(gametime / 320);
--	gameminute	= math.floor((gametime - 320 * gamehour) / 320 * 60);

		gametimer1	= memory.readbyte(0x0031);
		gametimer2	= memory.readbyte(0x0030);

		text(190, 8, string.format("Timer: %02X %02X", gametimer1, gametimer2));
		text(189, 23, string.format("GameMode: %02x", gamemode));
		lifebar(191,  16, 60,  2, gametimer1, 0x80, "#ffffff", "#777777", true);
		lifebar(191,  20, 60,  0, gametimer2, 0x3c, "#cccccc", "#555555", true);
	end;

	FCEU.frameadvance();

end;
