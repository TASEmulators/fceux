--Memory Watch
--Creates a Dialog box for monitoring a game's RAM values.

--Version History
-- v0.01a (not yet done!)
-- include some iup stuff and take care of cleanup
require 'auxlib';

local function toHexStr(n)
	return string.format("%X",n);
end;


local mat = iup.matrix {numcol=4, numlin=10,numcol_visible=4, numlin_visible=10, width1="40", width2="80", width3="30", width4="30", ALIGNMENT1="ARIGHT", ALIGNMENT2="ALEFT"};
mat.resizematrix = "YES";
mat:setcell(0,1,"Address"); -- index 0 (i or j) will set title
mat:setcell(0,2,"Title");
mat:setcell(0,3,"Dec");
mat:setcell(0,4,"Hex");

dialogs = dialogs + 1;
handles[dialogs] = 
	iup.dialog{
		iup.vbox{
			iup.label{
				title="Enter a number in the first column (can be hex)."
			},
			iup.label{
				title="Third and fourth are values of that address."
			},
			iup.fill{size="5"},
			mat,
			iup.fill{},
		},
		title="St00pid memory watch",
		size="215x160",
		margin="10x10"
	};

handles[dialogs]:showxy(iup.CENTER, iup.CENTER);

while (true) do
	local cols = tonumber(mat.numcol);
	for i=1,cols do
		local val = tonumber(mat:getcell(i,1));
		if (
			--mat["edit_mode1x"..i] ~= "YES" and -- who knows what you're editing in there
			val and val >= 0 and val <= 0xFFFF -- check bounds
		) then 
			local mem = memory.readbyte(val);
			mat:setcell(i,3,mem.."");
			mat:setcell(i,4,toHexStr(mem));
		end;
	end;
	mat.redraw = "C3:4";
	
	FCEU.frameadvance();
end;
