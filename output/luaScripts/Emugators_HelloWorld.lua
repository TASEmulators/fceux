	emu.print("Go Gators!")
	local y = 0

while(true) do
	gui.drawtext(0, y, "Hello World")
	if(y < 256) then
		y = y + 1
	else
		y = 0
	end
	print(y)
	emugator.yieldwithflag(); -- call this if you want the script to run without emulation (game running)
	--emu.frameadvance()
end