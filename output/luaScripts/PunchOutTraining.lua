-- Just something quick for Mike Tyson's Punch Out!!
-- Intended to help time hits in real time.
--FatRatKnight


local EHP= 0x0398  -- Enemy HP address
local TMR= 23      -- Frames in advance for your punches.
local BND= -8      -- KEEP NEGATIVE!! Frames after the golden zone.

local DISPx= 180
local DISPy= 180
local DISPx2= DISPx+11 -- Right side of box. Adjust that plus to your need

local timer= 0

local EnemyHP
local lastEHP
local LastHit=-50
local HitTiming= BND-1

--*****************************************************************************
function Is_Hit()
--*****************************************************************************
    if lastEHP then
        if EnemyHP < lastEHP then
            return true
        end    
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
        HitTiming= LastHit
        LastHit= BND-1
        timer= -18
    end

    if Is_Hit() then
        LastHit= TMR
    end


    for i= 1, TMR do
        local color= "black"
        if i == LastHit then
            color= "green"
        elseif i == HitTiming and timer < 0 then
            color= "red"
            gui.text(100,80,"early " .. i)
        end
        gui.drawbox(DISPx, DISPy-3 - 4*i,DISPx2, DISPy-1 - 4*i,color)
    end

    if HitTiming == 0 and timer < 0 then
        gui.text(128,80,"OK")
        gui.drawbox(128,80,144,90,"green")
        local color= "white"
        if     (timer % 3) == 0 then
            color= "green"
        elseif (timer % 3) == 1 then
            color= "blue"
        end
        gui.drawbox(DISPx  , DISPy  , DISPx2, DISPy+2, color)
        gui.drawbox(DISPx-2, DISPy-2, DISPx2+2, DISPy+4, color)
    else
        local color= "black"
        if i == LastHit then
            color= "green"
        end
        gui.drawbox(DISPx  , DISPy  ,DISPx2, DISPy+2,color)
        gui.drawbox(DISPx-2, DISPy-2,DISPx2+2, DISPy+4,"white")
    end

    for i= BND, -1 do
        local color= "black"
        if i == LastHit then
            color= "green"
        elseif i == HitTiming and timer < 0 then
            color= "red"
            gui.text(146,80,"late " .. -i)
        end
        gui.drawbox(DISPx, DISPy+3 - 4*i,DISPx2, DISPy+5 - 4*i,color)
    end

    LastHit= LastHit-1
    FCEU.frameadvance()
    lastEHP= EnemyHP
    timer= timer+1
end