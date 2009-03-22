--quick and dirty script that shows zapper position and fire button presses

Z_LSPAN = 20 --life span (in frames) of white box
Z_LSPAN_CLICK = 30 --life span of red box
Z_MAX = 60 --maximum amount of boxes on screen

zbuf = {}
zindex = 0
timer = 0

function zapper_add_coord(x,y,click)
	zbuf[zindex] = {t=timer,x=x,y=y,click=click}
	zindex = zindex + 1
	if(zindex>Z_MAX) then
		zindex = 0
	end
end

function box(x1,y1,x2,y2,color1,color2)
	if(x1>=0 and y1>=0 and x2<=255 and y2<=255) then
		gui.drawbox(x1, y1, x2, y2, color1, color2)
	end
end


lastclick = zapper.read().click
lastx=zapper.read().x
lasty=zapper.read().y
 
 while(true) do
	x = zapper.read().x
	y = zapper.read().y
	click = zapper.read().click
	gui.text(0, 8, string.format("x=%d",x));
	gui.text(0, 18, string.format("y=%d",y));
	gui.text(0, 28, string.format("click=%d",click));
	if(click==1 and click~=lastclick) then
		zapper_add_coord(x,y,1)
	elseif(x~=lastx or y~=lasty) then
		zapper_add_coord(x,y,0)
	end
	lastclick=click
	lastx=x
	lasty=y
	box(x-3, y-3, x+3, y+3, "white", 0)
	
	for i=0,100 do
		if(zbuf[i]) then
			ltime = timer-zbuf[i].t
			if(zbuf[i].click==0) then
				if(ltime<Z_LSPAN) then
					boxsize = (zbuf[i].t-timer+Z_LSPAN) / (Z_LSPAN/3)
					c = "white"
					box(zbuf[i].x-boxsize, zbuf[i].y-boxsize, zbuf[i].x+boxsize, zbuf[i].y+boxsize, c, 0)
				end
			elseif(zbuf[i].click==1) then
				if(ltime<Z_LSPAN_CLICK) then
					boxsize = (timer-zbuf[i].t) / (Z_LSPAN_CLICK/10)
					c = "red"
					box(zbuf[i].x-boxsize, zbuf[i].y-boxsize, zbuf[i].x+boxsize, zbuf[i].y+boxsize, c, 0)
				end
			end
		end
	end
	
 	FCEU.frameadvance();
	timer = timer + 1
 end
 