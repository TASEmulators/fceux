SCRIPT_TITLE	= "Super Mario Bros. 3 Rainbow Riding"
SCRIPT_VERSION	= "0.1"

require "m_utils"
m_require("m_utils",0)

--[[
Super Mario Bros. 3 Rainbow Riding
version 0.1 by miau

Visit http://morphcat.de/lua/ for the most recent version and other scripts.

This script turns smb3 into a new game very similar to Kirby's Canvas Curse.
It's still incomplete, messy and full of bugs, so don't expect too much. Next version
will fix most of that... hopefully.
Probably slow on old computers, you may want to turn down emulator speed anyway to
decrease difficulty.

Controls
	Left-click on mario - jump
	Middle-click anywhere - run
	Draw vertical lines to make mario change his walking direction
	Draw horizontal lines to help mario move over obstacles

Supported roms
	Super Mario Bros 3 (J), Super Mario Bros 3 (U) (PRG 0), Super Mario Bros 3 (U) (PRG 1),
	Super Mario Bros 3 (E)

Known Bugs/TODO list
	Too many to list em all, actually...
	- game objects are occasionally catapulted out of screen - bad divisions!?
	- mario falling through lines (clean up collision detection)
	- long vertically scrolling levels won't work
	- boss battles are glitchy
	- disable auto move in mini games?
	- option to disable auto move?
	- make collision detection work with fire balls (fire mario or fire piranha plants)
	- improve map screen check
	- add easy mode: decrease mario's speed
	- improve enemy hit boxes
	- (add suicide button in case mario gets stuck)
-]]

--configurable vars
local show_cursor = false
--------------------------------------------------------------------------------------------



local Z_LSPAN = 400 --240
local Z_MAX = 128 --256 --maximum amount of lines on screen
local NUM_SPRITES = 20

local zbuf = {}
local zindex = 1
local timer = 0
local zprev = 0
local last_inp={}
local spr = {} --game's original sprites
local mario = {}
local clickbox={x1=-4,y1=4,x2=18,y2=32}
local collx = 0
local colly = 0
--local coll = {} --debug
local paintmeter = 0
local lastpaint = -100
local jp = {}


--accepts two tables containing these elements [1]=x1, [2]=y1, [3]=x2, [4]=y2
function get_line_intersection(a,b)
	local Asx,Asy,Bsx,Bsy,s,t
	local Ax1,Ay1,Ax2,Ay2
	local Bx1,By1,Bx2,By2
	Ax1 = a[1]
	Ay1 = a[2]
	Ax2 = a[3]
	Ay2 = a[4]
	Bx1 = b[1]
	By1 = b[2]
	Bx2 = b[3]
	By2 = b[4]
	Asx = Ax2 - Ax1
	Asy = Ay2 - Ay1
	Bsx = Bx2 - Bx1
	Bsy = By2 - By1
	
	s = (-Asy * (Ax1 - Bx1) + Asx * (Ay1 - By1)) / (-Bsx*Asy + Asx*Bsy)
	t = (Bsx * (Ay1 - By1) - Bsy * (Ax1 - Bx1)) / (-Bsx*Asy + Asx*Bsy)
	if(s>=0 and s<=1 and t>=0 and t<=1) then
		local Ix,Iy
		Ix = Ax1 + t * Asx
		Iy = Ay1 + t * Asy
		return Ix,Iy
	else
		return nil
	end
end

function close_to_line(Px,Py,a,range) --a[4] line segment
	local Ax=a[1]
	local Ay=a[2]
	local Bx=a[3]
	local By=a[4]
	local APx = Px - Ax
	local APy = Py - Ay
	local ABx = Bx - Ax
	local ABy = By - Ay
	local absq = ABx*ABx + ABy*ABy
	local apab = APx*ABx + APy*ABy
	local t = apab / absq
	
	if (t < 0.0) then
		t = 0.0
	elseif (t > 1.0) then
		t = 1.0
	end
	local Cx,Cy
	Cx = Ax + ABx * t
	Cy = Ay + ABy * t
	if(getdistance(Px,Py,Cx,Cy)<=range) then
		return true
	else
		return false
	end
end

function ride_line(s,xoffs,yoffs)

	local function move(zx1,zy1,zx2,zy2)
		local avx,avy,cvx,cvy
		cvx,cvy = getvdir(zx1,zy1,zx2,zy2)
		if(math.abs(spr[s].vx)<math.abs(spr[s].vy)) then
			avx = spr[s].vx
			avy = spr[s].vx
		else
			avx = spr[s].vy
			avy = spr[s].vy
		end
		if(spr[s].movetimer) then
			avx = cvx*5
			avy = cvy*5
		else
			avx = cvx*2.5
			avy = cvy*3
		end
		
		set_sprite_velocity(s,avx,avy)
		--coll.a = {spr[s].x+xoffs,spr[s].y+yoffs-1,spr[s].x+xoffs+avx*64,spr[s].y+yoffs+avy*64-1}
	end
	
	local function getcoords(i)
		local j
		j = i - 1
		if(j < 1) then
			j = Z_MAX
		end
		if(zbuf[i] and zbuf[j]) then
			return zbuf[j].x,zbuf[j].y,zbuf[i].x,zbuf[i].y
		else
			return nil
		end
	end
	
	local function domovemagic(x,y,firstline)
		local zx1,zy1,zx2,zy2 = getcoords(spr[s].riding)
		if(zx1 and close_to_line(x,y,{zx1,zy1,zx2,zy2},5)) then
			move(zx1,zy1,zx2,zy2)
		else
			--get next line.. check for collision
			spr[s].riding = spr[s].riding + 1
			if(spr[s].riding > Z_MAX) then
				spr[s].riding = 1
			end
			if(spr[s].riding==firstline) then
				spr[s].riding=nil
				return
			end
			domovemagic(x,y,firstline)
		end
	end
	
	if(spr[s].riding==nil) then
		return
	end
	
	local x = spr[s].x+xoffs
	local y = spr[s].y+yoffs
	domovemagic(x,y,spr[s].riding)
end

function collisioncheck(s)
	local c=false
	local cvx,cvy
	local avx,avy

	local function checkpixel(xoffs,yoffs,i,j)
		local x = spr[s].x+xoffs
		local y = spr[s].y+yoffs
		local px2 = x+spr[s].vx
		local py2 = y+spr[s].vy
		local px1 = x
		local py1 = y
		local zx = zbuf[i].x
		local zy = zbuf[i].y
		local zx2 = zbuf[j].x
		local zy2 = zbuf[j].y
		if(spr[s].vx~=0 or spr[s].vy~=0) then
			if(spr[s].vx>0) then --we need at least one pixel!!?
				px2 = px2 + 2
			elseif(spr[s].vx<0) then
				px2 = px2 - 2
			end
			if(spr[s].vy>0) then
				py2 = py2 + 2
			elseif(spr[s].vy<0) then
				py2 = py2 - 2
			end
			
			local cx,cy = get_line_intersection({px1,py1,px2,py2},{zx,zy,zx2,zy2})
			if(cx~=nil) then
				cx = math.floor(cx)
				cy = math.floor(cy)
				local avx,avy,cvx,cvy
				
				--coll = {--[[a={px1,py1,px2+spr[s].vx*64,py2+spr[s].vy*64},-]]b={zbuf[i].x,zbuf[i].y,zbuf[j].x,zbuf[j].y}} --debug
				collx = cx
				colly = cy
				
				avx,avy = getvdir(px1,py1,px2,py2)
				cvx,cvy = getvdir(zbuf[j].x,zbuf[j].y,zbuf[i].x,zbuf[i].y)
				
				
					local x,y
					x = cx-xoffs
					y = cy-yoffs
					if(spr[s].x==x and spr[s].y==y) then
						--FCEU.message("boo"..timer)
					else
						set_sprite_pos(s,x,y)
					end
					if(s==0) then --mario/luigi
						if(math.abs(cvy)==1 and math.abs(cvx) < 0.5) then
							change_sprite_dir(s)
							set_sprite_velocity(s,-avx,nil)
						else
							spr[s].riding = i
							set_sprite_velocity(s,0,0)
						end
					else --enemies, moving platforms
					end
					
					
				return true
			end
		else
			if(close_to_line(x,y,{zx,zy,zx2,zy2},4)) then
				if(s==0) then
					spr[s].riding = i
					set_sprite_velocity(s,0,0)
				end
				return true
			else
				spr[s].riding = nil
			end
		end
		return false
	end
	
	if(spr[s].riding) then
		ride_line(s,8,30)
		return
	end
	
	for i=1,Z_MAX do
		if(zbuf[i] and zbuf[i].connected) then
			--if(zbuf[i].x >= spr[s].x+hitbox.x1 and zbuf[i].y >= spr[s].y+hitbox.y1
			--	and zbuf[i].x <= spr[s].x+hitbox.x2 and zbuf[i].y <= spr[s].y+hitbox.y2) then
				--local px = spr[s].x-spr[s].vx
				--local py = spr[s].y-spr[s].vy
				local j
				j = i - 1
				if(j < 1) then
					j = Z_MAX
				end
				
				if(zbuf[j]) then --check if line is still valid (if nil, node disappeared)
					if(s==0) then --mario
						if(checkpixel(8,30,i,j)) then
							return
						end
					else
						if(checkpixel(0,0,i,j) or checkpixel(8,8,i,j)
							--[[or checkpixel(0,8,i,j) or checkpixel(8,0,i,j)-]]) then
							destroy_sprite(s)
							return
						end
					end
				end
				
			--end
		end
	end
	
end


function screen_to_game_pos(x,y)
	return x+scroll_x,y+scroll_y
end

function game_to_screen_pos(x,y)
	return x-scroll_x,y-scroll_y
end


function destroy_sprite(s)
	if(s<10) then
		memory.writebyte(0x660+s,0x06)
		--memory.writebyte(0x660+s,0x00)
		--set_sprite_velocity(s,10,10)
	else
		memory.writebyte(0x6C7+s-10,0x01)
		--set_sprite_pos(s,0,0)
	end
end

function set_sprite_velocity(s,vx,vy)
	if(s<10) then
		if(s==0) then
			memory.writebyte(0xD8,1) --air flag?
		end
		if(vx) then
			memory.writebyte(0xBD+s,vx*16)
			spr[s].vx = vx
		end
		if(vy) then
			memory.writebyte(0xCF+s,vy*16)
			spr[s].vy = vy
		end
	end
end

function set_sprite_pos(i,x,y)
	memory.writebyte(0x90+i,AND(x,255))
	memory.writebyte(0x75+i,math.floor(x/256))
	memory.writebyte(0xA2+i,AND(y,255))
	memory.writebyte(0x87+i,math.floor(y/256))
	spr[i].x = x
	spr[i].y = y
	spr[i].sx,spr[i].sy = game_to_screen_pos(spr[i].x,spr[i].y)
end

function change_sprite_dir(s)
	if(spr[s].dir == 1) then
		spr[s].dir = -1
	elseif(spr[s].dir == -1) then
		spr[s].dir = 1
	end
end


function add_rainbow_coord(cx,cy,connected)
	zbuf[zindex] = {t=timer,x=cx,y=cy,connected=connected}
	zprev = zbuf[zindex]
	zindex = zindex + 1
	if(zindex>Z_MAX) then
		zindex = 1
	end
end

function drawrainbow(x1,y1,x2,y2,coloffs)
	local cx,cy
	local vx,vy
	local color = coloffs
	vx,vy = getvdir(x1,y1,x2,y2)
	cx = x1
	cy = y1
	
	for i=1,200 do
		if(cx>=0 and cy>=0 and cx<=253 and cy<=253) then
			local rcolor = color/1.8+161 --165
			gui.drawpixel(cx,cy,rcolor)
			gui.drawpixel(cx,cy+1,rcolor)
			gui.drawpixel(cx+1,cy,rcolor)
			gui.drawpixel(cx+1,cy+1,rcolor)
			
			gui.drawpixel(cx+2,cy,rcolor)
			gui.drawpixel(cx,cy+2,rcolor)
			gui.drawpixel(cx+2,cy+1,rcolor)
			gui.drawpixel(cx+1,cy+2,rcolor)
			gui.drawpixel(cx+2,cy+2,rcolor)
		end
		if((x2>x1 and cx>x2) or (x2<x1 and cx<x2) or (y2>y1 and cy>y2) or (y2<y1 and cy<y2)) then
			break
		end
		cx = cx + vx
		cy = cy + vy
		color = color + 17
		color = AND(color,15)
	end
	return color
end


function initialize()
	for i=0,NUM_SPRITES-1 do
		spr[i] = {
					a=0,
					id=0,
					id2=0,
					x=0,
					y=0,
					hp=0,
				}
	end
	spr[0].dir = 1
	spr[0].lastposchange = -999
end


function update_sprites()
	mario = spr[0]
	--also change dir when colliding with game objects
	local x = memory.readbyte(0x90)+memory.readbyte(0x75)*256
	local y = memory.readbyte(0xA2)+memory.readbyte(0x87)*256
	if(x~=mario.x or y~=mario.y) then
		mario.lastposchange = timer
	end
	if(mario.dir~=0 and mario.riding==nil and timer-mario.lastposchange>60) then
		mario.lastposchange = timer
		change_sprite_dir(0)
	end
	
	--load game sprites from ram
	for i=0,NUM_SPRITES-1 do
		if(i<10) then
			spr[i].x = memory.readbyte(0x90+i)+memory.readbyte(0x75+i)*256
			spr[i].y = memory.readbyte(0xA2+i)+memory.readbyte(0x87+i)*256
			spr[i].vx = memory.readbytesigned(0xBD+i)/16
			spr[i].vy = memory.readbytesigned(0xCF+i)/16
			spr[i].a = (memory.readbytesigned(0x660+i)~=0)
			spr[i].id = memory.readbytesigned(0x670+i)
			spr[i].sx,spr[i].sy = game_to_screen_pos(spr[i].x,spr[i].y)
		else
			--TODO: 0x5D3?
			local xcomp = memory.readbyte(0xFD)
			local ycomp = memory.readbyte(0xFC)
			spr[i].x = memory.readbyte(0x5C9+i-10)
			spr[i].y = memory.readbyte(0x5BF+i-10)
			spr[i].a = true
			spr[i].vx = 0
			spr[i].vy = 0
			spr[i].sx = AND(spr[i].x-xcomp+256,255)
			spr[i].sy = AND(spr[i].y-ycomp+256,255)
			spr[i].x = spr[i].sx + scroll_x
			spr[i].y = spr[i].sy + scroll_y
		end
		if(spr[i].a) then
			collisioncheck(i)
		end
	end

end

function update_vars()
	--disabling input not possible anymore in FCEUX 2.1?
	jp = {}
	if(mario.riding==nil) then
		if(mario.movetimer) then
			jp.B = 1
			mario.movetimer = mario.movetimer - 1
			if(mario.movetimer == 0) then
				mario.movetimer = nil
			end
		end
		if(mario.dir==1) then
			jp.right=1
		elseif(mario.dir==-1) then
			jp.left=1
		end
		--if(AND(timer,1)==0) then --automatically enter doors and pipes... not a very good idea actually :P
			--jp.up=1
			--if(memory.readbyte(0xD8)==1) then --air flag set? try to stay in air as long as possible... makes it easier to rescue mario if collision detection screws up.. sucks in water levels
			--	jp.A=1
			--end
		--else
		--	jp.down=1 --automatically enter pipes
		--end
	end
	
	inp = input.get()
	
	scroll_x = memory.readbyte(0xFD)+memory.readbyte(0x12)*256
	scroll_y = memory.readbyte(0xFC)--+memory.readbyte(0x13)*256 --not quite right, long vertical scrolling levels won't work
	
	--0xD8 = 0 -> touch ground, 1 -> air
	
	update_sprites()
	
	
	if(inp.middleclick) then
		mario.movetimer = 30
	end
	
	if(inp.leftclick==nil) then
		mario.jumping=false
	end
	if(inp.leftclick and last_inp.leftclick==nil and 
		inp.xmouse>=mario.sx+clickbox.x1 and inp.xmouse<=mario.sx+clickbox.x2 and 
		inp.ymouse>=mario.sy+clickbox.y1 and inp.ymouse<=mario.sy+clickbox.y2)
		then
		jp.A = 1
		mario.jumping = true
	elseif(inp.leftclick and mario.jumping) then
		jp.A = 1
	elseif(inp.leftclick) then
		if(paintmeter>0) then
			local x,y=screen_to_game_pos(inp.xmouse,inp.ymouse)
			if(last_inp.leftclick==nil) then
				add_rainbow_coord(x,y,false)
				outofpaint = nil
			elseif(outofpaint==nil) then
				if(zprev and getdistance(x,y,zprev.x,zprev.y)>8) then
					add_rainbow_coord(x,y,true)
					paintmeter = paintmeter - 2
					lastpaint = timer
				end
			end
		else
			outofpaint = true
		end
	end
	
	joypad.set(1,jp)
	
	last_mario = mario
	last_inp = inp
end

function render()
	--bctext(0,20,string.format("Memory usage: %.2f KB",collectgarbage("count")))
	
	
	local j = 0
	for i=1,Z_MAX do
		if(zbuf[i]) then
			j = j + 1
		end
	end
	--bctext(0,20,"Lines: "..j)
	--bctext(0,20,mario.x..","..mario.y)
	--bctext(0,30,mario.sx..","..mario.sy)
	--bctext(0,40,mario.vx..","..mario.vy)
	--bctext(0,50,"D "..mario.dir)
	
	--bctext(0,70,"scroll_x: "..scroll_x)
	--bctext(0,80,"scroll_y: "..scroll_y)
	--bctext(0,90,AND(mario.x,255))
	--if(mario.riding) then
	--	bctext(0,100,"R")
	--end
	bcbox(mario.sx+clickbox.x1,mario.sy+clickbox.y1,mario.sx+clickbox.x2,mario.sy+clickbox.y2,"white")
	
	local prev_x, prev_y, prev_connected
	local i=zindex
	local color = AND(timer/2,15)
	for j=1,Z_MAX do
		if(zbuf[i]) then
			local ltime = timer-zbuf[i].t
			if(ltime<Z_LSPAN) then
				local x,y=game_to_screen_pos(zbuf[i].x,zbuf[i].y)
				if(prev_x and prev_connected) then
					color = drawrainbow(prev_x,prev_y,x,y,color)
				end
				prev_x = x
				prev_y = y
				prev_connected = zbuf[i].connected
			else
				zbuf[i] = nil
			end
		end
		i = i - 1
		if(i<1) then
			i = Z_MAX
		end
	end
	
	
	local cx,cy = game_to_screen_pos(mario.x+8,mario.y+30)
	bcpixel(cx,cy,string.format("#%02X%02X%02X",math.random(0,255),math.random(0,255),math.random(0,255)))
	
	local pmx=78
	local pmy=226
	if(paintmeter>0) then
		drawrainbow(pmx+paintmeter,pmy,pmx,pmy,timer/6)
	end
	bcbox(pmx-2,pmy-1,pmx+102,pmy+3,"white")
	
	if(timer-lastpaint>60) then
		paintmeter = paintmeter + 1
		if(paintmeter>100) then
			paintmeter=100
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


function domagic()
	if(memory.readbyte(0x73)==0x20) then --map screen(?)
		update_vars()
		render()
	else
		bcpixel(1,10,"clear")
	end
end

initialize()
gui.register(domagic)

while(true) do
 	FCEU.frameadvance()
	timer = timer + 1
end
