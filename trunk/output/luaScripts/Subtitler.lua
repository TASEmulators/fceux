--FCEUX Subtitler Express v2, by FatRatKnight
-- I find the built-in subtitles for FCEUX's .fm2 is somewhat inconvenient.
-- This script should make the process smoother.

-- Run the script. Type whatever you like. Letters and such will appear at the
-- top of the game display. The script can output a line of text that FCEUX
-- can understand and translates into the appropriate subtitle. It can ouput to
-- a file or to the display box

-- Whatever the script outputs, you can copy that and paste it into the .fm2
-- file in question, right before where all the actual frames begin. I suggest
-- WordPad to open the .fm2 file.

-- If you're really that lazy, run a movie, subtitle it, then hit pageup twice.
-- The movie will be stopped, but this script will replace the old subtitles
-- with the new ones. It will actually create a new file appended with _S in
-- case the script royally screws up somehow.

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


-- List of commands:
local DelChar= "backspace"  -- Delete a single character
local DelSub=  "delete"     -- Erase the whole subtitle for this frame
local ShoLine= "enter"      -- Prints displayed line in .fm2 readable format
local SubCnt=  "home"       -- Counts current number of stored subtitles
local PntAll=  "end"        -- Prints all subtitles formatted for .fm2
local SaveTxt= "pagedown"   -- Saves subtitles to file
local T_AutoS= "insert"     -- Toggles whether the script saves on exit
local SvToMov= "pageup"     -- Saves subtitles to the movie file itself



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
numpad6="6",numpad7="7",numpad8="8",numpad9="9",numpad0="0",

space= " ", SPACE= " "}

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

--LengthsTable= {}

--for k,v in pairs(KeyTable) do
--    LengthsTable[v]= gui.text(0,0,v)
--end


local fc, lastfc= 0, 0
--*****************************************************************************
local function UpdateFC()  lastfc= fc; fc= movie.framecount()  end
--*****************************************************************************
-- Quick function. I just want it to make use of movie.framecount convenient.


--*****************************************************************************
local function GetSortedSubs()
--*****************************************************************************
-- Returns a sorted array of frame numbers for every subtitle

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
-- Displays subtitles. Needs a little support from emu.registerbefore to know
-- how to see an old subtitle after you decide to erase a new one.

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
local function SaveToFm2()
--*****************************************************************************
-- Saves to a copy of the currently playing .fm2. It will append _S to the new
-- filename, so run MySweetMovie.fm2, this script makes MySweetMovie_S.fm2
-- By the way, this script tells FCEUX to stop "owning" the file as a way to
-- guarantee that this script can access it without interference.

    FixSubs()

    if not movie.active() then
        print("Error: No movie!")
        return false
    end

    local OurMovie= movie.name()

    if string.match(OurMovie,".zip|") then
        print("Error: File inside a zip!")
        return false
    end

    movie.close()
    local MyFile, err= io.open(OurMovie,"r")
    if not MyFile then
        print("Error: Unable to open file for some strange reason.")
        print(err)
        return false
    end

    local MovieData= MyFile:read("*a")
    MyFile:close()

    local PreSub=  string.find(MovieData,"\nsubtitle",1,true)
    local PreData= string.find(MovieData,"\n|%d|")
--    local RemovedSubs= nil
    if PreSub then
--        RemovedSubs= string.sub(MovieData,PreSub,PreData-1)
        MovieData= (string.sub(MovieData,1,PreSub-1)
                 .. string.sub(MovieData,PreData))
        PreData= PreSub
    end

    local S= GetSortedSubs()
    local Write= ""
    for i= 1, #S do
        Write= Write .. "subtitle " .. S[i] .. " ".. WordyWords[S[i]] .. "\n"
    end

    OurMovie= string.sub(OurMovie,1,-5) .. "_S" .. string.sub(OurMovie,-4)

    MyFile= io.open(OurMovie,"w")
    MyFile:write(string.sub(MovieData,1,PreData)
              .. Write
              .. string.sub(MovieData,PreData+1))
    MyFile:close()

--    print(RemovedSubs)
    print("Success")
    return true
end


--*****************************************************************************
local function SaveToTxt()
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



--*****************************************************************************
local KeyFunctions= {}
--*****************************************************************************
-- Greatly simplifies KeyReader for me. Also, faster processing should the list
-- get pretty large.
-- Various odds and ends that I want done shall be done here.

function KeyFunctions._DelChar()  --Remove a single character
    if CurrentStr ~= "" then
        CurrentStr= string.sub(CurrentStr,1,-2)
    end
end
KeyFunctions[DelChar]= KeyFunctions._DelChar


function KeyFunctions._DelSub()  --Remove the whole subtitle
    CurrentStr= ""
end
KeyFunctions[DelSub]= KeyFunctions._DelSub


function KeyFunctions._ShoLine()  --Dump the currently displayed line
    print("subtitle",fc,CurrentStr)
end
KeyFunctions[ShoLine]= KeyFunctions._ShoLine


function KeyFunctions._SubCnt()  --Count subtitles
    local fr= GetSortedSubs()
    print("Subtitle count:",#fr)
end
KeyFunctions[SubCnt]= KeyFunctions._SubCnt


function KeyFunctions._PntAll()  --Dump all subtitles
                FixSubs()
                print("===========================")
                local S= GetSortedSubs()
                for i= 1, #S do
                    print("subtitle",S[i],WordyWords[S[i]])
                end
end
KeyFunctions[PntAll]= KeyFunctions._PntAll


function KeyFunctions._SaveTxt()  --Save to a file
    SaveToTxt()
end
KeyFunctions[SaveTxt]= KeyFunctions._SaveTxt


function KeyFunctions._T_AutoS()  --Toggle Save-On-Exit
    SaveOnExit= not SaveOnExit
    if SaveOnExit then print("SaveOnExit activated!")
    else               print("Removed SaveOnExit.") end
end
KeyFunctions[T_AutoS]= KeyFunctions._T_AutoS

local AmIClicked= false
function KeyFunctions._SvToMov()  --Saves directly to the movie itself
    if not movie.active() then
        print("No movie detected. Doing nothing...")
        return
    end

    if not AmIClicked then
        AmIClicked= true
        print("Movie detected. Hit",SvToMov,"again to stop movie and",
              "save subtitles directly into movie! Advance a frame to cancel.")
    else
        SaveToFm2()
    end
end
KeyFunctions[SvToMov]= KeyFunctions._SvToMov


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
            elseif KeyFunctions[k] then
                KeyFunctions[k]()
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
        if not SaveToTxt() then
            print("Dumping subtitles in message box instead!")
            print("===========================")
            local S= GetSortedSubs()
            for i= 1, #fr do
                print("subtitle",S[i],WordyWords[S[i]])
            end
        end
    end
end

--*****************************************************************************
local function Bfr()
--*****************************************************************************
    AmIClicked= false

    if CurrentStr ~= "" then
        LastSubfc= fc
        LastSub= CurrentStr
    end

    FixSubs()
end

gui.register(KeyReader)
emu.registerbefore(Bfr)
emu.registerexit(Ex)