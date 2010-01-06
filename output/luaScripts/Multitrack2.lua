local players= 2             -- You may tweak the number of players here.


local PlayerSwitch= "S"    -- For selecting other players
local opt= "space"         -- For changing control options

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

local key = {"right", "left", "down", "up",   "L",      "O",   "J", "K"}
local btn = {"right", "left", "down", "up", "start", "select", "B", "A"}




local plmin , plmax= 1 , 1
local fc= 0

local BufInput, BufLen= {},{}
local InputList= {}
local ThisInput= {}
local TrueSwitch, FalseSwitch= {}, {}
for pl= 1, players do
    InputList[pl]= {}
    ThisInput[pl]= {}
    BufInput[pl]= {}
    BufLen[pl]= 0

    TrueSwitch[pl]= {}
    FalseSwitch[pl]= {}
    for i= 1, 8 do
        TrueSwitch[pl][i]= "inv"
        FalseSwitch[pl][i]= nil
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
--FatRatKnight
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
function limits( value , low , high )
--*****************************************************************************
    return math.max(math.min(value,high),low)
end


local keys, lastkeys= {}, {}
--*****************************************************************************
function press(button)
--*****************************************************************************
    return (keys[button] and not lastkeys[button])
end

local repeater= 0
--*****************************************************************************
function pressrepeater(button)
--*****************************************************************************
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
function JoyToNum(Joys)
--*****************************************************************************
    local joynum= 0

    for i= 1, 8 do
        if Joys[btn[i]] then
            joynum= bit.bor(joynum, bit.rshift(0x100,i))
        end
    end

    return joynum
end

local MsgTmr, Message= 0, "Activated multitrack script. Have fun!"
--*****************************************************************************
function NewMsg(text)
--*****************************************************************************
    MsgTmr= 0
    Message= text
end

local MsgDuration= 60
--*****************************************************************************
function DispMsg()
--*****************************************************************************
    if MsgTmr < MsgDuration then
        MsgTmr= MsgTmr + 1
        gui.text(0,20,Message)
    end
end


--*****************************************************************************
function ShowOnePlayer(x,y,color,button,pl)
--*****************************************************************************
    x= x + 4*button - 3
    Draw[btn[button]](x,y,color)
end

--*****************************************************************************
function ShowManyPlayers(x,y,color,button,pl)
--*****************************************************************************
    x= x + (2*players + 1)*(button - 1) + 2*pl - 1
    gui.box(x,y,x+1,y+2,0,color)
end


local DispX, DispY= 190, 70
local Past, Future= -12, 20
local Opaque= 1
--*****************************************************************************
function DisplayOptions(width)
--*****************************************************************************

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
local shade= 0x00000080
local white= "#FFFFFFFF"
local red=   "#FF2000FF"
local green= 0x00FF00FF
local blue=  0x0040FFFF

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
                    color= green
                else
                    color= red
                end

                HighlightButton(DispX,Y,color,button,pl)
            end
        end
    end
end


--*****************************************************************************
function ReadJoynum(input, button)
--*****************************************************************************
    return ( bit.band(input , bit.rshift( 0x100,button )) ~= 0 )
end


--*****************************************************************************
function SetInput()
--*****************************************************************************
    for pl= 1, players do
        local temp
        if BufLen[pl] > 0 then
            temp= BufInput[pl][BufLen[pl]]
        else
            temp= InputList[pl][fc-BufLen[pl]]
        end

        for i= 1, 8 do
            if temp and ReadJoynum(temp,i) then
                ThisInput[pl][btn[i]]= TrueSwitch[pl][i]
            else
                ThisInput[pl][btn[i]]= FalseSwitch[pl][button]
            end
        end
    end
end

--*****************************************************************************
function ApplyInput()
--*****************************************************************************
    if movie.mode() ~= "playback" then
        for pl= 1, players do
            joypad.set(pl, ThisInput[pl])
        end
    end
end

--*****************************************************************************
function HandleOptions()
--*****************************************************************************

end

--*****************************************************************************
function CatchInput()
--*****************************************************************************
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
    fc= movie.framecount()
    SetInput()
    ApplyInput() -- Sanity versus unpaused stateload
end
savestate.registerload(OnLoad)


--*****************************************************************************
function ItIsYourTurn()
--*****************************************************************************
    local fc_= movie.framecount()
    keys= input.get()

    if fc ~= fc_ then -- Sanity versus unusual jump (Reset cycle?)
        OnLoad()
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
        gui.text(20,1,plmin)
    else
        gui.text(20,1,"A")
    end

    if keys[opt] then
        HandleOptions()
    else
        DisplayInput()
    end

    ApplyInput()

    lastkeys= keys
    gui.pixel(0,0,"clear")
end
gui.register(ItIsYourTurn)


emu.pause()

while true do
    emu.frameadvance()
end