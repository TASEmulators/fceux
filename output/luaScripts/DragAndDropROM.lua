--Written by Steven Phang
--EmuGators

--You can change the number of controllers tracked here.
require 'lfs' --Lua File Syayem, useful for searching file directories

local roms = {}
local framecount = 0


function loadRom(index)
	--Load ROM file into emulator
	emu.loadrom(roms[index].path)
    emu.message('Successfully Loaded ' .. roms[index].name)

end

function unloadRom(dummyPath)
	--Load ROM file into emulator
	emu.loadrom(dummyPath)
    emu.message('Disk Ejected')

end

function SearchForROMS(filepath)
    local count = 1;
	for file in lfs.dir[filepath] do
        if lfs.attributes(file,"mode") == "file" then
            roms[count] = {}
            roms[count].index = count
            roms[count].path = file

            local truncLoc = strfind (filename, '.')
            local friendlyName = strsub(filename, 1, truncLoc)

            roms[count].name = friendlyName
            count = count + 1
        end
    end
end

--On startup
movie.load("loadingScreen.fm2")
local dummyPath = "dummy.fds"
local isLoadingScreen = true
unloadRom(dummyPath)

while true do
    --TODO: Implement Logic to detect when ROM should be loaded (via GUI event)
    --TODO: On 'Disk dragged to Console Event' toggle isLoadingScreen variable
    --      and perform corresponding actions

    --Poll for GUI Event here, update isLoadingScreen if it occurs, then...
    if isLoadingScreen --no disk loaded, play animation
        if !movie.active() then
            movie.play()
        end
        frameCount = frameCount + 1
        if framecount == movie.length() then
            framecount = 0
            movie.replay()
        end
    else --disk loaded, stop animation
        if !movie.active() then
            movie.stop()
            movie.close()
        end

        loadRom(index)
    end

    emu.frameadvance()
end