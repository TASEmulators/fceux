-- Multi-track rerecording for FCEUX 2.1.2 or later (script V2)
-- To make up for the lack of an internal multi-track rerecording in FCEUX.
-- It may get obsolete when TASEdit comes around.
--FatRatKnight
-- Rewind (by Antony Lavelle) added by DarkKobold

--   Instructions:
-- The script has its own buttons, defined in the junk below.
-- If you're not sure what they do, experiment by pressing the button!
-- If you're still not sure, I'll give you some basic idea here.

-- With the script buttons, defined by key down there, you can
-- toggle the input on or off for the next frame. Just push
-- the key button, and you'll see a light change. I don't
-- recommend HOLDing down the key button, especially if they
-- overlap with the actual controls you've set up on your
-- emulator. Especially there, since that's where problems begin.

-- I also have options to determine whether the input from
-- the emulator should apply or whether the script should
-- bother. Hold space (default keys), and you can switch
-- individual buttons on or off. The script will decide
-- whether to ignore the script keys here or the input stored
-- away in its list, with the options that pop up.

-- There's a player switch as well. Just hit the S key
-- (default keys) and it'll change to player 2. Or 3, or 4.
-- Or all players at once. This will make the script keys
-- start working on other players instead of just 1.

-- Then there's the joypad input. home or end (default keys)
-- grants varying levels of control to the emulator joypad.
-- If you want to get rid of all control from the joypad setup
-- of the emulator, you can do that. The defaults are to allow
-- the controllers to toggle whatever input the script has.
-- I did not implement a way to give this decision to
-- individual joypad keys.

-- That's the basics of controlling input with this script.
-- The script buttons let you decide input without a frame
-- advance, and the joypad keys are there if you're so used
-- to holding things down while pressing frame advance.


-- A few more advanced things are in here. I will briefly mention them:
  -- Rewind:  Back up a frame or two with a press of a button!
    -- Seems to hog up CPU greatly. Disable if you want things run smoothly.
  -- Insert/Delete: Found a time saver, yet want to keep later input?
    -- This lets you slide a chunk of frames forward or back.
    -- I display a number that says how many frames are buffered.
  -- Immediate: Good if you want to change input while rewinding.
    -- Okay, its use is quite rare, but it's there.

-- Hopefully, this is enough to let you get started. Good luck!



-- Stuff that you are encouraged to change to fit what you need.
--*****************************************************************************
-- Control scheme. Just experiment a little with the buttons.

local selectplayer = "S"     -- For selecting which player
local recordingtype= "space" -- For selecting how to record
local SetImmediate = "numpad5"--Toggle immediate option
local TrueSwitch   = "home"  -- Toggle whether joypad can turn off input
local FalseSwitch  = "end"   -- Toggle whether joypad can turn on input

local show, hide   = "pageup", "pagedown"  -- Opacity adjuster
local scrlup,   scrldown = "numpad8", "numpad2"  -- Move the input display
local scrlleft, scrlright= "numpad4", "numpad6"  -- Not a fast method, though.
local MorePast,   LessPast  = "numpad7", "numpad1"  -- See more or less input!
local MoreFuture, LessFuture= "numpad3", "numpad9"  -- Good use of screen space

local add, remove  = "insert", "delete"   -- Moves input around
local rewind = "R"    -- The button used to rewind things

local key = {"right", "left", "down", "up",   "L",      "O",   "J", "K"}
local btn = {"right", "left", "down", "up", "start", "select", "B", "A"}

-- key links to the keyboard keys, so change those to fit your control needs.
-- btn links to joypad buttons. Don't change what's in there.
-- However, feel free to reorder the strings in btn! It will change the display!


--*****************************************************************************
-- Values to change. players, saveMax, and MsgDuration aren't changed in the
-- code. All others are simply default values which can dynamically change.

local players= 2             -- You may tweak the number of players here.

local saveMax= 1000          -- Max Rewind Power. Set to 0 to disable

local opaque= 0              -- Default opacity. 0 is solid, 4 is invisible.

local dispX, dispY= 10, 99   -- Default input display position.

local Past, Future= -10, 10  -- Keep Past negative; Range of input display

local immediate= false       -- true:  Changes apply to list upon happening
                             -- false: Changes only apply on frame advance
local JoyOn = "inv"        -- true   OR  "inv"   If true, disable turn-off
local JoyOff= nil          -- false  OR  nil     If false, disable turn-on

local MsgDuration= 60        -- How long should messages stay?


--*****************************************************************************

-- Stuff that really shouldn't be messed with, unless you plan to change the code significantly.
local optType= {"Both", "Keys", "List", "Off"} -- Names for option types
local keys, lastkeys= {}, {} -- To hold keyboard input
local Pin, thisInput= {}, {} -- Input manipulation array
local BufInput, BufLen= {},{}-- In case you want to insert or remove frames.
local option= {}             -- Options for individual button input by script
local fc, lastfc= 0, 0       -- Frame counters
local FrameAdvance= false    -- Frame advance detection
local pl= 1                  -- Player selected
local plmin, plmax= 1, 1     -- For the all option
local repeater= 0            -- for holding a button
local rewinding= false       -- For unpaused rewinding
local MsgTmr= 0              -- To time how long messages stay
local Message= "Activated Multitrack recording script. Have fun!"


--The stuff below is adapted from the original Rewinding script by Antony Lavelle

local saveArray = {};--the Array in which the save states are stored
local saveCount = 0;--used for finding which array position to cycle through
local saver; -- the variable used for storing the save state
local rewindCount = 0;--this stops you looping back around the array if theres nothing at the end
local savePreventBuffer = 1;--Used for more control over when save states will be saved, not really used in this version much.


-- Initialize tables for each player.
for i= 1, players do
    Pin[i]        = {}
    thisInput[i]  = {}
    option[i]     = {}
    BufInput[i]   = {}
    BufLen[i]     = 0
    for j= 1, 8 do
        option[i][btn[j]]= 1
    end
end

-- Intention of options:
-- "Both"  Script buttons and input list used
-- "Keys"  Script buttons used
-- "List"  Input list used
-- "Off"   Script will not interfere with button


--*****************************************************************************
function press(button)
--*****************************************************************************
--FatRatKnight
-- Checks if a button is pressed.
-- The tables it accesses should be obvious in the next line.

    if keys[button] and not lastkeys[button] then
        return true
    end

    return false
end


--*****************************************************************************
function pressrepeater(button)
--*****************************************************************************
--DarkKobold; Changes by FatRatKnight. Description by FatRatKnight
-- Checks if a button is pressed. Will keep returning true if held long enough.
-- Accesses: keys[], lastkeys[], repeater

    if keys[button] then
       if not lastkeys[button] or repeater >= 3 then
           return true
       else
           repeater = repeater + 1;
       end

    elseif lastkeys[button] then   -- To allow more calls for other buttons
       repeater= 0
    end;
    return false
end


--*****************************************************************************
function NewMsg(text)
--*****************************************************************************
--FatRatKnight
-- Sets the message and resets the timer
-- Accesses: MsgTmr, Message

    MsgTmr= 0
    Message= text
end


--*****************************************************************************
function DispMsg()
--*****************************************************************************
--FatRatKnight
-- Merely displays whatever text happens to be in Message
-- Accesses: MsgTmr, Message, MsgDuration

    if MsgTmr < MsgDuration then
        MsgTmr= MsgTmr + 1
        gui.text(0,20,Message)
    end
end


--*****************************************************************************
function RewindThis()
--*****************************************************************************
--Added by DarkKobold; Made into function that returns T/F by FatRatKnight
-- Loads a state that was saved just the frame prior
-- Lets you know whether it was successful by returning true or false.
-- Accesses: rewindCount, saveArray, saveCount, saveMax

    if rewindCount<=0 then
        return false    -- Failed to rewind
    else
        savestate.load(saveArray[saveCount]);
        saveCount = saveCount-1;
        rewindCount = rewindCount-1;
        if saveCount<=0 then
            saveCount = saveMax;
        end;
    end;
    return true         -- Yay, rewind!
end


--*****************************************************************************
function GetNextInput(P)
--*****************************************************************************
--FatRatKnight
-- Gets the input from the input table.
-- Accesses: BufLen[], BufInput[], fc, Pin[]

    if BufLen[P] > 0 then
        return BufInput[P][BufLen[P]]
    end
    return Pin[P][fc-BufLen[P]]
end


--*****************************************************************************
function LoadStoredInput(P)
--*****************************************************************************
--FatRatKnight
-- Loads up input into thisInput.
-- Found a good reason why it should be called in several spots.
-- Accesses: option[], thisInput[]

    local next= GetNextInput(P)
    if next then
        for i= 1, 8 do
            local op= option[P][btn[i]]
            if op == 1  or  op == 3 then  -- "Both" or "List"
                thisInput[P][btn[i]]= next[btn[i]]
            else
                thisInput[P][btn[i]]= nil
            end
        end
    else -- No input to load, so erase stuff.
        for i= 1, 8 do
            thisInput[P][btn[i]]= nil
        end
    end
end


--*****************************************************************************
function GetList(P,Offset)
--*****************************************************************************
--FatRatKnight
-- Fetches the input from the list relative to current framecount.
-- It takes into account insertions and deletions.
-- Accesses: thisInput[], Pin[], BufLen[], BufInput[]

    if     Offset == 0 then
        return thisInput[P]
    elseif Offset <  0 then
        return Pin[P][fc+Offset]
    end

    if BufLen[P] > 0  and  BufLen[P] > Offset then
        return BufInput[P][BufLen[P]-Offset]
    end
    return Pin[P][fc+Offset-BufLen[P]]
end


--*****************************************************************************
function InputSnap(P)
--*****************************************************************************
--FatRatKnight
-- Will shove the input list over.
-- Might end up freezing for a moment if there's a few thousand frames to shift
-- Accesses: framed, BufLen, BufInput[], Pin[], players


    if BufLen[P] < 0 then  -- Squish!
        local pointer= lastfc
        while Pin[P][pointer] do
            Pin[P][pointer]= Pin[P][pointer-BufLen[P]]
            pointer= pointer+1
        end
    end


    if BufLen[P] > 0 then -- Foom!
        local pointer= lastfc
        while Pin[P][pointer] do
            pointer= pointer+1
        end
        -- pointer is now looking at a null frame.
        -- Assume later frames are also null.

        while pointer > lastfc do
            pointer= pointer-1
            Pin[P][pointer+BufLen[P]]= Pin[P][pointer]
        end
        -- pointer should now match lastfc.
        -- Everything at lastfc and beyond should be moved over by BufLen.

        for i=0, BufLen[P]-1 do
            Pin[P][lastfc +i]= BufInput[P][BufLen[P]-i]
        end
    end

    BufLen[P]= 0 -- If it ain't zero before, we did stuff to make us want it zero.
end


--*****************************************************************************
function NewFrame()
--*****************************************************************************
--FatRatKnight
-- Psuedo-detection of frame-advance or state load.
-- The lack of a function to otherwise detect state loads calls for this function
-- Accesses: fc, lastfc, FrameAdvance

    fc= movie.framecount()

    for P= 1, players do
        InputSnap(P)
        LoadStoredInput(P)
    end
end

savestate.registerload(NewFrame)


--*****************************************************************************
local Draw= {}   --Draw[button]( Left , Top , color )
--*****************************************************************************
--FatRatKnight
-- A set of functions for which to draw unique objects for each button.
-- The functions are in a table, so I can index them easy.
-- For use with DisplayInput()

function Draw.right(x,y,color)       --    ##
    gui.line(x  ,y  ,x+1,y  ,color)  --      #
    gui.line(x  ,y+2,x+1,y+2,color)  --    ##
    gui.pixel(x+2,y+1,color)
end

function Draw.left(x,y,color)        --     ##
    gui.line(x+1,y  ,x+2,y  ,color)  --    #
    gui.line(x+1,y+2,x+2,y+2,color)  --     ##
    gui.pixel(x  ,y+1,color)
end

function Draw.up(x,y,color)          --     #
    gui.line(x  ,y+1,x  ,y+2,color)  --    # #
    gui.line(x+2,y+1,x+2,y+2,color)  --    # #
    gui.pixel(x+1,y  ,color)
end

function Draw.down(x,y,color)        --    # #
    gui.line(x  ,y  ,x  ,y+1,color)  --    # #
    gui.line(x+2,y  ,x+2,y+1,color)  --     #
    gui.pixel(x+1,y+2,color)
end

function Draw.start(x,y,color)       --     #
    gui.line(x+1,y  ,x+1,y+2,color)  --    ###
    gui.line(x  ,y+1,x+2,y+1,color)  --     #
end

function Draw.select(x,y,color)      --    ###
    gui.box(x  ,y  ,x+2,y+2,color)   --    # #
end                                  --    ###

function Draw.A(x,y,color)           --    ###
    gui.box(x  ,y  ,x+2,y+1,color)   --    ###
    gui.pixel(x  ,y+2,color)         --    # #
    gui.pixel(x+2,y+2,color)
end

function Draw.B(x,y,color)           --    # #
    gui.line(x  ,y  ,x  ,y+2,color)  --    ##
    gui.line(x+1,y+1,x+2,y+2,color)  --    # #
    gui.pixel(x+2,y  ,color)
end


--*****************************************************************************
function DisplayInput()
--*****************************************************************************
--FatRatKnight; Changes by DarkKobold.
-- Shows the input stream that is stored in this script.
-- Converted into function form. There might be other ways of handling it.
-- Accesses: dispX, dispY, Pin[], thisInput[], players

dispX= math.min(math.max(dispX,-2),219)
if pl > players then -- All players, man?
    dispX= math.min(dispX,243-16*players)
end

if (Future-Past) > 53 then
    Past= math.max(Past, -53)
    Future= Past + 53
end
Past  = math.min(Past  ,0)
Future= math.max(Future,0)
dispY = math.min(math.max(dispY, 3-4*Past),217-4*Future)

    for i= Past, Future do

        local X,Y= dispX,dispY
        if i < 0 then
            Y= Y-3
        elseif i > 0 then
            Y= Y+3
        end

        for P= plmin, plmax do
            local temp= {} -- I may want to pick thisInput instead of the frame list.
            temp= GetList(P,i)

            local color
            for j= 1, 8 do
                if not temp then
                    color= "white"
                elseif temp[btn[j]] then
                    color= "green"
                else
                    color= "red"
                end

                if pl <= players then
                    Draw[btn[j]]( X +4*j , Y +4*i , color)
                else  -- all players
                    local bx= X+1 +j +2*players*(j-1) +2*P
                    gui.box(bx , Y+4*i , bx+1 , Y+4*i +2  ,color)
                end
            end
        end
    end

    local color= "white" -- immediate == true
    if not immediate then
        color= "blue"
        for P= plmin, plmax do
            local TI= GetNextInput(P)
            for i=1, 8 do
                if not TI or (TI[btn[i]] and not thisInput[P][btn[i]]) or (not TI[btn[i]] and thisInput[P][btn[i]])then
                    color= "green"
                end
            end
        end
    end
    if pl <= players then
        gui.box(dispX+2,dispY-2,dispX+36,dispY+4,color)
    else
        gui.box(dispX+2,dispY-2,dispX+12 +16*players,dispY+4,color)
    end
end


-- Primary function. ... Why did I pick itisyourturn, anyway?
--*****************************************************************************
function itisyourturn()
--*****************************************************************************
    keys= input.get()       -- We need the keyboard and mouse!
    fc= movie.framecount()  -- We also need the Frame Count.

-- State: Rewind
 if saveMax > 0 then  -- Disable Rewind if none exists
    if pressrepeater(rewind) or rewinding then
        rewinding= false
        if RewindThis() then
            NewFrame()
            movie.rerecordcounting(true)
        end
        if rewindCount <= 0 then
            emu.pause()
        end
    end
 end

    if (fc == 0) and (lastfc ~= 0) then  -- Hacky bit of script...
        NewFrame()                       -- In case of resets that aren't
    end                                  -- considered stateloads.

-- Selection: Players
    if press(selectplayer) then
        pl= pl + 1
        if (pl > players+1)  or  (players == 1) then
            pl= 1
        end

-- For standard player loops for allplayer option
        if pl > players then
            plmin= 1
            plmax= players
        else
            plmin= pl
            plmax= pl
        end
    end


-- Input: Insert or delete frames
    if press(add) then                 -- Part of the reason for
        for P= plmin, plmax do         -- speedy insertions and
            BufLen[P]= BufLen[P]+1     -- deletions is due to the
            if BufLen[P] > 0 then      -- fact that I don't shift
                BufInput[P][BufLen[P]]= {} -- frames immediately.
            end
            LoadStoredInput(P)
        end
    end                                -- I only shift once you
    if press(remove) then              -- begin rewind or load a
        for P= plmin, plmax do         -- state. At that point,
           BufLen[P]= BufLen[P]-1      -- it may take a while if
           LoadStoredInput(P)          -- there's enough frames
        end                            -- to shift around.
    end

-- Input: Should thisInput always instantly write to input table?
    if press(SetImmediate) then
        if not immediate then
            immediate= true
        else
            immediate= false
        end
    end

-- Input: Allow joypad to toggle input?
    if press(TrueSwitch) then
        if JoyOn == true then
            JoyOn= "inv"
            NewMsg("Joypad may now cancel input from the list")
        else
            JoyOn= true
            NewMsg("Disabled joypad input cancellation")
        end
    end
    if press(FalseSwitch) then
        if JoyOff == false then
            JoyOff= nil
            NewMsg("Joypad may now write input into the list")
        else
            JoyOff= false
            NewMsg("Disabled joypad input activation")
        end
    end


-- Option: Opacity
    if press(hide) and opaque < 4 then
        opaque= opaque+1
    end
    if press(show) and opaque > 0 then
        opaque= opaque-1
    end
    gui.transparency(opaque)

-- Option: Move input display by keyboard
    if pressrepeater(scrlup) then
        dispY= dispY - 1
    end
    if pressrepeater(scrldown) then
        dispY= dispY + 1
    end
    if pressrepeater(scrlleft) then
        dispX= dispX - 1
    end
    if pressrepeater(scrlright) then
        dispX= dispX + 1
    end

-- Option: Move input display by mouse
    if keys["leftclick"] and lastkeys["leftclick"] then
        dispX= dispX + keys.xmouse - lastkeys.xmouse
        dispY= dispY + keys.ymouse - lastkeys.ymouse
    end

-- Option: Adjust input display range
    if pressrepeater(MorePast) then
        Past= Past-1                  -- Remember, Past is negative.
    end                               -- Making it more negative would
    if pressrepeater(LessPast) then   -- let you see more of the past.
        Past= Past+1
    end
    if pressrepeater(MoreFuture) then -- On the other hand, I want to
        Future= Future+1              -- make the Future positive.
    end                               -- Don't we all want to be
    if pressrepeater(LessFuture) then -- positive about the future?
        Future= Future-1
    end


-- Begin start of frame change
    if FrameAdvance then
        for P= 1, players do
            if not Pin[P][lastfc] then
                InputSnap(P)     -- If last frame was empty, kill the shift
            end                  -- Don't track more than one buffer for loads

-- Scroll the BufInput if there's anything in there.
-- This can potentially slow things if you insert enough frames.
            if BufLen[P] > 0 then
                for i= BufLen[P], 1, -1 do
                    BufInput[P][i]= BufInput[P][i-1]
                end
                BufInput[P][1]= Pin[P][lastfc]
            end

-- Store input
            Pin[P][lastfc]= joypad.get(P)

            LoadStoredInput(P)
        end -- Loop for each player

-- Resets key input, so it counts as pressed again if held through frame advance.
        for i= 1, 8 do
            lastkeys[key[i]]= nil
        end

    end
-- Done checking for a new frame


-- Begin checking key input
-- Are you setting button options?
    if keys[recordingtype] then
        gui.transparency(0)  -- Override current opacity
        for i= 1, 8 do -- key loop

-- Set current options
            for P= plmin, plmax do
                if press(key[i]) then
                    option[P][btn[i]]= option[plmax][btn[i]]+1
                    if option[P][btn[i]] > 4 then
                        option[P][btn[i]]= 1
                    end
                end
            end

-- Display current options
            gui.text(20,20+12*i,btn[i])

            if pl >= players + 1 then
                local q= optType[option[1][btn[i]]]
                for z= 2, players do
                    if q ~= optType[option[z][btn[i]]] then
                        q= "???"
                        break
                    end
                end
                gui.text(50,20+12*i,q)
            else
                gui.text(50,20+12*i,optType[option[pl][btn[i]]])
            end

        end -- key loop

-- Are you setting actual input?
    else
        for i= 1, 8 do

-- Toggle keyed input depending on options
            for P= plmin, plmax do
                op= option[P][btn[i]]
                if op == 1  or  op == 2 then   -- "Both" or "Keys"
                    if press(key[i]) then
                        if thisInput[P][btn[i]] then
                            thisInput[P][btn[i]]= nil
                        else
                            thisInput[P][btn[i]]= true
                        end
                    end
                end
            end

        end -- key loop

-- Done with input.
-- The next stuff is just displaying input table.
-- It's here to hide it when setting options.
        if opaque < 4 then
            DisplayInput()
        end
    end -- Done with "options" if

-- Force copy of thisInput into the input table if immediate is on
    if immediate then
        for P= 1, players do
            if not GetNextInput(P) then
                Pin[P][fc-BufLen[P]]= {}
            end -- Inserted frames always defined! Don't check for that

            if BufLen[P] > 0 then
                for i= 1, 8 do
                    BufInput[P][BufLen[P]][btn[i]]= thisInput[P][btn[i]]
                end
            else
                for i= 1, 8 do
                    Pin[P][fc-BufLen[P]][btn[i]]= thisInput[P][btn[i]]
                end
            end
        end
    end

-- Display selected player. Or other odds and ends.
    if pl <= players then
       gui.text(30,10,pl)
       gui.text(45,10,BufLen[pl])
    else
       gui.text(30,10,"A")
    end;

    DispMsg() -- Also display whatever message is in there.

-- Change thisInput to conform to options
    for P= 1, players do
        for i= 1, 8 do
            if thisInput[P][btn[i]] then
                thisInput[P][btn[i]]= JoyOn  -- Use the right kind of true
            else
                thisInput[P][btn[i]]= JoyOff -- And the right kind of false
            end
        end
    end

-- Feed the input to the emulation.
    if movie.mode() ~= "playback" then
        for P= 1, players do
            joypad.set(P, thisInput[P])
        end
    end

    lastfc= fc        -- Standard "this happened last time" stuff
    lastkeys= keys    -- Don't want to keep registering key hits.
    FrameAdvance= false

end  -- Yes, finally! The end of itisyourturn!
--*****************************************************************************


gui.register(itisyourturn) -- Need to call while in between frames

emu.pause();               -- Immediate pause is nice

--*****************************************************************************
while true do  -- Main loop
--*****************************************************************************
-- Most of the stuff here by DarkKobold. Minor stuff by FatRatKnight
-- Rewinding feature originally by Antony Lavelle.
-- Keep in mind stuff here only happens on a frame advance or when unpaused.

    FrameAdvance= true
    emu.frameadvance()

 if saveMax > 0 then  -- Don't process if Rewind is disabled
    if keys[rewind] and rewindCount > 0 then
        rewinding= true
    else
        saveCount=saveCount+1;
        rewindCount = math.min(rewindCount + 1,saveMax);
        if saveCount==saveMax+1 then
             saveCount = 1;
        end

        if saveArray[saveCount] == nil then
            saver = savestate.create();
        else
            saver = saveArray[saveCount];
        end;
        savestate.save(saver);
        saveArray[saveCount] = saver; -- I'm pretty sure this line is unnecessary.
        movie.rerecordcounting(false)
    end
 end

end -- Main loop ends