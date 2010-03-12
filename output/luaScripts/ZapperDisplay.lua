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

--Draw bull's eye
gui.box(zap.xmouse-1,zap.ymouse-1,zap.xmouse+1,zap.ymouse+1,"clear","red")
gui.box(zap.xmouse-6,zap.ymouse-6,zap.xmouse+6,zap.ymouse+6,"clear",color)
gui.box(zap.xmouse-12,zap.ymouse-12,zap.xmouse+12,zap.ymouse+12,"clear",color)
gui.line(zap.xmouse-12,zap.ymouse-12,zap.xmouse+12,zap.ymouse+12,color)
gui.line(zap.xmouse+12,zap.ymouse-12,zap.xmouse-12,zap.ymouse+12,color)
gui.pixel(zap.xmouse,zap.ymouse,"red")

emu.frameadvance()
end