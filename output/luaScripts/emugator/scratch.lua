local gd = require("gd")

while(true) do

	local im = gd.createFromJpeg("gui/famicom.jpg")
	--gui.gdoverlay(0, 3, im:gdStr()) 
	gui.rect(20, 20, 256-20, 60 , "white")
	emugator.yieldwithflag()
end