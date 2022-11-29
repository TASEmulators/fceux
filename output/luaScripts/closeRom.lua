while true do
local inpt = input.read()
emu.frameadvance()
emu.closeRom()
emugator.yieldwithflag()
end

