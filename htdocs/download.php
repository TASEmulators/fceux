<?php
	$title = "Download";
	include("header.php");
?>

<table class="mainbox" cellpadding="5" cellspacing="0" border="1">
<tr><th class="boxhead" colspan="3">Download</th></tr>
<tr><td colspan="3">
<ul>
    <li>There are two ports: SDL and Win32. The ports are significantly different. Win32 has an extensive set of native gui amenities and tools.</li>
    <li>Network play is not presently functional. Fixing it is a high priority.</li>
    <li>SDL:</li>
    <ul>
     <li>This build is working, but may have a few problems. Swing by and help out if you want to help shape it up!
     <li>Uses scons
     <li>Requires SDL, libz, and liblua5.1. OpenGL is optional, and recommended.
    </ul>
    <li>Win32:
    <ul>
     <li>Lua is enabled by use of the [<a href="http://fceux.com/luapack">fceux.com/luapack</a>] which should be unzipped alongside your fceux.exe. 
     <li>Be sure to check out the .chm file. We work pretty hard on keeping that up to date.
    </ul>
</ul>
</td></tr>
<tr><th class="boxhead">Version</th><th class="boxhead">Date</th><th class="boxhead">Description</th></tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceux-2.0.0.win32.zip?download">2.0.0 win32 binary</a></td>
    <td>02 August 2008</td>
    <td>This is the latest official release</td>
</tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceux-2.0.0.src.tar.bz2?download">2.0.0 source</a></td>
    <td>02 August 2008</td>
    <td>This is the latest official release</td>
</tr>
</table>

<?php include("footer.php"); ?>
