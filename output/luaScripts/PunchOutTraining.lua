-- Just something quick for Mike Tyson's Punch Out!!
-- Intended to help time hits in real time.
-- Just quickly worked something up. Didn't bother with bells and whistles.
--FatRatKnight


local EHP= 0x0398  -- Enemy HP address
local TMR= 23      -- Frames in advance for your punches.
local BND= -8      -- KEEP NEGATIVE!! Frames after the golden zone.

local DISPx= 180
local DISPy= 180

local timer= 0

local EnemyHP
local lastEHP
local LastHit=-50
local poke
local FreakingAwesome

--*****************************************************************************
function Is_Hit()
--*****************************************************************************
    if EnemyHP ~= lastEHP then
        return true
    end
    return false
end


local LastButtons= {}
local Buttons= {}
--*****************************************************************************
function IsPress()
--*****************************************************************************
    LastButtons["A"]= Buttons["A"]
    LastButtons["B"]= Buttons["B"]

    Buttons= joypad.get(1)
    if (Buttons["A"] and not LastButtons["A"]) or (Buttons["B"] and not LastButtons["B"]) then
        return true
    end
    return false
end


--*****************************************************************************
while true do
--*****************************************************************************
    EnemyHP= memory.readbyte(EHP)
    gui.text(144,22,EnemyHP)

    if IsPress() then
        poke= true
    end

    if Is_Hit() then
        LastHit= TMR
        poke= false
    end

    for i= 1, TMR do
        local color= "black"
        if i == LastHit then
            if poke then
               color= "red"
            else
                color= "green"
            end
        end
        gui.drawbox(DISPx, DISPy-3 - 4*i,DISPx+7, DISPy-1 - 4*i,color)
    end

    if FreakingAwesome then
        gui.text(128,50,"OK")
        local color= "white"
        if (timer % 3) == 0 then
            color= "green"
        elseif (timer % 3) == 1 then
            color= "blue"
        end
        gui.drawbox(DISPx  , DISPy  , DISPx+7, DISPy+2, color)
        gui.drawbox(DISPx-2, DISPy-2, DISPx+9, DISPy+4, color)
    else
        local color= "black"
        if LastHit == 0 then
            if poke then
                color= "blue"
            else
                color= "green"
            end
        end
        gui.drawbox(DISPx  , DISPy  ,DISPx+7, DISPy+2,color)
        gui.drawbox(DISPx-2, DISPy-2,DISPx+9, DISPy+4,"white")
    end

    for i= BND, -1 do
        local color= "black"
        if i == LastHit then
            if poke then
                color= "red"
            else
                color= "green"
            end
        end
        gui.drawbox(DISPx, DISPy+3 - 4*i,DISPx+7, DISPy+5 - 4*i,color)
    end

    if not poke then
        LastHit= LastHit-1
    end
    FCEU.frameadvance()
    lastEHP= EnemyHP
    timer= timer+1
end