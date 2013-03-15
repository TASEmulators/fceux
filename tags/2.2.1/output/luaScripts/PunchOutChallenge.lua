--Mike Tyson's Punch Out!!
--Intended to Challenge the PROs!
--adelikat

local NumStars = 0x0342
local UntilStar = 0x0347		--Number of punches to land until Mac can get a star
local NumHearts   = 0x0321	--Number of hearts, single digit
local NumHearts10 = 0x0322	--Number of hearts, 10's digit
local CurrentRoundA = 0x0006 --Address for which round we are in
local Round	 = 0				 --Stores which round we are on
local OppMove = 0x0090			 --What move the opponent is making

local DizzyFlag = 0x00BA  -- If Opponent will be insta-knocked down by any punch
local StarKOFlag = 0x03CB -- If OPponent will be insta-knocked down by a star
local KaiserKOFlag = 0x004B -- Setting this to 0 disables his ability to be stunned, thus you can't KO Star him

local OppDodgeStar = 0x0348 -- Counter for how many unstunned stars before the opponent starts to dodge

local OpponentIDA = 0x0031
local OpponentID = 0 --Stores contents of OpponentIDA

--Oppenent ID Table
--Round 1, R2, R3
	--36,47,64, 66	Glass Joe	47 = After round 1 Jump back begins
	--114,116,118,120=R1	Von Kaiser
	--220	Piston Honda I
	--225	Don I
	--52,62,66 	King Hippo
	--226,241	Great Tiger
	--207	Bald Bull
	--23	Piston Honda
	--71,87,95	Soda Popinski
	--111	Bald Bull II
	--19	Don II
	--231	Sandman
	--105	Super Macho Man
	--34	Mike Tyson (Mr. Dream)

	
local AltOppIDAddress = 0x0330 --Alternate flag for determining the opponent
--0 = Kaiser R2, PistonI R1 R2 R3
--1 = Tiger R1
--2 = Don I R2, Don I R3
--3 = Don I R1
--4 = Tyson R1
--5 = Glass Joe R3, Von R1, Von R3, Soda R1
--6 = Piston II R1
--7 = Bull I R1, BullI R1, Sandman R1, Macho R1
--9 = Glass R1, Glass R2, KHippo RI & Don II









local OppIDAlt	--stores the contents of 0x0330
	
local EHP = 0x0398  -- Enemy HP address
local EHPx= 178
local EHPy= 14
local EnemyHP = 0
local lastEHP = 0
local EMod = 0	--Amount that the opponents health will be modded by at the end of a frame

local MHP = 0x0391 -- Mac HP address
local MHPx = 122
local MHPy = 14
local MacHP = 0
local lastMHP = 0

local OppKnockedDown = 0x0005 -- Oppoenent is on the canvas flag
local OppDown		 -- Stores contents of 0x0005
local OppDx = 130
local OppDy = 70
local OppWillGetUpWith = 0x039E -- Health that the oppoenent will get up with
local OppWillGet			  -- Stores contents of 0x039E

local OppHitFlag = 0x03E0
local OppHit
local OppHitTimer = 0
local OppHitToDisplay = 0
local OppJustHit = false

local joy = {}
joy.select = 1

OHitValuex = 100
OHitValuey = 100

--*****************************************************************************
function IsOppDown()
--*****************************************************************************
	OppDown = memory.readbyte(OppKnockedDown)
	if OppDown > 0 then
		return true
	end
      return false
end

--*****************************************************************************
function OppIsHit()
--*****************************************************************************
      OppHit = memory.readbyte(OppHitFlag)
	if OppHit > 0 then
		return true
	end
      return false
end



--*****************************************************************************
while true do
--*****************************************************************************
    Timer1 = memory.readbyte(0x0302)		--Keep track of the clock in the game
    Timer2 = memory.readbyte(0x0304)
    Timer3 = memory.readbyte(0x0305)
    Timer4 = memory.readbyte(0x0306)
    Timer5 = memory.readbyte(0x0307)	

    EnemyHP = memory.readbyte(EHP)
    gui.text(EHPx,EHPy,EnemyHP)

    MacHP = memory.readbyte(MHP)
    gui.text(MHPx,MHPy,MacHP)

    OpponentID = memory.readbyte(OpponentIDA)
    OppIDAlt = memory.readbyte(AltOppIDAddress)
    OppWhichMove = memory.readbyte(OppMove)
    
    Round = memory.readbyte(CurrentRoundA)
    
    
    
    --***************************************
    --Display how much health the opponent will get up with

    if IsOppDown() then
	    OppWillGet = memory.readbyte(OppWillGetUpWith)
	    gui.text(OppDx, OppDy, OppWillGet)
	    gui.text(OppDx+16,OppDy, "Next health")

	    --Unfair opponent health boosts!
	    --This will affect the oppenent both on his knock downs and Mac's :)
          if OppWillGet >= 88 then
	    memory.writebyte(OppWillGetUpWith, 128)
	    end
	    if OppWillGet > 64 then
	    memory.writebyte(OppWillGetUpWith, 96)
	    end
	    if OppWillGet > 9 then
	    memory.writebyte(OppWillGetUpWith, 64)
	    end
	    
	    if OppWillGet and EnemyHP < 64 and EnemyHP > 0 then
	    memory.writebyte(OppWillGetUpWith, 64)
	    end

	    if OppWillGet and EnemyHP < 9 and EnemyHP > 0 then
	    memory.writebyte(OppWillGetUpWith, 128)	--Now that's just rude.
	    end
    end
    --***************************************

    --***************************************	
    --Get the amount Mac damaged the opponent
    if OppIsHit() then
	    OppHitToDisplay = lastEHP - EnemyHP
	    OppHitTimer = 60
	    OppJustHit = true
    else  OppJustHit = false
    end
    --***************************************

    --***************************************
    --Display damage amount
    if OppHitTimer > 0 then
	    gui.text(OHitValuex, OHitValuey, OppHitToDisplay)
    end
    
    if OppHitTimer > 0 then
        OppHitTimer = OppHitTimer - 1
    end
    --***************************************
  

    --***************************************
    --Force the pressing of select between rounds!
    x = memory.readbyte(0x0004)
    if x == 1 then
	joypad.set(1, joy)
    end
    --***************************************

    
----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------  

    --***************************************
    --Glass Joe custom mods
    --***************************************
    if OpponentID == 36 or OpponentID == 47 or OpponentID == 64 then
		if OppJustHit and OppHitToDisplay < 5 and EnemyHP > 1 then
			EMod = EnemyHP + 3
		end
        
       if OppWhichMove == 25 then
			memory.writebyte(OppMove, 1)	--Remove his jump back and replace with a regular punch
       end
    end
    
    --***************************************
    --Von Kaiser custom mods
    --***************************************
    if OppIDAlt == 5 then
		gui.text(10,10,"Von Kaiser")
		memory.writebyte(KaiserKOFlag, 1)
		if Timer1 == 0 and Timer2 == 0 and Round == 1 then
			memory.writebyte(OppDodgeStar, 2)
		end
    end
    
    --***************************************
    --Piston Honda I custom mods
    --***************************************
    if (OppIDAlt == 0 and (Timer3 > 0 or Timer2 > 0 or Timer1 > 0)) then
		gui.text(10,10,"Piston Honda I")
		if Timer1 == 0 and Timer2 == 0 and Round == 1 then
			memory.writebyte(OppDodgeStar, 2)
		end
		
		if OppWhichMove == 25 then
		    memory.writebyte(OppMove, 1)	--Remove his jump back and replace with a regular punch
       end
    end
    
    --***************************************
    --Don Flamenco I custom mods
    --***************************************
    if OppIDAlt == 3 then
		gui.text(10,10,"Don Flamenco I")
		if Timer1 == 0 and Timer2 == 0 and Round == 1 then
			memory.writebyte(OppDodgeStar, 0)
		end
    end
    
    --***************************************
    --King Hippo custom mods
    --***************************************
    if OpponentID == 52 then	--Round 1 only
    	if Timer1 == 0 and Timer2 == 0 then
			EMod = 128
		end
	end
	if OpponentID == 52 or OpponentID == 62 or OpponentID == 66 then	--All rounds
		gui.text(10,10,"King Hippo")
		if OppJustHit and OppHitToDisplay < 4 and OppHitToDisplay > 0 and EnemyHP > 1 then
			EMod = EnemyHP + 2
		end
    end

    
    --***************************************
    --Great Tiger custom mods
    --***************************************
    if OppIDAlt == 3 then
    	gui.text(10,10,"Great Tiger")
    end
    if OpponentID == 230 then	--Round 1 Tiger punch only
        IsTigerDizzy = memory.readbyte(DizzyFlag)
	    if IsTigerDizzy > 0 then
		memory.writebyte(DizzyFlag, 0)
	    end
    end
    
    --***************************************
    --Bald Bull I Custom mods
    --***************************************
    
    --***************************************
    --Piston Honda II Custom mods
    --***************************************
    

    --***************************************
    --Soda Popinski Custom mods
    --***************************************
    if OpponentID == 71 or OpponentID == 87 or OpponentID == 95 then
    gui.text(10,10,"Soda Popinski")
      --Nullify the instant star knockdown, and punish mac for trying!
      Soda = memory.readbyte(StarKOFlag)	--TODO declare a variable instead of using 0x03CB (this is the soda instand knockdown flag)
	if Soda > 0 then
		
		memory.writebyte(0x03CB, 0)      --Return it back to 0
		memory.writebyte(NumStars, 0)    --Mac loses his stars
		memory.writebyte(UntilStar, 12)  --Mac won't get a star, and can't get a star in the next 10 punches!
		EMod = 128				  --Ouch!
      end
   
    end
    
    --***************************************
    --Bald Bull II Custom mods
    --***************************************
        
    --***************************************
    --Don Flamenco II Custom mods
    --***************************************
    
    --***************************************
    --Mr Sandman Custom mods
    --***************************************
    
    --***************************************
    --Super Macho Man Custom mods
    --***************************************
    
    --***************************************
    --Mike Tyson
    --***************************************
    
----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
    FCEU.frameadvance()
    if EMod > 0 then    
	memory.writebyte(EHP, EMod) 
	EMod = 0  
    end
    lastEHP = EnemyHP
    lastMHP = MacHP 
end
