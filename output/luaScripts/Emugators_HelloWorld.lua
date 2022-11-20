	emu.print("okay, but where does this one go?")
	local y = 6
while(true) do
	gui.drawtext(0, y, "Hello World")
	y = y + 1
	soup("this is cool")
	emugator.yieldwithflag();
	
	--emu.frameadvance()
end