
x_func_version	= 6;

--[[
	Minor version history:

	v5 -----------
	- Added Bisqwit's 'clone table' function.
	
	v6 -----------
	- added pairs by keys
	- added hitbox functions



]]




-- Draws a line from x1,y1 to x2,y2 using color

function line(x1,y1,x2,y2,color)
	if (x1 >= 0 and x1 <= 255 and x2 >= 0 and x2 <= 255 and y1 >= 0 and y1 <= 244 and y2 >= 0 and y2 <= 244) then
		local success = pcall(function() gui.drawline(x1,y1,x2,y2,color) end);
		if not success then 
			text(60, 224, "ERROR: ".. x1 ..",".. y1 .." ".. x2 ..",".. y2);
		end;
	end;
end;


-- Puts text on-screen (duh)
function text(x,y,str)
	if str == nil then
		str = "nil";
	end;
	if (x >= 0 and x <= 255 and y >= 0 and y <= 240) then
		gui.text(x,y,str);
	end;
end;

-- Sets the pixel at x, y to color
function pixel(x,y,color)
	if (x >= 0 and x <= 255 and y >= 0 and y <= 240) then
		gui.drawpixel(x,y,color);
	end;
end;


-- Draws a rectangle from x1,y1 to x2, y2
function box(x1,y1,x2,y2,color)
	if (x1 >= 0 and x1 <= 255 and x2 >= 0 and x2 <= 255 and y1 >= 0 and y1 <= 244 and y2 >= 0 and y2 <= 244) then
--[[	local success = pcall(function() gui.drawbox(x1,y1,x2,y2,color); end);
		if not success then 
			text(60, 150, string.format("%3d %3d %3d %3d", x1, y1, x2, y2));
			FCEU.pause();
		end;
]]		gui.drawbox(x1,y1,x2,y2,color);
	end;
end;


-- Draws a filled box from x1, y1 to x2, y2 (warning: can be slow if drawing large boxes)
function filledbox(x1,y1,x2,y2,color)
	for i = 0, math.abs(y1 - y2) do
		line(x1,y1 + i,x2,y1 + i,color);
	end;
end;



-- Draws a life-bar.
-- x, y: position of top-right
-- sx, sy: width and height of ACTUAL BAR (0 is the smallest height (1px))
-- a1, a2: Amounts (a1/a2*100 = % of bar filled)
-- oncolor, offcolor: "bar", "background"
-- noborder: set to "true" if you just want a black outline without the white

function lifebar(x, y, sx, sy, a1, a2, oncolor, offcolor, outerborder, innerborder)
	-- this function will have the effect of drawing an HP bar
	-- keep in mind xs and ys are 2px larger to account for borders
	
	x1	= x;
	x2	= x + sx + 4;
	y1	= y;
	y2	= y + sy + 4;
	w	= math.floor(a1 / math.max(1, a1, a2) * sx);
	if not a2 then w = 0 end;

	outer	= outerborder;
	inner	= innerborder;

	if (outer == true or outer == false) and (inner == nil) then
		-- legacy
		outer	= nil;
		inner	= "#000000";

	elseif outer == nil and inner == nil then
		outer	= "#000000";
		inner	= "#ffffff";

	elseif inner == nil then
		inner	= "#ffffff";
	end;
	
	if (inner) then 
		box(x1 + 1, y1 + 1, x2 - 1, y2 - 1, inner);
	end;
	if (outer) then
		box(x1    , y1    , x2    , y2    , outer);
	end; 

	if (w < sx) then
		filledbox(x1 + w + 2, y1 + 2, x2 - 2, y2 - 2, offcolor);
	end;		

	if (w > 0) then
		filledbox(x1 + 2, y1 + 2, x1 + 2 + w, y2 - 2, oncolor);
	end;		

end;



function graph(x, y, sx, sy, minx, miny, maxx, maxy, xval, yval, color, border, filled)


	if (filled ~= nil) then
		filledbox(x + 1, y + 1, x+sx, y+sy, "#000000");
	end;
	if (border ~= nil) then
		box(x, y, x+sx+1, y+sy+1, color);
	end;
	

	xp	= (xval - minx) / (maxx - minx) * sx;
	yp	= (yval - miny) / (maxy - miny) * sy;

	line(x + 1     , yp + y + 1, x + sx + 1, yp + y + 1, color);
	line(xp + x + 1, y + 1     , xp + x + 1, y + sy + 1, color);

	return true;
end;


-- Bisqwit's clone table feature.
table.clone = function(table)
  local res = {}
  for k,v in pairs(table)do
    res[k]=v
  end
  return res
end

spairs		= function (t, f)
	local a = {}
	for n in pairs(t) do table.insert(a, n) end
	table.sort(a, f)
	local i = 0      -- iterator variable
	local iter = function ()   -- iterator function
		i = i + 1
		if a[i] == nil then return nil
		else return a[i], t[a[i]]
		end
	end
	return iter
end


-- ****************************************************************************
-- * hitbox( coords1, coords2, con, coff )
-- * Checks if any point of coords1 is within coords2. 
-- * con/coff determine what colors of box to draw.
-- ****************************************************************************
function hitbox(b1x1, b1y1, b1x2, b1y2, b2x1, b2y1, b2x2, b2y2, con, coff)

	if not b1x1 then
		text(0, 8, "ERROR!!!!");
		return;
	end;

	local noboxes	= false;
	if con == nil and coff == nil then
		noboxes	= true;
	else
		if coff == nil then
			coff = "#00ff00"
		end;

		if con == nil then
			con	= "#dd0000";
		end;
		if coff == nil then
			coff = "#00ff00"
		end;
	end;

	boxes	= {{
			x	= {b1x1, b1x2},
			y	= {b1y1, b1y2},
		}, {
			x	= {b2x1, b2x2},
			y	= {b2y1, b2y2},
		}};

	hit	= false;

	for xc = 1, 2 do
		for yc = 1, 2 do

			if	(boxes[1]['x'][xc] >= boxes[2]['x'][1]) and
				(boxes[1]['y'][yc] >= boxes[2]['y'][1]) and
				(boxes[1]['x'][xc] <= boxes[2]['x'][2]) and
				(boxes[1]['y'][yc] <= boxes[2]['y'][2]) then

				hit	= true;
				-- TODO: make this break out of the for loop? might not be worth it
			end;
		end;
	end;

	if hit == true then
		if not noboxes then box(b2x1, b2y1, b2x2, b2y2, con); end;
		return true;
	else
		if not noboxes then box(b2x1, b2y1, b2x2, b2y2, coff); end;
		return false;
	end;

	return true;

end;



function x_requires(required)
	-- Sanity check. If they require a newer version, let them know.
	timer	= 1;
	if x_func_version < required then

		while (true) do
			timer = timer + 1;

			for i = 0, 32 do
				box( 6, 28 + i, 250, 92 - i, "#000000");
			end;

			text( 10, 32, string.format("This Lua script requires version %02d or greater.", required));
			text( 43, 42, string.format("Your x_functions.lua is version %02d.", x_func_version));
			text( 29, 58, "Please check for an updated version at");
			text( 14, 69, "http://xkeeper.shacknet.nu:5/");
			text(114, 78, "emu/nes/lua/x_functions.lua");

			warningboxcolor	= string.format("%02X", math.floor(math.abs(30 - math.fmod(timer, 60)) / 30 * 0xFF));
			box(7, 29, 249, 91, "#ff" .. warningboxcolor .. warningboxcolor);

			FCEU.frameadvance();
		end;

	end;
end;