---------------------------------------------------------------------------
-- Boulder Dash - Displaying Amoeba Pointer Movements
-- by AnS, 2012
-- requested by CtrlAltDestroy
---------------------------------------------------------------------------
-- Showcases following functions:
-- * memory.registerexec()
---------------------------------------------------------------------------
-- Usage:
-- Start playing any level of the BoulderDash that has the Amoeba enemy,
-- for example Level 2-3, Level 4-1 or Level 6-3.
-- Come close to the Amoeba enemy and see if you can find any regularities
-- in its movement. When you are convinced that its behavior is completely
-- erratic, run the script and unpause emulation. Now the script will draw
-- various arrows that show you the actual rules of the enemy AI.
--
-- You can control the "playback slider" in the bottom of the screen (click
-- or drag it with mouse). Also you can pause and unpause this mini-playback
-- while the emulator is still paused. This is necessary because the Amoeba
-- makes a new set of moves every 8 frames, and this time span is not enough
-- for you to replay all the logged events.
---------------------------------------------------------------------------
-- Explanations:
-- The Amoeba AI is defined by UpdateAmoeba() function (see the disassembly
-- code at the end of this script).
-- The UpdateAmoeba() makes many steps with the Amoeba Pointer (so the arrow
-- may move from tile to tile 100-200 times within the span of one frame!
-- This complexity makes it look unpredictable for player, even though the
-- rules of movement are very simple (move clockwise, rotate anticlockwise).
-- 
-- This script allows you to see all steps of this Pointer, so you can grasp
-- the logic of Amoeba movement/multiplication. The script registers hooks
-- to all essential points of the UpdateAmoeba() code, and logs the whole
-- process of the function execution. Then it pauses emulation and replays
-- the log of events by drawing arrows over the game sprites.
-- The log auto-resets at the beginning of every new call of UpdateAmoeba().
--
-- Similar approach can be used for displaying complex AI in other games.
---------------------------------------------------------------------------

-- constants

CELL_SIZE = 16;
CELL_HALFSIZE = CELL_SIZE / 2;
UPPER_BORDER_SIZE = 64;
LEFT_BORDER_SIZE = 32;
INTRO_DELAY = 1;
OUTRO_DELAY = 1;
MAX_ARROW_SIZE = 9;

PAUSE_BUTTON_X = 2;
PAUSE_BUTTON_Y = 172;
PAUSE_BUTTON_WIDTH = 16;
PAUSE_BUTTON_HEIGHT = 16;

TIMELINE_X = 20;
TIMELINE_LENGTH = 232;
TIMELINE_HEIGHT = 8;
TIMELINE_Y = 180;

ColorStatuses = { "#00A000FF", "#00FF00FF", "#A0A0A0FF", "#00FFFFFF", "#FFFFFFFF", "#C0C000FF" };
STATUS_NORMAL = 1;	-- Green: Normal movements
STATUS_ROTATED = 2;	-- Light-green: just rotated 90 degrees (anticlockwise) relative to current movement direction
STATUS_BUMPED = 3;	-- Grey: bumped into obstacle
STATUS_SET = 4;		-- Blue: Set amoeba
STATUS_REFUSED = 5;	-- White: Refused to set amoeba
STATUS_END = 6;		-- Yellow: RTS from UpdateAmoeba

-- variables

animation_timer = 0;
current_step = 0;
animation_speed = 0.5;
animation_paused = false;
mouse_held = false;
dragging_timeline = false;

log_size = 0;
creation_frame = -1;

-- arrays for storing the log data

Amoeba_X = {};
Amoeba_Y = {};
AmoebaDirection = {};
AmoebaPhase = {};
AmoebaTimer = {};
ColorStatus = {};

------------------------------------------------------------------------
function reset_log()
	log_size = 0;
	animation_timer = 0;
	creation_frame = movie.framecount();
	emu.pause();
end

function append_log_normal()
	log_size = log_size + 1;
	Amoeba_X[log_size] = memory.readbyte(0x0050);	-- Temp_X
	Amoeba_Y[log_size] = memory.readbyte(0x004F);	-- Temp_Y
	AmoebaDirection[log_size] = memory.readbyte(0x004E);	-- Temp_AmoebaDirection
	AmoebaPhase[log_size] = memory.readbyte(0x00B7);
	AmoebaTimer[log_size] = memory.readbyte(0x00BD);
	ColorStatus[log_size] = STATUS_NORMAL;	-- Green: Normal movements
end

function append_log_rotated()
	log_size = log_size + 1;
	Amoeba_X[log_size] = memory.readbyte(0x0050);	-- Temp_X
	Amoeba_Y[log_size] = memory.readbyte(0x004F);	-- Temp_Y
	AmoebaDirection[log_size] = memory.readbyte(0x004E);	-- Temp_AmoebaDirection
	AmoebaPhase[log_size] = memory.readbyte(0x00B7);
	AmoebaTimer[log_size] = memory.readbyte(0x00BD);
	ColorStatus[log_size] = STATUS_ROTATED;	-- Light-green: just rotated 90 degrees (anticlockwise) relative to current movement direction
end

function append_log_bumped()
	log_size = log_size + 1;
	Amoeba_X[log_size] = memory.readbyte(0x0050);	-- Temp_X
	Amoeba_Y[log_size] = memory.readbyte(0x004F);	-- Temp_Y
	AmoebaDirection[log_size] = memory.readbyte(0x004E);	-- Temp_AmoebaDirection
	AmoebaPhase[log_size] = memory.readbyte(0x00B7);
	AmoebaTimer[log_size] = memory.readbyte(0x00BD);
	ColorStatus[log_size] = STATUS_BUMPED;	-- Grey: bumped into obstacle
end

function append_log_set()
	log_size = log_size + 1;
	Amoeba_X[log_size] = memory.readbyte(0x0050);	-- Temp_X
	Amoeba_Y[log_size] = memory.readbyte(0x004F);	-- Temp_Y
	AmoebaDirection[log_size] = memory.readbyte(0x004E);	-- Temp_AmoebaDirection
	AmoebaPhase[log_size] = memory.readbyte(0x00B7);
	AmoebaTimer[log_size] = memory.readbyte(0x00BD);
	ColorStatus[log_size] = STATUS_SET;	-- Blue: Set amoeba
end

function append_log_refused()
	log_size = log_size + 1;
	Amoeba_X[log_size] = memory.readbyte(0x0050);	-- Temp_X
	Amoeba_Y[log_size] = memory.readbyte(0x004F);	-- Temp_Y
	AmoebaDirection[log_size] = memory.readbyte(0x004E);	-- Temp_AmoebaDirection
	AmoebaPhase[log_size] = memory.readbyte(0x00B7);
	AmoebaTimer[log_size] = memory.readbyte(0x00BD);
	ColorStatus[log_size] = STATUS_REFUSED;	-- White: Refused to set amoeba
end

function append_log_rts()
	log_size = log_size + 1;
	Amoeba_X[log_size] = memory.readbyte(0x00BC);
	AmoebaDirectionAnd_Y = memory.readbyte(0x00BB);
	Amoeba_Y[log_size] = AND(AmoebaDirectionAnd_Y, 0x3F);
	AmoebaDirection[log_size] = AND(AmoebaDirectionAnd_Y, 0xC0);
	AmoebaPhase[log_size] = memory.readbyte(0x00B7);
	AmoebaTimer[log_size] = memory.readbyte(0x00BD);
	ColorStatus[log_size] = STATUS_END;	-- Yellow: RTS from UpdateAmoeba
end

------------------------------------------------------------------------
function DrawArrow(x, y, direction, size, color)
	x = x + CELL_HALFSIZE;
	y = y + CELL_HALFSIZE;
	if (direction == 00) then
		-- Left
		gui.line(x, y - size, x - size, y, color);
		gui.line(x - size, y, x, y + size, color);
		gui.line(x, y + size, x, y - size, color);
	end
	if (direction == 0x40) then
		-- up
		gui.line(x - size, y, x, y - size, color);
		gui.line(x, y - size, x + size, y, color);
		gui.line(x + size, y, x - size, y, color);
	end
	if (direction == 0x80) then
		-- Right
		gui.line(x, y - size, x + size, y, color);
		gui.line(x + size, y, x, y + size, color);
		gui.line(x, y + size, x, y - size, color);
	end
	if (direction == 0xC0) then
		-- Down
		gui.line(x - size, y, x, y + size, color);
		gui.line(x, y + size, x + size, y, color);
		gui.line(x + size, y, x - size, y, color);
	end
end
------------------------------------------------------------------------
-- This function is called every frame when the emulator is unpaused,
-- and 20 times per second when the emulator is paused.

function update()
	AmoebaOrigin_X = memory.readbyte(0x00B9);
	AmoebaOriginDirectionAnd_Y = memory.readbyte(0x00B8);
	AmoebaOrigin_Y = AND(AmoebaOriginDirectionAnd_Y, 0x3F);
	AmoebaOriginDirection = AND(AmoebaOriginDirectionAnd_Y, 0xC0);

	Camera_X_Lo = memory.readbyte(0x00BE);
	Camera_X_Hi = memory.readbyte(0x00BF);
	Camera_Y_Lo = memory.readbyte(0x00C0);
	Camera_Y_Hi = memory.readbyte(0x00C1);
	Camera_X = (256 * Camera_X_Hi + Camera_X_Lo) - UPPER_BORDER_SIZE;
	Camera_Y = (256 * Camera_Y_Hi + Camera_Y_Lo) - LEFT_BORDER_SIZE;

	if (log_size > 0) then

		inp = input.get()
		if (inp.leftclick) then
			xm = inp.xmouse
			ym = inp.ymouse
			-- check click on the Pause/Resume button
			if (not mouse_held) then
				if (xm >= PAUSE_BUTTON_X and ym >= PAUSE_BUTTON_Y and xm < PAUSE_BUTTON_X + PAUSE_BUTTON_WIDTH and ym < PAUSE_BUTTON_Y + PAUSE_BUTTON_HEIGHT) then
					animation_paused = not animation_paused;
				end
			end
			-- check click on the timeline
			if (dragging_timeline or (xm >= TIMELINE_X and ym >= TIMELINE_Y - TIMELINE_HEIGHT and xm < TIMELINE_X + TIMELINE_LENGTH and ym < TIMELINE_Y + TIMELINE_HEIGHT)) then
				shift = xm - TIMELINE_X;
				if (shift < 0) then
					shift = 0;
				else
					if (shift > TIMELINE_LENGTH) then shift = TIMELINE_LENGTH; end
				end
				animation_timer = (INTRO_DELAY + log_size + OUTRO_DELAY) * shift / TIMELINE_LENGTH;
				dragging_timeline = true;
			end
			mouse_held = true;
		else
			mouse_held = false;
			dragging_timeline = false;
		end

		if (not animation_paused and not mouse_held) then
			-- animate
			animation_timer = animation_timer + animation_speed;
			if (animation_timer > INTRO_DELAY + log_size + OUTRO_DELAY) then
				animation_timer = 0;
			end
		end

		current_step = math.floor(animation_timer - INTRO_DELAY);
		if (current_step > log_size) then
			current_step = log_size;
		else
			if (current_step < 1) then current_step = 1; end
		end

		-- show info
		if (ColorStatus[current_step] == STATUS_END) then
			-- emphasize the phase change by yellow
			gui.text(3, 11, "AmoebaPhase = " .. AND(AmoebaPhase[current_step], 3), "yellow");
		else
			gui.text(3, 11, "AmoebaPhase = " .. AND(AmoebaPhase[current_step], 3));
		end
		if ((ColorStatus[current_step] == STATUS_SET) or (ColorStatus[current_step] == STATUS_REFUSED)) then
			-- emphasize the decrement by red
			gui.text(3, 20, "AmoebaTimer = " .. AmoebaTimer[current_step], "red");
		else
			gui.text(3, 20, "AmoebaTimer = " .. AmoebaTimer[current_step]);
		end

		-- draw origin
		DrawArrow(AmoebaOrigin_X * CELL_SIZE - Camera_X, AmoebaOrigin_Y * CELL_SIZE - Camera_Y, AmoebaOriginDirection, 5, "#00FFFFFF");

		-- draw tail of pointers
		for i = (MAX_ARROW_SIZE - 1), 0, -1 do
			step = current_step - i;
			if (step > 0) then
				for s = 1, (MAX_ARROW_SIZE - i) do
					-- draw previous position of pointer
					DrawArrow(Amoeba_X[step] * CELL_SIZE - Camera_X, Amoeba_Y[step] * CELL_SIZE - Camera_Y, AmoebaDirection[step], s, ColorStatuses[ColorStatus[step]]);
				end
			end
		end
		-- draw black border around the last (current) pointer
		DrawArrow(Amoeba_X[current_step] * CELL_SIZE - Camera_X, Amoeba_Y[current_step] * CELL_SIZE - Camera_Y, AmoebaDirection[current_step], MAX_ARROW_SIZE + 1, "black");

		-- draw GUI
		-- Pause/Resume button
		gui.box(PAUSE_BUTTON_X, PAUSE_BUTTON_Y, PAUSE_BUTTON_X + PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_Y + PAUSE_BUTTON_HEIGHT, "grey", "black");
		if (animation_paused) then
			for s = 1, 6 do
				DrawArrow(PAUSE_BUTTON_X - 2, PAUSE_BUTTON_Y, 0x80, s, "black");
			end
		else
			gui.box(PAUSE_BUTTON_X + (PAUSE_BUTTON_WIDTH / 2) - 3, PAUSE_BUTTON_Y + 3, PAUSE_BUTTON_X + (PAUSE_BUTTON_WIDTH / 2) - 1, PAUSE_BUTTON_Y + PAUSE_BUTTON_HEIGHT - 3, "black", "black");
			gui.box(PAUSE_BUTTON_X + (PAUSE_BUTTON_WIDTH / 2) + 3, PAUSE_BUTTON_Y + 3, PAUSE_BUTTON_X + (PAUSE_BUTTON_WIDTH / 2) + 1, PAUSE_BUTTON_Y + PAUSE_BUTTON_HEIGHT - 3, "black", "black");
		end

		-- line
		gui.box(TIMELINE_X, TIMELINE_Y - TIMELINE_HEIGHT, TIMELINE_X + TIMELINE_LENGTH, TIMELINE_Y + TIMELINE_HEIGHT, "#00000040", "black");
		gui.box(TIMELINE_X, TIMELINE_Y - 1, TIMELINE_X + TIMELINE_LENGTH, TIMELINE_Y + 1, "white", "black");
		-- slider
		shift = TIMELINE_LENGTH * animation_timer / (INTRO_DELAY + log_size + OUTRO_DELAY);
		gui.box(TIMELINE_X + shift - 1, TIMELINE_Y - TIMELINE_HEIGHT, TIMELINE_X + shift + 1, TIMELINE_Y + TIMELINE_HEIGHT, "grey", "black");
		-- info
		gui.text(TIMELINE_X + 80 + creation_frame % 3, TIMELINE_Y + TIMELINE_HEIGHT + 2, "Displaying " .. current_step .. " / " .. log_size);
		gui.text(TIMELINE_X + 80 + creation_frame % 3, TIMELINE_Y + TIMELINE_HEIGHT + 11, "Created at frame " .. creation_frame);
	end
end
------------------------------------------------------------------------
gui.register(update);

memory.registerexec(0xCE60, reset_log);			-- ReallyUpdateAmoeba
--memory.registerexec(0xCE66, reset_log);			-- RestartFromOrigin

memory.registerexec(0xCE95, append_log_normal);		-- TryMultiplying_Left
memory.registerexec(0xCEAE, append_log_normal);		-- TryMultiplying_Up
memory.registerexec(0xCEC7, append_log_normal);		-- TryMultiplying_Right
memory.registerexec(0xCEE0, append_log_normal);		-- TryMultiplying_Down
memory.registerexec(0xCEF9, append_log_normal);		-- TryMultiplying_Left2
memory.registerexec(0xCF12, append_log_normal);		-- TryMultiplying_Up2
memory.registerexec(0xCF2B, append_log_normal);		-- TryMultiplying_Right2

memory.registerexec(0xCEA3, append_log_rotated);	-- Rotated from left to down
memory.registerexec(0xCEBC, append_log_rotated);	-- Rotated from up to left
memory.registerexec(0xCED5, append_log_rotated);	-- Rotated from right to up
memory.registerexec(0xCEEE, append_log_rotated);	-- Rotated from down to right
memory.registerexec(0xCF07, append_log_rotated);	-- Rotated from left to down
memory.registerexec(0xCF20, append_log_rotated);	-- Rotated from up to left
memory.registerexec(0xCF39, append_log_rotated);	-- Rotated from right to up

memory.registerexec(0xCE9C, append_log_bumped);		-- Bumped into obstacle
memory.registerexec(0xCEB5, append_log_bumped);		-- Bumped into obstacle
memory.registerexec(0xCECE, append_log_bumped);		-- Bumped into obstacle
memory.registerexec(0xCEE7, append_log_bumped);		-- Bumped into obstacle
memory.registerexec(0xCF00, append_log_bumped);		-- Bumped into obstacle
memory.registerexec(0xCF19, append_log_bumped);		-- Bumped into obstacle
memory.registerexec(0xCF32, append_log_bumped);		-- Bumped into obstacle

memory.registerexec(0xCF97, append_log_normal);		-- Jumped to another amoeba
memory.registerexec(0xCFA5, append_log_set);		-- Set amoeba
memory.registerexec(0xCFA3, append_log_refused);	-- Refused to set amoeba
memory.registerexec(0xCF5D, append_log_rts);		-- RTS from UpdateAmoeba

------------------------------------------------------------------------
-- Disassembly of the BoulderDash code in question:
-- 
-- UpdateAmoeba: 
-- ; Called every frame, but works only once per 8 frames
-- >01:CE51:A5 B7     LDA AmoebaPhase = #$82
--  01:CE53:F0 0A     BEQ ExitWithoutUpdate
--  01:CE55:C9 FF     CMP #$FF
--  01:CE57:F0 06     BEQ ExitWithoutUpdate
--  01:CE59:A5 FE     LDA FrameCounter = #$6A
--  01:CE5B:29 07     AND #$07
--  01:CE5D:F0 01     BEQ ReallyUpdateAmoeba
-- ExitWithoutUpdate: 
--  01:CE5F:60        RTS -----------------------------------------------------------
-- ReallyUpdateAmoeba: 
--  01:CE60:A5 B7     LDA AmoebaPhase = #$82
--  01:CE62:29 03     AND #$03
--  01:CE64:D0 0C     BNE PrepareForLoop25
-- RestartFromOrigin: 
-- ; Every time the amoeba multiplies or fails to multiply within 32 frames, it restarts to the original point
--  01:CE66:A9 00     LDA #$00
--  01:CE68:85 B7     STA AmoebaPhase = #$82
--  01:CE6A:A5 B8     LDA AmoebaOriginDirectionAnd_Y = #$41
--  01:CE6C:85 BB     STA AmoebaDirectionAnd_Y = #$41
--  01:CE6E:A5 B9     LDA AmoebaOrigin_X = #$14
--  01:CE70:85 BC     STA Amoeba_X = #$18
-- PrepareForLoop25: 
--  01:CE72:A9 19     LDA #$19
--  01:CE74:85 4D     STA 25Tries = #$0A
-- ; Temp_X = Amoeba_X; Temp_Y = AmoebaDirectionAnd_Y & 00111111b; Temp_AmoebaDirection = AmoebaDirectionAnd_Y & 11000000b
--  01:CE76:A5 BB     LDA AmoebaDirectionAnd_Y = #$41
--  01:CE78:29 3F     AND #$3F
--  01:CE7A:A8        TAY
--  01:CE7B:A5 BB     LDA AmoebaDirectionAnd_Y = #$41
--  01:CE7D:29 C0     AND #$C0
--  01:CE7F:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CE81:A6 BC     LDX Amoeba_X = #$18
--  01:CE83:84 4F     STY Temp_Y = #$38
--  01:CE85:86 50     STX Temp_X = #$0A
-- Loop25Tries: 
--  01:CE87:A5 4E     LDA Temp_AmoebaDirection = #$00
--  01:CE89:C9 40     CMP #$40
--  01:CE8B:F0 21     BEQ TryMultiplying_Up
--  01:CE8D:C9 80     CMP #$80
--  01:CE8F:F0 36     BEQ TryMultiplying_Right
--  01:CE91:C9 C0     CMP #$C0
--  01:CE93:F0 4B     BEQ TryMultiplying_Down
-- TryMultiplying_Left: 
--  01:CE95:20 5E CF  JSR CheckMultiplying_Left
--  01:CE98:B0 05     BCS NextTimeTryDown
--  01:CE9A:F0 0A     BEQ OkSetAmoeba
--  01:CE9C:4C AC CE  JMP Restore_X
-- NextTimeTryDown: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CE9F:A9 C0     LDA #$C0
--  01:CEA1:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CEA3:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CEA6:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CEA9:90 01     BCC Restore_X
--  01:CEAB:60        RTS -----------------------------------------------------------
-- Restore_X: 
--  01:CEAC:E6 50     INC Temp_X = #$0A
-- TryMultiplying_Up: 
--  01:CEAE:20 6A CF  JSR CheckMultiplying_Up
--  01:CEB1:B0 05     BCS NextTimeTryLeft
--  01:CEB3:F0 0A     BEQ OkSetAmoeba
--  01:CEB5:4C C5 CE  JMP Restore_Y
-- NextTimeTryLeft: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CEB8:A9 00     LDA #$00
--  01:CEBA:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CEBC:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CEBF:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CEC2:90 01     BCC Restore_Y
--  01:CEC4:60        RTS -----------------------------------------------------------
-- Restore_Y: 
--  01:CEC5:E6 4F     INC Temp_Y = #$38
-- TryMultiplying_Right: 
--  01:CEC7:20 76 CF  JSR CheckMultiplying_Right
--  01:CECA:B0 05     BCS NextTimeTryUp
--  01:CECC:F0 0A     BEQ OkSetAmoeba
--  01:CECE:4C DE CE  JMP Restore_X
-- NextTimeTryUp: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CED1:A9 40     LDA #$40
--  01:CED3:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CED5:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CED8:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CEDB:90 01     BCC Restore_X
--  01:CEDD:60        RTS -----------------------------------------------------------
-- Restore_X: 
--  01:CEDE:C6 50     DEC Temp_X = #$0A
-- TryMultiplying_Down: 
--  01:CEE3:B0 05     BCS NextTimeTryRight
--  01:CEE5:F0 0A     BEQ OkSetAmoeba
--  01:CEE7:4C F7 CE  JMP Restore_X
-- NextTimeTryRight: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CEEA:A9 80     LDA #$80
--  01:CEEC:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CEEE:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CEF1:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CEF4:90 01     BCC Restore_X
--  01:CEF6:60        RTS -----------------------------------------------------------
-- Restore_X: 
--  01:CEF7:C6 4F     DEC Temp_Y = #$38
-- TryMultiplying_Left2: 
--  01:CEF9:20 5E CF  JSR CheckMultiplying_Left
--  01:CEFC:B0 05     BCS NextTimeTryDown2
--  01:CEFE:F0 0A     BEQ OkSetAmoeba
--  01:CF00:4C 10 CF  JMP Restore_X
-- NextTimeTryDown2: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CF03:A9 C0     LDA #$C0
--  01:CF05:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CF07:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CF0A:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CF0D:90 01     BCC Restore_X
--  01:CF0F:60        RTS -----------------------------------------------------------
-- Restore_X: 
--  01:CF10:E6 50     INC Temp_X = #$0A
-- TryMultiplying_Up2: 
--  01:CF12:20 6A CF  JSR CheckMultiplying_Up
--  01:CF15:B0 05     BCS NextTimeTryLeft2
--  01:CF17:F0 0A     BEQ OkSetAmoeba
--  01:CF19:4C 29 CF  JMP Restore_Y
-- NextTimeTryLeft2: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CF1C:A9 00     LDA #$00
--  01:CF1E:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CF20:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CF23:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CF26:90 01     BCC Restore_Y
--  01:CF28:60        RTS -----------------------------------------------------------
-- Restore_Y: 
--  01:CF29:E6 4F     INC Temp_Y = #$38
-- TryMultiplying_Right2: 
--  01:CF2B:20 76 CF  JSR CheckMultiplying_Right
--  01:CF2E:B0 05     BCS NextTimeTryUp2
--  01:CF30:F0 0A     BEQ OkSetAmoeba
--  01:CF32:4C 42 CF  JMP Restore_X
-- NextTimeTryUp2: 
-- ; Since we just moved the pointer to another amoeba, rotate direction anticlockwise
--  01:CF35:A9 40     LDA #$40
--  01:CF37:85 4E     STA Temp_AmoebaDirection = #$00
--  01:CF39:4C 44 CF  JMP DoUntilAll25TriesExpired
-- OkSetAmoeba: 
--  01:CF3C:20 99 CF  JSR IntendToSetAmoebaToMap
--  01:CF3F:90 01     BCC Restore_X
--  01:CF41:60        RTS -----------------------------------------------------------
-- Restore_X: 
--  01:CF42:C6 50     DEC Temp_X = #$0A
-- DoUntilAll25TriesExpired: 
--  01:CF44:C6 4D     DEC 25Tries = #$0A
--  01:CF46:F0 03     BEQ All25TriesExpired
--  01:CF48:4C 87 CE  JMP Loop25Tries
-- All25TriesExpired: 
-- ; Exits from UpdateAmoeba
--  01:CF4B:A5 4F     LDA Temp_Y = #$38
--  01:CF4D:05 4E     ORA Temp_AmoebaDirection = #$00
--  01:CF4F:85 BB     STA AmoebaDirectionAnd_Y = #$41
--  01:CF51:A5 50     LDA Temp_X = #$0A
--  01:CF53:85 BC     STA Amoeba_X = #$18
--  01:CF55:E6 B7     INC AmoebaPhase = #$82
--  01:CF57:A5 B7     LDA AmoebaPhase = #$82
--  01:CF59:29 83     AND #$83
--  01:CF5B:85 B7     STA AmoebaPhase = #$82
--  01:CF5D:60        RTS -----------------------------------------------------------
-- 
-- CheckMultiplying_Left: 
-- ; Returns C=1 when the pointer is on another amoeba, Z=1 when can multiply
--  01:CF5E:A4 4F     LDY Temp_Y = #$38
--  01:CF60:C6 50     DEC Temp_X = #$0A
--  01:CF62:A6 50     LDX Temp_X = #$0A
--  01:CF64:20 7D CC  JSR GetObjectFromMap
--  01:CF67:4C 8B CF  JMP CheckMultiplying
-- CheckMultiplying_Up: 
-- ; Returns C=1 when the pointer is on another amoeba, Z=1 when can multiply
--  01:CF6A:C6 4F     DEC Temp_Y = #$38
--  01:CF6C:A4 4F     LDY Temp_Y = #$38
--  01:CF6E:A6 50     LDX Temp_X = #$0A
--  01:CF70:20 7D CC  JSR GetObjectFromMap
--  01:CF73:4C 8B CF  JMP CheckMultiplying
-- CheckMultiplying_Right: 
-- ; Returns C=1 when the pointer is on another amoeba, Z=1 when can multiply
--  01:CF76:E6 50     INC Temp_X = #$0A
--  01:CF78:A4 4F     LDY Temp_Y = #$38
--  01:CF7A:A6 50     LDX Temp_X = #$0A
--  01:CF7C:20 7D CC  JSR GetObjectFromMap
--  01:CF7F:4C 8B CF  JMP CheckMultiplying
-- CheckMultiplying_Down: 
-- ; Returns C=1 when the pointer is on another amoeba, Z=1 when can multiply
--  01:CF82:E6 4F     INC Temp_Y = #$38
--  01:CF84:A4 4F     LDY Temp_Y = #$38
--  01:CF86:A6 50     LDX Temp_X = #$0A
--  01:CF88:20 7D CC  JSR GetObjectFromMap
-- CheckMultiplying: 
-- ; Returns C=1 when the pointer is on another amoeba, Z=1 when can multiply
--  01:CF8B:C9 D0     CMP #$D0
--  01:CF8D:F0 08     BEQ OccupiedByAmoeba
--  01:CF8F:C9 00     CMP #$00
--  01:CF91:F0 02     BEQ $CF95
--  01:CF93:C9 20     CMP #$20
-- ; Return result in Z flag: Z=1 when can multiply
--  01:CF95:18        CLC
--  01:CF96:60        RTS -----------------------------------------------------------
-- OccupiedByAmoeba: 
-- ; The place is occupied by another amoeba, so we can't multiply there
-- ;  but instead we will move the pointer there!
--  01:CF97:38        SEC
--  01:CF98:60        RTS -----------------------------------------------------------
-- 
-- IntendToSetAmoebaToMap: 
-- ; Uses Temp_X/Temp_Y, returns C=1 on success, and C=0 when --Countdown_BD != 0
--  01:CF99:A5 B7     LDA AmoebaPhase = #$82
--  01:CF9B:09 80     ORA #$80
--  01:CF9D:85 B7     STA AmoebaPhase = #$82
--  01:CF9F:C6 BD     DEC Amoeba_Timer = #$1D
--  01:CFA1:F0 02     BEQ ReallySetAmoebaToMap
-- DontWantToMultiply: 
-- ; The IntendToSetAmoebaToMap() must be called many times, only then the multiplication will be made
--  01:CFA3:18        CLC
--  01:CFA4:60        RTS -----------------------------------------------------------
-- ReallySetAmoebaToMap: 
-- ; Uses Temp_X/Temp_Y, returns C=1 on success
--  01:CFA5:A4 4F     LDY Temp_Y = #$38
--  01:CFA7:A6 50     LDX Temp_X = #$0A
--  01:CFA9:A9 D0     LDA #$D0
--  01:CFAB:20 77 D0  JSR SetObjectToMap
--  01:CFAE:A4 4F     LDY Temp_Y = #$38
--  01:CFB0:A6 50     LDX Temp_X = #$0A
--  01:CFB2:A9 D0     LDA #$D0
--  01:CFB4:20 B7 D0  JSR AddObjectToBuffer
-- SheduleNextTimer: 
-- ; Every time the waiting period decreases by 4
--  01:CFB7:A5 BA     LDA AmoebaTimerResetValue = #$3F
--  01:CFB9:38        SEC
--  01:CFBA:E9 04     SBC #$04
--  01:CFBC:85 BA     STA AmoebaTimerResetValue = #$3F
--  01:CFBE:85 BD     STA Amoeba_Timer = #$1D
-- ; Next update should reset the pointer to the origin
--  01:CFC0:A9 80     LDA #$80
--  01:CFC2:85 B7     STA AmoebaPhase = #$82
--  01:CFC4:38        SEC
--  01:CFC5:60        RTS -----------------------------------------------------------
------------------------------------------------------------------------

