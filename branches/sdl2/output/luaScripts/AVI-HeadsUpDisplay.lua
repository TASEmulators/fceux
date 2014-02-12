-- Script by amaurea, andymac and feos for FCEUX 2.2.0 and earlier versions.
-- Allows customizable recording of Frame, Lag, Timer and Input display to AVI dump.
-- Drag and drop HUD items with mouse, use Numpad 1-6 to switch them on/off.

print("Drag and drop HUD items with mouse, use Numpad 1-6 to switch them on/off.")

screen = {w=256,h=231}
move = {object=nil,offx=0,offy=0}

pads = {
	{num=1,on=true, color="red",   x=9,  y=220,w=34,h=10,toggle="numpad1"},
	{num=2,on=true, color="yellow",x=54, y=220,w=34,h=10,toggle="numpad2"},
	{num=3,on=false,color="green", x=99, y=220,w=34,h=10,toggle="numpad3"},
	{num=4,on=false,color="orange",x=144,y=220,w=34,h=10,toggle="numpad4"}
}

buttons = {
	A      = {x=30,y=5,w=3,h=3},
	B      = {x=24,y=5,w=3,h=3},
	select = {x=18,y=7,w=3,h=1},
	start  = {x=12,y=7,w=3,h=1},
	up     = {x=4, y=1,w=2,h=2},
	down   = {x=4, y=7,w=2,h=2},
	left   = {x=1, y=4,w=2,h=2},
	right  = {x=7, y=4,w=2,h=2}
}

text  = {on=true,x=1,  y=9,w=30,h=16,toggle="numpad5"}
timer = {on=true,x=197,y=9,w=58,h= 7,toggle="numpad6"}

function drawpad(pad)
	gui.drawbox( pad.x,   pad.y,   pad.x+pad.w, pad.y+pad.h, "#3070ffb0" )
	gui.drawbox( pad.x+4, pad.y+4, pad.x+6,     pad.y+6,     "black"     )
	controller = joypad.read(pad.num)
	for name, b in pairs(buttons) do
		gui.drawbox( pad.x + b.x, pad.y + b.y, pad.x + b.x + b.w, pad.y + b.y + b.h,
			         controller[name] and pad.color or "black" )
	end
end

function mouseover(pad, margin)
	return keys.xmouse >= pad.x-margin and keys.xmouse <= pad.x+pad.w+margin and
		keys.ymouse >= pad.y-margin and keys.ymouse <= pad.y+pad.h+margin
end

function inrange(upper, lower, testval)
	if testval >= upper then return upper
	elseif testval <= lower then return lower
	else return testval
	end
end

function concat(tables)
	local res = {}
	for _, tab in ipairs(tables) do
		for _, val in ipairs(tab) do
			table.insert(res, val)
		end
	end
	return res
end

prev_keys = input.get()
objects = concat({pads, {text, timer}})

function everything()
	keys = input.get()

	-- Are we moving anything around?
	if move.object then
		if keys["leftclick"] then
		    -- Do not go outside screen
			local safex = inrange(screen.w - move.object.w, 0, keys.xmouse - move.offx)
			local safey = inrange(screen.h - move.object.h, 8, keys.ymouse - move.offy)
			move.object.x = safex
			move.object.y = safey
		else move.object = nil end
		
	-- Try to pick something up
	elseif keys["leftclick"] then 
		for _,object in ipairs(objects) do
			if mouseover(object,0) then
				move.object = object
				move.offx = keys.xmouse - object.x
				move.offy = keys.ymouse - object.y
			end
		end
	end

	-- Toggle displays
	for _, object in ipairs(objects) do
		if keys[object.toggle] and not prev_keys[object.toggle] then
			object.on = not object.on
		end
	end

	-- Actually draw the stuff
	if timer.on then
		mins = math.floor(movie.framecount()/3600)
		secs = movie.framecount()/60-mins*60
		gui.text( timer.x, timer.y, string.format("%s:%05.2f",os.date("!%H:%M",mins*60),secs), "white" )
	end

	if text.on then
		local done = movie.mode() == "finished" or movie.mode() == nil
		gui.text( text.x, text.y,     movie.framecount(), done          and "red" or "white" )
		gui.text( text.x, text.y + 9, FCEU.lagcount(),    FCEU.lagged() and "red" or "green" )
	end

	for _, pad in ipairs(pads) do
		if pad.on then drawpad(pad) end
	end

	prev_keys = keys
end

gui.register(everything)

while (true) do
	FCEU.frameadvance()
end