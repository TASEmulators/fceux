-- Show Palette
-- Click for other palette boxes
-- P00 - P3F are NES palette values.
-- P40 - P7F are LUA palette values.

-- True or False
ShowTextLabels=true;

function DecToHex(numberin)
 if (numberin < 16) then
  return string.format("0%X",numberin);
 else
  return string.format("%X",numberin);
 end;
end;

function text(x,y,str,text,back)
	if (x > 0 and x < 255 and y > 0 and y < 240) then
		gui.text(x,y,str,text,back);
	end;
end;

local ButtonWasPressed;
local CurrentPaletteDisplay=0;



while(true) do

FCEU.frameadvance();

for i = 0, 7 do
 gui.box(0 + (30*i),0,29 + (30*i),29,"P" .. DecToHex(0+i+(CurrentPaletteDisplay * 8)),"P" .. DecToHex(0+i+(CurrentPaletteDisplay * 8)));
 gui.box(0 + (30*i),30,29 + (30*i),59,"P" .. DecToHex(16+i+(CurrentPaletteDisplay * 8)),"P" .. DecToHex(16+i+(CurrentPaletteDisplay * 8)));
 gui.box(0 + (30*i),60,29 + (30*i),89,"P" .. DecToHex(32+i+(CurrentPaletteDisplay * 8)),"P" .. DecToHex(32+i+(CurrentPaletteDisplay * 8)));
 gui.box(0 + (30*i),90,29 + (30*i),119,"P" .. DecToHex(48+i+(CurrentPaletteDisplay * 8)),"P" .. DecToHex(48+i+(CurrentPaletteDisplay * 8)));
 if(ShowTextLabels == true) then
  text(6 + (30*i),11,"P" .. DecToHex(0+i+(CurrentPaletteDisplay * 8)))
  text(6 + (30*i),41,"P" .. DecToHex(16+i+(CurrentPaletteDisplay * 8)))
  text(6 + (30*i),71,"P" .. DecToHex(32+i+(CurrentPaletteDisplay * 8)))
  text(6 + (30*i),101,"P" .. DecToHex(48+i+(CurrentPaletteDisplay * 8)))
 end;
end;

mousestuff = input.get()

if (not ButtonWasPressed) then
 if (mousestuff.leftclick) then
  ButtonWasPressed = 1; 
  CurrentPaletteDisplay=CurrentPaletteDisplay+1;
  if (CurrentPaletteDisplay == 2) then
   CurrentPaletteDisplay=8;
  end;
  if (CurrentPaletteDisplay == 10) then
   CurrentPaletteDisplay=0;
  end;
 end;
end

ButtonWasPressed = (mousestuff.leftclick);

end;