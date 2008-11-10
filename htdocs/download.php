<?php
	$title = "Download";
	include("header.php");
?>

<table class="mainbox" cellpadding="5" cellspacing="0" border="1">
<tr><th class="boxhead" colspan="3">Download</th></tr>
<tr><td colspan="3">
<ul>
    <li>There are two ports: SDL and Win32. The ports are significantly 
different. Win32 has an extensive set of native gui amenities and 
tools.  The SDL port supports many of the features of the Win32 build 
(Lua scripting, movie recording), however some of the Win32 features are 
Win32 only.  A GTK2 GUI has been also developed for the SDL port.  The 
SDL port should run any UNIX-type OS (Linux/Solaris/BSD).</li>
    <li>Network play is not presently functional. Fixing it is a high priority.</li>
    <li>Win32:
    <ul>
     <li>Lua is enabled by use of the [<a href="http://fceux.com/luapack">fceux.com/luapack</a>] which should be unzipped alongside your fceux.exe. 
     <li>Be sure to check out the .chm file. We work pretty hard on keeping that up to date.
     <li>The latest win32 interim build is generally fresh within a day 
or two at <a href="http://fceux.com/zip">http://fceux.com/zip</a> -- If you are 
working with a developer to fix an issue affecting you then this is 
where you will find your fixed build.
    </ul>
    <li>SDL:</li>
    <ul>
     <li>This build is working, but may have a few problems. Swing by and help out if you want to help shape it up!
     <li><a href=http://scons.org>Scons</a> is required to build from 
source.</li>
     <li>Requires SDL, libz, zenity, and liblua5.1. OpenGL is optional, and recommended.</li>
     <li><a href="http://dietschnitzel.com/gfceu">GFceuX</a>, a GTK2 GUI 
for fceux, is included in the source release.  It is recommened for all non-windows 
users of the SDL port, unless you particullarly prefer the command-line.</li>
	<li>For instructions on installing on an Ubuntu or Debian system, check the howto at 
	<a href="http://ubuntuforums.org/showthread.php?t=971455">Ubuntuforums</a>.</li>
    </ul>
</ul>
</td></tr>
<tr><th class="boxhead">Version</th><th class="boxhead">Date</th><th class="boxhead">Description</th></tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceux-2.0.3.win32.zip?download">2.0.3 win32 binary</a></td>
    <td>02 November 2008</td>
    <td>This is the latest official release</td>
</tr>
<tr>
    <td><a href="http://prdownloads.sourceforge.net/fceultra/fceux-2.0.3.src.tar.bz2?download">2.0.3 source</a></td>
    <td>02 November 2008</td>
    <td>This is the latest official release</td>
</tr>
</table>

<?php include("footer.php"); ?>
