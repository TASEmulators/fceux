require 'winapi'

fceuxWindowDY = 0;
fceuxWindowDX = 0;
MIN_JUMP_POWER = -20;

function jumpingWindow()
	desktopWidth, desktopHeight = winapi.get_desktop_window():get_bounds();

	fceuxWindow = winapi.find_window("FCEUXWindowClass", nil);
	fceuxWindowX, fceuxWindowY = fceuxWindow:get_position();
	fceuxWindowWidth, fceuxWindowHeight = fceuxWindow:get_bounds();

	fceuxWindowDY = fceuxWindowDY + 1;	-- gravity
	fceuxWindowY = fceuxWindowY + fceuxWindowDY;
	if (fceuxWindowY + fceuxWindowHeight >= desktopHeight and fceuxWindowDY >= 0) then
		fceuxWindowY = desktopHeight - fceuxWindowHeight - 1;
		-- bounce from floor
		fceuxWindowDY = (0 - fceuxWindowDY) * 0.9;
		if (fceuxWindowDY > MIN_JUMP_POWER) then fceuxWindowDY = MIN_JUMP_POWER - math.random(10); end
		fceuxWindowDX = math.random(-7, 7);
	end

	fceuxWindowX = fceuxWindowX + fceuxWindowDX;
	if ((fceuxWindowX < 0 and fceuxWindowDX < 0) or (fceuxWindowX + fceuxWindowWidth >= desktopWidth and fceuxWindowDX > 0)) then
		-- bounce from sides
		fceuxWindowDX = 0 - fceuxWindowDX;
	end

	fceuxWindow:resize(fceuxWindowX, fceuxWindowY, fceuxWindowWidth, fceuxWindowHeight);
end

emu.registerbefore(jumpingWindow);
