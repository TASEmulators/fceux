--Written by Brandon Evans

--You can change the number of controllers tracked here.
local controllers = 2
--You can change the position of the text here.
local x = 0
local y = 8

local players = {}
local states = {}
local pressed = 0
local inputted = 0

function table.copy(t)
	local t2 = {}
	for k, v in pairs(t) do
		t2[k] = v
	end
	return t2
end

function counts()
	--Display the counts of the buttons pressed and inputted.
	gui.text(x, y, 'Pressed: ' .. pressed)
	gui.text(x, y + 8, 'Inputted: ' .. inputted)
end

function load(slot)
	--As Lua starts counting from 1, and there may be a slot 0, increment.
	slot = slot + 1
	if not states[slot] or states[slot].inputted == nil then
		gui.text(x, y + 16, 'No data loaded from slot ' .. tostring(slot - 1))
		counts()
		return
	end
	--Load the data if there is any available for this slot.
	players = table.copy(states[slot].players)
	pressed = states[slot].pressed
	inputted = states[slot].inputted
	gui.text(x, y + 16, 'Data loaded from slot ' .. tostring(slot - 1))
	counts()
end

function parse()
	--If there is an open, read-only FM2 file, parse it for the initial data.
	if not movie.active() then
		return false
	end
	local fh = io.open(movie.name())
	if not fh or movie.mode() == 'record' or movie.name():match(
	'.%.(%w+)$'
	) ~= 'fm2' then
		return false
	end
	local frame = -1
	local last = {}
	local match = {
		'right', 'left', 'down', 'up', 'start', 'select', 'B', 'A'
	}
	--Parse up until two frames before the current one.
	while frame ~= emu.framecount() - 2 do
		line = fh:read()
		if not line then
			break
		end
		--This is only a frame if it starts with a vertical bar.
		if string.sub(line, 0, 1) == '|' then
			frame = frame + 1
			players = {}
			local player = -1
			--Split up the sections by a vertical bar.
			for section in string.gmatch(line, '[^|]+') do
				player = player + 1
				--Only deal with actual players.
				if player ~= 0 then
					local button = 0
					--Run through all the buttons.
					for text in string.gmatch(section, '.') do
						button = button + 1
						--Check if this button is pressed.
						if text ~= ' ' and text ~= '.' then
							inputted = inputted + 1
							--If the button was not previously pressed,
							--increment.
							if table.maxn(last) < player or not last[player][
								match[button]
							] then
								pressed = pressed + 1
							end
							if table.maxn(players) < player then
								table.insert(players, {})
							end
							--Mark this button as pressed.
							players[player][match[button]] = true
						end
					end
				end
			end
			--Save the players to compare with the next frame in the file.
			last = players
		end
	end
	return true
end

function save(slot)
	--As Lua starts counting from 1, and there may be a slot 0, increment.
	slot = slot + 1
	while table.maxn(states) < slot do
		table.insert(states, {})
	end
	--Mark the current data as the data for this slot.
	states[slot].players = table.copy(players)
	states[slot].pressed = pressed
	states[slot].inputted = inputted
	gui.text(x, y + 16, 'Data saved to slot ' .. tostring(slot - 1))
	counts()
end

if parse() then
	gui.text(x, y + 16, 'Movie parsed for data')
else
	gui.text(x, y + 16, 'No movie parsed for data')
end
if savestate.registerload then
	savestate.registerload(load)
	savestate.registersave(save)
end

while true do
	--If this is the first frame, reset the data.
	if emu.framecount() == 0 then
		players = {}
		pressed = 0
		inputted = 0
	end
	--Check players one and two.
	for player = 1, controllers do
		local buttons = joypad.getdown(player)
		--Run through all of the pressed buttons.
		for i, v in pairs(buttons) do
			inputted = inputted + 1
			--If in the previous frame the button was not pressed, increment.
			if table.maxn(players) >= player and not players[player][i] then
				pressed = pressed + 1
			end
		end
		if table.maxn(players) < player then
			table.insert(players, true)
		end
		--Mark these buttons as pressed.
		players[player] = buttons
	end
	counts()
	emu.frameadvance()
end