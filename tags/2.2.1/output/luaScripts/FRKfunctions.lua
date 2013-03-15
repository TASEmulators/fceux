_FRK_Fn= {}
_FRK_Fn.version= 1

--FatRatKnight
-- Various little functions for use.

-- Shortly, I will list out functions and keywords that the script provides
-- access to. But first, I feel a need to explain a few things...

-- Option keywords are simply global variables that this auxillary script looks
-- for. If they are anything other than false or nil by the time your script
-- calls requrie("FRKfunctions"), this script will react accordingly.

-- Reserved variable names are those the script uses, but allows access for
-- other scripts. Sorry about taking valuble names away from you, but I figure
-- you might want direct access to them.

-- Functions are just that. Functions that do stuff, the whole point of this
-- auxillary script. I copied down their line numbers for your convenience.
-- If the quick description isn't enough, then perhaps the more detailed
-- comments and actual code might be.

-- Options for individual functions can be changed by altering its number
-- after the require("FRKfunctions") line.



--List of option keywords:

-- _FRK_SkipRename        Convenient names aren't used for these functions
-- _FRK_NoAutoReg         Will not automatically fill registers


--List of reserved variable names:

-- keys                     For checking current keys.
-- lastkeys                 For checking previous keys.


--List of functions:

--  82 UpdateKeys()                           Updates this script's key tables
--  97 Press(button)                          Checks for a key hit
-- 109 PressRepeater(button)                  Like Press, but if held for long
-- 133 Release(button)                        Checks for when a key is released
-- 144 MouseMoved()                           Returns how far the mouse moved

-- 166 FBoxOldFast(x1, y1, x2, y2, color)     Faster at drawing, but stupid
-- 180 FBoxOld(x1, y1, x2, y2, color)          Border-only box; corners fix
-- 210 FakeBox(x1, y1, x2, y2, Fill, Border)   Fill-and-Border box; corners fix
-- 230 Draw[digit](Left,Top,color)               Paints tiny digit on screen
-- 354 _FRK_Fn._DN_AnyBase(right, top, Number, color, bkgnd, div)
    --     Paints a right-aligned number using any base from 2 to 16.
-- 405 DrawNum(right, top, Number, color, bkgnd)  Paints a beautiful number
-- 413 DrawNumx(right, top, Number, color, bkgnd) Hexidecimal version

-- 504 ProcessReg()             Removes functions in registers and stores 'em
-- 521 ApplyReg()               Sets registers
-- 534 AddReg(name, function)
-- 543 RemoveReg(name, pos)

-- 564 limits(value,low,high)                 Returns value or specified limits
-- 575 within(value,low,high)                 Returns T/F; Is it within range?


--List of options for individual functions:

_FRK_Fn.RepeaterDelay= 5        --How long should PressRepeater wait?










-- input.get family

--*****************************************************************************
function _FRK_Fn.UpdateKeys()
--*****************************************************************************
-- Moves keys into lastkeys, then puts fresh keyboard input into keys.
-- Meant to simplify the process of registering keyhits.
-- Stick this thing at the beginning of a gui.register function or main loop.

    lastkeys= keys
    keys= input.get()
end

keys= input.get() -- Sanity. Want xmouse and ymouse to exist.
lastkeys= keys    -- Otherwise, MouseMoved pings an error at me.


--*****************************************************************************
function _FRK_Fn.Press(button)  -- Expects a key
--*****************************************************************************
-- Returns true or false. It's true at the moment the user hits the button.
-- Generally, if keys is pressed, the next loop around will set lastkeys,
-- and thus prevent returning true more than once if held.

    return (keys[button] and not lastkeys[button])
end


local repeater= 0
--*****************************************************************************
function _FRK_Fn.PressRepeater(button)  -- Expects a key
--*****************************************************************************
--DarkKobold & FatRatKnight
-- Returns true or false.
-- Acts much like press if you don't hold the key down. Once held long enough,
-- it will repeatedly return true.
-- It works fine if there are multiple calls to different buttons. Not so fine
-- if there's multiple calls to the same button. Only one repeater variable...

    if keys[button] then
        if not lastkeys[button] or repeater >= _FRK_Fn.RepeaterDelay then
            return true
        else
            repeater = repeater + 1
        end

    elseif lastkeys[button] then   -- To allow more calls for other buttons
        repeater= 0
    end
    return false
end


--*****************************************************************************
function _FRK_Fn.Release(button)  -- Expects a key
--*****************************************************************************
-- Returns true or false. It's true at the moment the user releases the button.
-- Might be a good idea to know when the user ain't holding it anymore.
-- Eh, I don't see any obvious application, but may as well have it.

    return ((not keys[button]) and lastkeys[button])
end


--*****************************************************************************
function _FRK_Fn.MouseMoved()
--*****************************************************************************
-- Returns two values: x,y
-- This function tells you how far the mouse moved since the last update.
-- It's simply the difference of what position it is now and what it once was.

    return (keys.xmouse - lastkeys.xmouse)  ,  (keys.ymouse - lastkeys.ymouse)
end










-- Display family
-- For helping you see

--*****************************************************************************
function _FRK_Fn.FBoxOldFast(x1, y1, x2, y2, color)
--*****************************************************************************
-- Gets around FCEUX's problem of double-painting the corners.
-- This particular function doesn't make sure the x and y are good.
-- Call this instead if you need processing speed and know its fine

    gui.line(x1  ,y1  ,x2-1,y1  ,color) -- top
    gui.line(x2  ,y1  ,x2  ,y2-1,color) -- right
    gui.line(x1+1,y2  ,x2  ,y2  ,color) -- bottom
    gui.line(x1  ,y1+1,x1  ,y2  ,color) -- left
end


--*****************************************************************************
function _FRK_Fn.FBoxOld(x1, y1, x2, y2, color)
--*****************************************************************************
-- Gets around FCEUX's problem of double-painting the corners.
-- This has several sanity checks to ensure a properly drawn box.
-- It acts like the old-style border-only box.

    if     (x1 == x2) and (y1 == y2) then  -- Sanity: Is it a single dot?
        gui.pixel(x1,y1,color)

    elseif (x1 == x2) or  (y1 == y2) then  -- Sanity: A straight line?
        gui.line(x1,y1,x2,y2,color)

    else --(x1 ~= x2) and (y1 ~= y2)
        local temp
        if x1 > x2 then
            temp= x1;   x1= x2;  x2= temp  -- Sanity: Without these checks,
        end                                -- This function may end up putting
        if y1 > y2 then                    -- two or four out-of-place pixels
            temp= y1;   y1= y2;  y2= temp  -- near the corners.
        end

        gui.line(x1  ,y1  ,x2-1,y1  ,color) -- top
        gui.line(x2  ,y1  ,x2  ,y2-1,color) -- right
        gui.line(x1+1,y2  ,x2  ,y2  ,color) -- bottom
        gui.line(x1  ,y1+1,x1  ,y2  ,color) -- left
    end
end


--*****************************************************************************
function _FRK_Fn.FakeBox(x1, y1, x2, y2, Fill, Border)
--*****************************************************************************
-- Gets around FCEUX's problem of double-painting the corners.
-- It acts like the new-style fill-and-border box. Not quite perfectly...
-- One "problem" is that, if you specify Fill only, it won't successfully
-- mimic the actual gui.box fill.

if not Border then   Border= Fill   end

    gui.box(x1,y1,x2,y2,Fill,0)
    FBoxOld(x1,y1,x2,y2,Border)
end


--###  #  ### ### # # ### ### ### ### ###   #  ##   #  ##  ### ###
--# # ##    #   # # # #   #     # # # # #  # # # # # # # # #   #
--# #  #  ### ### ### ### ###  ## ### ###  # # ##  #   # # ##  ##
--# #  #  #     #   #   # # #  #  # #   #  ### # # # # # # #   #
--### ### ### ###   # ### ###  #  ### ###  # # ##   #  ##  ### #
--*****************************************************************************
_FRK_Fn.Draw= {}   --Draw[button]( Left , Top , color )
--*****************************************************************************
--Coordinates is the top-left pixel of the 3x5 digit.
--Used for drawing compact, colored numbers.

local d= _FRK_Fn.Draw

function d.D0(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+2,top  ,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+4,color)
end

function d.D1(left, top, color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.line(left+1,top  ,left+1,top+3,color)
    gui.pixel(left  ,top+1,color)
end

function d.D2(left, top, color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.line(left  ,top+3,left+2,top+1,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left  ,top+2,color)
    gui.pixel(left+2,top+2,color)
end

function d.D3(left, top, color)
    gui.line(left  ,top  ,left+1,top  ,color)
    gui.line(left  ,top+2,left+1,top+2,color)
    gui.line(left  ,top+4,left+1,top+4,color)
    gui.line(left+2,top  ,left+2,top+4,color)
end

function d.D4(left, top, color)
    gui.line(left  ,top  ,left  ,top+2,color)
    gui.line(left+2,top  ,left+2,top+4,color)
    gui.pixel(left+1,top+2,color)
end

function d.D5(left, top, color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.line(left  ,top+1,left+2,top+3,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left  ,top+2,color)
    gui.pixel(left+2,top+2,color)
end

function d.D6(left, top, color)
    gui.line(left  ,top  ,left+2,top  ,color)
    gui.line(left  ,top+1,left  ,top+4,color)
    gui.line(left+2,top+2,left+2,top+4,color)
    gui.pixel(left+1,top+2,color)
    gui.pixel(left+1,top+4,color)
end

function d.D7(left, top, color)
    gui.line(left  ,top  ,left+1,top  ,color)
    gui.line(left+2,top  ,left+1,top+4,color)
end

function d.D8(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+2,top  ,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+2,color)
    gui.pixel(left+1,top+4,color)
end

function d.D9(left, top, color)
    gui.line(left  ,top  ,left  ,top+2,color)
    gui.line(left+2,top  ,left+2,top+3,color)
    gui.line(left  ,top+4,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+2,color)
end

function d.DA(left, top, color)
    gui.line(left  ,top+1,left  ,top+4,color)
    gui.line(left+2,top+1,left+2,top+4,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+3,color)
end

function d.DB(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+1,top  ,left+2,top+1,color)
    gui.line(left+1,top+4,left+2,top+3,color)
    gui.pixel(left+1,top+2,color)
end

function d.DC(left, top, color)
    gui.line(left  ,top+1,left  ,top+3,color)
    gui.line(left+1,top  ,left+2,top+1,color)
    gui.line(left+1,top+4,left+2,top+3,color)
end

function d.DD(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+2,top+1,left+2,top+3,color)
    gui.pixel(left+1,top  ,color)
    gui.pixel(left+1,top+4,color)
end

function d.DE(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+1,top  ,left+2,top  ,color)
    gui.line(left+1,top+4,left+2,top+4,color)
    gui.pixel(left+1,top+2,color)
end

function d.DF(left, top, color)
    gui.line(left  ,top  ,left  ,top+4,color)
    gui.line(left+1,top  ,left+2,top  ,color)
    gui.pixel(left+1,top+2,color)
end


d[0],d[1],d[2],d[3],d[4]= d.D0, d.D1, d.D2, d.D3, d.D4
d[5],d[6],d[7],d[8],d[9]= d.D5, d.D6, d.D7, d.D8, d.D9
d[10],d[11],d[12],d[13],d[14],d[15]= d.DA, d.DB, d.DC, d.DD, d.DE, d.DF

--*****************************************************************************
function _FRK_Fn._DN_AnyBase(right, top, Number, color, bkgnd, div)
--*****************************************************************************
-- Works with any base from 2 to 16. Paints the input number.
-- Returns the x position where it would paint another digit.
-- It only works with integers. Rounds fractions toward zero.

    if div < 2 then return end  -- Prevents the function from never returning.

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
        Digit[i]= Number % div
        Number= math.floor(Number/div)
    end

    if Negative then  i= i+1  end
    local left= right - i*4
    FakeBox(left+1, top-1, right+1, top+5,bkgnd,bkgnd)

    i= 1
    while _FRK_Fn.Draw[Digit[i]] do
        _FRK_Fn.Draw[Digit[i]](right-2,top,color)
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
function _FRK_Fn.DrawNum(right, top, Number, color, bkgnd)
--*****************************************************************************
-- Paints the input number as right-aligned. Decimal version.

    return _FRK_Fn._DN_AnyBase(right, top, Number, color, bkgnd, 10)
end

--*****************************************************************************
function _FRK_Fn.DrawNumx(right, top, Number, color, bkgnd)
--*****************************************************************************
-- Paints the input number as right-aligned. Hexadecimal version.

    return _FRK_Fn._DN_AnyBase(right, top, Number, color, bkgnd, 16)
end










-- Registry family
-- For overloading the usual registers with multiple separate functions.

local RegNames= {"Before", "After", "Exit", "Gui", "Save", "Load"}

_FRK_Fn.RegList= {}
_FRK_Fn.RegList.Before={}
_FRK_Fn.RegList.After= {}
_FRK_Fn.RegList.Exit=  {}
_FRK_Fn.RegList.Gui=   {}
_FRK_Fn.RegList.Save=  {}
_FRK_Fn.RegList.Load=  {}

--*****************************************************************************
_FRK_Fn.RegFn= {}       --RegFn[name]()   For various registers
--*****************************************************************************

function _FRK_Fn.RegFn.Before()
    local i= 1
    while (_FRK_Fn.RegList.Before[i]) do
        _FRK_Fn.RegList.Before[i]()
        i= i+1
    end
end

function _FRK_Fn.RegFn.After()
    local i= 1
    while (_FRK_Fn.RegList.After[i]) do
        _FRK_Fn.RegList.After[i]()
        i= i+1
    end
end

function _FRK_Fn.RegFn.Exit()
    local i= 1
    while (_FRK_Fn.RegList.Exit[i]) do
        _FRK_Fn.RegList.Exit[i]()
        i= i+1
    end
end

function _FRK_Fn.RegFn.Gui()
    local i= 1
    while (_FRK_Fn.RegList.Gui[i]) do
        _FRK_Fn.RegList.Gui[i]()
        i= i+1
    end
end

function _FRK_Fn.RegFn.Save()
    local i= 1
    while (_FRK_Fn.RegList.Save[i]) do
        _FRK_Fn.RegList.Save[i]()
        i= i+1
    end
end

function _FRK_Fn.RegFn.Load()
    local i= 1
    while (_FRK_Fn.RegList.Load[i]) do
        _FRK_Fn.RegList.Load[i]()
        i= i+1
    end
end


local EmuRegisters={
    emu.registerbefore,
    emu.registerafter,
    emu.registerexit,
    gui.register,
    savestate.registersave,
    savestate.registerload
}
--*****************************************************************************
function _FRK_Fn.ProcessRegisters()
--*****************************************************************************
-- Any functions in registers, such as those that might be inserted by
-- auxillary files, are removed from their registers and inserted into my
-- table of functions to execute.
-- All registers will be empty after excecution of this function.

    for i= 1, 6 do
        local test= EmuRegisters[i]()
-- Make sure the function exists. Also, make sure it isn't ourselves...
        if test and (test ~= _FRK_Fn.RegFn[ RegNames[i] ]) then
            table.insert(_FRK_Fn.RegList[ RegNames[i] ],1,test)
        end
    end
end

--*****************************************************************************
function _FRK_Fn.ApplyRegisters()
--*****************************************************************************
-- Fills the registers with functions that will excecute functions stored in
-- my tables.

    for i= 1, 6 do
        if _FRK_Fn.RegList[ RegNames[i] ][ 1 ] then
            EmuRegisters[i](_FRK_Fn.RegFn[ RegNames[i] ])
        end
    end
end

--*****************************************************************************
function _FRK_Fn.AddRegister(name, fn)
--*****************************************************************************
-- Name should be "Before", "After", "Exit", "Gui", "Save", or "Load".
-- Recommended for use if you plan to add functions to the register.

    table.insert(_FRK_Fn.RegList[name],fn)
end

--*****************************************************************************
function _FRK_Fn.RemoveRegister(name, pos)
--*****************************************************************************
-- Name should be "Before", "After", "Exit", "Gui", "Save", or "Load".
-- You may omit pos. Doing so will remove and return the first function that's
-- been inserted into my table.

    return table.remove(_FRK_Fn.RegList[name],pos)
end










-- Miscellaneous family

--*****************************************************************************
function _FRK_Fn.limits( value , low , high )  -- Expects numbers
--*****************************************************************************
-- Returns value, low, or high. high is returned if value exceeds high,
-- and low is returned if value is beneath low.
-- Sometimes, you'd rather crop the number to some specific limits.

    return math.max(math.min(value,high),low)
end


--*****************************************************************************
function _FRK_Fn.within( value , low , high )  -- Expects numbers
--*****************************************************************************
-- Returns true if value is between low and high, inclusive. False otherwise.
-- Sometimes, you just want to know if the number is within a certain range.

    return ( value >= low ) and ( value <= high )
end









--=============================================================================
if not _FRK_SkipRename then
--=============================================================================
    UpdateKeys=    _FRK_Fn.UpdateKeys
    Press=         _FRK_Fn.Press
    PressRepeater= _FRK_Fn.PressRepeater
    Release=       _FRK_Fn.Release
    MouseMoved=    _FRK_Fn.MouseMoved

    FBoxOldFast=   _FRK_Fn.FBoxOldFast
    FBoxOld=       _FRK_Fn.FBoxOld
    FakeBox=       _FRK_Fn.FakeBox
    Draw=          _FRK_Fn.Draw
    DrawNum=       _FRK_Fn.DrawNum
    DrawNumx=      _FRK_Fn.DrawNumx

    ProcessReg=    _FRK_Fn.ProcessRegisters
    ApplyReg=      _FRK_Fn.ApplyRegisters
    AddReg=        _FRK_Fn.AddRegister
    RemoveReg=     _FRK_Fn.RemoveRegister

    limits=        _FRK_Fn.limits
    within=        _FRK_Fn.within
end

--=============================================================================
if not _FRK_NoAutoReg then
--=============================================================================
    emu.registerbefore(    _FRK_Fn.RegFn.Before)
    emu.registerafter(     _FRK_Fn.RegFn.After )
    emu.registerexit(      _FRK_Fn.RegFn.Exit  )
    gui.register(          _FRK_Fn.RegFn.Gui   )
    savestate.registersave(_FRK_Fn.RegFn.Save  )
    savestate.registerload(_FRK_Fn.RegFn.Load  )
end