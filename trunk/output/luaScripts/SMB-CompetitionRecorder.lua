-- Super Mario Bros. script by ugetab.
-- 2010, April 20th.
-- Competition Recorder:
-- Start the script, then make a recording from Start.
-- Play until you get a score you'd like to keep, then end the movie.
-- A record of your best scores, and the filename you got it in, will be saved.
-- You can easily find your best score, because it will be sorted highest to lowest.
-- The last entry is always erased, but is also always the lowest score.

-- The best score for your current movie file will be displayed above the coin counter.

-- The reason this is good for competition is that you can't get away with cheating,
-- unless the game allows it. A movie file is a collection of button presses which
-- can be played back. If it doesn't play back the same for someone else, then there's
-- probably a problem with the game file, or with what the person recording did.

function text(x,y,str)
	if (x > 0 and x < 255 and y > 0 and y < 240) then
		gui.text(x,y,str);
	end;
end;

function bubbleSort(table)
--http://www.dreamincode.net/code/snippet3406.htm
--check to make sure the table has some items in it
        if #table < 2 then
                --print("Table does not have enough data in it")
                return;
        end;
 
        for i = 0, (#table / 2) -1 do --> array start to end. Arrays start at 1, not 0.
                for j = 1, (#table / 2) -1 do
                        if tonumber(table[(j*2)+1]) > tonumber(table[(j*2)-1]) then -->switch
                                temp1 = table[(j*2) - 1];
                                temp2 = table[(j*2)];
                                table[(j*2) - 1] = table[(j*2) + 1];
                                table[(j*2)] = table[(j*2) + 2];
                                table[(j*2) + 1] = temp1;
                                table[(j*2) + 2] = temp2;
                        end;
                end;
        end;
	return;
end;

filesizefile = io.open("MaxScore_LUA.txt","r");

if filesizefile == nil then
	filewritefile = io.open("MaxScore_LUA.txt","w");
	filewritefile:close();
	filesizefile = io.open("MaxScore_LUA.txt","r");
end;

filesize = filesizefile:seek("end");
filesizefile:close();
scores = {"0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0"};

if filesize == nil then
	filesize = 0;
end;

if filesize < 20 then
	fileinit = io.open("MaxScore_LUA.txt","w+")
	for i = 1, #scores do
		fileinit:write(scores[i]..'\n');
	end;
	fileinit:close();
end;

activescoring = false;
moviename = ""; 

maxscoresave = -9999999;

text(83,8,"-Inactive-");

while (true) do
	if (movie.mode() == "record") then
		if movie.ispoweron() then
			maxscoretest = memory.readbyte(0x07d7)..memory.readbyte(0x07d8)..memory.readbyte(0x07d9)..memory.readbyte(0x07da)..memory.readbyte(0x07db)..memory.readbyte(0x07dc).."0";
			activescoring = true;

			--if (tonumber(maxscoretest) >= tonumber(maxscoresave)) then
				if (tonumber(maxscoretest) <= 9999990) then
					maxscoresave = maxscoretest;
					moviename = movie.getname();
				end;
			--end;

			text(83,8,maxscoresave);
		end;
	end;
	if (movie.mode() == nil) then
		if (activescoring == true) then
			activescoring = false;
			text(83,8,"-Inactive-");
			readfile = io.open("MaxScore_LUA.txt","r");
			linecount = 1
			for line in readfile:lines() do
				if linecount <= 20 then
					scores[linecount] = line;
				end;
 				linecount = linecount + 1;
			end;
			readfile:close ();
				
			--if tonumber(maxscoresave) > tonumber(scores[19]) then
			scores[19] = maxscoresave;
			scores[20] = moviename;
			bubbleSort(scores);
			savefile = io.open("MaxScore_LUA.txt","w+")
			for i = 1, #scores do
				savefile:write(tostring(scores[i]));
				savefile:write('\n');
			end;
			savefile:close();
			--end;
		end;
	end;
	FCEU.frameadvance();
end;
