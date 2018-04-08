<?php
	$title = "Download";
	include("header.php");
?>

<table class="mainbox" cellpadding="5" cellspacing="0" border="1">
<tr><th class="boxhead" colspan="3">Download</th></tr>
<tr><td colspan="3">
<ul>
    <li>FCEUX is written for two platforms: SDL and Win32. The two ports have
	some significant differences.  The Win32 port has an extensive set of native gui 
	amenities and tools.  The SDL port supports many of the features of the Win32 build 
(Lua scripting, movie recording), however some of the Win32 features are 
exclusive to the platform.  The Win32 port has a built-in GUI, whereas the SDL port has a 
GTK launcher GUI.  The SDL port should run any UNIX-like OS (Linux/Solaris/BSD).</li>
    <li>Network play is not presently functional. Fixing it is a high priority.</li>
    <li>Win32:
    <ul>
      <li>Lua is enabled and has all necessary files in 2.1.x releases.  It also comes with a number of examples which show some of Lua's full potential</li>
      <li>These files are also available in the [<a href="http://fceux.com/luapack">fceux.com/luapack</a>] (to use these, they must be in the same folder as fceux.exe). 
     <li>Be sure to check out the .chm file. We work pretty hard on keeping that up to date.
     <li>The latest win32 interim build is generally fresh within a day 
or two at <a href="http://fceux.com/zip">http://fceux.com/zip</a> -- If you are 
working with a developer to fix an issue affecting you then this is 
where you will find your fixed build.
    </ul>
    <li>SDL:</li>
    <ul>
     <li><a href=http://scons.org>Scons</a> is required to build from 
source.</li>
     <li>Requires SDL, libz, and zenity.  Liblua5.1 and OpenGL 
	 are optional, but recommended.</li>
     <li><a href="http://dietschnitzel.com/gfceu">GFCEUX</a>, a GTK2 GUI 
for fceux, is included in the source release.  All non-windows users of the SDL
port are reccommended to install and use this, unless you particullarly prefer the command-line.</li>
	<li>For instructions on installing on an Ubuntu or Debian system, check the howto at 
	<a href="http://ubuntuforums.org/showthread.php?t=971455">Ubuntuforums</a>.</li>
    </ul>
</ul>
</td></tr>
<tr><th class="boxhead">Version</th><th class="boxhead">Date</th><th class="boxhead">Description</th></tr>

<tr>
    <td><a href="http://downloads.sourceforge.net/fceultra/fceux-2.1.2-win32.zip">2.1.2 win32 binary</a></td>
    <td>04 November 2009</td>
    <td>This is the latest official release</td>
</tr>

<tr>
    <td><a href="http://downloads.sourceforge.net/fceultra/fceux-2.1.2.src.tar.bz2">2.1.2 source</a></td>
    <td>04 November 2009</td>
</tr>


<tr>
    <td><a href="http://downloads.sourceforge.net/fceultra/fceux-2.1.1.win32.zip">2.1.1 win32 binary</a></td>
    <td>29 July 2009</td>
</tr>
<tr>
    <td><a href="http://downloads.sourceforge.net/fceultra/fceux-2.1.1.src.tar.bz2">2.1.1 source</a></td>
    <td>29 July 2009</td>
</tr>
<tr>
    <td><a 
href="http://downloads.sourceforge.net/fceultra/fceux_2.1_i386.deb">2.1.0 i386 deb package</a></td>
    <td>29 March 2009</td>
</tr>
<tr>
</table>

<?php include("footer.php"); ?>
