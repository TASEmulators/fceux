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
