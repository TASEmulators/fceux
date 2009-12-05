<?php
	$title = "FAQ";
	include("header.php");
?>

  <center><h1>FCE Ultra FAQ</h1></center>
  <center><i>Last updated August 11, 2004<br />Valid as of FCE Ultra 0.98.11</i></center>
<p>
 <b>Table of Contents:</b>
 <ul>
  <li><a href="#general">General</a>
  <ul>
   <li><a href="#general-slow">Why is FCE Ultra so slow?</a>
  </ul>
  <li><a href="#emu">Emulation</a>
  <ul>
   <li><a href="#emu-soundpop">Why do some games make a popping sound(Rad Racer 2, Final Fantasy 3)?</a>
   <li><a href="#emu-badnsf">Why don't some NSF rips work correctly on FCE Ultra</a>
   <li><a href="#emu-gamenowork">Why don't some games work correctly on FCE Ultra?</a>
   <li><a href="#emu-smb">Why does Super Mario Bros. start off on level 0?</a>
   <li><a href="#emu-hack">Why does my hack/translation not work correctly?</a>
  </ul>
</ul>
<hr width="100%">
<a name="general"><h2>General</h2></a>
<a name="general-slow"><b>Why is FCE Ultra so slow?</b></a>
<blockquote>
FCE Ultra is much slower than emulators like LoopyNES
and NESA.  These emulators are written in assembly, which gives them a definite advantage
over FCE Ultra, which is written in C.  FCE Ultra is also more accurate that these emulators,
and some accuracy can only be achieved by taking more CPU time.
<p />FCE Ultra <u>could</u> be faster than what it is now.  However, this is not a high priority.
FCE Ultra should run full speed, with sound disabled, on a Pentium 2 300MHz machine, or
an "equivalent" processor.  I have run *older* versions on Pentium MMX 200MHz machines.
Recent versions of gcc produce executables that seem to perform poorly on AMD Athlon processors, so it may be beneficial 
to download the source code and compile a version of FCE Ultra with CPU-specific
optimization flags.
<p />FCE Ultra will perform fastest on older systems(or systems with limited video acceleration) when using an 8BPP video mode.  16BPP video modes come next in terms of speed, followed by 32BPP, and lastly 24BPP.  Avoid 24BPP video modes if at all possible.
<p />In MS Windows, while FCE Ultra is in windowed mode, it will perform best with a "desktop" bit-depth setting of 16BPP, as expected.
</blockquote>
<a name="emu"><h2>Emulation</h2></a>
<a name="emu-soundpop"><b>Why do some games make a popping sound(Rad Racer 2, Final Fantasy 3)?</b></a>
<blockquote>
These games do a very crude drum imitation by causing a large jump in
   the output level for a short period of time via the register at $4011.
   The analog filters on a real Famicom make it sound decent(better).
   <p />
   I have not completely emulated these filters. 
</blockquote>

<a name="emu-badnsf"><b>Why don't some NSF rips work correctly on FCE Ultra?</b></a>
<blockquote>Some NSF rips are bad.  Some read from addresses that are not specified
   in the NSF specifications, expecting certain values to be returned.
   Others execute undocumented instructions that have no effect on
   less-accurate software NSF players, but will cause problems on NSF players 
   that emulate these instructions.  Also, the playback rate specified
   in the NSF header is currently ignored, as it is inherently inaccurate.
   <p />  
   Some NSF rips neglect to write to $4017, which can cause notes to terminate too quickly
   and other similar effects.  Many Konami NSF rips are affected by this.
</blockquote>

<a name="emu-gamenowork"><b>Why don't some games work correctly on FCE Ultra?</b></a>
<blockquote>Many factors can make a game not work on FCE Ultra:
<ul>
	<li> If the ROM image is in the iNES format(typically files that have 
	  the extension "nes"), its header may be incorrect.  This 
	  incorrectness may also be because of garbage in the 
	  header.  Certain utilities used to put text in the reserved 
	  bytes of the iNES header, then those reserved bytes were 
	  later assigned functions.  FCE Ultra recognizes and 
  	  automatically removes(from the ROM image in RAM, not on the 
	  storage medium) SOME header junk.

	  If the game has graphical errors while scrolling, chances are
	  the mirroring is set incorrectly in the header.  
	  
	  You can try to edit the header with a utility(such as <a href="http://ucon64.sourceforge.net/">uCON64</a>) or a hex editor.

	<li> The on-cart hardware the game uses may not be emulated 
	  correctly.

	<li> Limitations of the ROM image format may prevent a game from
	  being emulated correctly without special code to recognize that
	  game.  This occurs quite often with many Koei MMC5(iNES mapper 5) 
	  and MMC1(iNES mapper 1) games in the iNES format.  FCE Ultra identifies
	  and emulates some of these games based on the ROM CRC32 value.

	<li> The ROM image may be encrypted.  The author of SMYNES seems to
	  have done this intentionally to block other emulators from 
	  playing "SMYNES only" games. 
</ul>
</blockquote>
<a name="emu-smb"><b>Why does Super Mario Bros. start off on level 0?</b></a>
<blockquote>
This happens if you're using a hacked copy of the Super Mario Bros. ROM image.  The
hacked version is reading from uninitialized RAM, apparently to get the starting level number.
This bad copy of SMB was likely extracted from a bootleg multicart, which would have had
a menu run before the game started, initializing RAM and setting the starting level.
</blockquote>
<a name="emu-hack"><b>Why does my hack/translation not work correctly?</b></a>
<blockquote>
If a hack or translation that you made does not work on FCE Ultra, when the
original game does, FCE Ultra may be applying header correction based
on CRC32 value.  These changes are only made to the copy of the game loaded in RAM.  
<p />To see what changes
have been made, go to "Help->Message Log" on GUI ports.  On command-line
ports, the information is printed to stdout when the game is loaded.  You
can then update your hack/translation with the corrected header information.

<?php include("footer.php"); ?>
