-- Sprite visualizer
--
-- Draws a box around all sprites on screen,
-- hover with the mouse to inspect data.
--
-- Original by tokumaru 2016-10-08:
-- https://forums.nesdev.com/viewtopic.php?p=181008#p181008

numSpriteHeight = 8
strFillColor = "#ffffff3f"
strOutlineColor = "#ff0000bf"
strHighlightColor = "#ffffffbf"

function readSpriteAttributes(a,s,v)
	local numAddress = v * 256
	tabSpriteAttributes = {}
	for numIndex = 0, 255 do
		table.insert(tabSpriteAttributes, memory.readbyte(numAddress + numIndex))
	end
end

function setSpriteHeight(a,s,v)
	if AND(a, 7) == 0 then
		numSpriteHeight = AND((v / 4), 8) + 8
	end
end

function drawBoxes()
	local tabInput, numSpriteX, numSpriteY, numDetailsBase, numTextY = input.read()
	if tabSpriteAttributes ~= nill then
		for numBase = 252, 0, -4 do
			numSpriteX0 = tabSpriteAttributes[numBase + 4]
			numSpriteY0 = tabSpriteAttributes[numBase + 1] + 1
			numSpriteX1 = numSpriteX0 + 7
			numSpriteY1 = numSpriteY0 + numSpriteHeight - 1
			if (tabInput.xmouse >= numSpriteX0) and (tabInput.xmouse <= numSpriteX1) and (tabInput.ymouse >= numSpriteY0) and (tabInput.ymouse <= numSpriteY1) then
				gui.box(numSpriteX0, numSpriteY0, numSpriteX1, numSpriteY1, strHighlightColor, strOutlineColor)
				numDetailsBase = numBase
			else
				gui.box(numSpriteX0, numSpriteY0, numSpriteX1, numSpriteY1, strFillColor, strOutlineColor)
			end
		end
	end
	if numDetailsBase ~= nil then
		numTextY = (1 - math.floor(tabInput.ymouse / 120)) * 127 + 16
		gui.text(16, numTextY, string.format("OAM Slot: %d", numDetailsBase / 4)); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("OAM Offset: $%02X", numDetailsBase)); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Sprite X: $%02X", tabSpriteAttributes[numDetailsBase + 4])); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Sprite Y: $%02X", tabSpriteAttributes[numDetailsBase + 1])); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Tile ID: $%02X", tabSpriteAttributes[numDetailsBase + 2])); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Palette: %d", AND(tabSpriteAttributes[numDetailsBase + 3], 0x03))); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Behind Background: %d", AND(tabSpriteAttributes[numDetailsBase + 3], 0x20) / 0x20)); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Flip X: %d", AND(tabSpriteAttributes[numDetailsBase + 3], 0x40) / 0x40)); numTextY = numTextY + 9
		gui.text(16, numTextY, string.format("Flip Y: %d", AND(tabSpriteAttributes[numDetailsBase + 3], 0x80) / 0x80)); numTextY = numTextY + 9
	end
	gui.box(tabInput.xmouse - 2, tabInput.ymouse - 2, tabInput.xmouse + 2, tabInput.ymouse + 2, strHighlightColor, strOutlineColor)
end

memory.registerwrite(0x4014, 0x0001, readSpriteAttributes)
memory.registerwrite(0x2000, 0x2000, setSpriteHeight)
gui.register(drawBoxes)

while (true) do
	emu.frameadvance()
end
