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
     The concept behind FCEUX  is to merge elements from FCE Ultra, FCEU rerecording, FCEUXD, FCEUXDSP, and FCEU-umm into a single branch of FCEU.  
     As the  X implies , it is a all encompassing FCEU emulator that gives the best of all worlds for the general player, 
     the ROM-hacking community, and the Tool-Assisted Speedrun Community.

   </p>
 </div>
</div>

<div class="mainbox">
<h2 class="boxhead">News</h2>   <!-- News section -->
<div class="boxbody">

<h3 class="boxsubhead">22 June 2008</h3>
<p>
    Round 1 of the attempt to update the content of this site.  Downloads are still outdated versions of FCEU Ultra.
</p>  
  
<h3 class="boxsubhead">5 June 2008</h3>
<p>
No, this project is not dead.  There are a few of us developers working in subversion.  However, we could always use more help.  We need both SDL and Windows developers.  If interested, just come chat with us in #fceu on irc.freenode.net<br>
<a href="mailto:ltsmooth42 _at_ gmail _dot _com">Lukas Sabota</a>
</p>

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
I, <a href="mailto:ltsmooth42 _at _ gmail _dot_ com">Lukas Sabota</a> have
been asked to join the FCE Ultra team.  I hope to improve this project by maintaining a graphical
interface for FCE Ultra.  You can check out my existing work <a href="http://dietschnitzel.com/gfceu">here</a>.
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

</div> <!-- End News div -->

</div> 

<?php include("footer.php"); ?>
