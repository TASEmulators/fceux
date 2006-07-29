<?php
	include("header.php");
?>

<div class="mainbox">
 <h2 class="boxhead">Introduction </h2>
 <div class="boxbody">
   <p>
   FCE Ultra is a NES (Nintendo Entertainment System) and Famicom (Family Computer)
   emulator for a variety of different platforms, based on Bero's <a href="http://www.geocities.co.jp/Playtown/2004/fce.htm">FCE</a>.  Game compatibility is very
   high, provided you provide non-corrupt ROM/disk images.  
  </p>
  <p>
   It has been tested (and runs)
   under DOS, Linux SVGAlib, Linux X, Mac OS X, and MS Windows.  A native GUI is provided for the MS Windows port, and the other ports use a command-line based interface.  However, GNOME users can optionally use <a href="http://gfceu.thepiratecove.org">the GNOME front-end</a>.  The SDL port
   should run on any modern UNIX-like operating system(such as FreeBSD) with
   no code changes.
  </p>
 </div>
</div>

<div class="mainbox">
<h2 class="boxhead">News</h2>   <!-- News section -->
<div class="boxbody">

<h3 class="boxsubhead">19 July 2006</h3>
<p>
We've created a mailing list for all kinds of discussion about
FCE Ultra.  You can subscribe at the
<a href="https://lists.sourceforge.net/lists/listinfo/fceultra-discuss">fceultra-discuss</a>
mailing list page.  Also, we're hoping to bring any FCE Ultra forks
into one combined effort.  If you work on a fork of fceu and you're
interested in creating an NES emulator to end all NES emulators,
you might want to join the fceultra-discuss mailing list and
introduce yourself :-)
</p>

<h3 class="boxsubhead">15 June 2006</h3>
<p>
I, <a href="mailto:punkrockguy318 _at _ comcast _dot_ net">Lukas Sabota</a> have
been asked to join the FCE Ultra team.  I hope to improve this project by maintaining a graphical
interface for FCE Ultra.  You can check out my existing work <a href="http://thepiratecove.org/gfceu/">here</a>.
</p>


<h3 class="boxsubhead">23 March 2006</h3>
<p>
We've added <a href="mailto:r0ni a.t users d.o.t sourceforge">Jay Lanagan</a>
as a developer to the project.  It seems that there
still is some interest in maintaining FCEUltra after all!
</p>


<h3 class="boxsubhead">19 March 2006</h3>
<p>
Alright, we've uploaded some files--please see
<a href="download.php">the download page</a>.  Who are "we,"
you ask?
<a href="mailto:agiorgio a.t users d.o.t sourceforge">Anthony
Giorgio</a> and
<a href="mailto:thekingant a.t users d.o.t sourceforge">Mark
Doliner</a>.  We're two guys who like FCE Ultra and didn't want
to see the project disappear.
</p>

<p>
What happens now?  Well I'm not sure either of us has very much
free time to spend working on things, so if you're interested in
working on the source, by all means drop us a line.  We have checked
the 0.98.13-pre source code into SourceForge's Subversion server
at https://svn.sourceforge.net/svnroot/fceultra/fceultra.  Feel
free to grab a copy and start hacking.
</p>

<p>
If you're not much of a coder you could always work on documentation,
or improve the web page (which is checked into Subversion at
https://svn.sourceforge.net/svnroot/fceultra/web).
</p>

<p>
I can't speak for Anthony, but here are some things I would work
on if I had the time:
</p>

<ul>
<li>Configuration file written as plain-text instead of binary</li>
<li>Better joystick configuration (currently with my Logitech
controller I think I'm forced to use the joystick rather than the
joypad)</li>
<li>Play sound in Linux using ALSA instead of OSS (this isn't already
possible when compiled with SDL, is it?)</li>
<li>Slighty better command line interface (a -h/--help switch)</li>
</ul>



<h3 class="boxsubhead">18 March 2006</h3>
<p>
The FCE Ultra project has been taken over!  Our current goal is only
to preserve the state of the project as it was left by the author,
Xodnizel.  But we'd love to get development going again, too.  We'll
post more information soon.
</p>



<h3 class="boxsubhead">29 October 2004</h3>
<p>
No new *official* releases of FCE Ultra will be made. I mean it this time. :b
</p>

<p>
The forum has been closed, as I will not be available to moderate it any longer, but the old posts will be viewable indefinitely.
</p>

<p>
Crazy people may download http://fceultra.sourceforge.net/fceu-0.98.13-pre.src.tar.bz2 which was intended to be 0.98.13.  It should be stable, and fixed several major bugs present in 0.98.12, but I have neither the time nor desire to do thorough tests for regressions, and thus it will not be an official release.
</p>



<h3 class="boxsubhead">12 September 2004</h3>
<p>
All future binary release(s) of FCE Ultra will be compiled without the emulation
debugger.  The emulator can still be built with the debugger from source, however.
</p>
<p>
This is being done to greatly speed up compilation times, decrease executable
size, and speed up emulation slightly.  Also, the presence of a debugger
in the official build(at least for MS Windows) will probably become meaningless
once a certain third-party branch of FCE Ultra is released.
</p>



<h3 class="boxsubhead">XX Something 2004</h3>
<p>
FCE Ultra 0.98.12 is out.  The MS Windows binary release should run using
fewer CPU cycles("faster") than previous 0.98.x MS Windows binary releases.
</p>



<h3 class="boxsubhead">17 May 2004</h3>
<p>FCE Ultra 0.98.10 is out.</p>
<pre>
0.98.10:

        Reimplemented network play.  It now requires a standalone network play server, which
        will be released as a later time.  For fun, "starmen.net" is running this server,
        which is publicly accessible.
        I also made various code fixes/improvements to allow for network play, particularly
        with the command handling code.

        Reworked much of the VS Unisystem emulation code, partially based on information from
        MAME.  The following games are now supported(in iNES format):

         Battle City
         Castlevania  
         Clu Clu Land 
         Dr. Mario
         Duck Hunt 
         Excitebike
         Excitebike (Japanese)
         Freedom Force
         Goonies, The
         Gradius
         Gumshoe
         Hogan's Alley
         Ice Climber
         Ladies Golf
         Mach Rider
         Mach Rider (Japanese)
         Mighty Bomb Jack (Japanese)
         Ninja Jajamaru Kun (Japanese)
         Pinball
         Pinball (Japanese)
         Platoon
         RBI Baseball
         Slalom
         Soccer
         Star Luster
         Stroke and Match Golf
         Stroke and Match Golf - Ladies
         Stroke and Match Golf (Japanese)
         Super Mario Bros.
         Super Sky Kid
         Super Xevious
         Tetris   
         TKO Boxing
         Top Gun   

        Win32-native:  Fixed a bug in the debugger's breakpoint list that appeared when
        one tried to delete a breakpoint(the control accidentally had auto-sort enabled,
        causing a discrepancy between what was displayed and what was contained in internal
        data structures).

        The current disk image XOR original disk image is now stored in save states.  This
        should greatly increase compressability(important for network play), and make
        it a little more legal to distribute such save states now.

        Modified the save state format to allow for more precise and larger version numbers.

        Various minor code changes.

        Fixed initialization of the FCEUGameInfo structure, which previously led
        to problems with sound output on the SexyAL-using ports(Linux).

        Apparently I added support for mapper 255 a while back.  Documentation updated.

        Added iNES header correction information for Armored Scrum Object and Alpha Mission.

        Merged banksw.h into ines.c, fixed some of its prototypes in ines.h.
</pre>

</div> <!-- End News div -->

</div> 

<?php include("footer.php"); ?>
