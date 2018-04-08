--Nightmare on Elm Street 4-Player TAS, lua script
--Displays important values on screen, including player positions, lag counter, and timers
--April 16th 2009
--Written by adelikat

local lag = 0;	    --Stores lag flag (00D6) value
local lagCounter = 0; --Stores lag change value (increments every time 
local screenpos = 0;  --Stores the screen position value - 00E8
local ZZZ1 = 0;	    --Day/Night timers
local ZZZ2 = 0;
local ZZZ3 = 0;
local HP1 = 0;	    --Boss health
local HP2 = 0;	    --Boss health

while (true) do

--Counters and Lag flag

--Get lag info
lag = memory.readbyte(0x00D6);
if (lag > 0) then
	lag = 1;
	lagCounter = lagCounter+1;
end

--Get screen position
screenpos = memory.readbyte(0x00E8);

--Get enemy HP values
HP1 = memory.readbyte(0x0113);
HP2 = memory.readbyte(0x0114);

--Get Day/night timers
ZZZ1 = memory.readbyte(0x06C8);
ZZZ2 = memory.readbyte(0x06CC);
ZZZ3 = memory.readbyte(0x06D0);

--dispay values
gui.text(191,8,"LagFlag:");
gui.text(235,8,lag);

gui.text(191,16,"LagCount:");
gui.text(235,16,lagCounter);

gui.text(191,24,"ScrnPos:");
gui.text(235,24,screenpos);

gui.text(1,8,"D/N:"..ZZZ1..","..ZZZ2..","..ZZZ3);

--display enemy HP only if there is something to display TODO: display only the value that is useful
if ( (HP1 > 0 and HP1 < 255) or (HP2 > 0 and HP2 < 255) ) then
	gui.text(1,16,"EnemyHPs:"..HP1..","..HP2);
end
	

	FCEU.frameadvance();
end;