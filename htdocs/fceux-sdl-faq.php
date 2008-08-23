<?php
	$title = "SDL FCEUX FAQ";
	include("header.php");
?>


<html>
 <head>
  <title>FCEUX SDL FAQ</title>
 </head>
 <body>
  <center><h1>FCEUX SDL FAQ</h1></center>
  <center><i>Last updated August 23, 2008<br />Valid as of FCEUX 2.0.2</i> by punkrockguy318<br>
  </center>
 <p>
 <b>Table of Contents:</b>
 <ul>
  <li /><a href="#win-vs-sdl">What are the differences between the windows and sdl versions?</a>
  <li /><a href="#config">How do I configure a gamepad?</a>
  <li /><a href="#lua">Why do I need lua5.1 to compile/run FCEUX?</a>
  <li /><a href="#features-expansion-genie">Game Genie</a>
  <li /><a href="#features-expansion-vs">VS Unisystem</a>
 </ul>
 
 </p>
 <hr width="100%">
 <a name="intro"><h2>What are the differences between the windows and sdl versions?</h2></a>
 <p>A lot of work that has been done on the FCE code base was been windows only, so 
 some features have not been implemented into the more portable SDL version. </p>
 <p>Here is a list of planned features that are currently in the Win32 build 
 but are missing from the SDL build:</p>
 <ul>
   <li />Autosaving
   <li />Toggle BG/FG rendering
   <li />More graphical ways of configuring FCEUX
 </ul>
 <p>I'm going to reiterate that much of the work that has been done of the FCEUX 
 code base has been windows only, so some features will probably never be ported 
 to the SDL version.  If you are a casual user that just wants to play old NES games, the SDL 
 build will be sufficient for you.  However, if you want to use the extensive
 TAS and debugging features that FCEUX has to offer, you are better of using
 the windows build in <a href=http://winehq.org>wine</a>.  Here is a list of features
 that will most likely never be ported to the SDL port.</p>
 <ul>
   <li />Memory watch
   <li />Ram filter
   <li />TAS edit
   <li />Most of the debugging features
 </ul>
 <a name="config"><h3>How do I configure a gamepad?</h3></a>
 <p>
  You can configure the first gamepad by running <b>fceux --inputcfg gamepad1 
  /any/rom/name/here/it/doesnt/matter/</b> </p>
  <p>When you do this, you'll be presented with a small titlebar with a title 
  indicating what button to map.  The input configurator will look for two
  of the same keypress/joystick event.  If it doesn't find two of the same 
  keypresses in a row, it will allow you to map the button to four seperate keys.</p>
  <p>FCEUX doesn't keep seperate input configurations for serperate games, it
  simply needs a rom to process on the command line.  You can even use /dev/null as 
  your filename in the command.</p>
  <p>I'd like to also note that I've started work on a GUI for setting gamepads, but
  it is current a WIP, but hopefully it will become functional when I get a chance.</p>
 </p>
 <a name="lua"><h4>Why do I need lua5.1 to compile/run fceux?</h4></a>
 <p>More and more of FCEUX is going to work around its lua scripting system.
 FCEUX's lua scripting system leaves endless possibilities for scripts.  An option
 to compile without lua may be added to a future release.</p>
        
 <a name="credits"><h2>Credits</h2></a>
 <p>
 <table border width="100%">
  <tr><th>Name:</th><th>Contribution(s):</th></tr>
  <tr><td>punkrockguy318</td><td>Author of this document</td></tr>
 </table>
 </p>
 </body>
</html>
