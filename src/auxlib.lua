local iuplua_open = package.loadlib("iuplua51.dll", "iuplua_open")
iuplua_open()
local iupcontrolslua_open = package.loadlib("iupluacontrols51.dll", "iupcontrolslua_open")
iupcontrolslua_open() 

function emu.OnClose.iuplua()
	if(emu.OnCloseIup ~= nil) then
		emu.OnCloseIup();
	end
	iup.Close(); 
end