	emu.print("Go Gators!")
	local cart = {x1=50, y1=50, x2=100, y2=100}
	local console = {x1=150, y1=50, x2=200, y2=100}
	local unloadButton = {x1 = 220, y1 = 220, x2 = 250, y2 = 230}
	local ejectInsertButton = {x1 = 5, y1 = 220, x2 = 64, y2 = 230}
	local switchButton = {x1 = 180, y1 = 220, x2 = 210, y2 = 230}
	local isDrag = false
	local wasClicked = false
	--local gd = require("gd")

while(true) do
	--need gd for this
		--local gdstr = gd.createFromPng("C:\Users\Super\Emugators\fceux\output\luaScripts\emugator\nesRomGeneric.png"):gdStr()
		--gui.gdoverlay(gdstr) 
	local inpt = input.read()

	if(emu.emulating() == false) then
		wasClicked = false
		if (inpt.leftclick == nil) then
			if ((inpt.xmouse > console.x1) and (inpt.xmouse < console.x2) and (inpt.ymouse > console.y1) and (inpt.ymouse < console.y2) and isDrag) then
				emu.loadrom("LegendOfZelda2_JP.fds")
			end
			isDrag = false
		end

		if isDrag then
			gui.rect(inpt.xmouse, inpt.ymouse, inpt.xmouse+50, inpt.ymouse+50, "red", "white")
			gui.text(inpt.xmouse + 12, inpt.ymouse+12, "Legend\nOf\nZelda")
		elseif ((inpt.xmouse > cart.x1) and (inpt.xmouse < cart.x2) and (inpt.ymouse > cart.y1) and (inpt.ymouse < cart.y2) and inpt.leftclick) then
			isDrag = true
		else
			gui.rect(cart.x1, cart.y1, cart.x2, cart.y2, "gray", "white")
			gui.text(cart.x1 + 12, cart.y1 + 12, "Legend\nof\nZelda\n2")
		end

		gui.rect(console.x1, console.y1, console.x2, console.y2, "blue", "white")
		gui.text(console.x1 + 9, console.y1 + 16, "Famicom\n/NES")

		emugator.yieldwithflag() -- call this if you want the script to run without emulation (game running)
	else
		gui.rect(unloadButton.x1, unloadButton.y1, unloadButton.x2, unloadButton.y2, "blue", "white")
		gui.text(unloadButton.x1+2, unloadButton.y1+2, "Unload")

		gui.rect(switchButton.x1, switchButton.y1, switchButton.x2, switchButton.y2, "blue", "white")
		gui.text(switchButton.x1+2, switchButton.y1+2, "Switch")

		gui.rect(ejectInsertButton.x1, ejectInsertButton.y1, ejectInsertButton.x2, ejectInsertButton.y2, "blue", "white")
		gui.text(ejectInsertButton.x1+2, ejectInsertButton.y1+2, "Eject/Insert")

		if ((inpt.xmouse > unloadButton.x1) and (inpt.xmouse < unloadButton.x2) and (inpt.ymouse > unloadButton.y1) and (inpt.ymouse < unloadButton.y2) and inpt.leftclick) then
			if (wasClicked == false) then
				emu.closeRom()
			end
			wasClicked = true
			emugator.yieldwithflag()
		elseif ((inpt.xmouse > switchButton.x1) and (inpt.xmouse < switchButton.x2) and (inpt.ymouse > switchButton.y1) and (inpt.ymouse < switchButton.y2) and inpt.leftclick) then
			if (wasClicked == false) then
			  emu.switchDisk()
			end
			wasClicked = true
			emu.frameadvance()
		elseif ((inpt.xmouse > ejectInsertButton.x1) and (inpt.xmouse < ejectInsertButton.x2) and (inpt.ymouse > ejectInsertButton.y1) and (inpt.ymouse < ejectInsertButton.y2) and inpt.leftclick) then
			if (wasClicked == false) then
			  emu.insertOrEjectDisk()
			end
			wasClicked = true
			emu.frameadvance()
		else
			wasClicked = false
			emu.frameadvance()
		end
	end
end