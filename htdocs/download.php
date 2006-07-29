<?php
	$title = "Download";
	include("header.php");
?>

<table class="mainbox" cellpadding="5" cellspacing="0" border="1">
<tr><th class="boxhead" colspan="3">Download</th></tr>
<tr><td colspan="3">
<p>
We want to point out that the original author of FCE Ultra, Xodnizel,
is no longer working on it.  Version 0.98.12 and 0.98.13 are the last
releases made by Xodnizel, and any releases after that are created by
different developers.
</p>
<p>Other notes:</p>
<ul>
    <li>Currently, only the Unix (Linux, FreeBSD, etc), Mac OS X, and Win32 ports will compile and run, mainly using SDL.</li>
    <li>Network play is functional as of 0.98.10.  The netplay server can be found <a href="support.php#netplay">here</a>.</li>
    <li>An alternate native Win32 + DirectX GUI'd target is available starting with version 0.98.6.</li>
    <li>Uses autotools (autoconf, automake).</li>
    <li>Dependencies: DirectX (Win32), SDL, OpenGL (optional, recommended), zlib</li>
</ul>
</td></tr>
<tr><th class="boxhead">Version</th><th class="boxhead">Date</th><th class="boxhead">Description</th></tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceu-0.98.12.src.tar.bz2?download">0.98.12 source</a></td>
    <td>28 August 2004</td>
    <td>This is the last official stable release by the author.</td>
</tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceu-0.98.13-pre.src.tar.bz2?download">0.98.13-pre source</a></td>
    <td>29 October 2004</td>
    <td>This is the most recent source code we have from the original author.  
    It works, but was not tested for regressions and was therefore not an official release.</td>
</tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceu-0.98.15-src.7z?download">0.98.15 source</a></td>
    <td>Unknown</td>
    <td>This is an unofficial version of FCE Ultra that has been patched to provide demo recording and playback.  Usage of this version is not recommended.</td>
</tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceu-0.98.15-rerecording.zip?download">0.98.15 Windows binary</a></td>
    <td>Unknown</td>
    <td>This is an unofficial version of FCE Ultra that has been patched to provide demo recording and playback.  Usage of this version is not recommended.</td>
</tr>
</table>

<?php include("footer.php"); ?>
