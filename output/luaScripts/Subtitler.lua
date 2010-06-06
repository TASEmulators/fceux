--FCEUX Subtitler Express, by FatRatKnight
-- I find the built-in subtitles for FCEUX's .fm2 is somewhat inconvenient.
-- This script should make the process smoother.

-- Run the script. Type whatever you like. Letters and such will appear at the
-- top of the game display. The script can output a line of text that FCEUX
-- can understand and translates into the appropriate subtitle. It can ouput to
-- a file or to the display box

-- Whatever the script outputs, you can copy that and paste it into the .fm2
-- file in question, right before where all the actual frames begin. I suggest
-- WordPad to open the .fm2 file

-- Let a frame pass, and the text becomes translucent. This is to signify that
-- the text will remain visible as an actual subtitle, but you can put in a new
-- subtitle here without destroying the old one. It will just override the old
-- subtitle when it's time for the new one to show up.

-- Still, when it comes to translucent text, it's pretty dumb around loading
-- states. Don't try to worry too much about it.

-- Beware of hotkeys you set in FCEUX, or even the defaults like I, P, shift+R,
-- shift+L, comma, and period. These can and will interfere with this script
-- if you don't disable them.

-- Please don't try to type while emulation is unpaused. This script is not
-- smart enough to know how to handle unpaused typing.

-- Don't let the warnings scare you. I hope the script makes subtitling easy.


local BypassFrameAdvanceKey= "backslash"   --Set this to the frameadvance key.

local SaveToThisFile= "ConvenientSubtitles.txt"  --Location to save a file

local SaveOnExit= true          --Dost thou wish to save when the script exits?


-- List of hardcoded commands:
--   Backspace: Delete a single character
--   Delete: Erase the subtitle for this frame
--   Enter: Prints the line containing the subtitle at this frame for .fm2
--   Home: Prints subtitle count
--   End: Prints all subtitles in a format for .fm2
--   Page Down: Save subtitles to file
--   Insert: Toggle SaveOnExit



local WordyWords= {}
local CurrentStr= ""

local KeyTable= {
a="a",b="b",c="c",d="d",e="e",f="f",g="g",h="h",i="i",j="j",k="k",l="l",m="m",
n="n",o="o",p="p",q="q",r="r",s="s",t="t",u="u",v="v",w="w",x="x",y="y",z="z",
tilde="`",minus="-",plus="=",
leftbracket="[",rightbracket="]",backslash="\\",
semicolon=";",quote="'",
comma=",",period=".",slash="/",

A="A",B="B",C="C",D="D",E="E",F="F",G="G",H="H",I="I",J="J",K="K",L="L",M="M",
N="N",O="O",P="P",Q="Q",R="R",S="S",T="T",U="U",V="V",W="W",X="X",Y="Y",Z="Z",
TILDE="~",MINUS="_",PLUS="+",
LEFTBRACKET="{",RIGHTBRACKET="}",BACKSLASH="|",
SEMICOLON=":",QUOTE="\"",
COMMA="<",PERIOD=">",SLASH="?",

numpad1="1",numpad2="2",numpad3="3",numpad4="4",numpad5="5",
numpad6="6",numpad7="7",numpad8="8",numpad9="9",numpad0="0"}

KeyTable["1"]="!"
KeyTable["2"]="@"
KeyTable["3"]="#"
KeyTable["4"]="$"
KeyTable["5"]="%"
KeyTable["6"]="^"
KeyTable["7"]="&"
KeyTable["8"]="*"
KeyTable["9"]="("
KeyTable["0"]=")"


local fc, lastfc= 0, 0
--*****************************************************************************
local function UpdateFC()  lastfc= fc; fc= movie.framecount()  end
--*****************************************************************************
-- Quick function. I just want it to make use of movie.framecount convenient.


--*****************************************************************************
local function GetSortedSubs()
--*****************************************************************************
    local ReturnTbl= {}
    local count= 0
    for frame,text in pairs(WordyWords) do
        count= count+1
        ReturnTbl[count]= frame
    end
    table.sort(ReturnTbl)
    return ReturnTbl
end



--*****************************************************************************
local function FixSubs()
--*****************************************************************************
-- Plants current subtitle into an array and fetches a new one.
-- Should be called every time the frame count changes. Every. Single. Time.
-- Can be called when the frame hasn't changed yet, if you need to.

    UpdateFC()

    if CurrentStr == "" then
        WordyWords[lastfc]= nil
    else
        WordyWords[lastfc]= CurrentStr
    end
 
   CurrentStr= WordyWords[fc]
    if not CurrentStr then  CurrentStr= ""  end
end

local LastSubfc= 0
local LastSub= ""
--*****************************************************************************
local function GhostifySubs()
--*****************************************************************************
    if CurrentStr ~= "" then
        gui.text(  3,  9,CurrentStr)
    elseif (fc < LastSubfc+300) and (fc > LastSubfc) then
        gui.opacity(.5)
        gui.text(  3,  9,LastSub)
    end
    gui.pixel(0,0,0)
    gui.opacity(1)
end


--*****************************************************************************
local function SaveToFile()
--*****************************************************************************
    FixSubs()
    local OutFile, Err= io.open(SaveToThisFile,"w")
    if not OutFile then
        print("File write fail")
        print(Err)
        return false
    end
    local Subs= GetSortedSubs()
    for i= 1, #Subs do
        OutFile:write("subtitle ".. Subs[i] .." ".. WordyWords[Subs[i]] .."\n")
    end
    OutFile:close()
    print("Saved",#Subs,"lines to",SaveToThisFile)

    return true
end


local lastkeys= input.get()
--*****************************************************************************
local function KeyReader()
--*****************************************************************************
  if fc ~= movie.framecount() then  --Egad, panic! New frame! Handle it!!
      FixSubs()  -- Whew. Problem solved.

  else -- Oh? Don't panic then

    local keys= input.get()
    for k,v in pairs(keys) do
        if not lastkeys[k] then
            if     k == BypassFrameAdvanceKey then
                --... It's a bypass. Don't process.
            elseif k == "backspace" then
                if CurrentStr ~= "" then
                    CurrentStr= string.sub(CurrentStr,1,-2)
                end
            elseif k == "delete" then
                CurrentStr= ""
            elseif k == "space" then
                CurrentStr= CurrentStr .. " "
            elseif k == "enter" then
                print("subtitle",fc,CurrentStr)
            elseif k == "end" then
                FixSubs()
                print("===========================")
                local fr= GetSortedSubs()
                for i= 1, #fr do
                    print("subtitle",fr[i],WordyWords[fr[i]])
                end
            elseif k == "home" then
                local fr= GetSortedSubs()
                print("Subtitle count:",#fr)
            elseif k == "pagedown" then
                SaveToFile()
            elseif k == "insert" then
                SaveOnExit= not SaveOnExit
                if SaveOnExit then print("SaveOnExit activated!")
                else               print("Removed SaveOnExit.") end
            else
                local ThisKey= k
                if keys.shift then ThisKey= string.upper(ThisKey)
                else               ThisKey= string.lower(ThisKey) end
                if KeyTable[ThisKey] then
                    CurrentStr= CurrentStr .. KeyTable[ThisKey]
                end
            end
        end
    end
    GhostifySubs()

    lastkeys= keys

  end -- New frame test

end

--*****************************************************************************
local function Ex()
--*****************************************************************************
    if SaveOnExit then
        if not SaveToFile() then
            print("Dumping subtitles in message box instead!")
            print("===========================")
            local fr= GetSortedSubs()
            for i= 1, #fr do
                print("subtitle",fr[i],WordyWords[fr[i]])
            end
        end
    end
end

--*****************************************************************************
local function Bfr()
--*****************************************************************************
    if CurrentStr ~= "" then
        LastSubfc= fc
        LastSub= CurrentStr
    end

    FixSubs()
end

gui.register(KeyReader)
emu.registerbefore(Bfr)
emu.registerexit(Ex)