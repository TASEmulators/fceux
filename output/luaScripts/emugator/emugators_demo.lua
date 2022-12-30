local gd = require("gd")

emu.print("Go Gators!")
MAX_PER_PAGE = 6
CART_WIDTH = 30
CART_HEIGHT = 30
DRAWER_OFFSET_X = 10
DRAWER_OFFSET_Y = 10
DRAWER_BUFFER_X = 10
DRAWER_BUFFER_Y = 10

local currPage = 1
local cart = {x1=50, y1=50, x2=100, y2=100}
local console = {x1=150, y1=50, x2=200, y2=100}
local unloadButton = {x1 = 220, y1 = 220, x2 = 250, y2 = 230}
local ejectInsertButton = {x1 = 5, y1 = 220, x2 = 64, y2 = 230}
local switchButton = {x1 = 180, y1 = 220, x2 = 210, y2 = 230}
local selectedRom = nil
local wasClicked = false

local FAMICOM_Roms = {}
local romDir = [[../../../emugator/ROMs/]]
local romCartDir = [[../../../emugator/ROM_Carts/]]

--Find ROMS
local totalRoms = 0
local pageNumber = 1
local pageSlot = 1

for rom in io.popen([[dir "]] ..romDir.. [[" /b]]):lines() do
	local dot = string.find(rom, "%.")
	ext = nil
	if(dot ~= nil) then
		ext = string.sub(rom, dot, -1)
	end

	if(ext == ".nes" or ext == ".fds") then
		print("found: " ..rom)
		if(FAMICOM_Roms[pageNumber] == nil) then
			FAMICOM_Roms[pageNumber] = {}
		end

		local xpos = DRAWER_OFFSET_X + DRAWER_BUFFER_X*(math.floor(((pageSlot-1)%2)) + 1) + math.floor(((pageSlot-1)%2))*CART_WIDTH
		local ypos = DRAWER_OFFSET_Y + DRAWER_BUFFER_Y*(math.floor(((pageSlot-1)/2)) + 1) + math.floor(((pageSlot-1)/2))*CART_HEIGHT
		local name = string.sub(rom, 1, dot-1)
		local dstImg = gd.create(CART_WIDTH, CART_HEIGHT)
		local srcImg = gd.createFromJpeg(romCartDir ..name.. [[.jpg]])

		if(srcImg == nil) then
			srcImg = gd.createFromJpeg(romCartDir ..name.. [[.jpeg]])
		end

		if(srcImg == nil) then
			srcImg = gd.createFromPng(romCartDir ..name.. [[.png]])
		end

		if(srcImg == nil) then
			dstImg:filledRectangle(0, 0, CART_WIDTH-1, CART_HEIGHT-1)
		else
			dstImg:copyResized(srcImg, 0, 0, 0, 0, CART_WIDTH, CART_HEIGHT, srcImg:sizeX(), srcImg:sizeY())
		end

		FAMICOM_Roms[pageNumber][pageSlot] = {rom = rom, image = dstImg, name = name, x = xpos, y = ypos, slot = pageSlot, isSelected = false}
		pageSlot = pageSlot + 1
		if(pageSlot > MAX_PER_PAGE) then
			pageSlot = 1
			pageNumber = pageNumber + 1
		end
		totalRoms = totalRoms + 1
	end
end

--Main Loop
while(true) do 
	local inpt = input.read()

	if(emu.emulating() == false) then --should be changed to allow gui to be swaped to while emulating. maybe check if paused?
		wasClicked = false

		--Load Cartridge if dropped on Console
		if (inpt.leftclick == nil) then
			if((inpt.xmouse > console.x1) and (inpt.xmouse < console.x2) and (inpt.ymouse > console.y1) and (inpt.ymouse < console.y2) and selectedRom ~= nil) then
				emu.loadrom(romDir ..FAMICOM_Roms[currPage][selectedRom].rom)
			end

			if(selectedRom ~= nil) then
				FAMICOM_Roms[currPage][selectedRom].isSelected = false
				selectedRom = nil
			end

		end

		--Draw Cartridges
		for _, rom in pairs(FAMICOM_Roms[currPage]) do
			if(rom.isSelected == false) then
				local gdstr = rom.image:gdStr()			
				gui.gdoverlay(rom.x, rom.y, gdstr)
			end
		end

		if (selectedRom == nil) then
			local index = 0
			for _, rom in pairs(FAMICOM_Roms[currPage]) do
				if ((inpt.xmouse > rom.x) and (inpt.xmouse < (rom.x+CART_WIDTH)) and (inpt.ymouse > rom.y) and (inpt.ymouse < (rom.y+CART_HEIGHT)) and inpt.leftclick) then
					selectedRom = rom.slot
					rom.isSelected = true
					break
				end
			end
		else
			--gui.rect(inpt.xmouse, inpt.ymouse, inpt.xmouse+50, inpt.ymouse+50, "red", "white")
			--gui.text(inpt.xmouse + 12, inpt.ymouse+12, "Legend\nOf\nZelda\nII")
			local gdstr = FAMICOM_Roms[currPage][selectedRom].image:gdStr()
			gui.gdoverlay(inpt.xmouse, inpt.ymouse, gdstr)
		end

		--Draw console
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