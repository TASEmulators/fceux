-- Multitrack v2 for FCEUX by FatRatKnight
-- Recent addition: Experimental save. Same format as in .fm2
-- Did not include support to not have it trigger on script exit. Sorry...
-- Additionally, there's no *loading* mechanism in this script. Eh...


--***************
local players= 2           -- You may tweak the number of players here.
--***************


-- Rewind options
local saveMax= 50          -- How many save states to keep? Set to 0 to disable
local SaveBuf= 12          -- How many frames between saves? Don't use 0, ever.
local rewind= "numpad0"    -- What key do you wish to hit for rewind?

--Keep in mind: Higher saveMax, more savestates, more memory needed.


--Control
local PlayerSwitch= "home" -- For selecting other players
local opt= "end"           -- For changing control options

local BackupReadSwitch=  "numpad-" -- Revert to backup copy
local BackupWriteSwitch= "numpad+" -- Copy input into backup

local key = {"right", "left", "down", "up",   "L",      "O",   "J", "K"}
local btn = {"right", "left", "down", "up", "start", "select", "B", "A"}

--Try to avoid changing btn. You may reorder btn's stuff if you don't
--like the default order, however. Line up with key for good reasons.
--key is the actual keyboard control over the buttons. Change that!


--Insertions and deletions
local Insert= "insert"     -- Inserts a frame at the frame you're at
local Delete= "delete"     -- Eliminates current frame


--Display
local BackupDisplaySwitch= "numpad." -- Care to see the backup?

local solid= "pageup"      -- Make the display less
local clear= "pagedown"    -- or more transparant.

local DispN= "numpad8"
local DispS= "numpad2"     -- For moving the
local DispE= "numpad6"     -- display around.
local DispW= "numpad4"

local MoreFutr= "numpad3"
local LessFutr= "numpad1"  -- These will determine
local MorePast= "numpad7"  -- how many frames you
local LessPast= "numpad9"  -- want to display.
local ResetFP=  "numpad5"


--Control options by keyboard
local OptUp= "numpad8"
local OptDn= "numpad2"     -- Controls for changing
local OptRt= "numpad6"     -- options by keyboard.
local OptLf= "numpad4"
local OptHit="numpad5"



--Various colors I'm using. If you wish to bother, go ahead.
local shade= 0x00000080
local white= "#FFFFFFFF"
local red=   "#FF2000FF"
local green= 0x00FF00FF
local blue=  0x0040FFFF
local orange="#FFC000FF"
local yellow="#FFFF00FF"
local cyan=  0x00FFFFFF
local purple="#8020FFFF"

--Default display stuff. Try to keep Past negative.
local DispX, DispY= 190, 70     --Position on screen to appear
local Past, Future= -12, 20     --How much input to see
local Opaque= 1                 --0=unseen   1=visible   Decimal in between works


local ListOfBtnClr= {  -- Colors for individual buttons
--NotExist Off      Ignored  On

  white,   red,     orange,  green,    -- No backup to display
  white,   red,     orange,  green,    -- Backup button is off
  blue,    blue,    purple,  cyan,     -- Backup button is on

  white,   red,     orange,  green,    -- Same stuff as above.
  white,   red,     orange,  green,    -- However, we're reading
  blue,    blue,    purple,  cyan,     -- from backup here.

  white,   red,     orange,  green,    -- Likewise, but instead
  white,   red,     orange,  green,    -- of read, we're writing
  blue,    blue,    purple,  cyan      -- to the backup. Joy...
}

local ListOfChangesClr= {  -- Colors for backup-main comparisons
--NoMain   NoMatch  Equal

  white,   green,   blue,              -- Backup just sits there...
  white,   orange,  green,             -- Reading from backup...
  white,   yellow,  purple             -- Writing to backup...
}



local BackupFileLimit= 9


--*****************************************************************************
--Please do not change the following, unless you plan to change the code:

local plmin , plmax= 1 , 1
local fc= movie.framecount()
local LastLoad= movie.framecount()
local saveCount= 0
local saveArray= {}
local rewinding= false

local BackupList= {}
local BackupOverride= {}
local DisplayBackup= true

local InputList= {}
local ThisInput= {}
local BufInput= {}
local BufLen= {}
local TrueSwitch, FalseSwitch= {}, {}
local ReadList= {}
local ScriptEdit= {}

for pl= 1, players do
    InputList[pl]= {}
    BackupList[pl]= {}
    BackupOverride[pl]= false
    ThisInput[pl]= {}
    BufInput[pl]= {}
    BufLen[pl]= 0

    TrueSwitch[pl]= {}
    FalseSwitch[pl]= {}
    ReadList[pl]= {}
    ScriptEdit[pl]= {}
    for i= 1, 8 do
        TrueSwitch[pl][i]= "inv"
        FalseSwitch[pl][i]= nil
        ReadList[pl][i]= true
        ScriptEdit[pl][i]= false
    end
end

--*****************************************************************************
-- Just painting instructions here.

print("Running Multitrack Script made by FatRatKnight.",
  "\r\nIts primary use is to preserve input after a loadstate.",
  "\r\nIt also gives precise control on changing this stored input.",
  "\r\nEdit the script if you need to change stuff. All options are near the top.",
  "\r\nThe script is currently set as follows:\r\n")

print("Players:",players)
if players > 1 then
    print("Player Switch key:",PlayerSwitch,
    "\r\nThere is an All Players view as well, beyond the last player.\r\n")
else
    print("With only one player, the PlayerSwitch button is disabled.\r\n")
end

print("Maximum rewind states:",saveMax)
local CalcSeconds= math.floor(saveMax*SaveBuf*100/60)/100
if saveMax > 0 then
    print("Frames between saves:",SaveBuf,
      "\r\nRewind key:",rewind,
      "\r\nThis can go backwards up to about",CalcSeconds,"seconds.",
      "\r\nRewind is a quick and easy way to back up just a few frames.",
      "\r\nJust hit",rewind,"and back you go!\r\n")
else
    print("Rewind function is disabled. Cheap on memory!\r\n")
end

print("Hold >",opt,"< to view control options.",
  "\r\nYou may use the mouse or keyboard to toggle options.",
  "\r\nFor keyboard, use",OptUp,OptDn,OptRt,OptLf,"to choose an option and",
  OptHit,"to toggle the option.",
  "\r\nFor mouse, it's as simple at point and click.",
  "\r\nIf viewing All Players, changing options will affect all players.\r\n")

print("For the frame display, you can drag it around with the mouse.",
  "\r\nTo move it by keyboard:",DispN,DispS,DispE,DispW,
  "\r\nTo change frames to display:",MoreFutr,LessFutr,MorePast,LessPast,
  "\r\nTo recenter display around current frame:",ResetFP,
  "\r\nLastly, hide it with >",clear,"< and show it with >",solid,"<\r\n")

print("Insert frame:",Insert,
  "\r\nDelete frame:",Delete,
  "\r\nThese keys allow you to shift frames around the current frame.",
      "Perhaps you found an improvement to a segment and want to delete frames.",
      "Or this small trick you wanted to add requires shifting a few frames forward.",
      "These buttons let you shift frames around without forcing you to",
      "modify each and every one of them individually.\r\n")


print("Script keys:",
  "\r\nJoy - Keyboard",
  "\r\n------------")
for i= 1, 8 do
    print(btn[i],"-",key[i])
end
print("By default, these are disabled. Enable them through the control options.",
  "\r\nThey are a one-tap joypad switch for precision control.",
  "\r\nIt will affect the currently displayed player, or all of them if",
      "it's displaying All Players.\r\n")

print("If you want to \"load\" an fm2 into this script, run the play file",
      "while running this script. As the movie on read-only plays out, the",
      "script will pick up its input. The script does not need to start from",
      "frame 1 -- Load a state near the end of a movie if you so wish!",
      "Once input is loaded, you may continue whatever you were doing.\r\n")

print("Remember, edit the script if you don't like the current options. All",
      "the options you should care about are near the top, clearly marked.",
      "Change the control keys through there, as well as number of players or rewind limits.",
  "\r\nThis script works with the joypad you actually set from FCEUX's config,",
      "and they can even cancel input stored in this script.",

"\r\n\r\nAnd have fun with this script. I hope your experiments with this script goes well.",
"\r\n\r\nLeeland Kirwan, the FatRatKnight.")

print("Players:",players,"  - -  ",CalcSeconds,"seconds of rewind.")



--*****************************************************************************
function FBoxOld(x1, y1, x2, y2, color)
--*****************************************************************************
-- Gets around FCEUX's problem of double-painting the corners.
-- The double-paint is visible with non-opaque drawing.
-- It acts like the old-style border-only box.
-- Slightly messes up when x2 or y2 are less than their counterparts.

    if     (x1 == x2) and (y1 == y2) then
        gui.pixel(x1,y1,color)

    elseif (x1 == x2) or  (y1 == y2) then
        gui.line(x1,y1,x2,y2,color)

    else --(x1 ~= x2) and (y1 ~= y2)
        gui.line(x1  ,y1  ,x2-1,y1  ,color) -- top
        gui.line(x2  ,y1  ,x2  ,y2-1,color) -- right
        gui.line(x1+1,y2  ,x2  ,y2  ,color) -- bottom
        gui.line(x1  ,y1+1,x1  ,y2  ,color) -- left
    end
end


--*****************************************************************************
function FakeBox(x1, y1, x2, y2, Fill, Border)
--*****************************************************************************
-- Gets around FCEUX's problem of double-painting the corners.
-- It acts like the new-style fill-and-border box.

if not Border then   Border= Fill   end

    gui.box(x1,y1,x2,y2,Fill,0)
    FBoxOld(x1,y1,x2,y2,Border)
end


--*****************************************************************************
local Draw= {}   --Draw[button]( Left , Top , color )
--*****************************************************************************

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
    gui.pixel(x  ,y+1,color)         --     #
    gui.pixel(x+2,y+1,color)
end

function Draw.select(x,y,color)      --    ###
    FBoxOld(x  ,y  ,x+2,y+2,color)   --    # #
end                                  --    ###

function Draw.A(x,y,color)           --    ###
    FBoxOld(x  ,y  ,x+2,y+1,color)   --    ###
    gui.pixel(x  ,y+2,color)         --    # #
    gui.pixel(x+2,y+2,color)
end

function Draw.B(x,y,color)           --    # #
    gui.line(x  ,y  ,x  ,y+2,color)  --    ##
    gui.line(x+1,y+1,x+2,y+2,color)  --    # #
    gui.pixel(x+2,y  ,color)
end



function Draw.D0(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+2,top  ,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+4,color)
end

function Draw.D1(left, top, color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.line(left+1,top  ,left+1,top+3,color)
    gui.pixel(left  ,top+1,color)
end

function Draw.D2(left, top, color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.line(left  ,top+3,left+2,top+1,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left  ,top+2,color)
    gui.pixel(left+2,top+2,color)
end

function Draw.D3(left, top, color)
    gui.line(left  ,top  ,left+1,top  ,color)
    gui.line(left  ,top+2,left+1,top+2,color)
    gui.line(left  ,top+4,left+1,top+4,color)
    gui.line(left+2,top  ,left+2,top+4,color)
end

function Draw.D4(left, top, color)
    gui.line(left  ,top  ,left  ,top+2,color)
    gui.line(left+2,top  ,left+2,top+4,color)
    gui.pixel(left+1,top+2,color)
end

function Draw.D5(left, top, color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.line(left  ,top+1,left+2,top+3,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left  ,top+2,color)
    gui.pixel(left+2,top+2,color)
end

function Draw.D6(left, top, color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.line(left  ,top+1,left  ,top+4,color)
    gui.line(left+2,top+2,left+2,top+4,color)
    gui.pixel(left+1,top+2,color)
    gui.pixel(left+1,top+4,color)
end

function Draw.D7(left, top, color)
    gui.line(left  ,top  ,left+1,top  ,color)
    gui.line(left+2,top  ,left+1,top+4,color)
end

function Draw.D8(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+2,top  ,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+2,color)
    gui.pixel(left+1,top+4,color)
end

function Draw.D9(left, top, color)
    gui.line(left  ,top  ,left  ,top+2,color)
    gui.line(left+2,top  ,left+2,top+3,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+2,color)
end


Draw[0],Draw[1],Draw[2],Draw[3],Draw[4]=Draw.D0,Draw.D1,Draw.D2,Draw.D3,Draw.D4
Draw[5],Draw[6],Draw[7],Draw[8],Draw[9]=Draw.D5,Draw.D6,Draw.D7,Draw.D8,Draw.D9
--*****************************************************************************
function DrawNum(right, top, Number, color, bkgnd)
--*****************************************************************************
-- Paints the input number as right-aligned.
-- Returns the x position where it would paint another digit.
-- It only works with integers. Rounds fractions toward zero.

    local Digit= {}
    local Negative= false
    if Number < 0 then
        Number= -Number
        Negative= true
    end

    Number= math.floor(Number)
    if not color then color= "white" end
    if not bkgnd then bkgnd= "clear" end

    local i= 0
    if Number < 1 then
        Digit[1]= 0
        i= 1
    end

    while (Number >= 1) do
        i= i+1
        Digit[i]= Number % 10
        Number= math.floor(Number/10)
    end

    if Negative then  i= i+1  end
    local left= right - i*4
    FakeBox(left+1, top-1, right+1, top+5,bkgnd,bkgnd)

    i= 1
    while Draw[Digit[i]] do
        Draw[Digit[i]](right-2,top,color)
        right= right-4
        i=i+1
    end

    if Negative then
        gui.line(right, top+2,right-2,top+2,color)
        right= right-4
    end
    return right
end


--*****************************************************************************
function limits( value , low , high )  -- Expects numbers
--*****************************************************************************
-- Returns value, low, or high. high is returned if value exceeds high,
-- and low is returned if value is beneath low.

    return math.max(math.min(value,high),low)
end

--*****************************************************************************
function within( value , low , high )  -- Expects numbers
--*****************************************************************************
-- Returns true if value is between low and high. False otherwise.

    return ( value >= low ) and ( value <= high )
end


local keys, lastkeys= {}, {}
--*****************************************************************************
function press(button)  -- Expects a key
--*****************************************************************************
-- Returns true or false.
-- Generally, if keys is pressed, the next loop around will set lastkeys,
-- and thus prevent returning true more than once if held.

    return (keys[button] and not lastkeys[button])
end

local repeater= 0
--*****************************************************************************
function pressrepeater(button)  -- Expects a key
--*****************************************************************************
--DarkKobold & FatRatKnight
-- Returns true or false.
-- Acts much like press if you don't hold the key down. Once held long enough,
-- it will repeatedly return true.
-- Try not to call this function more than once per main loop, since I've only
-- set one repeater variable. Inside a for loop is real bad.

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
function JoyToNum(Joys)  -- Expects a table containing joypad buttons
--*****************************************************************************
-- Returns a number from 0 to 255, representing button presses.
-- These numbers are the primary storage for this version of this script.

    local joynum= 0

    for i= 1, 8 do
        if Joys[btn[i]] then
            joynum= bit.bor(joynum, bit.rshift(0x100,i))
        end
    end

    return joynum
end

--*****************************************************************************
function ReadJoynum(input, button)  -- Expects... Certain numbers!
--*****************************************************************************
-- Returns true or false. True if the indicated button is pressed
-- according to the input. False otherwise.
-- Helper function
    return ( bit.band(input , bit.rshift( 0x100,button )) ~= 0 )
end


--*****************************************************************************
function RewindThis()
--*****************************************************************************
--DarkKobold & FatRatKnight; Original concept by Antony Lavelle

    if saveCount <= 0 then
        return false    -- Failed to rewind
    else
        savestate.load(saveArray[saveCount]);
        saveCount = saveCount-1;
    end;
    return true         -- Yay, rewind!
end


--*****************************************************************************
function ShowOnePlayer(x,y,color,button,pl)
--*****************************************************************************
-- Displays an individual button.
-- Helper function for DisplayInput.

    x= x + 4*button - 3
    Draw[btn[button]](x,y,color)
end

--*****************************************************************************
function ShowManyPlayers(x,y,color,button,pl)
--*****************************************************************************
-- Displays an individual button. Uses 2x3 boxes instead of button graphics.
-- Helper function for DisplayInput.

    x= x + (2*players + 1)*(button - 1) + 2*pl - 1
    FBoxOld(x,y,x+1,y+2,color)
end


Future= limits(Future,Past,Past+53) --Idiot-proofing
--*****************************************************************************
function DisplayOptions(width)  -- Expects width calculated by DisplayInput
--*****************************************************************************
-- Returns true if Opaque is 0, as a signal to don't bother painting.
-- Separated from DisplayInput to make it clear which half is doing what.


-- Change opacity?
    if pressrepeater(solid) then Opaque= Opaque + 1/8 end
    if pressrepeater(clear) then Opaque= Opaque - 1/8 end
    Opaque= limits(Opaque,0,1)

    gui.opacity(Opaque)
    if Opaque == 0 then  return true  end
    -- Don't bother processing display if there's none to see.

-- How many frames to show?
    if pressrepeater(LessFutr) then
        Future= Future-1
        if Future < Past then Past= Future end
    end
    if pressrepeater(MoreFutr) then
        Future= Future+1
        if Future > Past+53 then Past= Future-53 end
    end
    if pressrepeater(MorePast) then
        Past= Past-1
        if Past < Future-53 then Future= Past+53 end
    end
    if pressrepeater(LessPast) then
        Past= Past+1
        if Past > Future then Future= Past end
    end

    if press(ResetFP) then Past= -12;  Future= 12   end


-- Move the display around?
    if pressrepeater(DispS) then DispY= DispY+1 end
    if pressrepeater(DispW) then DispX= DispX-1 end
    if pressrepeater(DispE) then DispX= DispX+1 end
    if pressrepeater(DispN) then DispY= DispY-1 end

    if keys["leftclick"] and lastkeys["leftclick"] then
        DispX= DispX + keys.xmouse - lastkeys.xmouse
        DispY= DispY + keys.ymouse - lastkeys.ymouse
    end

--Miscellaneous
    if press(BackupDisplaySwitch) then DisplayBackup= not DisplayBackup end
    DispX= limits(DispX,2,254-width)
    DispY= limits(DispY,3-4*Past,218-4*Future)


    return nil -- Signal to display the input
end


--*****************************************************************************
function DisplayInput()
--*****************************************************************************
-- Paints on the screen the current input stored within the script's list.

--Are we showing all players or just one?
    local HighlightButton= ShowOnePlayer
    local width= 32
    if plmin ~= plmax then
        HighlightButton= ShowManyPlayers
        width= 16*players + 8
    end

--For both setting options and asking if we should bother displaying ourselves
    if DisplayOptions(width) then  return  end

--Display frame offsets we're looking at.
    local RtDigit= DispX + width + 22
    if RtDigit > 254 then  RtDigit= DispX - 4  end
    local TpDigit= DispY + 4*Past - 2
    DrawNum(RtDigit,TpDigit,Past,white,shade)

    if Future > Past+1 then
        TpDigit= DispY + 4*Future
        DrawNum(RtDigit,TpDigit,Future,white,shade)
    end

--Show cute little box around current frame
    if Past <= 0 and Future >= 0 then
        local color= blue
        for pl= plmin, plmax do
            local ThisFrame= InputList[pl][fc-BufLen[pl]]
            if BufLen[pl] > 0 then  ThisFrame= BufInput[pl][1]  end
            if ThisFrame ~= JoyToNum(ThisInput[pl]) then  color= green  end
        end
        FBoxOld(DispX-1,DispY-2,DispX+width+1,DispY+4,color)
    end

--Shade the proper regions efficiently
    if Past < 0 then
        local Y1= DispY + 4*Past -3
        local Y2= DispY - 3
        if Future < -1 then  Y2= DispY + 4*Future +1  end
        FakeBox(DispX,Y1,DispX+width,Y2,shade,shade)
    end
    if Future > 0 then
        local Y1= DispY + 5
        local Y2= DispY + 4*Future+5
        if Past > 1 then  Y1= DispY + 4*Past +1  end
        FakeBox(DispX,Y1,DispX+width,Y2,shade,shade)
    end
    if  Past <= 0  and  Future >= 0  then
        FakeBox(DispX,DispY-1,DispX+width,DispY+3,shade,shade)
    end

--Finally, we get to show the actual buttons!
    for i= Past, Future do
        local Y= DispY + 4*i
        if     i < 0 then  Y=Y-2
        elseif i > 0 then  Y=Y+2 end

        local BackupColor= nil -- For displaying a line for backup
        for pl= plmin, plmax do
            local scanz
            if     i  < 0 then scanz= InputList[pl][fc+i]
            elseif i == 0 then scanz= JoyToNum(ThisInput[pl])
            elseif i  < BufLen[pl]  then
                               scanz= BufInput[pl][i+1]
            else               scanz= InputList[pl][fc+i-BufLen[pl]]
            end
            for button= 1, 8 do

-- Take a number, oh mighty color. There's too much of ya!
                local color
                if not scanz then
                    color= 1        --Does not exist
                elseif ReadJoynum(scanz,button) then
                    if (i <= 0) or (ReadList[pl][button]) then
                        color= 4    --Button on
                    else
                        color= 3    --Button on, will be ignored
                    end
                else
                    color= 2        --Button off
                end

                if DisplayBackup then
                    if BackupList[pl][fc+i] then
                        if ReadJoynum(BackupList[pl][fc+i],button) then
                            color= color+8
                        else
                            color= color+4
                        end
                    end
                    if     BackupOverride[pl] == "read"  then
                        color= color+12
                    elseif BackupOverride[pl] == "write" then
                        color= color+24
                    end
                end

                HighlightButton(DispX,Y,ListOfBtnClr[color],button,pl)

            end -- button loop

-- Draw cute lines if we wish to see the backup
            if DisplayBackup and BackupList[pl][fc+i] then
                if not BackUpColor then
                    if not scanz then
                        BackupColor= 1
                    elseif scanz == BackupList[pl][fc+i] then
                        BackupColor= 3
                    else
                        BackupColor= 2
                    end
                    if     BackupOverride[pl] == "read"  then
                        BackupColor= BackupColor+3
                    elseif BackupOverride[pl] == "write" then
                        BackupColor= BackupColor+6
                    end

                else -- Eh, we already have a BackupColor?
                    local CheckColor= (BackupColor-1)%3
                    if not scanz then
                        BackupColor= BackupColor - CheckColor
                    elseif (scanz ~= BackupList[pl][fc+i]) and (CheckColor == 2) then
                        BackupColor= BackupColor - 1
                    end
                end
            end

        end -- player loop

        if BackupColor then
            gui.line(DispX-2,Y,DispX-2,Y+2,ListOfChangesClr[BackupColor])
        end
    end -- loop from Past to Future
end --function


--*****************************************************************************
function SetInput()
--*****************************************************************************
-- Prepares ThisInput for joypad use.

    for pl= 1, players do
        local temp
        if BackupOverride[pl] == "read" then
            temp= BackupList[pl][fc]
        else
            if BufLen[pl] > 0 then
                temp= BufInput[pl][1]
            else
                temp= InputList[pl][fc-BufLen[pl]]
            end
        end

        for i= 1, 8 do
            if temp and ReadJoynum(temp,i) and ReadList[pl][i] then
                ThisInput[pl][btn[i]]= TrueSwitch[pl][i]
            else
                ThisInput[pl][btn[i]]= FalseSwitch[pl][i]
            end
        end
    end
end


--*****************************************************************************
function ApplyInput()
--*****************************************************************************
-- Shoves ThisInput into the actual emulator joypads.

    if movie.mode() ~= "playback" then
        for pl= 1, players do
            joypad.set(pl, ThisInput[pl])
        end
    end
end


--*****************************************************************************
function InputSnap()
--*****************************************************************************
-- Will shove the input list over.
-- Might end up freezing for a moment if there's a few thousand frames to shift

    for pl= 1, players do

       if BufLen[pl] < 0 then  -- Squish!
           local pointer= fc
           while InputList[pl][pointer] do
               InputList[pl][pointer]= InputList[pl][pointer-BufLen[pl]]
               pointer= pointer+1
           end
       end


       if BufLen[pl] > 0 then -- Foom!
           local pointer= fc
           while InputList[pl][pointer] do
               pointer= pointer+1
           end
           -- pointer is now looking at a null frame.
           -- Assume later frames are also null.

           while pointer > fc do
               pointer= pointer-1
               InputList[pl][pointer+BufLen[pl]]= InputList[pl][pointer]
           end
           -- pointer should now match fc.
           -- Everything at fc and beyond should be moved over by BufLen.

           for i=0, BufLen[pl]-1 do
               InputList[pl][fc +i]= table.remove(BufInput[pl],1)
           end
       end

       BufLen[pl]= 0 -- If it ain't zero before, we want it zero now!

    end
end




local XStart, XDiff= 30, 25
local YStart, YDiff= 12, 15
local Status= { {},{},{},{} }
local TargA= {FalseSwitch,TrueSwitch,ReadList,ScriptEdit}
local TrueA= {   nil     ,   "inv"  ,  true  ,  true    }
local FalsA= {   false   ,   true   ,  false ,  false   }
local BtnX, BtnY= 0, 0

local BasicText= {
"Activate: Lets the joypad activate input.",
"Remove: Allows the joypad to remove input.",
"List: Should the script remember button presses?",
"Keys: Allow the script-specific keys to work?"}

local AdvText= {
{"With both joypad options on, you have full control.",
 "Held buttons will toggle the script's stored input."}, -- 1

{"With this on-off set-up, auto-hold will not",
 "accidentally cancel input, such as from load state."}, -- 2

{"With this off-on set, you can use auto-hold to",
 "elegantly strip off unwanted input."},                 -- 3

{"With both joypad options off, you ensure you can't",
 "accidentally change what's stored in the script."},    -- 4

{"List on: Script will apply input stored within.",
 "This is the whole point of the script, ya know."},     -- 5

{"List off: Stored input is ignored. A good idea if",
 "you don't want the script interfering."},              -- 6

{"Keys on: Script keys will toggle the button press",
 "for the current frame. Meant for precise edits."},     -- 7

{"Keys off: Script-specific keys are no longer a",
 "factor. Long-term control is meant for joypad!"},      -- 8

{"Apparently, you've selected 'All players'",
 "and they have different options set."},                -- 9

{"This is the 'All' button. Selecting this will set",
 "all 'Yes' or all 'No' for this particular option."}    --10
}

--*****************************************************************************
function ControlOptions()
--*****************************************************************************
-- Lets the user make adjustments on the various controls of this script.
-- The interface, apparently, is most of the work!


-- Button selection
    if (keys.xmouse ~= lastkeys.xmouse) or (keys.ymouse ~= lastkeys.ymouse) then
        BtnX= math.floor((keys.xmouse- XStart+4)/XDiff)
        BtnY= math.floor((keys.ymouse- YStart-6)/YDiff)
    else
        if press(OptUp) then
            BtnX= limits(BtnX  ,1,4)
            BtnY= limits(BtnY-1,1,9)
        end
        if press(OptDn) then
            BtnX= limits(BtnX  ,1,4)
            BtnY= limits(BtnY+1,1,9)
        end
        if press(OptRt) then
            BtnX= limits(BtnX+1,1,4)
            BtnY= limits(BtnY  ,1,9)
        end
        if press(OptLf) then
            BtnX= limits(BtnX-1,1,4)
            BtnY= limits(BtnY  ,1,9)
        end
    end


--Did you punch something?
    if press("leftclick") or press(OptHit) then
        if within( BtnY , 1 , 9 ) then
            if within( BtnX , 1 , 4 ) then
                local LoopS, LoopF= 1, 8
                if BtnY ~= 9 then
                    LoopS, LoopF= BtnY, BtnY
                end

                local Target, TstT, TstF= TargA[BtnX], TrueA[BtnX], FalsA[BtnX]

                for pl= plmin, plmax do
                    for Loop= LoopS, LoopF do
                        if Target[plmax][LoopF] == TstT then
                            Target[pl][Loop]= TstF
                        else
                            Target[pl][Loop]= TstT
                        end
                    end
                end

            end
        end
    end


--Figure out the status
    for i= 1, 4 do
        for j= 1, 8 do
            Status[i][j]= TargA[i][plmin][j]
            local pl= plmin+1
            while (pl <= plmax) do
                if Status[i][j] ~= TargA[i][pl][j] then
                    Status[i][j]= "???"
                    break
                end
                pl= pl+1
            end
            if     Status[i][j] == TrueA[i] then  Status[i][j]= "Yes"
            elseif Status[i][j] == FalsA[i] then  Status[i][j]= "No" end
        end
    end

--=============================================================================
--Begin drawing. Finally...


--Shade the whole freaking screen
    FakeBox(0,0,255,223,shade,shade)

-- Silly little graphics
-- Perhaps I shouldn't always assume that button right is a right arrow...
    Draw.B(     XStart + XDiff     ,YStart,red)
    Draw.right( XStart + XDiff  + 5,YStart,white)
    Draw.B(     XStart + XDiff  +10,YStart,green)

    Draw.B(     XStart + XDiff*2   ,YStart,green)
    Draw.right( XStart + XDiff*2+ 5,YStart,white)
    Draw.B(     XStart + XDiff*2+10,YStart,red)

    Draw[btn[1]](XStart + XDiff*3   ,YStart  ,green)
    Draw[btn[1]](XStart + XDiff*3   ,YStart+4,green)
    Draw[btn[2]](XStart + XDiff*3+ 4,YStart  ,green)
    Draw[btn[2]](XStart + XDiff*3+ 4,YStart+4,red)
    Draw[btn[3]](XStart + XDiff*3+ 8,YStart  ,red)
    Draw[btn[3]](XStart + XDiff*3+ 8,YStart+4,green)
    Draw[btn[4]](XStart + XDiff*3+12,YStart  ,red)
    Draw[btn[4]](XStart + XDiff*3+12,YStart+4,red)

    gui.box(XStart+XDiff*4,YStart,XStart+XDiff*4+14,YStart+6,0,green)
    Draw[btn[6]](XStart + XDiff*4+ 2,YStart+2,green)
    Draw[btn[7]](XStart + XDiff*4+ 6,YStart+2,green)
    Draw[btn[8]](XStart + XDiff*4+10,YStart+2,green)


--Highlight button
    if within( BtnY , 1 , 9 ) then
        if within( BtnX , 1 , 4 ) then
            local LowX=BtnX*XDiff +XStart -4
            local LowY=BtnY*YDiff +YStart -3
            gui.box(LowX,LowY,LowX+XDiff-2,LowY+YDiff-2,blue,blue)

--Handle text. Feels sort of hard-coded, however
            gui.text(1,160,BasicText[BtnX])

            local adv
            if BtnY == 9 then
                adv= 10
            elseif Status[BtnX][BtnY] == "???" then
                adv=  9
            elseif within(BtnX,1,2) and ((Status[3-BtnX][BtnY] == "???")) then
                adv=  9
            elseif within(BtnX,1,2) then
                adv=  1
                if Status[2][BtnY] == "No" then
                    adv= adv + 1
                end
                if Status[1][BtnY] == "No" then
                    adv= adv + 2
                end
            else
                adv= 5
                if BtnX == 4 then
                    adv= adv+2
                end
                if Status[BtnX][BtnY] == "No" then
                    adv= adv+1
                end
            end
            gui.text(1,175,AdvText[adv][1])
            gui.text(1,183,AdvText[adv][2])
        end
    end


--Lines!
    for i= 1, 5 do
        local Xpos= XStart + XDiff*i -5
        gui.line(Xpos,0,Xpos,YStart + YDiff*10-4,green)
    end

    for j= 1, 9 do
        local Ypos= YStart + YDiff*j -4
        gui.line( 0, Ypos, XStart + XDiff*5-5, Ypos, green)
    end
    gui.line(XStart+XDiff-4, YStart + YDiff*10-4, XStart + XDiff*5-5, YStart + YDiff*10-4, green)


--Button labels!
    for j= 1, 8 do
        local Ypos= YStart + YDiff*j
        gui.text( 1, Ypos, btn[j])
        Draw[btn[j]](XStart + XDiff - 12, Ypos, green)

        for i= 1, 4 do
            gui.text(XStart + XDiff*i, Ypos, Status[i][j])
        end
    end

    for i= 1, 4 do
        gui.text(XStart + XDiff*i, YStart + YDiff*9, "All")
    end

end


--*****************************************************************************
function CatchInput()
--*****************************************************************************
-- For use with registerbefore. It will get the input and paste it into
-- the input list.

    fc= movie.framecount()
    for pl= 1, players do
        if BufLen[pl] > 0 then
            table.remove(BufInput[pl],1)
            table.insert(BufInput[pl],InputList[pl][fc-1])
        end
        local NewJoy= JoyToNum(joypad.get(pl))
        InputList[pl][fc-1]= NewJoy
        if not BackupList[pl][fc-1]  or  (BackupOverride[pl] == "write") then
            BackupList[pl][fc-1]= NewJoy
        end
    end
    SetInput()
end
emu.registerbefore(CatchInput)


--*****************************************************************************
function OnLoad()
--*****************************************************************************
-- For use with registerload. It updates ThisInput so we're not stuck with
-- junk being carried over when we load something.
-- Also clears whatever rewind stuff is stored.

    InputSnap()
    for pl= 1, players do    --Assume user is done messing with backup on load
        BackupOverride[pl]= false
    end

    fc= movie.framecount()
    LastLoad= fc
    saveCount= 0
    SetInput()
    ApplyInput() -- Sanity versus unpaused stateload
end
savestate.registerload(OnLoad)


--*****************************************************************************
function ItIsYourTurn()
--*****************************************************************************
-- For use with gui.register. I need to catch input while paused!
-- Needed for a half-decent interface.

--Ensure things are nice, shall we?
    local fc_= movie.framecount()
    lastkeys= keys
    keys= input.get()

    if fc ~= fc_ then -- Sanity versus unusual jump (Reset cycle?)
        OnLoad()
    end

--Process Rewind
 if saveMax > 0 then  -- Don't process if Rewind is disabled
    if pressrepeater(rewind) or rewinding then
        rewinding= false
        if RewindThis() then
            InputSnap()
            movie.rerecordcounting(true)
            fc= movie.framecount()
            SetInput()
            if saveCount < 1 then  emu.pause()  end
        end
    end
 end

--Should we bother the backups today?
    for pl= plmin, plmax do
        if press(BackupReadSwitch) then
                if BackupOverride[pl] ~= "read" then
                    BackupOverride[pl]= "read"
                else
                    BackupOverride[pl]= false
                end
        end
        if press(BackupWriteSwitch) then
            if BackupOverride[pl] ~= "write" then
                BackupOverride[pl]= "write"
            else
                BackupOverride[pl]= false
            end
        end
    end

    if keys[BackupReadSwitch] and keys[BackupWriteSwitch] then
        gui.text(30,1,"Hey, quit holding both buttons!")
    elseif keys[BackupReadSwitch] then
        gui.text(30,1,"This lets the script revert to the backup.")
    elseif keys[BackupWriteSwitch] then
        gui.text(30,1,"This stores input into the backup.")
    end

--Switch players on keypress
    if press(PlayerSwitch) then
        if plmin ~= plmax then
            plmax= 1
        elseif plmin == players then
            plmin= 1
        else
            plmin= plmin+1
            plmax= plmax+1
        end
    end

--Display which player we're selecting. Upperleft corner seems good.
    if plmin == plmax then
        gui.text(1,2,plmin)
        gui.text(11,2,BufLen[plmin])
    else
        gui.text(1,2,"A")
    end

--Check if we want to see control options
    if keys[opt] then
        gui.opacity(1)
        ControlOptions()
        SetInput()

--Otherwise, handle what we can see
    else

--Inserts and deletes.
--table functions don't work well on tables that don't connect from 1.
        if pressrepeater(Insert) then
            for pl= plmin, plmax do
                BufLen[pl]= BufLen[pl] + 1
                if BufLen[pl] > 0 then
                    table.insert(BufInput[pl],1,0)
                end
            end
            SetInput()
        end
        if pressrepeater(Delete) then
            for pl= plmin, plmax do
                if BufLen[pl] > 0 then
                    table.remove(BufInput[pl],1)
                end
                BufLen[pl]= BufLen[pl] - 1
            end
            SetInput()
        end

--Script key handler
        for pl= plmin, plmax do
            for i= 1, 8 do
                if press(key[i]) and ScriptEdit[pl][i] then
                    if ThisInput[pl][btn[i]] then
                        ThisInput[pl][btn[i]]= FalseSwitch[pl][i]
                    else
                        ThisInput[pl][btn[i]]= TrueSwitch[pl][i]
                    end
                end
            end
        end

        DisplayInput()
    end

--Last bits of odds and ends
    ApplyInput()

    collectgarbage("collect")
end
gui.register(ItIsYourTurn)


--*****************************************************************************
function Rewinder()
--*****************************************************************************
-- Contains the saving part of the Rewind engine.
-- Original by Antony Lavelle, added by DarkKobold, messed by FatRatKnight
 if saveMax > 0 then  -- Don't process if Rewind is disabled
    if keys[rewind] and saveCount > 0 then
        rewinding= true

    elseif (fc - LastLoad)%SaveBuf == 0 then
        if saveCount >= saveMax then
            table.remove(saveArray,1)
        else
            saveCount= saveCount+1
        end

        if saveArray[saveCount] == nil then
            saveArray[saveCount]= savestate.create();
        end

        savestate.save(saveArray[saveCount]);

        movie.rerecordcounting(false)
    end
 end

end

--emu.registerafter(Rewinder)








local TestNumOfDoom= 0
local TestFile= io.open("Backup" .. TestNumOfDoom .. ".txt" , "r")
while TestFile do
    TestFile:close()
    TestNumOfDoom= TestNumOfDoom+1
    if TestNumOfDoom >= BackupFileLimit then break end
    TestFile= io.open("Backup" .. TestNumOfDoom .. ".txt" , "r")
end




local FCEUXbtn= {"right", "left", "down", "up", "start", "select", "B", "A"}
local FCEUXfrm= {"R","L","D","U","T","S","B","A"}
local FCEUXorder= {}

for i= 1, 8 do
    for j= 1, 8 do
        if btn[i] == FCEUXbtn[j] then
            FCEUXorder[j]= i
            break
        end
    end
end

--*****************************************************************************
function SaveToFile()
--*****************************************************************************
-- Creates a file that, effectively, stores whatever's in the script at the
-- time.
-- List of known glitches:
--   It only reads player 1 when deciding when to start or stop.

    InputSnap()
    local LatestIndex= 0
    local EarliestIndex= math.huge
    for Frame, Pads in pairs(InputList[1]) do
        LatestIndex= math.max(LatestIndex,Frame)
        EarliestIndex= math.min(EarliestIndex,Frame)
    end


    local BFile = io.open("Backup" .. TestNumOfDoom .. ".txt" , "w")
    BFile:write("StartFrame: " .. EarliestIndex .. "\n")

    for Frame= EarliestIndex, LatestIndex do
        BFile:write("|0")
        for pl= 1, 4 do
            BFile:write("|")
            if InputList[pl] then
                if InputList[pl][Frame] then
                    local Pads= InputList[pl][Frame]
                    for i= 1, 8 do
                        if ReadJoynum(Pads,FCEUXorder[i]) then
                            BFile:write(FCEUXfrm[i])
                        else
                            BFile:write(".")
                        end
                    end
                else
                    BFile:write("        ")
                end
            end
        end
        BFile:write("\n")
    end
    BFile:close()

end

emu.registerexit(SaveToFile)











emu.pause()

while true do
    Rewinder()

    emu.frameadvance()
end