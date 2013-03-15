-- temporary file because this will be huge.

handles			= {};

handles[   1]	= {	x1 =  0, y1 =  0, x2 = 16, y2 = 24 };	-- Green & Red Koopa
handles[   2]	= {	x1 =  0, y1 =  8, x2 = 16, y2 = 24 };	-- Goomba
handles[   3]	= {	x1 =  0, y1 =  0, x2 = 16, y2 = 16 };	-- generic 16x16
handles[   4]	= {	x1 =  0, y1 =  0, x2 =  8, y2 =  8 };	-- generic  8x 8


enemylist		= {};

-- type:
-- Default (0): selectable, standard
-- + 0x01: requires special handling
-- + 0x02: can't be selected (dupe, whatever)
-- + 0x10: requires two spaces to summon
-- + 0x20: powerup
--[[
enemylist[]	= { id = 0x00,	name = "Green Koopa",       handle =   1, hitbox = 0x03, type = 0x00 };
enemylist[]	= { id = 0x01,	name = "Red Koopa",         handle =   1, hitbox = 0x03, type = 0x01 };	-- Will jitter back and forth if moved into air

enemylist[]	= { id = 0x06,	name = "Goomba",            handle =   2, hitbox = 0x09, type = 0x00 };
enemylist[]	= { id = 0x07,	name = "Blooper",           handle =   1, hitbox = 0x09, type = 0x01 };  -- Uhh? It just vanishes out of nowhere...

enemylist[]	= { id = 0x0A,	name = "Gray Cheep-Cheep",  handle =   2, hitbox = 0x09, type = 0x00 };
enemylist[]	= { id = 0x0B,	name = "Red Cheep-Cheep",   handle =   2, hitbox = 0x09, type = 0x00 };

enemylist[]	= { id = 0x0D,	name = "Pirahna Plant",     handle =   1, hitbox = 0x09, type = 0x01 };	-- x speed is really y speed, main position needs hacked
enemylist[]	= { id = 0x0E,	name = "Green Paratroopa",  handle =   1, hitbox = 0x03, type = 0x00 };

enemylist[]	= { id = 0x14,	name = "Flying Cheepcheep", handle =   2, hitbox = 0x09, type = 0x00 };


enemylist[]	= { id = 0x2E,	name = "Mushroom",          handle =   3, hitbox = 0x03, type = 0x20, powerupval = 0x00 };
enemylist[]	= { id = 0x2E,	name = "Flower",            handle =   3, hitbox = 0x03, type = 0x20, powerupval = 0x01 };
enemylist[]	= { id = 0x2E,	name = "Starman",           handle =   3, hitbox = 0x03, type = 0x20, powerupval = 0x02 };
enemylist[]	= { id = 0x2E,	name = "1-up Mushroom",     handle =   3, hitbox = 0x03, type = 0x20, powerupval = 0x03 };
enemylist[]	= { id = 0x30,	name = "Flagpole flag",     handle =   3, hitbox =   -1, type = 0x20 };
enemylist[]	= { id = 0x31,	name = "Castle flag",       handle =   3, hitbox =   -1, type = 0x00 };
enemylist[]	= { id = 0x32,	name = "Springboard",       handle =   1, hitbox =   -1, type = 0x00 };


]]