<?php
	$title = "cross-platform NES emulation for professionals";
	include("header.php");
?>

<div class="mainbox">
 <h2 class="boxhead">Introduction </h2>
 <div class="boxbody">
   <p>
     FCEUX is a cross platform, NTSC and PAL Famicom/NES emulator that is an evolution of the original FCE Ultra emulator.  
     Over time FCE Ultra had separated into many separate branches.
   </p>
   <p>
     The concept behind FCEUX  is to merge elements from FCE Ultra, FCEU rerecording, FCEUXD, FCEUXDSP, and FCEU-mm into a single branch of FCEU.
     As the X implies, it is an all-encompassing FCEU emulator that gives the best of all worlds for the casual player,
     the ROM-hacking community, Lua Scripters, and the Tool-Assisted Speedrun Community.
   </p>
   <p>
     We are particularly excited by the Lua scripting capabilities which have been ported over from FCEU rerecording and prepped for 
further development.  FCEUX2.1 comes pre-packaged with all Lua related .dlls.  In order to use Lua in windows builds prior to 2.1 you will need the luapack [<a 
href="http://fceux.com/luapack">fceux.com/luapack</a>] which should be unzipped alongside your fceux.exe. The large numbers of dlls 
wasn't our choice; but it will enable the iup user interface library which will permit scripts to create their own guis, as well as 
rendering the base FCEUX distribution more compact.  Examples of lua scripts may be found in the subversion repository <a 
href="http://fceultra.svn.sourceforge.net/viewvc/fceultra/fceu/output/luaScripts/">here</a>.  In addition, FCEUX2.1 comes pre-packaged with these scripts.
  <p>
    You can find out what we've been up to since the last release by checking the <a href="http://fceultra.svn.sourceforge.net/viewvc/fceultra/fceu/changelog.txt">changelog</a> directly.
</p><p>
    The latest win32 interim build is generally fresh within a day or two at <a href="http://fceux.com/zip">http://fceux.com/zip</a> -- If you are working with a developer to fix an issue affecting you then this is where you will find your fixed build. 
   <p>
     We have a mailing list for all kinds of discussion about FCEUX.  You can subscribe at the
     <a href="https://lists.sourceforge.net/lists/listinfo/fceultra-discuss">fceultra-discuss</a>
     mailing list page.
   </p>
   <p>
     Of course, we are always hanging out at irc.libera.chat #fceu
   </p>
   
 </div>
</div>

<div class="mainbox">
  <h2 class="boxhead">News</h2>
  <!-- News section -->
  <div class="boxbody">

    <h3 class="boxsubhead">3 November 2009</h3>
    <p>
      <strong>FCEUX 2.1.2 release</strong>
    </p>
    <p>
      Jump to the <a href="download.php">download</a> page.
      <p>
        The 2.1.2 release that fixes some bugs of 2.1.1, increases game compatibility, launches a new PPU core, and adds usability enhancements to the windows port.
      </p>
        
      <p>
      Checkout our <a href="pressrelease-2.1.2.html">Press release</a> for details.
      
      or
      
      Jump to the <a href="download.php">download</a> page.
      </p>
	<p>
	-adelikat
	</p>


<div class="mainbox">
  <h2 class="boxhead">News</h2>
  <!-- News section -->
  <div class="boxbody">

    <h3 class="boxsubhead">29 July 2009</h3>
    <p>
      <strong>FCEUX 2.1.1 release</strong>
    </p>
    <p>
      Jump to the <a href="download.php">download</a> page.
      <p>
        2.1.1 features a new, more accurate sound core.  In addition it provides numerous small fixes and enhancements to both the Win32 and SDL ports.
        <pre>
          <b>Common - Bug fixes</b>
          *Fixed reported issue 2746924 (md5_asciistr() doesn't produce correct string)
          *made default save slot 0 instead of 1

          <b>Improved Sound core/PPU</b>
          *Fixed the noise value, it seems that the noise logic was shifting the values to the left by 1 when reloading, but this doesnt work for PAL since one of the PAL reload value is odd, so fix the logic and used the old tables. Revert a stupid CPU ignore logic in PPU. Sorry about that.
          *Updated with the correct values for the noise and DMC table,
          *Fixed the CPU unofficial opcode ATX, ORing with correct constant $FF instead of $EE, as tested by blargg's. These fixes passes the IRQ flags test from blargg, and also one more  opcode test from blargg's cpu.nes test.
          *Square 1 & square 2 volume controls no longer backwards
          *Length counters for APU now correct variables

          <b>NewPPU (still experimental, enabled by setting newppu 1 in the config file)</b>
          *Added experimental $2004 reading support to play micro machines with (little) shakes, and fixed some timing in the new PPU.
          *Added palette reading cases for the new PPU.

          <b>Win32</b>
          *Minor Bug fixes
          *Replay movie dialog - Stop movie at frame x feature - fixed off by 1 error on the stop frame number
          *Hex Editor - changed ROM values again dsiplay as red, saved in the config as RomFreezeColor
          *Fixed bug in memory watch that would make the first watch value drawn in the wrong place if watch file was full
          *Debugger - Step type functions now update other dialogs such as ppu, nametable, code/data, trace logger, etc.
          *"Disable screen saver" gui option now also diables the monitor powersave
          *Recent menus - no longer crash if item no longer exists, instead it ask the user if they want to remove the item from the list
          *Sound Config Dialog - When sound is off, all controls are grayed out
          *Memory Watch - fixed a regression made in 2.0.1 that broke the Save As menu item
          *Memory Watch - save menu item is grayed if file hasn't changed


          <b>GUI/Enhancements</b>
          *Last save slot used is stored in the config file
          *Made fullscreen toggle (Alt+Enter) remappable
          *Hex editor - Reverted fixedFontHeight to 13 instead of 14.  Gave the option of adjusting the height by modifying RowHeightBorder in the .cfg file
          *Hex Editor - allowed the user to customize the color scheme by use of RGB values stored in the .cfg file
          *Hex editor - freeze/unfreeze ram addresses now causes the colors to update immediately, but only with groups of addresses highlighted at once (single ones still don't yet update)
          *Hex Editor - Save Rom As... menu option enabled and implemented
          *Window caption shows the name of the ROM loaded
          *Recent Movie Menu added
          *Load Last Movie context menu item added
          *Save Movie As... context menu item (for when a movie is loaded in read+write mode)
          *Drag & Drop support for all files related to FCEUX including:
          *.fcm (autoconverts to .fm2 and begins movie playback)
          *Savestates
          *Palette files (.pal)
          *Commandline - -palette commandline option
          *Memory Watch - option to bind to main window, if checked it gives GENS dialog style control, where there is no extra task bar item, and it minimizes when FCEUX is minimized

          <b>SDL</b>

          *added --subtitles
          *fixed Four Score movie playback
          *added --ripsubs for converting fm2 movie subtitles to an srt file
          *Lua is optional again, fixed the real issue
          *Lua is NO longer optional, so the SConscripts have been updated to reflect that change.  This fixes the mysterious non-working input issue.
          *implemented saving/loading a savestate from a specific file on Alt+S/L
          *implemented starting an FM2 movie on Alt+R
          *added --pauseframe to pause movie playback on frame x
          *dropped UTFConverter.c from SDL build
          *added hotkey Q for toggling read-only/read+write movie playback
        </pre>
      </p>
    </p>
    adelikat
    </p>

<div class="mainbox">
  <h2 class="boxhead">News</h2>
  <!-- News section -->
  <div class="boxbody">

    <h3 class="boxsubhead">05 April 2009</h3>
    <p>
      <strong>FCEUX 2.1.0a release</strong>
    </p>
    <p>
      Jump to the <a href="download.php">download</a> page.
      <p>
        2.1.0a fixes a minor issue involving movie recording on both platforms.  It fixes an issue where extra bytes where being appended 
        to the author field of the .fm2 file, resulting in bloated files.
       
      </p>
    </p>
    -- adelikat
    </p>

<div class="mainbox">
  <h2 class="boxhead">News</h2>
  <!-- News section -->
  <div class="boxbody">

    <h3 class="boxsubhead">29 March 2009</h3>
    <p>
      <strong>FCEUX 2.1.0 release</strong>
    </p>
    <p>
      Jump to the <a href="download.php">download</a> page.
      <p>
        The 2.1.0 is a major new release that incorporates new mappers and mapper fixes from FCEU-mm.  In addition it offers a multitude of bug fixes, and 
        feature enhancements to both the Win32 and SDL ports.
        <pre>
<b>Major Bug / Crash Bug Fixes</b>

*Fixed throttling problems that resulted on AMD Dualcore processors. (Caused FCEUX to appear to be in turbo mode).
*Fix major crash issue where NROM game (such as SMB) savestates were writing erroneous information if a non NROM game was loaded prior.
*Fixed a bug that caused a new sav file to not get created when loading a 2nd battery backed game.
*Fix Directory Overrides so to allow users to have no override.  Also fixes directory override reset bug.

<b>Minor Bug fixes</b>

*Hotkeys - prevent "Hotkey explosion" where some laptop keys set off all unassigned hotkeys
*Timing - "disable throttling when sound is off" now only affects FCEUX when sound is off
*Clip Left and Right sides taken into account when drawing on screen (record/play/pause, lag & frame counters, messages, etc)
*Fixed bug where having sound off and Mute turbo caused chirps when toggling
*Video settings - fixed bug when both aspect ratio correction and special scaling 3x are set, video was getting resized incorrectly
*Auto-save cleanup -prevent loading an auto-save from previous session.  Added flags for enabling auto-save menu item.
*Fixed issues related to big endian compiling.
*Fix bug so that Escape can now be assigned as a hotkey
*Fixed bug in screenshot numbering that caused numbering to not reset when changing games.

<b>SDL</b>
*SDL Movie subtitle support and subtitle toggle hotkey added.
*SDL Added fcm to fm2 converter tool to SDL version.
*SDL Improved the SDL sound code; drastically improves quality of sound.
*SDL Savestate slots are now mappable.
*SDL Major updates to SDL documentation
*SDL Added Shift+M for toggling automatic movie backups.
*SDL Added option to mute FCEUX for avi capturing, check the documentation for more details.
*SDL Added --noconfig command line option
*SDL Frame Advance Skip Lag frames toggle implemented

<b>New Features Win32</b>

*The latest mappers and mapper fixes from FCEU-mm.  Adds support for many new games such as Warioland II (Unl), Shu Qi Yu,  and Street Dance
*Full screen mode fixed!  Also, Alt+Enter properly toggles full screen.
*Individual control for sound channels! (See sound config for details).
*Undo/Redo Savestate/Loadstate features installed!  No more loss of data to unintentional presses.  (See getting started for details).
*Movie subtitles can now be included in .fm2 files.  See .fm2 documentation for details and Movie options for details on customizing.
*Auto-backup for movie files.  (See movie options for details).
*A Ram change monitor for the Memory watch dialog. (see memwatch for details).
*Frame counter works even without a movie loaded.
*AVI Directory Override option.

<b>GUI / Menu Enhancements</b>

*A right-click context menu added!  Includes many commonly used items for a variety of situations.
*Menu items that are hotkey mappable now show their current hotkey mapping
*Major overhaul to the Menu organization.
*All FCEUX features are now accessible in the menu
*Alt Menu Shortcuts properly configured
*Menu items are properly grayed when not useable
*All movie related menu items moved to a Movie options dialog
*Removed hard-coded Accel keys and replaced with re-mappable hotkeys (Open & Close ROM)
*Drag & Drop for .fm2 and .lua files
*Many new functions added to the context menu (See context menu for details)
*New Mappable Hotkeys: Open Cheats, Open ROM, Close ROM, Undo/Redo savestate, Toggle Movie Subtitles

<b>Lua</b>

*Added input.get() !  Returns the mouse info and all keyboard buttons pressed by the user.
*Fixed joypad.set().  False now sets a button to off.  Nil does not affect the button at all (allowing the user to still control it).
*gui.text() Increased height (to approx. 7 lines).
*speedmode("turbo") now turns on turbo (which employs frame-skipping) rather than max speed.
*memory.readbyte will recognize frozen addresses (cheats).
*movie.framecount() always return a number, even when no movie is playing (since the frame counter is implemented without a movie loaded).
*Added FCEU.poweron()
*Added FCEU.softreset()
*Added FCEU.lagged()
*Added FCEU.lagcount()
*Added FCEU.getreadonly()
*Added FCEU.setreadonly()
*Added FCEU.fceu_setrenderplanes(sprites, background)
*Added movie.active()
*Added movie.rerecordcount()
*Added movie.length()
*Added movie.getname()
*Added movie.playbeginning()
*Added -lua command line argment, loads a Lua script on startup
*Added zapper.read() - returns the zapper (mouse) data.  (Currently does return zapper data in movie playback).
*Added joypad.write and joypad.get for naming consistency.
*Added rom.readbyte()
*Added rom.readbytesigned()

<b>Sound Config</b>

*Turning sound off disabled sound config controls
*Re-enabled sound buffer time slider control

<b>Hex Editor</b>

*Freezing ram addresses automatically updates the Cheats dialog if it is open.
* Added prevention from freezing more than 256 addresses at once (doing so caused crash bugs).
*Dialog remembers window size.
*Dump Rom & Dump PPU to file Dialogs use ROM to build default filename
*Maximize and minimize buttons added.
*Help menu item added

<b>Memory Watch</b>

*Dialog now includes Ram change monitoring. (see memwatch for details).
*Dialog is now collapsible to 1 column.
*No longer crashes when attempting to load an invalid file from the recent file menu.
*Cancel option added to the save changes dialog.
*Memory address values that are frozen by the debugger or hex editor are displayed in blue.
*Fixed bug that caused dialog to "disappear" due to saving -32000 as its window position.
*Save as dialog uses ROM name to build a default memory watch filename if there was no last used memory watch filename
*Drag and drop for .txt (memory watch) files.
*Minor menu and hotkey fixes.
*Watch values now compatible with custom windows dialog colors.

<b>Debugger</b>

*Shows scanlines and PPU pixel values
*Shows scanlines even while in VBlank
*Added a Run Line button (runs 1 scanline per click)
*Run 128 Lines button (runs 128 scanlines per click)
*Number of active cheats listed.
*Cheats list automatically updated if ram addresses are frozen in the hex editor.
*Fixed bug that caused dialog to "disappear" due to saving -32000 as its window position.
*Debugger now has a minimum valid size
*Added "Restore original window size" button

<b>PPU Viewer</b>

*Default refresh value set to 15
*Refresh value stored in the .cfg file

<b>Nametable Viewer</b>

*Default refresh value set to 15
*Refresh value stored in the .cfg file

<b>Trace Logger</b>

*Fixed bug where user can't scroll the log window while it is auto-updating.
*Changed message about F2 pause (left over from FCEUXDSP) to display the current hotkey mapping.

<b>Text Hooker</b>

*Saving a .tht file no longer crashes
*Dialog updates every frame
*Initialization error checking reinstalled,
*Dialog remembers window position
*Fixed bug where canceling save as produces an error message.
*Save As produces default filename based on the current ROM

<b>Message Log</b>

*Remembers X,Y position
*Resized width and height
*Allowed more lines of text to appear on the screen at once.

<b>Metadata</b>

*Remembers window position
*Can be called from the context menu if a movie is loaded (see context menu for details).

<b>TASEdit</b>

*added help menu item
*disabled menu items that are not currently implemented.

<b>Turbo</b>
*Turbo now employs frame skip, greatly increasing its speed
*The mute turbo option completely bypasses sound processing (another big speed boost)
*Turbo now works with the Lazy wait for VBlank sync setting
        </pre>
      </p>
    </p>
    -- adelikat
    </p>

    <div class="mainbox">
<h2 class="boxhead">News</h2>   <!-- News section -->
<div class="boxbody">

 <h3 class="boxsubhead">02 November 2008</h3>
  <p>
<strong>FCEUX 2.0.3 release</strong>
  </p>
  <p>Jump to the <a href="download.php">download</a> page.
  <p>
We have been trying to address issues which are holding back adoption of FCEUX for both the SDL and windows builds.
<pre>
emulator:
bug: fix fcm conversion, recording, and playback of reset and power commands
bug: prevent excessively long movie filenames
bug: SF [ 2085437 ] should fix issues with missing author field in fcm convert crashing fceux
bug: fix savestate recovery code which prevented aborted savestate loads from recovering emulator state correctly.
bug: gracefully handle non-convertible broken utf-8 text without crashing
enh: permit user optionally to proceed through the movie savestate mismatch error condition, in case he knows what he is doing.
enh: Adding an experimental (optional and undocumented) new PPU. Slow, but more precise in some ways. 
     Contact developers if you think you need more precision

sdl: 
bug: fixed issue where windowed mode would always be set to 32 bpp
bug: SF [ 2062823 ] fixed ppc build errors and added LSB_FIRST option to build scripts
bug: SF [ 2057008 ] lua is now optional, thanks shinydoofy for a patch.  also fixed some build issues.
bug: SF [ 2008437 ] fixed an issue where flawed movie would crash fceux on every startup
enh: added support for AVI creation for SDL, see documention/Videolog.txt for more
enh: toggle lag frame counter for SDL, default hotkey F8
enh: toggle skipping of lag frames for SDL, default hotkey F6
enh: made the input config window more usable
enh: --inputcfg can now be used without a filename
enh: SF [ 2179829 ] user ability to toggle "bind savestates to movie" added for SDL, default hotkey F2
enh: SF [ 2047057 ] added uninstall script for gfceux

windows:
bug: SF [ 2073113 ] Child windows inside debugging window get invalid sizes
bug: Sound config dialog will now look to see if Mute Turbo should be checked
bug: hex editor find dialog does not reopen (fixed)
bug: SF [ 2058942 ] Load state as... does not use the savestate override dir (fixed; now, it does)
bug: fix problem where replay dialog couldnt work when the process current directory 
     had changed to something other than emulator base directory
bug: fix issue where keyboard keys get stuck when switching between debugger window and main window
enh: added a toggle for binding savestates to movies
enh: added -cfg (config file) command line argument
enh: add maximize button to debugger window
enh: bind a menu option for display framecounter
enh: autoload the only useful rom or movie from an archive, in cases where there is only one
enh: don't read every archive file when scanning for replay dialog. scan them, and only look for *.fm2

emulua:
bug: SF [ 2153843 ] Lua ignores second joypad.set()
enh: add execute timeout functions useful for lua contests
</pre>
</p>
  </p>
   -- zeromus
  </p>

  <h3 class="boxsubhead">14 August 2008</h3>
  <p>
  <strong>FCEUX 2.0.2 release</strong>
  </p>
  <p>Jump to the <a href="download.php">download</a> page.
  <p>
This release focuses on top-rated user bug and enhancement issues; stabilizing the SDL build; and repairing things that we had recently broken while adding other features to the emulator.
<pre>
emulator:
bug: restore support for old-format savestates.
bug: restore savestate load error-recovery system
bug: restore IPS patching capability which was lost when archive support was added
bug: restore ungzipping (and unzipping in sdl) capability which was lost when archive support was added
bug: SF [ 2046985 ] SRAM not wiped on power cycle (during movies)
bug: palflag 1 in .fm2 files crashes fceux
enh: add lagcounter and lagflag to savestate
enh: support loading movies from archives

mappers:
bug: remove cnrom chr rom size limit for homebrew roms
bug: upgrade to cah4e3's latest mapper 163&164 code to fix a crash in a game
bug: mmc5 - 64KB WRAM games now work correctly
bug: mmc5 - use of chr A regs for BG in sprite 8x8 mode is fixed

windows:
bug: fix a bug which caused fourscore emulation to fail in some cases
bug: SF [ 2030405 ] Avi recording: no sound b0rks format
bug: SF [ 2040448 ] View Slots bug - does not include new savestate naming
bug: prevent windows from memory re-positioning themselves in the abyss when theyre closed while minimized
bug: removed broken hotkey ctrl+x (prevented cut from working in accel dialogs)
bug: fixed the (null) in the default lua directory listing
bug: SF [ 2037878 ] Convert .fcm doesn't do special characters
bug: SF [ 2046984 ] Player 3/4 input events recorded even when not used
bug: SF [ 2047001 ] Low speeds crash FCEUX
bug: SF [ 2050371 ] FCM>FM2 converter should release file handle
enh: added input display to the FCEUX main menu
enh: auto-fill .fcs extension in save state as dialog
enh: change config filename from fceu98.cfg to fceux.cfg
enh: new toggles: frame adv, lag skip (menu item + hotkey mapping + saved in config)
enh: added a mute turbo option in sound config
enh: add an option to pick a constant color to draw in place of BG when BG rendering is disabled (look for gNoBGFillColor in config; 255 means to use palette[0])
enh: added shift+L as default hotkey for reload lua script
enh: SF [ 2040463 ] Add an "author" text field in the record movie dialog
enh: SF [ 2047004 ] Moviefilenames without extension don't automatically get fm2 (and they need to)
enh: print a special message when trying to open an FCM reminding user to convert
enh: movie replay dialog displays fractions of a second

debugger:
bug: restore the debugger snap functionality
enh: add FORBID breakpoints - regions which block breakpoints from happening if they contain the PC
enh: debugger window is now resizeable and therefore useable at 1024x768
enh: nametable viewer correctly displays more cases (some MMC5 cases I needed)

sdl:
bug: all commandline options now use --argument as they always should have.
bug: fix issue which was resetting config file
bug: Saner sound defaults for less choppy sound
bug: Fixed a bug that would crash fceux if the emulation speed was overincreased
bug: fixed --input(1-4) options
bug: build scripts now look for lua5.1 and lua (distributions package lua differently)
bug: prevent frame advance from crashing emulator
enh: New default hotkeys more closely match win32 defaults
enh: --special option fixed for special video scaling filters
enh: rename options --no8lim -> --nospritelim and --color -> --ntsccolor
enh: Screenshots now always prepend the game name.
enh: Changed default A/B from numpad 2 and 3 to j and k.
enh: Enable frameskip by default
enh: change config path to ~/.fceux/fceux.cfg
enh: Added lua script loading hotkey (f3).  Non-win32 SDL requires zenity for this to function.

emulua:
bug: SF [ 2041944 ] Savestates remember Lua painting (and they shouldn't)
enh: add memory.readbyterange to emulua
</pre>
</p>
  </p>
   -- zeromus
  </p>
  <h3 class="boxsubhead">04 August 2008</h3>
  <p>
  <strong>FCEUX 2.0.1 maintenance release</strong>
  </p>
  <p>Jump to the <a href="download.php">download</a> page.
  </p>
   -- zeromus
  </p>
  <h3 class="boxsubhead">02 August 2008</h3>
  <p>
  FCE Ultra is BACK! Today, version 2.0.0 has been released as the new software FCEUX.
  </p>
  <p><strong>All users of FCE Ultra are encouraged to switch to FCEUX and bug us if something isn't working well.</strong>
  <p>Head to the <a href="download.php">download</a> page to grab the most recent windows binary or portable source code.
  </p>
   -- zeromus
  </p>

</div> <!-- End News div -->

</div> 

<?php include("footer.php"); ?>
