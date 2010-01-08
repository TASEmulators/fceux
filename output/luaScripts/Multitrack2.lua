-- Multitrack v2 for FCEUX by FatRatKnight


--***************
local players= 2           -- You may tweak the number of players here.
--***************


-- Rewind options
local saveMax= 50          -- How many save states to keep?
local SaveBuf= 12          -- How many frames between saves? Don't use 0, ever.
local rewind= "R"          -- What key do you wish to hit for rewind?


--Control
local PlayerSwitch= "home" -- For selecting other players
local opt= "end"           -- For changing control options

local key = {"right", "left", "down", "up",   "L",      "O",   "J", "K"}
local btn = {"right", "left", "down", "up", "start", "select", "B", "A"}

--Try to avoid changing btn. You may reorder btn's stuff if you don't
--like the default order, however. Line up with key for good reasons.
--key is the actual script control over the buttons. Change that!


--Insertions and deletions
local Insert= "insert"     -- Inserts a frame at the frame you're at
local Delete= "delete"     -- Eliminates current frame


--Display
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




local shade= 0x00000080
local white= "#FFFFFFFF"
local red=   "#FF2000FF"
local green= 0x00FF00FF
local blue=  0x0040FFFF
local orange="#FFC000FF"
--*****************************************************************************
--Please do not change the following, unless you plan to change the code:

local plmin , plmax= 1 , 1
local fc= 0
local LastLoad= movie.framecount()
local saveCount= 0
local saveArray= {}
local rewinding= false

local InputList= {}
local ThisInput= {}
local TrueSwitch, FalseSwitch= {}, {}
local ReadList= {}
local ScriptEdit= {}
for pl= 1, players do
    InputList[pl]= {}
    ThisInput[pl]= {}

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
    gui.line(x  ,y+1,x+2,y+1,color)  --     #
end

function Draw.select(x,y,color)      --    ###
    gui.box(x  ,y  ,x+2,y+2,0,color) --    # #
end                                  --    ###

function Draw.A(x,y,color)           --    ###
    gui.box(x  ,y  ,x+2,y+1,0,color) --    ###
    gui.pixel(x  ,y+2,color)         --    # #
    gui.pixel(x+2,y+2,color)
end

function Draw.B(x,y,color)           --    # #
    gui.line(x  ,y  ,x  ,y+2,color)  --    ##
    gui.line(x+1,y+1,x+2,y+2,color)  --    # #
    gui.pixel(x+2,y  ,color)
end



function Draw.D0(left, top, color)
    gui.box(left  ,top  ,left+2,top+4,0,color)
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
    gui.box(left  ,top+2,left+2,top+4,0,color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.pixel(left  ,top+1,color)
end

function Draw.D7(left, top, color)
    gui.line(left  ,top  ,left+1,top  ,color)
    gui.line(left+2,top  ,left+1,top+4,color)
end

function Draw.D8(left, top, color)
    gui.box(left,top,left+2,top+4,0,color)
    gui.pixel(left+1,top+2,color)
end

function Draw.D9(left, top, color)
    gui.box(left  ,top  ,left+2,top+2,0,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left+2,top+3,color)
end

Draw[0],Draw[1],Draw[2],Draw[3],Draw[4]=Draw.D0,Draw.D1,Draw.D2,Draw.D3,Draw.D4
Draw[5],Draw[6],Draw[7],Draw[8],Draw[9]=Draw.D5,Draw.D6,Draw.D7,Draw.D8,Draw.D9
--*****************************************************************************
function DrawNum(right, top, Number, color, bkgnd)
--*****************************************************************************
-- Paints the input number as right-aligned.
-- Returns the x position where it would paint another digit.
-- It only works with integers. Rounds fractions toward zero.

    local Negative= false
    if Number < 0 then
        Number= -Number
        Negative= true
    end

    Number= math.floor(Number)
    if not color then color= "white" end
    if not bkgnd then bkgnd= "clear" end

    if Number < 1 then
        gui.box(right+1,top-1,right-2,top+5,bkgnd,bkgnd)
        Draw[0](right-2,top,color)
        right= right-4
    end

    while (Number >= 1) do
        local digit= Number % 10
        Number= math.floor(Number/10)

        gui.box(right+1,top-1,right-2,top+5,bkgnd,bkgnd)
        Draw[digit](right-2,top,color)
        right= right-4
    end

    if Negative then
        gui.box(right+1,top-1,right-2,top+5,bkgnd,bkgnd)
        gui.line(right, top+2,right-2,top+2,color)
        right= right-4
    end
    gui.line(right+1,top-1,right+1,top+5,bkgnd)
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
    gui.box(x,y,x+1,y+2,0,color)
end


local DispX, DispY= 190, 70
local Past, Future= -12, 20
local Opaque= 1
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

    DispX= limits(DispX,1,254-width)
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
            local ThisFrame= JoyToNum(ThisInput[pl])
            if ThisFrame ~= InputList[pl][fc] then  color= green  end
        end
        gui.box(DispX-1,DispY-2,DispX+width+1,DispY+4,0,color)
    end

--Finally, we get to show the actual buttons!
    for i= Past, Future do
        local Y= DispY + 4*i
        if     i < 0 then  Y=Y-2
        elseif i > 0 then  Y=Y+2 end
        gui.box(DispX,Y-1,DispX+width,Y+3,shade,shade)
        for pl= plmin, plmax do
            local ThisFrame= JoyToNum(ThisInput[pl])
            local scanz= InputList[pl][fc+i]
            if i == 0 then  scanz= ThisFrame  end
            for button= 1, 8 do

                local color
                if not scanz then
                    color= white
                elseif ReadJoynum(scanz,button) then
                    if (i > 0) and not (ReadList[pl][button]) then
                        color= orange
                    else
                        color= green
                    end
                else
                    color= red
                end

                HighlightButton(DispX,Y,color,button,pl)
            end
        end
    end
end


--*****************************************************************************
function SetInput()
--*****************************************************************************
-- Prepares ThisInput for joypad use.

    for pl= 1, players do
        local temp= InputList[pl][fc]

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




local XStart, XDiff= 30, 25
local YStart, YDiff= 12, 15
local Status= { {},{},{},{} }
local TargA= {FalseSwitch,TrueSwitch,ReadList,ScriptEdit}
local TrueA= {   nil     ,   "inv"  ,  true  ,  true    }
local FalsA= {   false   ,   true   ,  false ,  false   }
local BtnX, BtnY= 0, 0

local BasicText= {"Activate: Lets the joypad activate input.",
                  "Remove: Allows the joypad to remove input.",
                  "List: Should the script remember button presses?",
                  "Keys: Allow the script-specific keys to work?"}

--*****************************************************************************
function HandleOptions()
--*****************************************************************************
-- Lets the user make adjustments on the various controls of this script.
-- The interface, apparently, is most of the work!

    Draw.B(    XStart + XDiff     ,YStart,red)
    Draw.right(XStart + XDiff  + 5,YStart,white)
    Draw.B(    XStart + XDiff  +10,YStart,green)

    Draw.B(    XStart + XDiff*2   ,YStart,green)
    Draw.right(XStart + XDiff*2+ 5,YStart,white)
    Draw.B(    XStart + XDiff*2+10,YStart,red)

    Draw.right(XStart + XDiff*3   ,YStart  ,green)
    Draw.right(XStart + XDiff*3   ,YStart+4,green)
    Draw.left( XStart + XDiff*3+ 4,YStart  ,green)
    Draw.left( XStart + XDiff*3+ 4,YStart+4,red)
    Draw.down( XStart + XDiff*3+ 8,YStart  ,red)
    Draw.down( XStart + XDiff*3+ 8,YStart+4,green)

    if (keys.xmouse ~= lastkeys.xmouse) or (keys.ymouse ~= lastkeys.ymouse) then
        BtnX= math.floor((keys.xmouse- XStart+4)/XDiff)
        BtnY= math.floor((keys.ymouse- YStart-6)/YDiff)
    else
        if press(DispN) then
            BtnX= limits(BtnX  ,1,4)
            BtnY= limits(BtnY-1,1,9)
        end
        if press(DispS) then
            BtnX= limits(BtnX  ,1,4)
            BtnY= limits(BtnY+1,1,9)
        end
        if press(DispE) then
            BtnX= limits(BtnX+1,1,4)
            BtnY= limits(BtnY  ,1,9)
        end
        if press(DispW) then
            BtnX= limits(BtnX-1,1,4)
            BtnY= limits(BtnY  ,1,9)
        end
    end

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

    if within( BtnY , 1 , 9 ) then
        if within( BtnX , 1 , 4 ) then
            local LowX=BtnX*XDiff +XStart -4
            local LowY=BtnY*YDiff +YStart -3
            gui.box(LowX,LowY,LowX+XDiff-2,LowY+YDiff-2,blue,blue)
            gui.text(1,170,BasicText[BtnX])
        end
    end

    for i= 1, 5 do
        local Xpos= XStart + XDiff*i -5
        gui.line(Xpos,0,Xpos,YStart + YDiff*10,green)
    end

    for j= 1, 8 do
        local Ypos= YStart + YDiff*j
        gui.line( 0, Ypos-4, XStart + XDiff*5, Ypos-4, green)
        gui.text( 1, Ypos, btn[j])
        Draw[btn[j]](XStart + XDiff - 12, Ypos, green)

        for i= 1, 4 do
            gui.text(XStart + XDiff*i, Ypos, Status[i][j])
        end
    end

    for i= 1, 4 do
        gui.text(XStart + XDiff*i, YStart + YDiff*9, "All")
    end

    if press("leftclick") or press(ResetFP) then
        if within( BtnY , 1 , 9 ) then
            if within( BtnX , 1 , 4 ) then
                local LoopS, LoopF= 1, 8
                if within( BtnY , 1 , 8 ) then
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

end

--*****************************************************************************
function CatchInput()
--*****************************************************************************
-- For use with registerbefore. It will get the input and paste it into
-- the input list, no questions asked.

    fc= movie.framecount()
    for pl= 1, players do
        InputList[pl][fc-1]= JoyToNum(joypad.get(pl))
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

    local fc_= movie.framecount()
    keys= input.get()

    if fc ~= fc_ then -- Sanity versus unusual jump (Reset cycle?)
        OnLoad()
    end

    if pressrepeater(rewind) or rewinding then
        rewinding= false
        if RewindThis() then
            movie.rerecordcounting(true)
            fc= movie.framecount()
            SetInput()
            if saveCount == 0 then  emu.pause()  end
        end
    end

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

    if plmin == plmax then
        gui.text(1,2,plmin)
    else
        gui.text(1,2,"A")
    end

    if keys[opt] then
        gui.opacity(1)
        HandleOptions()
        SetInput()
    else
        if pressrepeater(Insert) then
            for pl= plmin, plmax do
                table.insert(InputList[pl],fc,0)
            end
            SetInput()
        end
        if pressrepeater(Delete) then
            for pl= plmin, plmax do
                table.remove(InputList[pl],fc)
            end
            SetInput()
        end

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

    ApplyInput()

    lastkeys= keys

    collectgarbage("collect")
end
gui.register(ItIsYourTurn)


emu.pause()

--*****************************************************************************
while true do  -- Main loop
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

    emu.frameadvance()
end



--Someday, I'll implement these texts!



--With both joypad options on, you have full control.
--Held buttons will toggle the script's stored input.

--With this on-off set-up, auto-hold will not
--accidentally cancel input, such as from load state.

--With this off-on set, you can use auto-hold to
--elegantly strip off unwanted input.

--With both joypad options off, you ensure you can't
--accidentally change what's stored in the script.


--List on: Script will apply input stored within.
--This is the whole point of the script, ya know.

--List off: Stored input is ignored. A good idea if
--you don't want the script interfering.


--Keys on: Script keys will toggle the button press
--for the current frame. Meant for precise edits.

--Keys off: Script-specific keys are no longer a
--factor. Long-term control is meant for joypad!


--Apparently, you've selected "All players" and
--they have different options set.


--This is the 'All' button. Selecting this will set
--all 'Yes' or all 'No' for this particular option.