SCRIPT_TITLE	= "Gradius - Bullet Hell"
SCRIPT_VERSION	= "1.0"

require "m_utils"
m_require("m_utils",0)

--[[
Gradius - Bullet Hell
version 1.0 by miau

Visit http://morphcat.de/lua/ for the most recent version and other scripts.


Controls
	Press select to fire a bomb that will destroy all enemy bullets and grant you
	invincibility for a short period of time.

Supported roms
	Gradius (J), Gradius (U), Gradius (E)

Known bugs
	- dying from blue bullets doesn't trigger the death sound effect
-]]

--configurable vars
local BOMBS_PER_LIFE = 5
local HITBOX = {-2, 4, 9, 9} --vic viper's hit box for collision detection with blue bullets
--------------------------------------------------------------------------------------------



local MAX_EXSPR = 64
local timer = 0
local spr = {}
local exspr = {}
local exsprdata = {}
local deathtimer = 0
local bombtimer = 0
local paused = 0
local bombs = BOMBS_PER_LIFE
local bulletimg = ""


function makebinstr(t)
	local str = ""
	for i,v in ipairs(t) do
		str = str..string.char(v)
	end
	return str
end

function initialize()
	--Transparency doesn't seem to work for gd images, bummer!
	bulletimg = makebinstr( {
			0xff, 0xfe,    0,  0x6,    0,  0x6,  0x1, 0xff,
			0xff, 0xff, 0xff,    0,    0,    0,    0,    0,
			   0,    0,    0,    0,  0x2, 0x33, 0x6e,    0,
			 0x2, 0x33, 0x6e,    0,    0,    0,    0,    0,
			   0,    0,    0,    0,    0,    0,    0,    0,
			 0x2, 0x33, 0x6e,    0, 0x4d, 0xb6, 0xff,    0,
			0xdb, 0xff, 0xfc,    0,  0x2, 0x33, 0x6e,    0,
			   0,    0,    0,    0,  0x2, 0x33, 0x6e,    0,
			0x4d, 0xb6, 0xff,    0, 0x4d, 0xb6, 0xff,    0,
			0xdb, 0xff, 0xfc,    0, 0xdb, 0xff, 0xfc,    0,
			 0x2, 0x33, 0x6e,    0,  0x2, 0x33, 0x6e,    0,
			0xdb, 0xff, 0xfc,    0, 0xdb, 0xff, 0xfc,    0,
			0x4d, 0xb6, 0xff,    0, 0x4d, 0xb6, 0xff,    0,
			 0x2, 0x33, 0x6e,    0,    0,    0,    0,    0,
			 0x2, 0x33, 0x6e,    0, 0xdb, 0xff, 0xfc,    0,
			0x4d, 0xb6, 0xff,    0,  0x2, 0x33, 0x6e,    0,
			   0,    0,    0,    0,    0,    0,    0,    0,
			   0,    0,    0,    0,  0x2, 0x33, 0x6e,    0,
			 0x2, 0x33, 0x6e,    0,    0,    0,    0,    0,
			   0,    0,    0 } )

	for i=0,31 do
		spr[i] = {
					status=0,
					id=0,
					x=0,
					y=0,
					timer=-1,
				}
	end
end


function createexsprite(s)
	for i=0,MAX_EXSPR-1 do
		if(exspr[i]==nil) then
			exspr[i]=s
			if(exspr[i].id==nil) then
				exspr[i].id=0
			end
			if(exspr[i].x==nil) then
				exspr[i].x=0
			end
			if(exspr[i].y==nil) then
				exspr[i].y=0
			end
			if(exspr[i].vx==nil) then
				exspr[i].vx=0
			end
			if(exspr[i].vy==nil) then
				exspr[i].vy=0
			end
			if(exspr[i].timer==nil) then
				exspr[i].timer=0
			end
			if(exspr[i].ai==nil) then
				exspr[i].ai=""
			end
			return exspr[i]
		end
	end
	return nil
end

function destroyallexsprites()
	exspr = {}
end

function destroyexsprite(i)
	exspr[i] = nil
end

function destroysprite(i)
	memory.writebyte(0x100+i,0x00)
	memory.writebyte(0x120+i,0x00)
	memory.writebyte(0x300+i,0x00)
end


function drawexsprite(id,x,y)
	gui.gdoverlay(x, y, bulletimg)
end


function getvicdistance(x,y)
	return getdistance(vic.x+4,vic.y+8,x,y)
end

function doexspriteai(s)
	if(s.ai=="Stray") then
		if(s.timer==0) then
			s.vx = -3
			s.vy = AND(timer,0x0F) / 8 - 1
		end
	elseif(s.ai=="AimOnce") then
		if(s.timer==0) then
			s.vx,s.vy = getvdir(s.x,s.y, vic.x,vic.y)
			s.vx = s.vx * 2
			s.vy = s.vy * 2
			--s.vy = AND(timer,0x0F) / 8 - 1
		end
	elseif(s.ai=="AimDelayed") then
		if(s.timer>=30 and s.timer<=33) then
			local x,y = getvdir(s.x,s.y, vic.x,vic.y)
			s.vx = (x+s.vx)
			s.vy = (y+s.vy)
		end
	elseif(s.ai=="AimRight") then
		if(s.timer==0) then
			s.vx,s.vy = getvdir(s.x,s.y, vic.x,vic.y)
			s.vx = s.vx * 2.5
			s.vy = s.vy * 1.5
		end
	elseif(s.ai=="AimLeft") then
		if(s.timer==0) then
			s.vx,s.vy = getvdir(s.x,s.y, vic.x,vic.y)
			s.vx = s.vx * 1.5
			s.vy = s.vy * 2.5
		end
	elseif(s.ai=="BossPattern1") then
		if(s.timer==0) then
			s.vx = math.random(-100,0)/50
			s.vy = math.random(-100,0)/70
		elseif(s.timer>=30 and s.timer <=35) then
			s.vx=s.vx/1.1
			s.vy=s.vy/1.1
		elseif(s.timer==70) then
			local x,y = getvdir(s.x,s.y, vic.x,vic.y)
			s.vx = x*3
			s.vy = y*3
		end
	elseif(s.ai=="LimitedLifeSpan") then
		if(s.timer>120) then
			s.x=999 --results in death
		end
	end
	
	if(s.timer>600) then --delete extended sprites after 10 seconds in case one gets stuck on screen
		s.x=999
	end
end

function doboss1ai(s)
	if(s.timer>80 and s.timer<320 and AND(s.timer,63)==0) then
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
		createexsprite({x=s.x,y=s.y,ai="BossPattern1"})
	end
	if(s.timer>320 and s.timer<520) then --pattern2 a
		if(math.mod(s.timer,27)<=9 and AND(s.timer,3)==3) then
			createexsprite({x=s.x,y=s.y,vx=-0.2,ai="AimDelayed"})
		end
	end
	if(s.timer>320 and s.timer<520) then --pattern2 b
		if(AND(s.timer,63)==0) then
			createexsprite({x=s.x,y=s.y,vx=-0.7,vy=-1.6})
			createexsprite({x=s.x,y=s.y,vx=-1.1,vy=-0.8})
			createexsprite({x=s.x,y=s.y,vx=-1.2,vy=-0})
			createexsprite({x=s.x,y=s.y,vx=-1.1,vy=0.8})
			createexsprite({x=s.x,y=s.y,vx=-0.7,vy=1.6})
		end
	elseif(s.timer>520) then
		s.timer = 60
	end
end


function dospriteai(s)
	if(s.id==0x85) then --Fan
		if(s.timer==30) then
			createexsprite({x=s.x,y=s.y,ai="Stray"})
		end
	elseif(s.id==0x86) then --Jumper
		if(AND(s.timer,127)==40) then
			createexsprite({x=s.x,y=s.y,vx=-1.5,vy=-0.5})
			createexsprite({x=s.x,y=s.y,vx=-1,vy=-1.1})
			createexsprite({x=s.x,y=s.y,vx=0,vy=-1.4})
			createexsprite({x=s.x,y=s.y,vx=1,vy=-1.1})
			createexsprite({x=s.x,y=s.y,vx=1.5,vy=-0.5})
		end
	elseif(s.id==0x88) then --Rugul
		if(AND(s.timer,63)==30) then
			createexsprite({x=s.x,y=s.y,ai="Stray"})
		end
	elseif(s.id==0x84 or s.id==0x9C) then --Fose, Uska(?)
		if(AND(s.timer,63)==30) then
			createexsprite({x=s.x,y=s.y,vx=0,vy=-2})
		elseif(AND(s.timer,63)==60) then
			createexsprite({x=s.x,y=s.y,vx=0,vy=2})
		end
	elseif(s.id==0x89 or s.id==0x8C) then --Rush
		if(AND(timer,63)==5) then
			createexsprite({x=s.x,y=s.y,vx=-1,vy=-1})
		end
	elseif(s.id==0x96) then --Moai
	elseif(s.id==0x97) then --Mother and Child
		if(AND(timer,7)==0) then
			local t = s.timer/15
			createexsprite({x=s.x+16,y=s.y+16,vx=math.sin(t)-1,vy=math.cos(t),ai="LimitedLifeSpan"})
		end
	elseif(s.id==0x8B) then --Zabu
		if(s.timer==5) then
			createexsprite({x=s.x,y=s.y,vx=0.5,ai="AimDelayed"})
		end
	elseif(s.id==0x92 or s.id==0x93 or s.id==0x87 or s.id==0x91) then --Dee-01, Ducker
		--if(s.timer==5) then
		local t = AND(s.timer,127)
		if(t>60 and t<79 and AND(s.timer,3)==3) then
			--createexsprite({x=s.x,y=s.y,ai="AimOnce"})
			createexsprite({x=s.x,y=s.y,ai="AimOnce"})
		end
	elseif(s.id==0x98) then --boss...
		doboss1ai(s)
	end
end


function updatevars()
	paused = memory.readbyte(0x0015)
	if(paused==1) then
		return
	end
	--load original sprites from ram
	for i=0,31 do
		--if(i==0 or memory.readbyte(0x0300+i)~=0) then
			if(memory.readbyte(0x0300+i)~=spr[i].id) then
				spr[i].timer = 0
			end
			spr[i].status=memory.readbyte(0x0100+i) --1=alive, 2=dead
			spr[i].id=memory.readbyte(0x0300+i)
			spr[i].x=memory.readbyte(0x0360+i)
			spr[i].y=memory.readbyte(0x0320+i)
			spr[i].timer=spr[i].timer+1
			if(spr[i].id==0) then
				spr[i].timer = 0
			else
				dospriteai(spr[i])
			end
			
			if(i>3 and bombtimer>0 and getvicdistance(spr[i].x+4,spr[i].y+4)<30) then
				if(spr[i].id~=0x29 and spr[i].id~=0x01 and spr[i].id~=0x99 and spr[i].id~=0x98 and spr[i].id~=0x94 and spr[i].id~=0x97 and spr[i].id~=0x96 and spr[i].id~=0x1E) then
				--excluded: hidden bonus (0x29), power ups and moai bullets (0x01), boss (0x99+0x98), tentacle (0x94), mother (0x97), moai (0x96), gate (0x1E)
					destroysprite(i)
				end
			end
		--end
	end
	vic = spr[0]
	

	
	
	if(deathtimer>0) then
		if(deathtimer==120) then
			destroyallexsprites()
			deathtimer = 0
			bombs = BOMBS_PER_LIFE
		else
			if(deathtimer==1) then
			--this is part of what gradius does to kill vic viper...
			--faster and without the annoying sound effect. who cares!
			memory.writebyte(0x4C,0x78)
			memory.writebyte(0x0100,0x02) --status = dead
			memory.writebyte(0x0160,0x00)
			memory.writebyte(0x0140,0x00)
			memory.writebyte(0x1B,0xA0)
			memory.writebyte(0x0120,0x2D) --chr?--
			end
			deathtimer = deathtimer + 1
		end
	end
	

	local jp = joypad.get(1)
	if(jp.select and jp.select~=last_select and bombs>0) then
		bombtimer = 1
		bombs = bombs - 1
		destroyallexsprites()
		--destroy bullets
		for i=1,31 do
			if(spr[i].id<4 and spr[i].id~=1) then
				destroysprite(i)
			end
		end
	end
	last_select = jp.select

	
	if(last_vic_status~=vic.status) then
		if(vic.status==2) then --vic died in the original game, start death timer to destroy all extended sprites
			deathtimer = 1
		end
	end
	last_vic_status = vic.status
	
	--calculations on extended sprites
	for i=0,MAX_EXSPR-1 do
		if(exspr[i]~=nil) then
			if(bombtimer>0 and getvicdistance(exspr[i].x+4,exspr[i].y+4)<25) then
				destroyexsprite(i)
			else
			doexspriteai(exspr[i])
			exspr[i].timer = exspr[i].timer + 1
			exspr[i].x=exspr[i].x+exspr[i].vx
			exspr[i].y=exspr[i].y+exspr[i].vy
			if(exspr[i].x>255 or exspr[i].x<0 or exspr[i].y>255 or exspr[i].y<0) then
				--destroy exsprite
				exspr[i]=nil
				break
			end
				--collision check with vic viper
				if(deathtimer==0 and vic.status==1 and exspr[i].x>=vic.x+HITBOX[1] and exspr[i].x<=vic.x+HITBOX[3]
					and exspr[i].y>=vic.y+HITBOX[2] and exspr[i].y<=vic.y+HITBOX[4]) then
					deathtimer = 1
				end
			end
		end
	end
	
	
end

function render()
	--bcbox(vic.x-2, vic.y+4, vic.x+9, vic.y+9, "#ffffff")
	
	gui.text(0, 8, string.format("bombs %d",bombs));
	--gui.text(0, 8, string.format("Lives %d",memory.readbyte(0x0020)));
	--gui.text(0, 28, string.format("bombtimer %d",bombtimer));
	
	--[[for i=1,31 do
		if(spr[i].id~=0) then
			gui.text(spr[i].x, spr[i].y, string.format("%X",spr[i].id));
		end
	end--]]
	
	for i=0,MAX_EXSPR-1 do
		if(exspr[i]~=nil) then
			drawexsprite(exspr[i].id,exspr[i].x,exspr[i].y)
		end
	end
	
	
	if(bombtimer>0) then
		if(bombtimer<12) then
			memory.writebyte(0x11,0xFF) --monochrome screen
		else
			memory.writebyte(0x11,0x1E)
		end
		bombtimer = bombtimer + 1
		for i=0,64 do
			local x = vic.x+4+math.sin(i)*bombtimer*4 + math.random(0,12)
			local y = vic.y+8+math.cos(i)*bombtimer*4 + math.random(0,12)
			local c = 255-bombtimer
			local cs = string.format("#%02x%02x00",c,c)
			bcpixel(x,y,cs)
			bcpixel(x-1,y,cs)
			bcpixel(x+1,y,cs)
			bcpixel(x,y+1,cs)
			bcpixel(x,y-1,cs)
			
			x = vic.x+4+math.sin(i)*(20+math.sin(bombtimer/5))
			y = vic.y+8+math.cos(i)*(20+math.sin(bombtimer/5))
			bcpixel(x,y,cs)
			bcpixel(x-1,y,cs)
			bcpixel(x,y-1,cs)
		end
		if(bombtimer==180) then
			bombtimer=0
		end
	end
end


initialize()
vic = spr[0]
while(true) do
	updatevars()
	render()
	EMU.frameadvance()
	timer = timer + 1
end
