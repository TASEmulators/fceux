<?php
	$title = "FCEUX: Documentation";
	include("header.php");
?>

<table class="mainbox" cellpadding="5" cellspacing="0" border="1">
<tr><th class="boxhead" colspan="3">Documentation</th></tr>
<tr><td colspan="3">
<ul>
 <li>(incomplete) Documentation for the SDL build is <a href="fceux-sdl-docs.php">available here</a>.
 <li>FAQ for the SDL build can be <a href="fceux-sdl-faq.php">found here</a>.
 <li>Windows users should look at the CHM which is packaged with the binary, or go <a href=http://fceultra.svn.sourceforge.net/viewvc/fceultra/fceu/src/drivers/win/help/fceux.chm>directly to it</a>; but you cannot open it directly out of IE, as Microsoft feels that this would be a security hazard. You'd better save it first and then unblock it from the file properties.
 <br>Some of the SDL documentation might be informative to Windows users--so if the CHM doesn't answer your question, give it a shot.
 <li><strong>TAS folks:</strong><ul>
   <li><a href="desync.php">Why FCEUX desyncs distressingly frequently relative to FCEUltra Rerecording and how to fix your movie</a>
   <li>You might like to look at the <a href="http://fceultra.svn.sourceforge.net/viewvc/fceultra/fceu/documentation/fm2.txt">FM2 spec</a>
 </ul>
 <li>You might be interested in the <a href="fceu-docs.php">old FCE ultra docs</a>, but they are dangerously out of date.
</tr>
</table>

<?php include("footer.php"); ?>

