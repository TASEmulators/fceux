-- LuaBot, concept bot for FCEU and ZSnes and any other emu that has a compatible Lua engine
-- qFox, 24 July 2008
-- version 0.1

local function realcount(t) -- accurately count the number of elements of a table, even if they contain nil values. not fast, but accurate.
	local n = 0;
	for i,_ in pairs(t) do
		n = n + 1;
	end;
	return n;
end;

local maxvalue = 100;    -- always true
local minvalue = 0;      -- always false

-- the bot kind of works like this function. for every option, a value is requested from the programmer. 
-- if that value is higher then a random value 0-99, this function returns true, else false.
-- the idea is that keys are pressed based on a heuristic value. if you want absolute values, simply
-- return values like minvalue or maxvalue, or their syntactic equals yes/no and press/release (to make
-- the sourcecode easier to read). you can use the variable maybe for 50% chance.
-- this function also reduces a lot of calls to random() if the user uses absolute values.
local function rand_if(n)
	if (n <= minvalue) return false;
	if (n >= maxvalue) return true;
	if (n > math.random(minvalue, maxvalue-1)) then return true; end;
	return false;
end;

local loopcounter = 0;   -- counts the main loop
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
segments[1] = {};        -- initialize the first segment, we initialize the savestate right after the before code has ran

local maxframes = 2;
local maxattempts = 2;
local maxsegments = 100;

local playingbest = false;    -- when going to the next segment, we need to play the best segment to record, this indicates when we're doing so
local keyrecording1 = {};
local keyrecording2 = {};

-- some constants
local press = maxvalue;
local release = minvalue;
local yes = maxvalue;
local maybe = maxvalue/2;
local no = minvalue;

-- static constants
local X = 95;
local Y = 30;
local Z = 0;
local P = 0;
local Q = 0;

local vars = {};         -- variable table. each cell holds a variable. variables are remembered accross segments

local outputcounter = 0;
local updateevery = 1000;

-- user defined functions

local function getScore() -- score of current attempt
	return 0;
end;

local function getTie1() -- tie breaker of current attempt in case score is equal
	return 0;
end;

local function getTie2() -- second tie breaker
	return 0;
end;

local function getTie3() -- third tie breaker
	return 0;
end;

local function getTie4() -- fourth tie breaker
	return 0;
end;

local function isRunEnd() -- gets called 3x! twice in the main loop (every frame). determines whether the bot should quit.
	return no;
end;

local function mustRollBack() -- drop back to previous segment? called at the end of a segment
	return no;
end;

local function isSegmentEnd() -- end of current segment? (usually just x frames or being really stuck (to rollback))
	return yes;
end;

local function isAttemptOk() -- is current run ok? like, did you die? (then the run is NOT ok... :). return no for no and yes for yes or be left by chance.
	return yes;
end;

local function isAttemptEnd() -- end of current attempt? (like when you die or reach a goal)
	return yes;
end;

local function pressKeyA1() -- 
	return no;
end;

local function pressKeyB1()
	return no;
end;

local function pressKeyStart1()
	return no;
end;

local function pressKeySelect1()
	return no;
end;

local function pressKeyUp1()
	return no;
end;

local function pressKeyDown1()
	return no;
end;

local function pressKeyLeft1()
	return no;
end;

local function pressKeyRight1()
	return no;
end;

local function pressKeyA2() -- magic number time... sorry will fix later
	return no;
end;

local function pressKeyB2()
	return no;
end;

local function pressKeyStart2()
	return no;
end;

local function pressKeySelect2()
	return no;
end;

local function pressKeyUp2()
	return no;
end;

local function pressKeyDown2()
	return no;
end;

local function pressKeyLeft2()
	return no;
end;

local function pressKeyRight2()
	return no;
end;

-- now follow the "events", one for the start and end of a frame, attempt, segment and whole bot

function onBotLoopStart() -- this code should run before the bot starts, for instance to start the game from power on and get setup the game
end;

local function onBotLoopEnd() -- code ran after the bot finishes  (no return)
end;

local function onSegmentStart() -- code ran after initializing a new segment, before onAttemptStart() (no return)
end;

local function onSegmentEnd() -- code ran after a segment finishes, before cleanup of segment vars
end;

local function onAttemptStart() -- code ran after initalizing a new attempt, before onInputStart(). not ran for playback
end;

local function onAttemptEnd(wasok) -- code ran after an attempt ends before cleanup code, argument is boolean true when attempt was ok, boolean false otherwise. not ran for playback
end;

local function onInputStart() -- code ran prior to getting input. not ran when playing back
end;

local function onInputEnd() -- code ran after getting input (last function before frame ends, you can still manipulate the input here!). not ran for playback
end;

-- the bot starts here..

onBotLoopStart();                                        -- run this code first

segments[segment].savestate = savestate.create();       -- create anonymous savestate obj for start of first segment
savestate.save(segments[segment].savestate);            -- save current state to it, it will be reloaded at the start of each frame
local startkey1 = key1;                                   -- save the last key pressed in the onBotLoopStart. serves as an anchor for the first segment
local startkey2 = key2;
local startvars = vars;                                 -- save the vars array (it might have been used by the onBotLoopStart)
lastkey1 = key1;                                        -- set this too
lastkey2 = key2;
--FCEU.speedmode("maximum");

onSegmentStart();
onAttemptStart();

collectgarbage();

-- This will loops for each frame, at the end of the while
-- the frameadvance is called, causing it to advance
while (rand_if(isRunEnd())) do
	loopcounter = loopcounter + 1; -- count the number of botloops
	gui.text(200,10,loopcounter);
	if (not playingbest and rand_if(isAttemptEnd())) then -- load save state, continue with next attempt (disabled when playing back best)
		-- record this attempt
		if (not segments[segment].prev) then segments[segment].prev = {}; end;
		segments[segment].prev.frames = frame;
		segments[segment].prev.attempt = attempt;
		segments[segment].prev.score = getScore();
		segments[segment].prev.tie1 = getTie1();
		segments[segment].prev.tie2 = getTie2();
		segments[segment].prev.tie3 = getTie3();
		segments[segment].prev.tie4 = getTie4();
		segments[segment].prev.ok = rand_if(isAttemptOk()); -- this is the check whether this attempt was valid or not. if not, it cannot become the best attempt.
		-- update ok/failed attempt counters
		if (segments[segment].prev.ok) then 
			okattempts = okattempts + 1;
			onAttemptEnd(true);
		else 
			failattempts = failattempts + 1;
			onAttemptEnd(false);
		end;


		-- if this attempt was better then the previous one, replace it
		-- its a long IF, but all it checks (lazy eval) is whether the current
		-- score is better then the previous one, or if its equal and the tie1
		-- is better then the previous tie1 or if the tie1 is equal to the prev
		-- etc... for all four ties. Only tie4 actually needs to be better, tie1
		-- through tie3 can be equal as well, as long as the next tie breaks the
		-- same tie of the previous attempt :)
		if (segments[segment].prev.ok and (not segments[segment].best or (getScore() > segments[segment].best.score or (getScore() == segments[segment].best.score and (getTie1() > segments[segment].best.tie1 or (getTie1() == segments[segment].best.tie1 and (getTie1() > segments[segment].best.tie1 or (getTie1() == segments[segment].best.tie1 and (getTie1() > segments[segment].best.tie1 or (getTie1() == segments[segment].best.tie1 and getTie1() > segments[segment].best.tie1)))))))))) then
			gui.text(100,150,"Kept");
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
		
		if (rand_if(isSegmentEnd())) then -- special case, replay the best attempt and continue from there onwards...
			onSegmentEnd();
			if (rand_if(mustRollBack())) then
				gui.text(50,50,"Rolling back to segment "..(segment-1));
				segments[segment] = nil; -- remove current segment data
				attempt = 0; -- will be incremented in a few lines to be 1
				segment = segment - 1;
				segments[segment].best = nil;
				segments[segment].prev = nil;
				collectgarbage();
			else
				playingbest = true;
			end;
		end;

		-- reset vars
		attempt = attempt + 1;
		frame = 0;
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

		if (rand_if(isRunEnd())) then break; end; -- if end of run, break out of main loop
		
		if (not playingbest) then onAttemptStart(); end;
	end; -- continues with (new) attempt

	-- increase framecounter after processing attempt-end
	frame = frame + 1; -- inrease the frame count (++frame?)

	if (playingbest and segments[segment].best) then -- press keys from memory (if there are any)
		gui.text(10,150,"frame "..frame.." of "..segments[segment].best.frames);
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
			onSegmentStart();
			onAttemptStart();
		else
			key1 = segments[segment].best.keys1[frame];
			key2 = segments[segment].best.keys2[frame];
			gui.text(10,10,"Playback best of segment "..segment.."\nFrame: "..frame);
		end;
	end;

	if (rand_if(isRunEnd())) then break; end; -- if end of run, break out of main loop
  -- note this is the middle, this is where a attempt or segment has ended if it would and started if it would!
	FCEU.pause();

	
	if (not playingbest) then
	 -- press keys from bot
		gui.text(10,10,"Attempt: "..attempt.." / "..maxattempts.."\nFrame: "..frame.." / "..maxframes);
		if (segments[segment] and segments[segment].best and segments[segment].prev) then 
			gui.text(10,30,"Last score: "..segments[segment].prev.score.." ok="..okattempts..", not="..failattempts.."\nBest score: "..segments[segment].best.score);
		elseif (segments[segment] and segments[segment].prev) then
			gui.text(10,30,"Last score: "..segments[segment].prev.score.."\nBest score: none"); 
		end;
		gui.text(10,50,"Segment: "..segment);
		
		key1 = {};
		key2 = {};

		onInputStart();

		-- player 1
		if (rand_if(pressKeyUp1()) )    then key1.up = 1; end;
		if (rand_if(pressKeyDown1()))   then key1.down = 1; end;
		if (rand_if(pressKeyLeft1()))   then key1.left = 1; end;
		if (rand_if(pressKeyRight1()))  then key1.right = 1; end;
		if (rand_if(pressKeyA1()))      then key1.A = 1; end;
		if (rand_if(pressKeyB1()))      then key1.B = 1; end;
		if (rand_if(pressKeySelect1())) then key1.select = 1; end;
		if (rand_if(pressKeyStart1()))  then key1.start = 1; end;

		-- player 2
		if (rand_if(pressKeyUp2()) )    then key2.up = 1; end;
		if (rand_if(pressKeyDown2()))   then key2.down = 1; end;
		if (rand_if(pressKeyLeft2()))   then key2.left = 1; end;
		if (rand_if(pressKeyRight2()))  then key2.right = 1; end;
		if (rand_if(pressKeyA2()))      then key2.A = 1; end;
		if (rand_if(pressKeyB2()))      then key2.B = 1; end;
		if (rand_if(pressKeySelect2())) then key2.select = 1; end;
		if (rand_if(pressKeyStart2()))  then key2.start = 1; end;

		gui.text(50,70,"key1: "..realcount(key1)..", key2: "..realcount(key2).."\nlast1: "..realcount(lastkey1)..", last2: "..realcount(lastkey2));

		onInputEnd();

		lastkey1 = key1;
		lastkey2 = key2;
	
		keyrecording1[frame] = key1; -- record these keys (offsets at 1!)
		keyrecording2[frame] = key2; -- record these keys (offsets at 1!)
	end;
	joypad.set(1, key1);
	joypad.set(1, key2);
	-- next frame
	FCEU.frameadvance();
	FCEU.pause();
end;

onBotLoopEnd(); -- allow user cleanup before starting the final botloop

collectgarbage();

while (true) do 
	if (segments[segment].best) then gui.text(30,100,"end: max attempt ["..segment.."] had score = "..segments[segment].best.score);
	elseif (segment > 1 and segments[segment-1].best) then gui.text(30,100,"end: no best attempt ["..segment.."]\nPrevious best score: "..segments[segment-1].best.score);
	else gui.text(30,100,"end: no best attempt ["..segment.."] ..."); end;
	FCEU.frameadvance();
end;

