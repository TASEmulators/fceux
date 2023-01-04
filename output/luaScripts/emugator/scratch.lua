local gd = require("gd")

while(true) do

	local im = gd.createFromPng("gui/famicom.png")
	gui.gdoverlay(0, 3, im:gdStr()) 
	emugator.yieldwithflag()
end