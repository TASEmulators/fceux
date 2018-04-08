-- Scrolling Pitch Display for FCEUX
-- 2014-07-19
-- Robert Hart (rnhart.net)

SCREEN_LEFT  = 0
SCREEN_RIGHT = 255
MIDI_LEFT  = 18
MIDI_RIGHT = 102

MIDI_RANGE = (MIDI_RIGHT + 1) - MIDI_LEFT
SCREEN_RANGE = (SCREEN_RIGHT + 1) - SCREEN_LEFT

BACKGROUND_COLOR = {0,0,0,70}

SCROLL_BOTTOM = 225
SCROLL_SIZE = 216
KEYBOARD_SIZE = 5

KEYBOARD_TOP    = SCROLL_BOTTOM
KEYBOARD_BOTTOM = SCROLL_BOTTOM + KEYBOARD_SIZE

KEYBOARD_FILLS = ({[0]="white","black","white","black","white","white","black","white","black","white","black","white"})


-- FCEUX 2.2.2 pitch fixes

SHORT_FIX_DOWN = 12 * ( math.log(93/2) / math.log(2) )
DPCM_FIX_DOWN  = 12 * ( math.log(11)   / math.log(2) )


function round(x)
  return math.floor( x + .5 )
end

function Scale(x)
  temp = math.floor ( (x - MIDI_LEFT) * SCREEN_RANGE / MIDI_RANGE + .5 )
  if (temp < SCREEN_LEFT) then temp = SCREEN_LEFT
  elseif (temp > SCREEN_RIGHT) then temp = SCREEN_RIGHT
  end
  return temp
end

WIDTH = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2}

COLOR = {[0]="P39", [1]="P15", [2]="P2C", [3]="P15", ["triangle"]="P12", ["noise-short"]="P30", ["noise-long"]="P10", ["dpcm"]="P1A", ["dpcm-melodic"]="P2A" }


function Reset()

  history = {}
  previous_frame = emu.framecount() - 1


  -- Set up DPCM visualization function

  hash = rom.gethash("md5")

  if (hash == "d4782e6bbf337f075174345fe4848983") then  -- My demo

    DELTAS_PERIOD = 64
    DPCM_KEY_DOWN = 12 * (math.log(DELTAS_PERIOD)/math.log(2))
    DPCM_KEY_DOWN = DPCM_KEY_DOWN + DPCM_FIX_DOWN

    DPCM_Function = function()
      if(snd.dpcm.volume > 0) then
        if (snd.dpcm.dmcaddress == 0xC000) then
          dpcm_key = Scale(snd.dpcm.midikey - DPCM_KEY_DOWN)
          dpcm_color = COLOR["dpcm-melodic"]
          dpcm_width = WIDTH[8]
        else
          dpcm_key = Scale(MIDI_LEFT + 1 + snd.dpcm.regs.frequency)
          dpcm_color = COLOR["dpcm"]
          dpcm_width = WIDTH[8]
        end
        return {dpcm_key, dpcm_color, dpcm_width}
      else
        return nil
      end
    end

  elseif (hash == "e02c7c510e0e37b1f2711103fff9e58a") then   -- Ikinari Musician

    DELTAS_PERIOD = 64
    DPCM_KEY_DOWN = 12 * (math.log(DELTAS_PERIOD)/math.log(2))
    DPCM_KEY_DOWN = DPCM_KEY_DOWN + DPCM_FIX_DOWN

    DPCM_Function = function()
      if(snd.dpcm.volume > 0) then
        if (snd.dpcm.dmcaddress == 0xFF00) then
          dpcm_key = Scale(MIDI_LEFT + 1 + snd.dpcm.regs.frequency)
          dpcm_color = COLOR["dpcm"]
          dpcm_width = WIDTH[round((snd.dpcm.dmcseed/127)*14)+1]
        elseif (snd.dpcm.dmcaddress == 0xFF80) then
          dpcm_key = Scale(snd.dpcm.midikey - DPCM_KEY_DOWN)
          dpcm_color = COLOR["dpcm-melodic"]
          dpcm_width = WIDTH[8]
        end
        return {dpcm_key, dpcm_color, dpcm_width}
      else
        return nil
      end
    end

  else   -- Default DPCM

    previous_seed = sound.get().rp2a03.dpcm.dmcseed

    DPCM_Function = function()
      if(snd.dpcm.volume > 0) then
        previous_seed = snd.dpcm.dmcseed
        return {Scale(MIDI_LEFT + 1 + snd.dpcm.regs.frequency), COLOR["dpcm"], WIDTH[8]}
      elseif ( snd.dpcm.dmcseed ~= previous_seed) then
        seed_change = math.abs(previous_seed - snd.dpcm.dmcseed)
        previous_seed = snd.dpcm.dmcseed
        return {Scale(MIDI_LEFT + 17), COLOR["dpcm"], (math.floor(((seed_change-1)/127)*3))}
      else
        return nil
      end
    end

  end

end


function Draw()
  if(emu.framecount() - previous_frame ~= 1) then Reset() end
  previous_frame = emu.framecount()

  gui.box(0,0,255,239,BACKGROUND_COLOR)

  snd = sound.get().rp2a03

  if(snd.square1.volume > 0) then
    c1 = {Scale(snd.square1.midikey),COLOR[snd.square1.duty], WIDTH[math.floor(15*snd.square1.volume)]}
  else
    c1 = nil
  end

  if(snd.square2.volume > 0) then
    c2 = {Scale(snd.square2.midikey),COLOR[snd.square2.duty], WIDTH[math.floor(15*snd.square2.volume)]}
  else
    c2 = nil
  end

  if((snd.triangle.volume > 0) and (snd.triangle.regs.frequency > 1)) then
    c3 = {Scale(snd.triangle.midikey),COLOR["triangle"], WIDTH[8]}
  else
    c3 = nil
  end

  if(snd.noise.volume > 0) then
    if(snd.noise.short) then
      noise_key = Scale(snd.noise.midikey - SHORT_FIX_DOWN)
      noise_color = COLOR["noise-short"]
    else
      noise_key = Scale(MIDI_LEFT + 16 - snd.noise.regs.frequency)
      noise_color = COLOR["noise-long"]
    end

    c4 = {noise_key, noise_color, WIDTH[math.floor(15*snd.noise.volume)]}
  else
    c4 = nil
  end

  c5 = DPCM_Function()

  table.insert(history, 1, {c1,c2,c3,c4,c5})
  table.remove(history, SCROLL_SIZE)

  -- draw keyboard

  for i = MIDI_LEFT,MIDI_RIGHT do
    fill   = KEYBOARD_FILLS[i%12]
    left   = Scale(i - .5)
    right  = Scale(i + .5)
    gui.box(left,KEYBOARD_TOP,right,KEYBOARD_BOTTOM,fill,"gray")
  end

  -- draw notes

  for i = 1, #history do
    for j = 1, 5 do
      if(type(history[i][j]) ~= "nil") then
        x = history[i][j][1]
        y = SCROLL_BOTTOM - i
        c = history[i][j][2]
        w = history[i][j][3]
        gui.box(x-w-1,y-1,x+w+1,y,"black")
        gui.line(x-w,y,x+w,y,c)
      end
    end
  end

end

Reset()
emu.registerafter(Draw)

emu.print(     "   blue	= triangle     	   gray	= noise, long*")
emu.print(     "   cyan	= pulse, 1/2   	  white	= noise, short")
emu.print(     "    red	= pulse, 1/4   	  dk gn	= dpcm, non-periodic*")
emu.print(     " yellow	= pulse, 1/8   	  green	= dpcm, periodic")
emu.print(     "")
emu.print(     "* percussive frequencies shown as 16 lowest pitches")

