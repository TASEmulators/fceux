---------------------------------------------------------------------------
-- Display Input at Selection cursor
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * taseditor.getselection()
-- * taseditor.getinput()
---------------------------------------------------------------------------
-- Usage:
-- Run the script, unpause emulation (or simply Frame Advance once).
-- Now you can see additional joypad icon at the bottom left corner of FCEUX screen.
-- This icon displays which buttons are held at the selected frame.
-- If you have several frames selected, the joypad icon will only display buttons
-- of the first selected frame.
-- By default the script only shows 1P joypad, but you can change "num=2, on=false"
-- to "num=2, on=true" to also display 2P joypad and so on.
---------------------------------------------------------------------------

pads = {
	{num=1, on=true, color="red", x=50, y=200, w=34, h=10},
	{num=2, on=true, color="yellow", x=90, y=200, w=34, h=10},
	{num=3, on=false, color="green", x=130, y=200, w=34, h=10},
	{num=4, on=false, color="orange", x=170, y=200, w=34, h=10}
}

buttons = {
	A      = {x=30, y=5, w=3, h=3, bitmask=1},
	B      = {x=24, y=5, w=3, h=3, bitmask=2},
	select = {x=18, y=7, w=3, h=1, bitmask=4},
	start  = {x=12, y=7, w=3, h=1, bitmask=8},
	up     = {x=4, y=1, w=2, h=2, bitmask=16},
	down   = {x=4, y=7, w=2, h=2, bitmask=32},
	left   = {x=1, y=4, w=2, h=2, bitmask=64},
	right  = {x=7, y=4, w=2, h=2, bitmask=128}
}

function draw_joypads()
	if (taseditor.engaged()) then
		selection_table = taseditor.getselection();
		if (selection_table ~= nil) then
			selection_start = selection_table[1];
			for _, pad in ipairs(pads) do
				if pad.on then
					gui.drawbox(pad.x,   pad.y,   pad.x+pad.w, pad.y+pad.h, "#3070ffb0")
					gui.drawbox(pad.x+4, pad.y+4, pad.x+6,     pad.y+6,     "black")
					controller = taseditor.getinput(selection_start, pad.num)
					for name, btn in pairs(buttons) do
						if (AND(controller, btn.bitmask) ~= 0) then
							gui.drawbox(pad.x + btn.x, pad.y + btn.y, pad.x + btn.x + btn.w, pad.y + btn.y + btn.h, pad.color)
						else
							gui.drawbox(pad.x + btn.x, pad.y + btn.y, pad.x + btn.x + btn.w, pad.y + btn.y + btn.h, "black")
						end
					end
				end
			end
		end
	else
		gui.text(1, 9, "TAS Editor is not engaged.");
	end
end

taseditor.registerauto(draw_joypads);


