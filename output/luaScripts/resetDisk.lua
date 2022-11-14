--Artur Bekker
--EmuGators
--Making function to reset disk in disk loader system

--current functionality soft resets emulator as a whole
--need to edit functionality?  specifically for CDs
while true do
	if (FCEU.emulating() then
		FCEU.frameadvance()
	end
	FCEU.frameadvance()
end