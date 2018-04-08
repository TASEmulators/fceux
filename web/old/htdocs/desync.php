<?php
	$title = "FCEUX: Documentation (regarding desyncs)";
	include("header.php");
?>

<table class="mainbox" cellpadding="5" cellspacing="0" border="1">
<tr><th class="boxhead" colspan="3">Regarding desyncs</th></tr>
<tr><td colspan="3">
The old FCEUltra had a big problem with its movie recording. Every single movie <i>was recorded with power-on, followed by a savestate</i>. This means that WRAM (or save RAM) was being stored in its initialized state. As a consequence, many games that use save systems would go through an accelerated boot process since the save files were already initialized.
<p>
Typically, a runner would load his game and let it sit at the title screen while he sipped his drink or adjusted his pants or steeled his nerves or whatever and then triggered the movie record. At that point the savefile was initialized, and the recording was <i>already compromised.</i>
<p>
When preparing FCEUX for release, we decided that this was unacceptable. Therefore, every FCEUX movie is recorded from a fresh power-on with no WRAM or save RAM. Additionally, the "record from reset" was taken out because it is even worse. Now the game would be starting from initialized WRAM <i>and</i> initialized main ram. This can further confuse matters.
<p>
So, your movies might desync because on FCEUX the game is taking longer to boot, while it is initializing its WRAM and/or main ram.
<p>
Luckily, these are often easy to fix: since FM2 is a text format, it is trivial to insert additional empty frames into the correct locations during the game boot-up process. We have successfully used this approach to salvage a few movies. So, if you have trouble, let us know, and maybe since we have a bit of experience, we can help.
<p>
Also, when a game is initializing, I bet it is uncommon for the RNG to be running so you might get lucky there also.
<p>
Another trick is to add a boot-up time window to the beginning of your movie. Add empty frames to the very beginning, followed by the very first converted frame which should include a reset flag. This simulates what FCEUltra did, giving the game time to initialize its WRAM and possibly main RAM  before your input events begin.
<p>
<strong>The good news</strong><br>
We have been excruciatingly careful not to do anything else which would adversely affect synching. We take this seriously. We have only found one desynching movie so far that we aren't able to fix.
</tr>
</table>

<?php include("footer.php"); ?>

