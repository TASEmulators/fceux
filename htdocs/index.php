<?php
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
     As the X implies, it is an all-encompassing FCEU emulator that gives the best of all worlds for the general player,
     the ROM-hacking community, and the Tool-Assisted Speedrun Community.
   </p>
   <p>
     We are particularly excited by the Lua scripting capabilities which have been ported over from FCEU rerecording and prepped for 
further development.  In order to use Lua in the windows build you will need the luapack [<a 
href="http://fceux.com/luapack">fceux.com/luapack</a>] which should be unzipped alongside your fceux.exe. The large numbers of dlls 
wasn't our choice; but it will enable the iup user interface library which will permit scripts to create their own guis, as well as 
rendering the base fceux distribution more compact.  Examples of lua scripts may be found in the subversion repository <a 
href="http://fceultra.svn.sourceforge.net/viewvc/fceultra/fceu/documentation/lua/">here</a>

   <p>
     We have a mailing list for all kinds of discussion about FCEUX.  You can subscribe at the
     <a href="https://lists.sourceforge.net/lists/listinfo/fceultra-discuss">fceultra-discuss</a>
     mailing list page.
   </p>
   <p>
     Of course, we are always hanging out at irc.freenode.net #fceu
   </p>
   
 </div>
</div>

<div class="mainbox">
<h2 class="boxhead">News</h2>   <!-- News section -->
<div class="boxbody">

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
