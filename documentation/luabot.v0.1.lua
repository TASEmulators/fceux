-- LuaBot, concept bot for FCEU and ZSnes and any other emu that has compatible Lua engine
-- qFox, 24 July 2008

local key1 = {};
local key2 = {};
local lastkey1 = {};     -- keys pressed in previous frame for player 1
local lastkey2 = {};     -- keys pressed in previous frame for player 2
local frame = 0;         -- number of frames (current value is current frame count, incremented at the start of a new frame)
local attempt = 1;       -- number of attempts (current value is current attempt, incremented after the end of an attempt)
local segment = 1;       -- number of segments (current value is current segment, incremented after the end of a segment)
local okattempts = 0;
local failattempts = 0;

local segments = {};     -- table to hold every segment
segments[1] = {};        -- initialize the first segment

local maxframes = 200;
local maxattempts = 500;
local maxsegments = 100;

local playingbest = false;    -- when going to the next segment, we need to play the best segment to record, this indicates when we're doing so
local keyrecording1 = {};
local keyrecording2 = {};

-- some constants
local maxvalue = 100;    -- always true
local minvalue = 0;      -- always false
local press = maxvalue;
local release = minvalue;

-- static constants
local X = 95;
local Y = 30;
local Z = 0;
local P = 0;
local Q = 0;

local vars = {};         -- variable table. each cell holds a variable. variables are remembered accross segments

local outputcounter = 0;
local updateevery = 1000;

local function getOk() -- is current run ok? heuristic value 0-100! 100 = always press, 0 = always release
end;

local function getScore() -- score of current attempt
	return release;
end;

local function getTie1() -- tie breaker of current attempt in case score is equal
	return release;
end;

local function getTie2() -- second tie breaker
	return release;
end;

local function getTie3() -- third tie breaker
	return release;
end;

local function getTie4() -- fourth tie breaker
	return release;
end;

local function getRunEnd()
	return release;
end;

local function getRollBack() -- drop back to previous segment? called at the end of a segment
	return release;
end;

local function getSegmentEnd()
	return release;
end;

local function getAttemptEnd() -- determine when a attempt should be restarted
	return release;
end;

local function getA1() -- magic number time... sorry will fix later
	return release;
end;

local function getB1()
	return release;
end;

local function getStart1()
	return release;
end;

local function getSelect1()
	return release;
end;

local function getUp1()
	return release;
end;

local function getDown1()
	return release;
end;

local function getLeft1()
	return release;
end;

local function getRight1()
	return release;
end;

local function getA2() -- magic number time... sorry will fix later
	return release;
end;

local function getB2()
	return release;
end;

local function getStart2()
	return release;
end;

local function getSelect2()
	return release;
end;

local function getUp2()
	return release;
end;

local function getDown2()
	return release;
end;

local function getLeft2()
	return release;
end;

local function getRight2()
	return release;
end;

function beforeBotLoop() -- this code should run before the bot starts
	return release;
end;

local function afterBotLoop() -- code ran after the bot finishes  (no return)
	return release;
end;

local function beforeSegmentCode() -- code ran after initializing a new segment, before beforeAttemptCode()
	return release;
end;

local function afterSegmentCode() -- code ran after a segment finishes, before cleanup of segment vars
	return release;
end;

local function beforeAttemptCode() -- code ran after initalizing a new attempt, before beforeInputCode(). not ran for playback
	return release;
end;

local function afterAttemptCode(wasok) -- code ran after an attempt ends before cleanup code, argument is boolean true when attempt was ok, boolean false otherwise. not ran for playback
	return release;
end;

local function beforeInputCode() -- code ran prior to getting input. not ran for playback
	return release;
end;

local function afterInputCode() -- code ran after getting input (last function before frame ends, you can still manipulate the input here!). not ran for playback
	return release;
end;

-- the bot starts here..

beforeBotLoop();                                        -- run this code first

segments[segment].savestate = savestate.create();       -- create anonymous savestate obj for start of first segment
savestate.save(segments[segment].savestate);            -- save current state to it, it will be reloaded at the start of each frame
local startkey1 = key1;                                   -- save the last key pressed in the beforeBotLoop. serves as an anchor for the first segment
local startkey2 = key2;
local startvars = vars;                                 -- save the vars array (it might have been used by the beforeBotLoop)
lastkey1 = key1;                                        -- set this too
lastkey2 = key2;
FCEU.speedmode("maximum");

beforeSegmentCode();
beforeAttemptCode();

-- This will loops for each frame, at the end of the while
-- the frameadvance is called, causing it to advance
while (getRunEnd() <= math.random(0,99)) do
	frame = frame + 1; -- inrease the frame count (++frame?)
	if (getAttemptEnd() > math.random(0, 99)) then -- load save state, continue with next attempt (disabled when playing back best)
		-- record this attempt
		if (not segments[segment].prev) then segments[segment].prev = {}; end;
		segments[segment].prev.frames = frame;
		segments[segment].prev.attempt = attempt;
		segments[segment].prev.score = getScore();
		segments[segment].prev.tie1 = getTie1();
		segments[segment].prev.tie2 = getTie2();
		segments[segment].prev.tie3 = getTie3();
		segments[segment].prev.tie4 = getTie4();
		segments[segment].prev.ok = (getOk() > math.random(0,99)); -- this is the check whether this attempt was valid or not. if not, it cannot become the best attempt.
		-- update ok/failed attempt counters
		if (segments[segment].prev.ok) then 
			okattempts = okattempts + 1;
			afterAttemptCode(true);
		else 
			failattempts = failattempts + 1;
			afterAttemptCode(false);
		end;


		-- if this attempt was better then the previous one, replace it
		-- its a long IF, but all it checks (lazy eval) is whether the current
		-- score is better then the previous one, or if its equal and the tie1
		-- is better then the previous tie1 or if the tie1 is equal to the prev
		-- etc... for all four ties. Only tie4 actually needs to be better, tie1
		-- through tie3 can be equal as well, as long as the next tie breaks the
		-- same tie of the previous attempt :)
		if (segments[segment].prev.ok and (not segments[segment].best or (getScore() > segments[segment].best.score or (getScore() == segments[segment].best.score and (getTie1() > segments[segment].best.tie1 or (getTie1() == segments[segment].best.tie1 and (getTie1() > segments[segment].best.tie1 or (getTie1() == segments[segment].best.tie1 and (getTie1() > segments[segment].best.tie1 or (getTie1() == segments[segment].best.tie1 and getTie1() > segments[segment].best.tie1)))))))))) then
			-- previous attempt was better then current best (or no current best
			-- exists), so we (re)place it.
			if (not segments[segment].best) then segments[segment].best = {}; end;
			if (not segments[segment].prev.frames) then fceu.x.y.z.crash(); end;
			segments[segment].best.frames = segments[segment].prev.frames;
			segments[segment].best.attempt = segments[segment].prev.attempt;
			segments[segment].best.score = segments[segment].prev.score;
			segments[segment].best.tie1 = segments[segment].prev.tie1;
			segments[segment].best.tie2 = segments[segment].prev.tie2;
			segments[segment].best.tie3 = segments[segment].prev.tie3;
			segments[segment].best.tie4 = segments[segment].prev.tie4;
			segments[segment].best.keys1 = keyrecording1; -- backup the recorded keys
			segments[segment].best.keys2 = keyrecording2; -- backup the recorded keys player 2
			segments[segment].best.lastkey1 = lastkey1; -- backup the lastkey
			segments[segment].best.lastkey2 = lastkey2; -- backup the lastkey
			segments[segment].best.vars = vars; -- backup the vars table
		end
		
		if (getSegmentEnd() > math.random(0,99)) then -- special case, replay the best attempt and continue from there onwards...
			afterSegmentCode();
			if (getRollBack() > math.random(0,99)) then
				segments[segment] = nil; -- remove current segment data
				attempt = 0; -- will be incremented in a few lines to be 1
				segment = segment - 1;
				segments[segment].best = nil;
				segments[segment].prev = nil;
			else
				playingbest = true;
			end;
		end;

		-- reset vars
		attempt = attempt + 1;
		frame = 1;
		keyrecording1 = {}; -- reset recording :)
		keyrecording2 = {}; -- reset recording :)
		-- set lastkey to lastkey of previous segment (or start, if first segment)
		-- also set the vars table to the table of the previous segment
		if (segment == 1) then 
			lastkey1 = startkey1;
			lastkey1 = startkey1;
			vars = startvars;
		else 
			lastkey1 = segments[segment-1].best.lastkey1;
			lastkey2 = segments[segment-1].best.lastkey2;
			vars = segments[segment-1].best.vars;
		end;
		-- load the segment savestate
		if (segments[segment].savestate) then -- load segment savestate and try again :)
			savestate.load(segments[segment].savestate); 
		else
			fceu.crash(); -- this crashes because fceu is a nil table :) as long as gui.popup() doesnt work...
		end;

		if (getRunEnd() > math.random(0,99)) then break; end; -- if end of run, break out of main loop
		
		if (not playingbest) then beforeAttemptCode(); end;
	end; -- continues with (new) attempt

	if (playingbest) then -- press keys from memory
		if (not frame) then x.y.z(); end;
		if (frame >= segments[segment].best.frames) then -- end of playback, start new segment
			playingbest = false;
			lastkey1 = segments[segment].best.lastkey1;
			lastkey2 = segments[segment].best.lastkey2;
			vars = segments[segment].best.vars;
			segment = segment + 1;
			segments[segment] = {};
			-- create a new savestate for the start of this segment
			segments[segment].savestate = savestate.create();
			savestate.save(segments[segment].savestate);
			-- reset vars
			frame = 1;
			attempt = 1;
			key1 = {};
			key2 = {};
			keyrecording1 = {}; -- reset recording :)
			keyrecording2 = {}; -- reset recording :)
			-- after this, the next segment starts because playingbest is no longer true
			beforeSegmentCode();
			beforeAttemptCode();
		else
			key1 = segments[segment].best.keys1[frame];
			key2 = segments[segment].best.keys2[frame];
			gui.text(10,10,"Playback best of segment "..segment.."\nFrame: "..frame);
		end;
	end;

	if (getRunEnd() > math.random(0,99)) then break; end; -- if end of run, break out of main loop
	
	if (not playingbest) then
	 -- press keys from bot
		gui.text(10,10,"Attempt: "..attempt.."\nFrame: "..frame);
		if (segments[segment] and segments[segment].best and segments[segment].prev) then 
			gui.text(10,30,"Last score: "..segments[segment].prev.score.." ok="..okattempts..", not="..failattempts.."\nBest score: "..segments[segment].best.score);
		elseif (segments[segment] and segments[segment].prev) then
			gui.text(10,30,"Last score: "..segments[segment].prev.score.."\nBest score: none"); 
		end;
		gui.text(10,50,"Segment: "..segment);
		
		beforeInputCode();

		-- player 1
		key1 = {};
		if (getUp1() > math.random(0, 99) )    then key1.up = 1; end;
		if (getDown1() > math.random(0, 99))   then key1.down = 1; end;
		if (getLeft1() > math.random(0, 99))   then key1.left = 1; end;
		if (getRight1() > math.random(0, 99))  then key1.right = 1; end;
		if (getA1() > math.random(0, 99))      then key1.A = 1; end;
		if (getB1() > math.random(0, 99))      then key1.B = 1; end;
		if (getSelect1() > math.random(0, 99)) then key1.select = 1; end;
		if (getStart1() > math.random(0, 99))  then key1.start = 1; end;
		lastkey1 = key1;
		-- player 2
		key2 = {};
		if (getUp2() > math.random(0, 99) )    then key2.up = 1; end;
		if (getDown2() > math.random(0, 99))   then key2.down = 1; end;
		if (getLeft2() > math.random(0, 99))   then key2.left = 1; end;
		if (getRight2() > math.random(0, 99))  then key2.right = 1; end;
		if (getA2() > math.random(0, 99))      then key2.A = 1; end;
		if (getB2() > math.random(0, 99))      then key2.B = 1; end;
		if (getSelect2() > math.random(0, 99)) then key2.select = 1; end;
		if (getStart2() > math.random(0, 99))  then key2.start = 1; end;
		lastkey2 = key2;

		afterInputCode();
	
		keyrecording1[frame] = key1; -- record these keys (offsets at 1!)
		keyrecording2[frame] = key2; -- record these keys (offsets at 1!)
	end;
	joypad.set(1, key1);
	joypad.set(1, key2);
	-- next frame
	FCEU.frameadvance()
end;

afterBotLoop(); -- allow user cleanup before starting the final botloop

while (true) do 
	if (segments[segment].best) then gui.text(30,100,"end: max attempt ["..segment.."] had score = "..segments[segment].best.score);
	elseif (segment > 1 and segments[segment-1].best) then gui.text(30,100,"end: no best attempt ["..segment.."]\nPrevious best score: "..segments[segment-1].best.score);
	else gui.text(30,100,"end: no best attempt ["..segment.."] ..."); end;
	FCEU.frameadvance();
end;

