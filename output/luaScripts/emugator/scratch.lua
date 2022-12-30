local gd = require("gd")

while(true) do

	local im = gd.createFromJpeg("superMarioBros.jpg"):gdStr()
	gui.gdoverlay(im) 

	emugator.yieldwithflag()
end