--Zapper display
--written by adelikat
--Purpose: To show the target of the zapper on screen
--			Primary use is for fullscreen where the mouse cursor is disabled

local zap	--get zapper.read() values
local color = "white" --color of outside bull's eye circles

while true do

zap = zapper.read()

--Red if firing
if (zap.click == 1) then
	color = "red"
else
	color = "white"
end

--gui.text(1,1,"X: " .. zap.x)
--gui.text(1,9,"Y: " .. zap.y)
--gui.text(1,17,"Click: " .. zap.fire)

--Draw bull's eye
gui.box(zap.x-1,zap.y-1,zap.x+1,zap.y+1,"clear","red")
gui.box(zap.x-6,zap.y-6,zap.x+6,zap.y+6,"clear",color)
gui.box(zap.x-12,zap.y-12,zap.x+12,zap.y+12,"clear",color)
gui.line(zap.x-12,zap.y-12,zap.x+12,zap.y+12,color)
gui.line(zap.x+12,zap.y-12,zap.x-12,zap.y+12,color)
gui.pixel(zap.x,zap.y,"red")

emu.frameadvance()
end