-- Just something quick for Mike Tyson's Punch Out!!
-- Intended to help time hits in real time.
-- Just quickly worked something up. Didn't bother with bells and whistles.
--FatRatKnight


local EHP= 0x0398  -- Enemy HP address
local TMR= 21      -- Frames in advance for your punches.
local BND= -8      -- KEEP NEGATIVE!! Frames after the golden zone.

local DISPx= 200
local DISPy= 180

local timer= 0     -- Unused variable

local EnemyHP
local lastEHP
local LastHit=-50

--*****************************************************************************
function IsHit()
--*****************************************************************************
    if EnemyHP ~= lastEHP then
        return true
    end
    return false
end


--*****************************************************************************
function IsPress()
--*****************************************************************************
-- Unused function

    local buttons= {}
    buttons= joypad.get()
    if buttons["A"] or buttons["B"] then
        return true
    end
    return false
end


--*****************************************************************************
while true do
--*****************************************************************************
    EnemyHP= memory.readbyte(EHP)
    local tap
    gui.text(144,22,EnemyHP)

    if IsHit() then
        LastHit= TMR
        -- Hrm... Not a lot I can think of...
    end


    for i= 1, TMR do
        local color= "black"
        if i == LastHit then
            color= "green"
        end
        gui.drawbox(DISPx, DISPy-3 - 4*i,DISPx+7, DISPy-1 - 4*i,color)
    end

    local color= "black"
    if LastHit == 0 then
        color= "green"
    end
    gui.drawbox(DISPx  , DISPy  ,DISPx+7, DISPy+2,color)
    gui.drawbox(DISPx-2, DISPy-2,DISPx+9, DISPy+4,"white")

    for i= BND, -1 do
        local color= "black"
        if i == LastHit then
            color= "green"
        end
        gui.drawbox(DISPx, DISPy+3 - 4*i,DISPx+7, DISPy+5 - 4*i,color)
    end

    LastHit= LastHit-1
    FCEU.frameadvance()
    lastEHP= EnemyHP
end