---------------------------------------------------------------------------
-- Custom Lag Indicator for "RoboCop Vs The Terminator (U) (Prototype).nes"
-- by AnS, 2012
---------------------------------------------------------------------------
-- Showcases following functions:
-- * emu.setlagflag()
---------------------------------------------------------------------------
-- Usage:
-- The "RoboCop Vs The Terminator" game, as well as "Battle City" and some
-- other NES games, polls gamepads input every frame (in NMI), even when
-- it's lagging. So standard way of detecting lag (used by FCEUX and other
-- emulators) does not work for such games, and you have to determine lag
-- manually.
-- 
-- In this game example we can detect lag by watching the RAM address 0x322.
-- This counter increases in non-lag frames only.
-- Debugger says the address increases by the instruction "INC $0322" which
-- is executed at ROM address 07:C6E9. So we should register a hook to this
-- instruction, and every time it's executed we can be sure that current
-- frame doesn't have lag ("no_lag = true").
-- Then we should register another hook to the point where the game
-- polls Input (actually slightly after the point), so we can override
-- standard lag detection.
-- It's easy to find the code where the game polls input (reading from $4016).
-- Se we register a hook to an instruction following the input polling code.
-- In this case its ROM address 07:DE58.
---------------------------------------------------------------------------

no_lag = false;

function set_nolag()
	no_lag = true;
end
memory.registerexecute(0xC6E9, set_nolag);	-- 07:C6E9 INC $0322

function determine_lagflag()
	if (no_lag) then
		emu.setlagflag(false);
		no_lag = false;			-- no_lag only affects once
	else
		emu.setlagflag(true);
	end
end
memory.registerexecute(0xDE58, determine_lagflag);	-- 07:DE58 The end of the cycle of reading from $4016-4017 (the point where FCEUX sets lagFlag to 0)

