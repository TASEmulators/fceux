-- BeeBee, LuaBot Frontend v1.07
-- qFox, 2 August 2008

-- we need iup, so include it here (also takes care of cleaning up dialog when script exits)
require 'auxlib';

local botVersion = 1; -- check this version when saving/loading. this will change whenever the botsave-file changes.

function createTextareaTab(reftable, tmptable, token, tab, fun, val) -- specific one, at that :)
	reftable[token] = iup.multiline{title="Contents",expand="YES", border="YES" }; -- ,value=val};
	tmptable[token] = iup.vbox{iup.label{title="function "..fun.."()\n  local result = no;"},reftable[token],iup.label{title="  return result;\nend;"}};
	tmptable[token].tabtitle = tab;
end;
function createTextareaTab2(reftable, tmptable, token, fun, arg) -- specific one, at that :) this one generates no return values
	reftable[token] = iup.multiline{title="Contents",expand="YES", border="YES" }; --,value=fun};
	if (arg) then
		tmptable[token] = iup.vbox{iup.label{title="function "..fun.."(wasOk) -- wasOk (boolean) is true when the attempt was ok\n"},reftable[token],iup.label{title="end;"}};
	else
		tmptable[token] = iup.vbox{iup.label{title="function "..fun.."()\n"},reftable[token],iup.label{title="end;"}};
	end;
	tmptable[token].tabtitle = fun;
end;

function createGUI(n)
	-- this table will keep the references for easy and fast lookup. <STRID, itemobj>
	local reftable = {};
	-- this table we wont keep, it holds references to (mostly) cosmetic elements of the dialog
	local tmptable = {};

	-- ok, dont be intimidated by the next eight blocks of code. they basically all say the same!
	-- every line creates an element for the gui and sets it up. every block is a tabbed pane and
	-- they are all put into another tabbed pane themselves. all references are put into a table
	-- paired with the tokens in the basicbot framework. this allows us to easily walk through
	-- all the pairs of <tokens,references> and replace them in the file, uppon writing.
	
	reftable.ROMNAME = iup.text{title="rom name", size="300x", value="something_or_the_other.rom"};
	tmptable.ROMNAME = iup.hbox{iup.label{title="Rom name: ", size="50x"}, reftable.ROMNAME, iup.fill{}};
	reftable.COMMENT = iup.text{title="comment", size="300x",value="a botscript for some game"};
	tmptable.COMMENT = iup.hbox{iup.label{title="Comment: ", size="50x"}, reftable.COMMENT, iup.fill{}};
	reftable.VERSION = iup.text{title="version", size="70x",value="1.00"};
	tmptable.VERSION = iup.hbox{iup.label{title="Version: ", size="50x"}, reftable.VERSION, iup.fill{}};
	tmptable.SAVE = iup.button{title="Save contents"};
	-- the callback is set after the dialog is created. we need the references to all controls for saving to work :)
	tmptable.LOAD = iup.button{title="Load contents"};
	tmptable.WRITE = iup.button{title="Write bot script"};
	general = iup.vbox{tmptable.ROMNAME,tmptable.COMMENT,tmptable.VERSION,iup.fill{size="5x",},tmptable.SAVE,iup.fill{size="5x",},tmptable.LOAD,iup.fill{size="5x",},tmptable.WRITE,iup.fill{}};
	general.tabtitle = "General";

	createTextareaTab(reftable, tmptable, "bA1",    "A",      "isPressedA1",      "a1");
	createTextareaTab(reftable, tmptable, "bB1",    "B",      "isPressedB1",      "b1");
	createTextareaTab(reftable, tmptable, "START1", "Start",  "isPressedStart1",  "start1");
	createTextareaTab(reftable, tmptable, "SELECT1","Select", "isPressedSelect1", "select1");
	createTextareaTab(reftable, tmptable, "UP1",    "Up",     "isPressedUp1",     "up1");
	createTextareaTab(reftable, tmptable, "DOWN1",  "Down",   "isPressedDown1",   "down1");
	createTextareaTab(reftable, tmptable, "LEFT1",  "Left",   "isPressedLeft1",   "left1");
	createTextareaTab(reftable, tmptable, "RIGHT1", "Right",  "isPressedRight1",  "right1");
	tabs1 = iup.vbox{iup.tabs{tmptable.bA1,tmptable.bB1,tmptable.START1,tmptable.SELECT1,tmptable.UP1,tmptable.DOWN1,tmptable.LEFT1,tmptable.RIGHT1}};
	tabs1.tabtitle = "Player 1";

	createTextareaTab(reftable, tmptable, "bA2",    "A",      "isPressedA2",      "a2");
	createTextareaTab(reftable, tmptable, "bB2",    "B",      "isPressedB2",      "b2");
	createTextareaTab(reftable, tmptable, "START2", "Start",  "isPressedStart2",  "start2");
	createTextareaTab(reftable, tmptable, "SELECT2","Select", "isPressedSelect2", "select2");
	createTextareaTab(reftable, tmptable, "UP2",    "Up",     "isPressedUp2",     "up2");
	createTextareaTab(reftable, tmptable, "DOWN2",  "Down",   "isPressedDown2",   "down2");
	createTextareaTab(reftable, tmptable, "LEFT2",  "Left",   "isPressedLeft2",   "left2");
	createTextareaTab(reftable, tmptable, "RIGHT2", "Right",  "isPressedRight2",  "right2");
	tabs2 = iup.vbox{iup.tabs{tmptable.bA2,tmptable.bB2,tmptable.START2,tmptable.SELECT2,tmptable.UP2,tmptable.DOWN2,tmptable.LEFT2,tmptable.RIGHT2}};
	tabs2.tabtitle = "Player 2";

	createTextareaTab(reftable, tmptable, "bA3",    "A",      "isPressedA3",      "a3");
	createTextareaTab(reftable, tmptable, "bB3",    "B",      "isPressedB3",      "b3");
	createTextareaTab(reftable, tmptable, "START3", "Start",  "isPressedStart3",  "start3");
	createTextareaTab(reftable, tmptable, "SELECT3","Select", "isPressedSelect3", "select3");
	createTextareaTab(reftable, tmptable, "UP3",    "Up",     "isPressedUp3",     "up3");
	createTextareaTab(reftable, tmptable, "DOWN3",  "Down",   "isPressedDown3",   "down3");
	createTextareaTab(reftable, tmptable, "LEFT3",  "Left",   "isPressedLeft3",   "left3");
	createTextareaTab(reftable, tmptable, "RIGHT3", "Right",  "isPressedRight3",  "right3");
	tabs3 = iup.vbox{iup.tabs{tmptable.bA3,tmptable.bB3,tmptable.START3,tmptable.SELECT3,tmptable.UP3,tmptable.DOWN3,tmptable.LEFT3,tmptable.RIGHT3}};
	tabs3.tabtitle = "Player 3";

	createTextareaTab(reftable, tmptable, "bA4",    "A",      "isPressedA4",      "a4");
	createTextareaTab(reftable, tmptable, "bB4",    "B",      "isPressedB4",      "b4");
	createTextareaTab(reftable, tmptable, "START4", "Start",  "isPressedStart4",  "start4");
	createTextareaTab(reftable, tmptable, "SELECT4","Select", "isPressedSelect4", "select4");
	createTextareaTab(reftable, tmptable, "UP4",    "Up",     "isPressedUp4",     "up4");
	createTextareaTab(reftable, tmptable, "DOWN4",  "Down",   "isPressedDown4",   "down4");
	createTextareaTab(reftable, tmptable, "LEFT4",  "Left",   "isPressedLeft4",   "left4");
	createTextareaTab(reftable, tmptable, "RIGHT4", "Right",  "isPressedRight4",  "right4");
	tabs4 = iup.vbox{iup.tabs{tmptable.bA4,tmptable.bB4,tmptable.START4,tmptable.SELECT4,tmptable.UP4,tmptable.DOWN4,tmptable.LEFT4,tmptable.RIGHT4}};
	tabs4.tabtitle = "Player 4";

	createTextareaTab2(reftable, tmptable, "ONSTART",       "onStart",        false);
	createTextareaTab2(reftable, tmptable, "ONFINISH",      "onFinish",       false);
	createTextareaTab2(reftable, tmptable, "ONSEGMENTSTART","onSegmentStart", false);
	createTextareaTab2(reftable, tmptable, "ONSEGMENTEND",  "onSegmentEnd",   false);
	createTextareaTab2(reftable, tmptable, "ONATTEMPTSTART","onAttemptStart", false);
	createTextareaTab2(reftable, tmptable, "ONATTEMPTEND",  "onAttemptEnd",   true);
	createTextareaTab2(reftable, tmptable, "ONINPUTSTART",  "onInputStart",   false);
	createTextareaTab2(reftable, tmptable, "ONINPUTEND",    "onInputEnd",     false);
	tabs5 = iup.vbox{iup.tabs{tmptable.ONSTART, tmptable.ONFINISH, tmptable.ONSEGMENTSTART, tmptable.ONSEGMENTEND, tmptable.ONATTEMPTSTART, tmptable.ONATTEMPTEND, tmptable.ONINPUTSTART, tmptable.ONINPUTEND}};
	tabs5.tabtitle = "Events";
	
	createTextareaTab(reftable, tmptable, "SCORE",    "score",      "getScore",      "score");
	createTextareaTab(reftable, tmptable, "TIE1",    "tie1",      "getTie1",      "tie1");
	createTextareaTab(reftable, tmptable, "TIE2",    "tie2",      "getTie2",      "tie2");
	createTextareaTab(reftable, tmptable, "TIE3",    "tie3",      "getTie3",      "tie3");
	createTextareaTab(reftable, tmptable, "TIE4",    "tie4",      "getTie4",      "tie4");
	tabs6 = iup.vbox{iup.tabs{tmptable.SCORE,tmptable.TIE1,tmptable.TIE2,tmptable.TIE3,tmptable.TIE4}};
	tabs6.tabtitle = "Score";
	
	createTextareaTab(reftable, tmptable, "ISRUNEND",     "isRunEnd",     "isRunEnd",     "isRunEnd");
	createTextareaTab(reftable, tmptable, "MUSTROLLBACK", "mustRollBack", "mustRollBack", "mustRollBack");
	createTextareaTab(reftable, tmptable, "ISSEGMENTEND", "isSegmentEnd", "isSegmentEnd", "isSegmentEnd");
	createTextareaTab(reftable, tmptable, "ISATTEMPTEND", "isAttemptEnd", "isAttemptEnd", "isAttemptEnd");
	createTextareaTab(reftable, tmptable, "ISATTEMPTOK",  "isAttemptOk",  "isAttemptOk",  "isAttemptOk");
	tabs7 = iup.vbox{iup.tabs{tmptable.ISRUNEND,tmptable.MUSTROLLBACK,tmptable.ISSEGMENTEND,tmptable.ISATTEMPTEND,tmptable.ISATTEMPTOK}};
	tabs7.tabtitle = "Selection";
	
	playertabs = iup.tabs{general,tabs1,tabs2,tabs3,tabs4,tabs5,tabs6,tabs7,title};
	handles[n] = iup.dialog{playertabs, title="BeeBee; BasicBot Frontend", size="450x200"}
	handles[n]:showxy(iup.CENTER, iup.CENTER)

	-- now set the callback function for the save button. this will use all the references above.
	-- these remain ok in the anonymous function by something called "closures". this means that
	-- these variables, although local to the scope of the function, will remain their value in
	-- the anonymous function. hence we can refer to them and fetch their contents, even though
	-- you cant refer to them outside the context of the createGUI function.
	tmptable.WRITE.action = 
		function(self, n)
			local file = iup.filedlg{allownew="YES",dialogtype="SAVE",directory="./lua",showhidden="YES",title="Save botfile"};
			file:popup(iup.ANYWHERE,iup.ANYWHERE);

			if (file.value == "NULL") then
				iup.Message("An error occurred trying to save your settings");
				return;
			elseif (file.status == "-1") then 
			  iup.Message("IupFileDlg","Operation canceled");
			  return;
			end

			-- ok, file selected, if an error occurred or user canceled, the function already returned, so lets write the bot!

			-- get the framework first. we need it to find the relevant tokens
			local fh = assert(io.open("luabot_framework.lua","r"));
			local framework = fh:read("*a");
			fh:close();

			-- now replace all tokens by gui values
			-- this is where the reftable comes in very handy :p
			for token,obj in pairs(reftable) do
				local st00pid = (reftable[token].value or "");
				framework = string.gsub(framework, "-- "..token, st00pid, 1); -- if nothing was entered, obj.value returns nil (not ""), so we have to make that translation
			end;

			-- open the file, if old file, clear it
			if (file.status == "1") then 
				fh = assert(io.open(file.value,"wb"));
			else -- (file.status == "0")
				fh = assert(io.open(file.value,"w+b")); -- clear file contents
			end;
			
			-- write it
			fh:write(framework);
			
			-- close it (automatically flushed)
			fh:close();
			fh = nil;
			
			iup.Message ("Success", "Bot written to "..file.value.."!");
		end;
	tmptable.SAVE.action =
		function(self, n)
			local file = iup.filedlg{allownew="YES",dialogtype="SAVE",directory="./lua",showhidden="YES",title="Save botfile",extfilter="BasicBot (*.bot)|*.bot|All files (*.*)|*.*|"};
			file:popup(iup.ANYWHERE,iup.ANYWHERE);

			if (file.status == 1) then -- cancel
				return;
			end;
			
			-- open the file, if old file, clear it
			if (file.status == "1") then 
				fh = assert(io.open(file.value,"wb"));
			else -- (file.status == "0")
				fh = assert(io.open(file.value,"w+b")); -- clear file contents
			end;

			-- allow us to detect the botfile version (warn the user if it's different?)
			fh:write(botVersion.."\n");

			-- now replace all tokens by gui values
			-- this is where the reftable comes in very handy :p
			for token,obj in pairs(reftable) do
				print("------");
				print(token.." control -> "..tostring(obj));
				print(".value: "..tostring(obj.value));
				local st00pid = obj.value;
				if (not st00pid) then st00pid = ""; end;
				print(string.len(st00pid));
				fh:write(string.len(st00pid).."\n");
				if (string.len(st00pid) > 0) then fh:write(st00pid); end;
				fh:write("\n");
			end;
			
			fh:close();
			iup.Message ("Success", "Settings saved!");
		end;
		tmptable.LOAD.action =
			function (self, n)
				-- this function currently crashes fceux without notification
				-- possibly because offsets are badly calculated, but serves as an example now
				local file = iup.filedlg{allownew="NO",dialogtype="OPEN",directory="./lua",showhidden="YES",title="Save botfile",extfilter="BasicBot (*.bot)|*.bot|All files (*.*)|*.*|"};
				file:popup(iup.ANYWHERE,iup.ANYWHERE);
				if (file.status == 1) then -- cancel
					iup.Message ("Success", "Canceled by you...");
					return;
				end;
				local nellen = string.len("\n"); -- platform independent
				fh = assert(io.open(file.value,"r"));

				fh:read("*n"); -- version
				fh:read("*l"); -- return
				
				local len;
				local data;
				for token,crap in pairs(reftable) do
					len = fh:read("*n"); -- read line (length)
					if (not len) then 
						iup.Message ("Warning", "End of file reached too soon!");
						break; 
					end; -- no more data... (should we erase the rest?)
					fh:read("*l"); -- return
					data = fh:read(len);
					if (not data) then 
						iup.Message ("Warning", "End of file reached too soon!");
						break; 
					end; -- no more data... (should we erase the rest?)
					reftable[token].value = data;
					fh:read("*l"); -- return
				end;
				iup.Message ("Success", "Settings loaded!");
			end;
end;

dialogs = dialogs + 1;
createGUI(dialogs);
while (true) do
	FCEU.frameadvance();
end;