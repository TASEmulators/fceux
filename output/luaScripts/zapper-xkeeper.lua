require "x_functions";

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

while true do

	zap		= zapper.read();

	box(zap['x'] - 5, zap['y'] - 5, zap['x'] + 5, zap['y'] + 5, "#ffffff");
	if zap['click'] == 1 then 
		box(zap['x'] - 7, zap['y'] - 7, zap['x'] + 7, zap['y'] + 7, "#ff0000");
	end;
	line(zap['x'] - 0, zap['y'] - 9, zap['x'] + 0, zap['y'] + 9, "#ffffff");
	line(zap['x'] - 9, zap['y'] - 0, zap['x'] + 9, zap['y'] + 0, "#ffffff");


	FCEU.frameadvance();

end