--Written by Steven Phang
--EmuGators

--You can change the number of controllers tracked here.
local roms = {}

function loadROM(filename)
	--Load ROM file into emulator
	emu.loadrom(filename)

    local truncLoc = strfind (filename, '.')
    local friendlyName = strsub(filename, 1, truncLoc)
    emu.message('Successfully Loaded ' .. friendlyName);
end

function SearchForROMS(string filepath)
	local 
end

while true do
	emu.frameadvance()
end