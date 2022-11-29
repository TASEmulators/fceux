	emu.print("Go Gators!")
	local cart = {x1=50, y1=50, x2=100, y2=100}
	local console = {x1=150, y1=50, x2=200, y2=100}
	local isDrag = false
	--local gd = require("gd")

while(true) do
	--need gd for this
		--local gdstr = gd.createFromPng("C:\Users\Super\Emugators\fceux\output\luaScripts\emugator\nesRomGeneric.png"):gdStr()
		--gui.gdoverlay(gdstr) 
	
	if(emu.emulating() == false) then
		local inpt = input.read()

		if (inpt.leftclick == nil) then
			if ((inpt.xmouse > console.x1) and (inpt.xmouse < console.x2) and (inpt.ymouse > console.y1) and (inpt.ymouse < console.y2) and isDrag) then
				emu.loadrom("superMario.nes")
			end
			isDrag = false
		end

		if isDrag then
			gui.rect(inpt.xmouse, inpt.ymouse, inpt.xmouse+50, inpt.ymouse+50, "red", "white")
			gui.text(inpt.xmouse + 12, inpt.ymouse+12, "Super\nMario\nBros.")
		elseif ((inpt.xmouse > cart.x1) and (inpt.xmouse < cart.x2) and (inpt.ymouse > cart.y1) and (inpt.ymouse < cart.y2) and inpt.leftclick) then
			isDrag = true
		else
			gui.rect(cart.x1, cart.y1, cart.x2, cart.y2, "gray", "white")
			gui.text(cart.x1 + 12, cart.y1 + 12, "Super\nMario\nBros.")
		end

		gui.rect(console.x1, console.y1, console.x2, console.y2, "blue", "white")
		gui.text(console.x1 + 9, console.y1 + 16, "Famicom\n/NES")

		emugator.yieldwithflag() -- call this if you want the script to run without emulation (game running)
	else
		emu.frameadvance()
	end

	--emu.frameadvance()
end