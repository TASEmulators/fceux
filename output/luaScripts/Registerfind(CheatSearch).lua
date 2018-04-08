--RegisterFind
--written by QFox
--Weeds out RAM addresses along the lines of Cheat Search and Ram Filter.  Has some features not present in either of those 2 hardcoded dialogs.

-- v0.1a (far from done!)
-- include some iup stuff and take care of cleanup
require 'auxlib';

local function toHexStr(n)
	return string.format("%X",n);
end;

local mems;
local memstart = 0;
local memend = 0x200;
local running = false;
local removetop = 0;

local from = iup.text{value="0x0000"};
local till = iup.text{value="0x0100"};
local output = iup.multiline{ expand="YES" };
local changecheck = iup.toggle{title="Remove changed addresses"};
local equalcheck = iup.toggle{title="Remove unchanged addresses"};
local init = iup.button{title="Init"};
init.action =
	function(self)
		mems = {};
		memstart = math.min(0xFFFE, math.max(0, tonumber(from.value)));
		memend = math.min(0xFFFF, math.max(1, tonumber(till.value)));
		for i=(memstart+1),(memend+1) do
			mems[i] = memory.readbyte(i-1);
		end;
	end;
local set = iup.button{title="Set"};
set.action =
	function(self)
		if (mems) then
			for i=(memstart+1),(memend+1) do
				if (mems[i]) then
					mems[i] = memory.readbyte(i);
				end;
			end;
			step:action();
		end;
	end;
local start = iup.button{title="Run"};
start.action =
	function(self)
		if (not mems) then init:action(); end; -- same as pressing the init button
		running = true;
	end;
local stop = iup.button{title="Stop"};
stop.action =
	function(self)
		running = false;
	end;
local step = iup.button{title="Step"};
step.action =
	function(self)
		if (not mems) then init:action(); end;
		local s = '';
		local count = 0;
		gui.text(50,50,"change: "..changecheck.value.." equal: "..equalcheck.value);
		for i=(memstart+1),(memend+1) do
			local nowval = memory.readbyte(i-1);
			if (
				mems[i] and                           -- no memory, no need to print it
				(	not running or                      -- always print (existing) values when the bot is not running
					(
					(changecheck.value == "OFF" or mems[i] == nowval) and -- its ok, we're just removing unequal values
					(equalcheck.value == "OFF" or mems[i] ~= nowval)     -- its ok, we're just removing equal values
					)
				)
				) then
				if (removetop > 0) then -- delete this (should be processed even while not running)
					mems[i] = false;
					removetop = removetop - 1; 
				else
					count = count + 1;
					s = s.."0x"..toHexStr(i-1)..": "..toHexStr(nowval).."\n";
				end;
			elseif (mems[i] and (removetop > 0 or running)) then -- we're still deleting results
				mems[i] = false;
				if (removetop > 0) then removetop = removetop - 1; end;
			end;
		end;
		output.value = "Have "..count.." addresses:\n"..s;
	end;
local removetext = iup.text{};
local remove = iup.button{title="del",size="30x"};
remove.action =
	function(self)
		removetop = tonumber(removetext.value);
		removetext.value = tonumber(removetext.value).."";
	end;
local fewertext = iup.text{};
local fewer = iup.button{title="del",size="30x"};
fewer.action =
	function(self)
		local than = tonumber(fewertext.value);
		if (mems and than) then
			for i=memstart,memend do
				if (mems[i] and mems[i] < than) then
					mems[i] = false;
				end;
			end;
		end;
		step:action();
	end;
local moretext = iup.text{};
local more = iup.button{title="del",size="30x"};
more.action =
	function(self)
		local than = tonumber(moretext.value);
		if (mems and than) then
			for i=memstart,memend do
				if (mems[i] and mems[i] > than) then
					mems[i] = false;
				end;
			end;
		end;
		step:action();
	end;
local sametext = iup.text{};
local same = iup.button{title="del",size="30x"};
same.action =
	function(self)
		local than = tonumber(sametext.value);
		if (mems and than) then
			for i=memstart,memend do
				if (mems[i] and mems[i] == than) then
					mems[i] = false;
				end;
			end;
		end;
		step:action();
	end;

dialogs = dialogs + 1;
handles[dialogs] = 
	iup.dialog{
		iup.vbox{
			iup.fill{size="5"},
			iup.hbox{
				iup.label{title="Start offset",size="50x"},
				from
			},
			iup.hbox{
				iup.label{title="End offset",size="50x"},
				till
			},
			iup.fill{size="5"},
			iup.hbox{
				remove,
				iup.fill{size="5"},
				iup.label{title="Remove top",size="50x"},
				iup.fill{size="5"},
				removetext,
				iup.fill{size="5"},
				iup.label{title="(next step)"},
				removetop
			},
			iup.fill{size="5"},
			iup.hbox{
				fewer,
				iup.fill{size="5"},
				iup.label{title="When less than: ",size="70x"},
				fewertext
			},
			iup.fill{size="5"},
			iup.hbox{
				more,
				iup.fill{size="5"},
				iup.label{title="When more than: ",size="70x"},
				moretext
			},
			iup.fill{size="5"},
			iup.hbox{
				same,
				iup.fill{size="5"},
				iup.label{title="When equal to: ",size="70x"},
				sametext
			},
			iup.fill{size="5"},
			iup.hbox{
				init,
				iup.fill{size="5"},
				start,
				iup.fill{size="5"},
				stop,
				iup.fill{size="5"},
				step,
				iup.fill{size="5"},
				set
			},
			iup.fill{size="5"},
			iup.label{title="Filters below are automatically applied when you"},
			iup.label{title="press run or press the step button."},
			iup.fill{size="5"},
			changecheck,
			equalcheck,
			iup.fill{size="5"},
			iup.hbox{
				output
			}
		},
		title="Lua Register Finder",
		size="200x300"
	};
handles[dialogs]:showxy(iup.CENTER, iup.CENTER);



while (true) do
	step:action(); -- same as pressing the step button
	FCEU.frameadvance();
end;
