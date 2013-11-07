-- ****************************************************************************
-- *
-- *  FCEUX Lua GUI tools
-- *
-- ****************************************************************************


control	= {};

last		= {};
inpt		= {		xmouse = 0,	ymouse = 0	};

-- ****************************************************************************
-- * input.update()
-- * Updates input changes and calculates mouse movement averages.
-- ****************************************************************************
function input.update()

	last	= table.clone(inpt);
	inpt	= input.get();

end;


-- ****************************************************************************
-- * control.button( xpos, ypos, width, height, t )
-- * Draws a button that can have some effect. Not quite implemented yet!
-- * width/height are for the outline and more or less have to be set manually.
-- * since height is fixed, though, it's always a multiple.
-- * c disables hitbox checking and sets the outline to that color
-- ****************************************************************************
function control.button(x, y, w, h, t, c, sc)
	local h	= h * 8;
	filledbox( x    , y    , x + w    , y + h    , "#000000");
	text(x - 1, y - 1, t);				-- Yeah, for some reason...

	if c and not sc then
		box( x - 1, y - 1, x + w + 1, y + h + 1, c);
	elseif c then
		return (hitbox(inpt['xmouse'], inpt['ymouse'], inpt['xmouse'], inpt['ymouse'], x - 1, y - 1, x + w + 1, y + h + 1, c, c) and (inpt['leftclick'] and not last['leftclick']));
		
	elseif hitbox(inpt['xmouse'], inpt['ymouse'], inpt['xmouse'], inpt['ymouse'], x - 1, y - 1, x + w + 1, y + h + 1, "#6666ff", "#0000ff") and (inpt['leftclick'] and not last['leftclick']) then
		box(x - 1, y - 1, x + w + 1, y + h + 1, "#ffffff");	-- for white-flash highlighting
		return true;
	end;
	return false;

end;



-- ****************************************************************************
-- * control.showmenu( xpos, ypos, menutable )
-- * Displays the given menu and takes action on it. Usually.
-- ****************************************************************************
function control.showmenu(x, y, menuinfo)


	if menuinfo['life'] > 0 then
		local temp		= 0;
		local i			= 0;
		local yscale	= 11 - math.max(0, menuinfo['life'] - 60);
--		text(x, y - 11, string.format("Life: %3d", menuinfo['life']));

		for k, v in spairs(menuinfo['menu']) do
			
			local buttoncolor	= nil;
			if v['menu'] and v['life'] > 0 then
				buttoncolor	= "#ffffff";
			end;

			buttonclicked	= control.button(x, y + yscale * i, menuinfo['width'], 1, v['label'], buttoncolor, true);
			if v['menu'] then
				text(x + menuinfo['width'] - 6, y + yscale * i - 1, ">");
			end;
			if v['marked'] == 1 then
				text(x, y + yscale * i - 1, ">");
			end;

			-- is a selection, not a submenu
			if buttonclicked and v['action'] then
				v['action'](v['args']);
				menuinfo['life']	= 0;
				return -1;

			-- a submenu
			elseif buttonclicked and v['menu'] then
				v['life']	= 70;
			end;

			if v['menu'] and v['life'] > 0 and temp >= 0 then
				temp2	= control.showmenu(x + menuinfo['width'] + 3, y + yscale * i, v);
				if temp2 >= 0 then
					temp	= math.max(temp, temp2);
				else
					temp	= temp2;
				end;
			end;

			i	= i + 1;

		end;
		if temp >= 0 and hitbox(inpt['xmouse'], inpt['ymouse'], inpt['xmouse'], inpt['ymouse'], x, y, x + menuinfo['width'] + 2, y + i * yscale) then
			menuinfo['life']	= math.max(menuinfo['life'] - 1, 60);
		elseif temp == -1 or (menuinfo['life'] < 60 and inpt['leftclick'] and not last['leftclick']) then
			menuinfo['life']	= 0;
			return 0;
		else
			menuinfo['life']	= math.max(math.min(60, temp), menuinfo['life'] - 1);
		end;
	end;

	return menuinfo['life'];

end;

