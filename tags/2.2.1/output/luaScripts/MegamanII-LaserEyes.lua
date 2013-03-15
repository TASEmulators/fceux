SCRIPT_TITLE	= "Mega Man 2 LASER EYES"
SCRIPT_VERSION	= "1.0"

require "m_utils"
m_require("m_utils",0)

--[[
Mega Man 2 LASER EYES
version 1.0 by miau

Visit http://morphcat.de/lua/ for the most recent version and other scripts.


Controls
	Press select to fire a laser from Mega Man's eyes aimed at the closest object.
	Left-click to drag and drop objects.
	Middle-clicking instantly moves Mega Man to the cursor position.

Supported roms
	Rockman 2 - Dr Wily no Nazo (J), Mega Man 2 (U), Mega Man 2 (E)

Known bugs
- shooting lasers at bosses is glitchy, do it at your own risk
- eyes might turn black for a short period of time after screen transition
- some objects have duplicate IDs which will result in wrong names being shown for them

-]]

--configurable vars
local show_info = false --show sprite info on startup
local show_cursor = false --set this to true when recording a video

--sound effects to use for various script events, nil to disable
local SFX_LASER = 0x25
local SFX_EXPLOSION = 0x2B
local SFX_TARGET = nil --0x39 --0x21
--------------------------------------------------------------------------------------------
--List of sound effects
--0x21 press
--0x22 peeewwwwwww
--0x23 metal blade
--0x24 mega buster
--0x25 sniper armor shot?
--0x26 hit1
--0x27 air shooter
--0x28 energy
--0x29 touch ground
--0x2A wily thing
--0x2B destroy1
--0x2C ??
--0x2D reflected shot
--0x2E ?? (metal blade-like noise channel usage, short)
--0x2F menu selector
--0x30 mega man appear
--0x31 leaf shield
--0x32 menu open
--0x33 ?? (short, low noise)
--0x34 gate open
--0x35 atomic fire charge 1
--0x36 atomic fire charge 2
--0x37 atomic fire charge 2
--0x38 atomic fire shot ?
--0x39 prop-top leap
--0x3A mega man disappearing
--0x3B diving into water
--0x3C appearing platform
--0x3D wily stage lava drops
--0x3E wily stage lava drops
--0x3F ?? (similar to 0x27)
--0x40 ?? (short noise)
--0x41 death
--0x42 e-tank/1up
--0x43 ?? (short noise in fixed intervals)






local sprdb = {} --enemy names source: http://megaman.wikia.com
sprdb[0x00] = {name="Squirt"}
sprdb[0x01] = {name="Squirt"} --spawned by Lantern Fish
sprdb[0x04] = {name="M-445"}
sprdb[0x07] = {name="Snapper Controller",invincible=true}
sprdb[0x08] = {name="Snapper"}
sprdb[0x0A] = {name="Crabbot"}
sprdb[0x0C] = {name="Croaker"}
sprdb[0x0D] = {name="Croaker (tiny)"}
sprdb[0x0E] = {name="",invincible=true} --bubble...underwater mega man
sprdb[0x0F] = {name="Lantern Fish"}
sprdb[0x13] = {name="Platform"}
sprdb[0x15] = {name="Beam"}
sprdb[0x16] = {name="Bubble Bat"}
sprdb[0x17] = {name="Robo-Rabbit"}
sprdb[0x18] = {name="Carrot"} --or mega man being hit
sprdb[0x1D] = {name="Mecha Monkey"}
sprdb[0x1E] = {name="Atomic Chicken Controller",invincible=true}
sprdb[0x1F] = {name="Atomic Chicken"}
--sprdb[0x1B] = {name="Climbing Mega Man"} --or hot dog fire
--sprdb[0x1C] = {name=""} --or hot dog fire controller?
sprdb[0x21] = {name="Telly Spawn Point",invincible=true}
sprdb[0x22] = {name="Telly"}
sprdb[0x23] = {name="Hothead"} --or Mega Buster
sprdb[0x24] = {name="Tackle Fire"}
--sprdb[0x27] = {name="Light Control"}
--sprdb[0x28] = {name="Light Control"}
sprdb[0x29] = {name="Cogwheel"}
sprdb[0x2A] = {name="Pierobot"}
sprdb[0x2B] = {name="Prop-Top Controller",invincible=true}
sprdb[0x2C] = {name="Prop-Top"}
sprdb[0x2E] = {name="Hot Dog"}
sprdb[0x2F] = {name="Gate"}
sprdb[0x30] = {name="Press"}
sprdb[0x31] = {name="Blocky"}
sprdb[0x32] = {name="Big Fish Controller",invincible=true}
--sprdb[0x33] = {name=""} --Bubble Lead
sprdb[0x34] = {name="Neo Met"}
sprdb[0x35] = {name="Bullet"} --Sniper Armor/Sniper Joe
sprdb[0x36] = {name="Fan Fiend"} --or Metal Blade
sprdb[0x37] = {name="Pipi Controller",invincible=true} --or Flash weapon (Time Stopper) controller(?)
sprdb[0x38] = {name="Pipi"}
sprdb[0x3A] = {name="Pipi Egg"} --or Item-3(?)
sprdb[0x3B] = {name="Pipi Egg Shell"}
sprdb[0x3C] = {name="Child Pipi"}
sprdb[0x3D] = {name="Lightning Lord"}
sprdb[0x3E] = {name="Thunder Chariot"}
--sprdb[0x3F] = {name="Lightning Bolt"} --or "splash"... (mega man diving into water)
sprdb[0x3F] = {invincible=true}
sprdb[0x40] = {name="Air Tikki"}
sprdb[0x41] = sprdb[0x40]
sprdb[0x44] = {name="Air Tikki Horn"}
sprdb[0x45] = {name="Gremlin"}
sprdb[0x46] = {name="Spring Head"}
sprdb[0x47] = {name="Mole Controller",invincible=true}
sprdb[0x48] = {name="Mole (rising)"}
sprdb[0x49] = {name="Mole (falling)"}
sprdb[0x4B] = {name="Crazy Cannon"}
sprdb[0x4D] = {name="Bullet (Crazy Cannon)"}
sprdb[0x4E] = {name="Sniper Armor"}
sprdb[0x4F] = {name="Sniper Joe"}
sprdb[0x50] = {name="Squirm"}
sprdb[0x51] = {name="Squirm Pipe"}

sprdb[0x5C] = {name="Metal Blade"}
sprdb[0x5D] = {name="Air Shooter"}
sprdb[0x5E] = {name="Crash Bomb"}

--[[sprdb[0x60] = {name="Bubble Man"}
sprdb[0x61] = sprdb[0x60]
sprdb[0x62] = sprdb[0x60]
sprdb[0x63] = {name="Metal Man"} --or Falling Blocks (Dragon) (id2:80)
sprdb[0x64] = sprdb[0x63] 	--or Falling Blocks (Dragon).. Dragon Controller? (id2:80)
sprdb[0x65] = sprdb[0x63]	--or Dragon Body (id2:8B)
sprdb[0x66] = {name="Air Man"} --or Dragon Bottom (id2:8B)
sprdb[0x67] = sprdb[0x66]
sprdb[0x68] = sprdb[0x66]
sprdb[0x69] = {name="Crash Man"}
sprdb[0x6A] = sprdb[0x69] --running or wily boss no.2 (id2:8B/CB/83/C3/80)
sprdb[0x6B] = sprdb[0x69] --jumping
sprdb[0x70] = {name="Dragon"} --(id2:8B)
sprdb[0x71] = {name="Big Fish"} --Wily Boss 3 (id2:83)
--]]

sprdb[0x76] = {name="Large Life Energy",invincible=true}
sprdb[0x77] = {name="Small Life Energy",invincible=true}
sprdb[0x78] = {name="Large Weapon Energy",invincible=true}
sprdb[0x79] = {name="Small Weapon Energy",invincible=true}
sprdb[0x7A] = {name="E-Tank",invincible=true}
sprdb[0x7B] = {name="1UP",invincible=true}


local eyes = {}
eyes.open = {
	{0,0,0,0,0,1,1,0,1,0,0,0,0},
	{0,0,0,0,0,1,1,0,1,0,0,0,0},
}
eyes.running = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,1,1,0,1,0,0,0,0},
	{0,0,0,0,0,1,1,0,1,0,0,0,0},
}
eyes.shooting = {
	{0,0,0,0,0,0,0,0,0,1,1,0,1},
	{0,0,0,0,0,0,0,0,0,1,1,0,1},
}
eyes.ladder = {
	{0,0,0,0,0,0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,1,1,0,0,0,0},
}
eyes.closed = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,1,1,1,0,1,1,0,0,0},
}
eyes.none = {}

local mmeyes = eyes.open
local timer = 0
local spr = {}
local last_inp = {}
local inp = {}
local laser = {timer=0}
local last_laser = 0



--very limited and hacky particle engine following
local P_MAX = 150
local particle = {p={}}

function particle.laserai(pi)
	local function checkcollision(px,py,rx,ry)
		if(px>=rx-8 and px<=rx+8 and py>=ry-8 and py<=ry+8) then
			return true
		else
			return false
		end
	end
	
	for i=1,31 do
		if(spr[i].a and is_enemy(i)) then
			local px = particle.p[pi].x
			local py = particle.p[pi].y
			local px2 = particle.p[pi].x+particle.p[pi].vx*6
			local py2 = particle.p[pi].y+particle.p[pi].vy*6
			local rx = spr[i].x+spr[i].hx*256
			local ry = spr[i].y
			if(checkcollision(px,py,rx,ry) or checkcollision(px2,py2,rx,ry)) then
				--if(spr[i].hp>5) then
				--	memory.writebyte(0x06C0+i,spr[i].hp-5)
				--else
					memory.writebyte(0x06C0+i,1)
					play_sfx(SFX_EXPLOSION)
					particle.explosion(spr[i].x+spr[i].hx*256,spr[i].y)
					destroy_sprite(i)
				--end
			end
		end
	end
end


function particle.create(new)
	for i=0,P_MAX-1 do
		if(particle.p[i]==nil) then
			particle.p[i]=new
			if(particle.p[i].x==nil) then
				particle.p[i].x=0
			end
			if(particle.p[i].y==nil) then
				particle.p[i].y=0
			end
			if(particle.p[i].vx==nil) then
				particle.p[i].vx=0
			end
			if(particle.p[i].vy==nil) then
				particle.p[i].vy=0
			end
			if(particle.p[i].timer==nil) then
				particle.p[i].timer=0
			end
			if(particle.p[i].lspan==nil) then
				particle.p[i].lspan=60
			end
			if(particle.p[i].color==nil) then
				particle.p[i].color="#FFFFFF"
			end
			return i
		end
	end
	return nil
end

function particle.explosion(x,y) --faster than using particle.create
	local j = 0
	for i=0,P_MAX-1 do
		if(particle.p[i]==nil) then
			local vx = math.random(-50,50)/30
			local vy = math.random(-50,50)/30
			local color = "#FF0000"
			particle.p[i] = {}
			particle.p[i].x = x
			particle.p[i].y = y
			particle.p[i].vx = vx
			particle.p[i].vy = vy
			particle.p[i].color = color
			particle.p[i].lspan = math.random(20,40)
			particle.p[i].timer=0
			if(j==40) then
				return
			end
			j = j + 1
		end
	end
end

function particle.destroy(i)
	particle.p[i] = nil
end


function particle.update()
	for i=0,P_MAX-1 do
		if(particle.p[i]) then
			if(particle.p[i].timer >= particle.p[i].lspan) then
				particle.destroy(i)
			else
				if(particle.p[i].aifunc) then
					particle.p[i].aifunc(i)
				end
				if(particle.p[i]) then
					particle.p[i].x = particle.p[i].x + particle.p[i].vx
					particle.p[i].y = particle.p[i].y + particle.p[i].vy
					particle.p[i].timer = particle.p[i].timer + 1
				end
			end
		end
	end
end

function particle.render(xdisp,ydisp)
	local j=0
	for i=0,P_MAX-1 do
		if(particle.p[i]) then
			bcline2(particle.p[i].x-xdisp,particle.p[i].y-ydisp, particle.p[i].x-xdisp+particle.p[i].vx*6,particle.p[i].y+particle.p[i].vy*6-ydisp, particle.p[i].color)
			if(particle.p[i].aifunc) then
				bcline2(particle.p[i].x-xdisp,particle.p[i].y+1-ydisp, particle.p[i].x-xdisp+particle.p[i].vx*6,particle.p[i].y+particle.p[i].vy*6+1-ydisp, particle.p[i].color)
			end
			j = j + 1
		end
	end
end




function initialize()
	for i=0,31 do
		spr[i] = {
					a=0,
					id=0,
					id2=0,
					x=0,
					y=0,
					hp=0,
					--timer=-1,
				}
	end
end


function unselect_all()
	for i=0,31 do
		spr[i].selected=false
		spr[i].moving = false
	end
end

function getx16(s)
	return s.hx*256+s.x
end

function get_target()
	--closest target
	local closest = nil
	local dmin = 9999
	for i=1,31 do
		if(spr[i].a and is_enemy(i) and spr[i].seltimer>=10 and 
			(  (spr[i].sx>=megaman.sx and megaman.dir==1)
			or (spr[i].sx<=megaman.sx and megaman.dir==-1)
			or megaman.dir==0)) then
			local d = getdistance(megaman.sx,megaman.sy,spr[i].sx,spr[i].sy)
			if(d<dmin) then
				dmin = d
				closest = i
			end
		end
	end
	return closest
end


function destroy_sprite(i)
	memory.writebyte(0x0400+i,0)
	memory.writebyte(0x0420+i,0)
	spr[i]={}
end

function is_enemy(i)
	if(i==0) then
		return false
	end
	if(spr[i].a) then
		local id = spr[i].id
		if(sprdb[id]) then
			if(sprdb[id].invincible) then
				return false
			end
		end
	end
	return true
end

function play_sfx(n)
	if(n) then
		memory.writebyte(0x66,0x01)
		memory.writebyte(0x0580,n)
	end
end

function update_vars()
	--paused = memory.readbyte(0x0015)
	--if(paused==1) then
	--	return
	--end
	
	scroll_x = memory.readbyte(0x1F)
	scroll_hx = memory.readbyte(0x20)
	
	particle.update()
	
	inp = input.get()
	
	--load original sprites from ram
	for i=0,31 do
		--if(i==0 or memory.readbyte(0x0300+i)~=0) then
			if(memory.readbyte(0x0300+i)~=spr[i].id) then
				spr[i].timer = 0
			end
			spr[i].id=memory.readbyte(0x0400+i)
			spr[i].id2=memory.readbyte(0x0420+i)
			spr[i].hx=memory.readbyte(0x0440+i)
			spr[i].x=memory.readbyte(0x0460+i)
			spr[i].y=memory.readbyte(0x04A0+i)
			spr[i].sx=AND(spr[i].x+255-scroll_x,255) --screen position
			spr[i].sy=spr[i].y
			spr[i].hp=memory.readbyte(0x06C0+i)
			spr[i].a=(memory.readbyte(0x0420+i)>=0x80)
			if(spr[i].a==false) then
				spr[i].seltimer=nil
			end
			
			if(spr[i].a) then
				if(inp.xmouse>=spr[i].sx-8 and inp.xmouse<=spr[i].sx+8 and inp.ymouse>=spr[i].sy-8 and inp.ymouse<=spr[i].sy+8) then
					spr[i].hover = true
					spr[i].clicked = inp.leftclick
					if(inp.leftclick and last_inp.leftclick==nil) then
						unselect_all()
						spr[i].selected=true
					end
				else
					spr[i].hover = false
					if(inp.leftclick==nil) then
						spr[i].clicked = false
					end
				end
				
				--auto-target
				if(is_enemy(i)) then
					if(spr[i].seltimer==nil) then
						spr[i].seltimer=1
					else
						spr[i].seltimer = spr[i].seltimer + 1
					end
				end
				
				if(spr[i].selected) then
					--[[if(inp.delete) then
						destroy_sprite(i)
					elseif(inp.pagedown) then
						memory.writebyte(0x0400+i,spr[i].id-1)
					elseif(inp.pageup) then
						memory.writebyte(0x0400+i,spr[i].id+1)
					end-]]
					
					if(spr[i].clicked) then
						if(inp.xmouse~=last_inp.xmouse or inp.ymouse~=last_inp.ymouse) then
							spr[i].moving = true
						end
					else
						spr[i].moving = false
					end
				end
				
				if(spr[i].moving) then
					local x = inp.xmouse+scroll_x
					memory.writebyte(0x0460+i,inp.xmouse+scroll_x)
					memory.writebyte(0x04A0+i,inp.ymouse)
					memory.writebyte(0x0440+i,scroll_hx+math.floor(x/256))
				end
				
			end
		--end
	end
	
	megaman = spr[0]
	if(megaman.id2==0x80) then
		megaman.dir=-1
	else
		megaman.dir=1
	end
	if(megaman.id==0x1B) then
		megaman.dir = 0
	end
	
	
	if(inp.middleclick) then --change mega man's coordinates
		local x = inp.xmouse+scroll_x
		memory.writebyte(0x0460,x)
		memory.writebyte(0x04A0,inp.ymouse)
		memory.writebyte(0x0440,scroll_hx+math.floor(x/256))
	end
	
	--LASER!!!
	jp = joypad.read(1)
	if(jp.select and (last_jp.select==nil or timer-last_laser>22)) then
		local t = get_target()
		local vx,vy
		play_sfx(SFX_LASER)
		if(t) then
			vx,vy = getvdir(megaman.sx,megaman.sy-3,spr[t].sx,spr[t].sy)
			vx = vx * 10
			vy = vy * 10
		else
			if(megaman.dir==0) then
				vx = 0
				vy = -10
			else
				vx = megaman.dir*10
				vy = 0
			end
		end
		last_laser = timer
		
		particle.create({x=getx16(megaman),y=megaman.y-3,vx=vx,vy=vy,color="#FF0000",aifunc=particle.laserai})
	end
	
	if(inp.xmouse<83 and inp.ymouse<18) then
		if(inp.leftclick and last_inp.leftclick==nil) then
			if(show_info) then
				show_info = false
			else
				show_info = true
			end
		end
	end
	
	last_jp = jp
	last_inp = inp
end

function render()
	
	if(show_info) then
		gui.text(0,8,"Sprite Info: ON")
	else
		gui.text(0,8,"Sprite Info: OFF")
	end
	if(inp.xmouse<83 and inp.ymouse<18) then
		if(show_info) then
			gui.drawbox(0,9,77,17,"red")
		else
			gui.drawbox(0,9,83,17,"red")
		end
	end

	particle.render(scroll_x+scroll_hx*256,0)
	
	--LASER EYES!!!
	--TODO: prevent play_sfx from playing sounds during title screen/stage select
	if(memory.readbyte(0x580) == 0x0D) then --title screen tune is being played
		megaman.sx=211
		megaman.sy=131
		megaman.dir = -1
		mmeyes = eyes.open
		
		--lasers during title screen
		if(AND(timer,7)==0 and math.random(0,1)==0) then
			--play_sfx(SFX_LASER)
			local vx,vy=getvdir(megaman.sx,megaman.sy-3,0,math.random(30,220))
			vx=vx*10
			vy=vy*10
			particle.create({x=megaman.sx+6,y=megaman.sy-3,vx=vx,vy=vy,color="#FF0000",aifunc=LaserAI})
			mmeyes = eyes.none
		end
	elseif(megaman.a) then --sprite active
		local f = memory.readbyte(0x06A0)*256 + memory.readbyte(0x0680)
		local frame = memory.readbyte(0x06A0)
		local ftimer = memory.readbyte(0x0680)
		if(memory.readbyte(0x69)~=0x0E) then --menu opened or "stage intro" tune
			mmeyes = eyes.none
		elseif(megaman.id==0x00)  then --BLINK! change occurs on next frame
			if(f >= 0xA01) then
				mmeyes = eyes.closed
			elseif(f>=0x001) then
				mmeyes = eyes.open
			end
		elseif(megaman.id==0x01 or megaman.id==0x03 or megaman.id==0x05 or megaman.id==0x07 or megaman.id==0x0B or megaman.id==0x11 or megaman.id==0x13) then
			mmeyes = eyes.shooting
		elseif(megaman.id==0x04 or megaman.id==0x0C or megaman.id==0x0D or megaman.id==0x10) then
			mmeyes = eyes.open
		elseif(megaman.id==0x08 or megaman.id==0x09 or megaman.id==0x14) then
			if((f>=0x201 and f<=0x300) or (f>=0x001 and f<=0x100)) then
				mmeyes = eyes.running
			else
				mmeyes = eyes.open
			end
		elseif(megaman.id==0x18) then --getting hurt
			mmeyes = eyes.closed
		elseif(megaman.id==0x1C or megaman.id==0x1E) then --mega buster, other weapons
			mmeyes = eyes.ladder
		else--if(megaman.id==0x1A or megaman.id==0x1B) then --respawning after death, climbing a ladder
			mmeyes = eyes.none
		end
		
		--if(megaman.hp==0) then
		--	mmeyes = eyes.none
		--end
	else
		mmeyes = eyes.none
	end
	
	if(mmeyes ~= eyes.none) then
		local eyecolor = "#FF0000"
		if(timer-last_laser<5) then
			eyecolor = "#FFFFFF"
		end
		if(megaman.dir==-1) then
			for y=1,4 do
				if(mmeyes[y]) then
					for x=1,14 do
						if(mmeyes[y][15-x]==1) then
							bcpixel(megaman.sx-9+x,megaman.sy-5+y,eyecolor)
						end
					end
				end
			end
		else
			for y=1,4 do
				if(mmeyes[y]) then
					for x=1,14 do
						if(mmeyes[y][x]==1) then
							bcpixel(megaman.sx-5+x,megaman.sy-5+y,eyecolor)
						end
					end
				end
			end
		end
	end

	
	for i=0,31 do
		if(spr[i].a) then
			local x,y
			local boxcolor
			if(spr[i].clicked) then
				boxcolor="#AA5500"
			elseif(spr[i].hover) then
				boxcolor="#FFCCAA"
			else
				boxcolor=nil
			end
			x = spr[i].sx
			y = spr[i].sy
			if(boxcolor) then
				bcbox(x-9,y-9,x+9,y+9,boxcolor)
			end
			
			if(spr[i].seltimer) then
				if(spr[i].seltimer<30) then
					local b=30-spr[i].seltimer
					local c = (30-spr[i].seltimer)*8
					bcbox(x-9-b,y-9-b,x+9+b,y+9+b,string.format("#CC%02X%02X",c,c))
					if(spr[i].seltimer==23) then
						play_sfx(SFX_TARGET)
					end
				elseif(spr[i].seltimer>90 or AND(timer,7)>2) then
					bcbox(x-9,y-9,x+9,y+9,"red")
				end
			end
			
			
			if(show_info and (spr[i].hover or spr[i].clicked or spr[i].seltimer)) then
				--bctext(AND(spr[i].x+255-scroll_x,255), spr[i].y, i);
				if(i==0) then
					bctext(x, y, "Blue Bomber")
					bctext(x, y+8, "("..spr[i].sx..","..spr[i].sy..")")
					bctext(x, y+16, "HP "..spr[i].hp)
				elseif(i>4) then
					if(sprdb[spr[i].id] and sprdb[spr[i].id].name --[[and i>4--]]) then
						bctext(x, y, sprdb[spr[i].id].name)
					else
						bctext(x, y, "("..spr[i].id..")")
					end
						bctext(x, y+8, "("..spr[i].sx..","..spr[i].sy..")")
					if(spr[i].hp~=0) then
						bctext(x, y+16, "HP "..spr[i].hp)
					end
				end
			end
		end
	end
	
	if(show_cursor) then
		local col2
		if(inp.leftclick) then
			col2 = "#FFAA00"
		elseif(inp.rightclick) then
			col2 = "#0099EE"
		elseif(inp.middleclick) then
			col2 = "#AACC00"
		else
			col2 = "white"
		end
		drawcursor(inp.xmouse,inp.ymouse,"black",col2)
	end
end



initialize()

while(true) do
	update_vars()
	render()
	EMU.frameadvance()
	timer = timer + 1
end


