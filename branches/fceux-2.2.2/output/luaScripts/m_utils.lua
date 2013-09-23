--Used for scripts written by Miau such as Gradius-BulletHell

--[[
A Compilation of general-purpose functions.

-]]
M_UTILS_VERSION = 0

--initialization
if(FCEU) then
	EMU = FCEU
elseif(snes9x) then
	EMU = snes9x
elseif(gens) then
	EMU = gens
end
if(EMU == nil) then
	error("Unsupported environment. This script runs on Lua-capable emulators only - FCEUX, Snes9x or Gens.")
end




--[[	m_require
	"require" with version checking
	
	Similar to the approach in Xk's x_functions.lua, but supports loading and automatic version checking of
	"compatible" modules, too. A central version checking function does have its advantages.
	
A compatible module looks like this:
	if(REQUIRED_MODULE_VERSION==0) then --global variable = the second parameter of m_require
		error("This module isn't backwards compatible to version 0.")
	end
	module("modulename")
	MODULE_VERSION = 3
	function test()
	end
	return _M
	
using it in your code:
	m_require("modulefile.lua",3) --second parameter being > MODULE_VERSION would raise an error
	modulename.test()
-]]
function m_require(module,requiredver)
	if(module==nil or module=="m_utils") then
		if(M_UTILS_VERSION>=requiredver) then
			return true
		else
			error("\n\nThis script requires m_utils version "..requiredver..".\n"..
					"Your m_utils.lua is version "..M_UTILS_VERSION.."\n"..
					"Check http://morphcat.de/lua/ for a newer version.")
		end
	else
		--give the module the possibility to be backwards compatible
		--not very elegant, but there seems to be no other way using require
		REQUIRED_MODULE_VERSION = requiredver
		local ret = require(module)
		if(requiredver==nil) then --got no version checking to do
			return ret
		elseif(ret==nil or ret==false) then
			error("Could not load module "..module.." or module invalid.")
		elseif(type(ret)~="table") then
			error("Invalid module "..module..", doesn't return itself.")
		elseif(ret.MODULE_VERSION==nil) then
			error("Invalid module "..module..", missing version information.")
		elseif(ret.MODULE_VERSION < requiredver) then
			error("\n\nThis script requires "..module.." version "..requiredver..".\n"..
					"Your "..module.." module is version "..ret.MODULE_VERSION..".\n"..
					"If it's one of miau's Lua modules check\n"..
					"http://morphcat.de/lua/ for a newer version.\n")
		else
			return ret
		end
	end
end


--drawing functions with bounds checking
function bcbox(x1,y1,x2,y2,color1)
	if(x1>=0 and y1>=0 and x2<=255 and y2<=255) then
		gui.drawbox(x1, y1, x2, y2, color1, 0)
	end
end

function bcpixel(x,y,color)
	if(x>=0 and y>=0 and x<=255 and y<=255) then
		gui.drawpixel(x,y,color)
	end
end

function bcline(x1,y1,x2,y2,color)
	if(x1>=0 and y1>=0 and x2<=255 and y2<=255
		and x2>=0 and y2>=0 and x1<=255 and y1<=255) then
		gui.drawline(x1, y1, x2, y2, color)
	end
end


--bc + clipping, just make sure x2,y2 is the point that will most likely leave the screen first
function bcline2(x1,y1,x2,y2,color)
	if(x1>=0 and y1>=0 and x1<=255 and y1<=255) then
		if(x2>255 or y2>255 or x2<0 or y2<0) then	
			local vx,vy=getvdir(x1,y1,x2,y2)
			--TODO: replace brute force-y method  with line intersection formula
			if(math.abs(vx)==1 or math.abs(vy)==1) then
				local x=x1
				local y=y1
				while(x<254 and x>1 and y<254 and y>1) do
					x = x + vx
					y = y + vy
				end
				x = math.floor(x)
				y = math.floor(y)
				--logf(" ("..x1..","..y1..")-("..x..","..y..")\r\n")
				gui.drawline(x1,y1,x,y,color)
			end
		else
			gui.drawline(x1,y1,x2,y2,color)
		end
	end
end

function bctext(x,y,text)
	if(x>=0 and y>=0 and x<=255 and y<=255) then
		--make sure the text is always visible or else... possible memory leaks(?) and crashes on FCEUX <= 2.0.3
		local len=string.len(text)*8+1
		if(x+len > 255) then
			x = 255-len
		end
		if(x < 0) then
			x = 0
		end
		
		if(y > 222) then --NTSC
			y=222
		end
		gui.text(x, y, text)
	end
end


function getdistance(x1,y1,x2,y2)
	return math.floor(math.sqrt((x1-x2)^2+(y1-y2)^2))
end

--returns direction vector of a line
function getvdir(srcx,srcy,destx,desty)
	local x1 = srcx
	local x2 = destx
	local y1 = srcy
	local y2 = desty
	local xc = x2-x1
	local yc = y2-y1
	if(math.abs(xc)>math.abs(yc)) then
		yc = yc/math.abs(xc)
		xc = xc/math.abs(xc)
	elseif(math.abs(yc)>math.abs(xc)) then
		xc = xc/math.abs(yc)
		yc = yc/math.abs(yc)
	else
		if(xc<0) then
			xc=-1
		elseif(xc>0) then
			xc=1
		else
			xc=0
		end
		if(yc<0) then
			yc=-1
		elseif(yc>0) then
			yc=1
		else
			yc=0
		end
	end
	
	return xc,yc
end


local m_cursor = {
	{1,0,0,0,0,0,0,0},
	{1,1,0,0,0,0,0,0},
	{1,2,1,0,0,0,0,0},
	{1,2,2,1,0,0,0,0},
	{1,2,2,2,1,0,0,0},
	{1,2,2,2,2,1,0,0},
	{1,2,2,2,2,2,1,0},
	{1,2,1,1,1,1,1,1},
	{1,1,0,0,0,0,0,0},
	{1,0,0,0,0,0,0,0},
}
function drawcursor(px,py,col1,col2)
	if(px>0 and py>0 and px<256-8 and py<256-10) then
		for y=1,10 do
			if(m_cursor[y]) then
				for x=1,8 do
					if(m_cursor[y][x]==1) then
						gui.drawpixel(px+x,py+y,col1)
					elseif(m_cursor[y][x]==2) then
						gui.drawpixel(px+x,py+y,col2)
					end
				end
			end
		end
	end
end

--debug functions
function logf(s)
	local fo = io.open("E:/emu/nes/fceux-2.0.4.win32-unofficial/lua/log.txt", "ab")
	if(fo~=nil) then
		fo:write(s)
		fo:close()
		return true
	end
end

function logtable(a)
	for i,v in pairs(a) do
		if(type(v)=="table") then
			printarrrec(v)
		else
			if(v==false) then
				v="false"
			elseif(v==true) then
				v="true"
			end
			logf(i.." = "..v.."\r\n")
		end
	end
end

