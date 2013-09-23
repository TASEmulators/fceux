-- Tetris - displays block stats and shows hitboxes
-- Written by QFox
-- http://www.datacrystal.org/wiki/Tetris:RAM_map
-- Tetris (U) [!].rom

local function getPiece(n) -- returns table with information about this piece
	-- every piece consists of 4 blocks
	-- so piece will contain the information about these blocks
	-- for every block is the offset compared to the x,y position of the piece
	-- the x,y is written in a 4x4 plane of blocks, 1x1 being the topleft block
	-- x,y is the position as given by tetris
	local piece = {};
	if (n == 0) then -- T right
		piece.pos = {{2,1},{1,2},{2,2},{3,2}};
		piece.rel = {{0,-1},{-1,0},{0,0},{1,0}};
		piece.anchor = {2,2};
	elseif (n == 1) then -- T up
		piece.pos = {{1,1},{1,2},{2,2},{1,3}};
		piece.rel = {{0,-1},{0,0},{1,0},{0,1}};
		piece.anchor = {1,2};
	elseif (n == 2) then -- T down
		piece.pos = {{1,1},{2,1},{3,1},{2,2}};
		piece.rel = {{-1,0},{0,0},{1,0},{0,1}};
		piece.anchor = {2,1};
	elseif (n == 3) then -- T left
		piece.pos = {{2,1},{1,2},{2,2},{2,3}};
		piece.rel = {{0,-1},{-1,0},{0,0},{0,1}};
		piece.anchor = {2,2};
	elseif (n == 4) then -- J left
		piece.pos = {{2,1},{2,2},{1,3},{2,3}};
		piece.rel = {{0,-1},{0,0},{-1,1},{0,1}};
		piece.anchor = {2,2};
	elseif (n == 5) then -- J up
		piece.pos = {{1,1},{1,2},{2,2},{3,2}};
		piece.rel = {{-1,-1},{-1,0},{0,0},{1,0}};
		piece.anchor = {2,2};
	elseif (n == 6) then -- J right
		piece.pos = {{1,1},{2,1},{1,2},{1,3}};
		piece.rel = {{0,-1},{1,-1},{0,0},{0,1}};
		piece.anchor = {1,2};
	elseif (n == 7) then -- J down
		piece.pos = {{1,1},{2,1},{3,1},{3,2}};
		piece.rel = {{-1,0},{0,0},{1,0},{1,1}};
		piece.anchor = {2,1};
	elseif (n == 8) then -- Z horz
		piece.pos = {{1,1},{2,1},{2,2},{3,2}};
		piece.rel = {{-1,0},{0,0},{0,1},{1,1}};
		piece.anchor = {2,1};
	elseif (n == 9) then -- Z vert
		piece.pos = {{2,1},{1,2},{2,2},{1,3}};
		piece.rel = {{1,-1},{0,0},{1,0},{0,1}};
		piece.anchor = {1,2};
	elseif (n == 10) then -- O
		piece.pos = {{1,1},{2,1},{1,2},{2,2}};
		piece.rel = {{-1,0},{0,0},{-1,1},{0,1}};
		piece.anchor = {2,1};
	elseif (n == 11) then -- S horz
		piece.pos = {{2,1},{3,1},{1,2},{2,2}};
		piece.rel = {{0,0},{1,0},{-1,1},{0,1}};
		piece.anchor = {2,1};
	elseif (n == 12) then -- S vert
		piece.pos = {{1,1},{1,2},{2,2},{2,3}};
		piece.rel = {{0,-1},{0,0},{1,0},{1,1}};
		piece.anchor = {1,2};
	elseif (n == 13) then -- L right
		piece.pos = {{1,1},{1,2},{1,3},{2,3}};
		piece.rel = {{0,-1},{0,0},{0,1},{1,1}};
		piece.anchor = {1,2};
	elseif (n == 14) then -- L down
		piece.pos = {{1,1},{2,1},{3,1},{1,2}};
		piece.rel = {{-1,0},{0,0},{1,0},{-1,1}};
		piece.anchor = {2,1};
	elseif (n == 15) then -- L left
		piece.pos = {{1,1},{2,1},{2,2},{2,3}};
		piece.rel = {{-1,-1},{0,-1},{0,0},{0,1}};
		piece.anchor = {2,2};
	elseif (n == 16) then -- L up
		piece.pos = {{3,1},{1,2},{2,2},{3,2}};
		piece.rel = {{1,-1},{-1,0},{0,0},{1,0}};
		piece.anchor = {2,2};
	elseif (n == 17) then -- I vert
		piece.pos = {{1,1},{1,2},{1,3},{1,4}};
		piece.rel = {{0,-2},{0,-1},{0,0},{0,1}};
		piece.anchor = {1,3};
	elseif (n == 18) then -- I horz
		piece.pos = {{1,1},{2,1},{3,1},{4,1}};
		piece.rel = {{-2,0},{-1,0},{0,0},{1,0}};
		piece.anchor = {3,1};
	else
		return nil;
	end;
	piece.id = n;
	return piece;
end;

	-- 0,0 - 10,20
	-- translated it's: <95,40> to <175,200>
	-- x starts at 1, y starts at 0...
	-- each block is 8x8
local areax = 95;
local areay = 40;
local blocksize = 8;

while (true) do
	-- get position of current piece (now)
	local x = (memory.readbyte(0x0060)*blocksize)+areax;
	local y = (memory.readbyte(0x0061)*blocksize)+areay;
	local now = getPiece(memory.readbyte(0x0062)); -- get piece info
	-- draw a nice box around it
	gui.drawbox(95,40,175,200,"red");
	gui.text(5,5,"xy: <"..x..", "..y..">");
	if (now and x > 0 and y > 0 and x < 250 and y < 200) then
		gui.drawbox(x+(now.rel[1][1]*blocksize),y+(now.rel[1][2]*blocksize),x+(now.rel[1][1]*blocksize)+blocksize,y+(now.rel[1][2]*blocksize)+blocksize,"green");
		gui.drawbox(x+(now.rel[2][1]*blocksize),y+(now.rel[2][2]*blocksize),x+(now.rel[2][1]*blocksize)+blocksize,y+(now.rel[2][2]*blocksize)+blocksize,"green");
		gui.drawbox(x+(now.rel[3][1]*blocksize),y+(now.rel[3][2]*blocksize),x+(now.rel[3][1]*blocksize)+blocksize,y+(now.rel[3][2]*blocksize)+blocksize,"green");
		gui.drawbox(x+(now.rel[4][1]*blocksize),y+(now.rel[4][2]*blocksize),x+(now.rel[4][1]*blocksize)+blocksize,y+(now.rel[4][2]*blocksize)+blocksize,"green");
	end;
	gui.text(5,15,"Now: "..memory.readbyte(0x0062).."\nNext: "..memory.readbyte(0x00BF));

	-- draw the filled field and do some counting
	local start = 0x0400; -- toprow
	local height = -1;
	local filled = 0;
	for j=0,19 do
		for i=0,9 do
				if (memory.readbyte(start+(j*10)+i) ~= 0xEF) then
					filled = filled + 1;
					if (height == -1) then height = j; end;
					gui.drawbox(areax+(i*blocksize),areay+(j*blocksize),areax+(i*blocksize)+blocksize,areay+(j*blocksize)+blocksize,"red");
				end;
		end;
	end;
	if (height == -1) then height = 0;
	else height = 20 - height; end;
	gui.text(5,30,"Height: "..height.."\nFilled: "..filled.." ("..math.floor(filled/(height)*10).."%)");


	--local inp1 = joypad.read(1);
	--joypad.set(1,{right=1});
	FCEU.frameadvance();
end;