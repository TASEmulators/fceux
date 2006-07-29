<?php
	$title = "General Documentation";
	include("header.php");
?>

<h1 class="center">FCE Ultra General Documentation</h1>
<p>
<i>Last updated September 15, 2004</i><br/>
<i>Valid as of FCE Ultra 0.98.13</i>
</p>

<p>
 <b>Table of Contents:</b>
</p>
<ul>
    <li><a href="#intro">Introduction</a>
        <ul>
            <li><a href="#intro-history">History of FCE Ultra</a></li>
        </ul>
    </li>
    <li><a href="#features">Core Features</a>
        <ul>
            <li><a href="#features-cpu">CPU</a></li>
            <li><a href="#features-ppu">PPU</a>
                <ul>
                    <li><a href="#features-ppu-palettes">Palettes</a></li>
                </ul>
            </li>
            <li><a href="#features-sound">Sound</a></li>
            <li><a href="#features-input">Input</a>
                <ul>
                    <li><a href="#features-input-zapper">Zapper</a></li>
                </ul>
            </li>
            <li><a href="#features-expansion">File Formats/Expansion Hardware</a>
                <ul>
                    <li><a href="#features-expansion-ines">iNES Format</a></li>
                    <li><a href="#features-expansion-unif">UNIF</a></li>
                    <li><a href="#features-expansion-fds">Famicom Disk System</a></li>
                    <li><a href="#features-expansion-genie">Game Genie</a></li>
                    <li><a href="#features-expansion-vs">VS Unisystem</a></li>
                </ul>
            </li>
            <li><a href="#features-ips">Automatic IPS Patching</a></li>
        </ul>
    </li>
    <li><a href="#using">Using FCE Ultra</a>
        <ul>
            <li><a href="#using-keys">Key Assignments</a>
                <ul>
                    <li><a href="#using-keys-vs">VS Unisystem</a></li>
                    <li><a href="#using-keys-fds">Famicom Disk System</a></li>
                    <li><a href="#using-keys-barcode">Barcode Readers</a></li>
                    <li><a href="#using-keys-gamepad">Game Pad</a></li>
                    <li><a href="#using-keys-powerpad">Power Pad</a></li>
                    <li><a href="#using-keys-fkb">Family Keyboard</a></li>
                    <li><a href="#using-keys-hypershot">HyperShot Controller</a></li>
                    <li><a href="#using-keys-mahjong">Mahjong Controller</a></li>
                    <li><a href="#using-keys-quiz">Quiz King Controller</a></li>
                </ul>
            </li>
            <li><a href="#using-cli">Command line</a></li>
            <li><a href="#using-gui">GUI</a></li>
        </ul>
    </li>
    <li><a href="cheat.php">Cheat Guide</a></li>
    <li><a href="faq.php">FAQ</a></li>
    <li><a href="netplay.html">Network Play</a></li>
    <li><a href="#hacks">Game-specific Emulation Hacks</a></li>
    <li><a href="#credits">Credits</a></li>
</ul>

 <hr />
<h2><a id="intro">Introduction</a></h2>
 <p>
        FCE Ultra is an NTSC and PAL Famicom/NES emulator for various
        platforms. It is based upon Bero's original FCE source code.  Current   
        features include good PPU, CPU, pAPU, expansion chip, and joystick 
        emulation.  Also a feature unique to this emulator(at the current
        time) is authentic Game Genie emulation.  Save states and snapshot
        features also have been implemented.  
 </p>
 <p>
 This document has been arranged to keep user interface details and emulation
 details as separate as possible, though this has not been accomplished entirely.
 </p>
 <p>
 In several places references are made to the "base directory".  If you
 are running a port on a UN*X-like system(Linux/*BSD/Mac OSX/SunOS/etc.), the
 base directory is "~/.fceultra", or in other words, 
 "your home directory plus .fceultra".  For all other ports(including DOS and
MS Windows), the base directory is the directory that the executable is in.
 </p>
 <h3><a id="intro-history">History of FCE Ultra</a></h3>
 <p>
 <i>This section is a work-in-progress.  Some details may be incorrect.</i><br/>
 Bero originally wrote a NES emulator that was referred to as <a href="http://www.geocities.co.jp/Playtown/2004/fce.htm">FCE</a>.
 This name was apparently meant only to serve as a temporary name, but its usage remained.  Xodnizel originally ported it to
 Linux SVGAlib, and made a few improvements.  This code base was abandoned,
 and work began anew, under DOS, with the original FCE source code.  
 At the end of November, 1998, FCE Ultra Beta 1 was released.
 </p>
 <p>
 FCE Ultra remained DOS-only until version 0.18, when it was ported to Linux
 SVGAlib, and released as a staticly-linked executable.  The first MS Windows
 port was released as version 0.25.
 </p>
 <p>
 The source code of 0.40 was released on November 12, 2000.  It retained
 the simple license of FCE for a long time, which stated that " This software is freeware.you can use it non-commercially."
 Almost two years later, in June 2002, 0.80 was released, and FCE Ultra was relicensed under the GNU GPL.
 </p>
 <hr />
 <h2><a id="features">Core Features</a></h2>
 <h3><a id="features-cpu">CPU</a></h3>
 <p>
 All official instructions of the NES' CPU, the 2A03, which is compatible(mostly)
 with the 6502, are emulated.  "Unofficial" instructions are also emulated,
 though probably not as accurately as the more well-defined official instructions.
 </p>
 <hr />
 <h3><a id="features-ppu">PPU</a></h3>
 <p>
  <ul>
   <li>8x8 and 8x16 sprites.</li>
   <li>Sprite hit emulation(including checking the bg color).</li>
   <li>8 sprite limit(and flag emulation).</li>
   <li>Screen/Sprite disabling, with correct color replacement.</li>
   <li>Color deemphasis(probably not 100% correct, more research is needed).</li>
   <li>Strip colorburst bit emulated.</li>
   <li>CPU-instruction granularity for special mid-scanline effects(mostly CHR switching, such as
    used in "Pirates!", "Marble Madness", and "Mother").</li>
  </ul>
 </p>
 <h4><a id="features-ppu-palettes">Palettes</a></h4>
 <p>
 FCE Ultra has many palette features, including loading a custom palette
 to replace the default NES palette.  The palette from an NTSC NES can
 also be generated on-the-fly.
 </p>
 <p>
 First, a note on on the format of external palettes; Palette files are expected to contain 64 8-bit RGB triplets(each in
 that order; red comes first in the triplet in the file, then green,
 then blue).  Each 8-bit value represents brightness for that particular
 color.  0 is minimum, 255 is maximum.
 </p>
 <p>
        Palettes can be set on a per-game basis.  To do this, put a palette
        file in the "gameinfo" directory with the same base filename
	as the game you wish to associate with and add the extension "pal".
        Examples:
	<pre>        
                File name:              Palette file name:
                 BigBad.nes              BigBad.pal
                 BigBad.zip              BigBad.pal
                 BigBad.Better.nes       BigBad.Better.pal
	</pre>
 </p>
 <p>
       With so many ways to choose a palette, figuring out which one will
        be active may be difficult.  Here's a list of what palettes will
        be used, in order from highest priority to least priority(if a 
        condition doesn't exist for a higher priority palette, the emulator 
        will continue down its list of palettes).
	<ul>
        <li>NSF Palette(for NSFs only)</li>
        <li>Palette loaded from the "gameinfo" directory.</li>
        <li>NTSC Color Emulation(only for NTSC NES games).</li>
        <li>VS Unisystem palette(if the game is a VS Unisystem game and a palette is available).</li>
        <li>Custom global palette.</li>
        <li>Default NES palette. </li>
	</ul>
 </p>
 <p>
 <hr />
 <h3><a id="features-sound"<Sound</a></h3>
 <p>
 All 5 internal sound channels are supported(2x rectangle, triangle, noise,
 and DMC/PCM).  Sound channels are emulated with CPU instruction granularity.
 There are two sound quality options.  Low-quality sound, the default sound
 quality option, generates sound data at 16x the playback rate and averages
 those samples together to 1 sample.  This method works fairly well and
 is reasonably fast, but there is still some aliasing and sound distortion.
 All sample rates between 8192Hz and 96000Hz are supported.
 </p><p>
 The high-quality sound emulation is much more CPU intensive, but the
 quality is worth it, if your machine is fast enough.  Sound data is
 generated at the NES' CPU clock rate(about 1.8MHz for the NTSC NES), and
 then resampled to the output sample rate.  Custom-designed 483rd order
 Parks-McClellan algorithm filter coefficients are used.  Supported playback rates are
 44100Hz, 48000Hz, and 96000Hz.  The following filter statistics apply
 for NTSC emulation.  
 <p>
 <table border>
 <tr><th>Rate:</th><th>Passband Upper Bound(Hz):</th><th>Passband ripple(dB):</th><th>Transition bandwidth(Hz):</th><th>Stopband attenuation(dB):</th></tr>
 <tr><td>44100</td><td>11982.5</td><td>0.20</td><td>10067.5</td><td>66.4</td></tr>
 <tr><td>48000</td><td>13932.5</td><td>0.10</td><td>10067.5</td><td>60.0</td></tr>
 <tr><td>96000</td><td>30000.0</td><td>0.01</td><td>18000.0</td><td>103.0</td></tr>
 </table>
 </p>
 <p>
 The "highest" sound quality mode is similar to the normal high-quality mode,
 but the filters are of a higher order(1024 coefficients).  Ripple is
 reduced, the upper bound of the passband is higher, and the stopband
 attenuation is slightly higher.  The highest-quality mode filter coefficients
 were created using "gmeteor".  The parameters used to create these filters
 can be found in the source code distribution.
 </p>
 <p>
 Besides the 5 internal NES sound channels, FCE Ultra emulates the extra
 sound capabilities of the Konami VRCVI, Konami VRCVII, Namco 106, Nintendo MMC5, and the Sunsoft FME-07 chips.  The extra sound
 channel in the Famicom Disk System is also emulated, but the support for
 its FM capabilities is limited.
 </p>
 <hr />
 <h3><a id="features-input">Input</a></h3>
 <p>
  FCE Ultra emulates the standard NES gamepad, the Four-Score multiplayer
  adapter, the Zapper, the Power Pad,  and the Arkanoid controller.  The 
  Famicom version of the Arkanoid controller, the "Space Shadow" gun, the 
  Famicom 4-player adapter, the Family Keyboard, the HyperShot controller, the Mahjong controller,
  the Oeka Kids tablet, the Quiz King buzzers, the Family Trainer, and the Barcode World barcode
  reader are also emulated.
 </p>
 <h4><a id="features-input-zapper">Zapper</a></h4>
 <p>
        Most Zapper NES games expect the Zapper to be plugged into port 2.
        and most VS Unisystem games expect the Zapper to be plugged
        into port 1.
 </p><p>
        The left mouse button is the emulated trigger button for the
        Zapper.  The right mouse button is also emulated as the trigger,
        but as long as you have the right mouse button held down, no color
        detection will take place, which is effectively like pulling the
        trigger while the Zapper is pointed away from the television screen.
        Note that you must hold the right button down for a short
        time to have the desired effect.
 </p>
 <hr />
 <h3><a id="features-expansion">File Formats/Expansion Hardware</a></h3>
 <p>
 FCE Ultra supports the iNES, FDS(raw and with a header), UNIF, and NSF file
 formats.  FDS ROM images in the iNES format are not supported; it would
 be silly to do so and storing them in that format is nonsensical.
 </p>
 <p>
 FCE Ultra supports loading ROM/disk images from some types of compressed files.
 FCE Ultra can load data from both PKZIP-format files and
 gzip-format files.  Only the "deflate" algorithm is supported, but
 this is the most widely used algorithm for these formats.
 </p>
 <p>
 All files in a PKZIP format archive will be scanned for the
        followings extensions:  .nes, .fds, .nsf, .unf, .nez, .unif  
        The first archived file to have one of these extensions will be
        loaded. If no archived file has one of these extensions, the
        first archived file will be loaded.
 </p>
 <h4><a id="features-expansion-ines">iNES Format</a></h4>
 <p>
 The battery-backed RAM, vertical/horizontal mirroring, four-screen
 name table layout, and 8-bit mapper number capabilities of the iNES
 format are supported.  The 512-byte trainer capability is also supported,
 but it is deprecated.  Common header corruption conditions are cleaned(let's
 go on a DiskDude hunt), though not all conditions can be automatically
 detected and fixed.  In addition, a few common header inaccuracies for
 games are also corrected(detected by CRC32 value).  Note that these
 fixes are not written back to the storage medium.
 </p>
 <p>
 Support for the recent VS System bit and "number of 8kB RAM banks" 
 is not implemented.  Too many iNES headers are corrupt where this new data
 is stored, causing problems for those games.
 </p>
 <p>
 The following table lists iNES-format "mappers" supported well in FCE Ultra.
 </p><p>
 <table width="100%" border>
 <tr><th>Number:</th><th>Description:</th><th>Game Examples:</th></tr>
 <tr><td>0</td><td>No bankswitching</td><td>Donkey Kong, Mario Bros</td></tr>
 <tr><td>1</td><td>Nintendo MMC1</td><td>MegaMan 2, Final Fantasy</td></tr>
 <tr><td>2</td><td>Simple 16KB PROM Switch(UNROM)</td><td>MegaMan, Archon, 1944</td></tr>
 <tr><td>3</td><td>Simple 8KB VROM Switch(CNROM)</td><td>Spy Hunter, Gradius</td></tr>
 <tr><td>4</td><td>Nintendo MMC3</td><td>Super Mario Bros. 3, Recca, Final Fantasy 3</td></tr>
 <tr><td>5</td><td>Nintendo MMC5</td><td>Castlevania 3, Just Breed, Bandit Kings of Ancient China</td></tr>
 <tr><td>6</td><td>FFE F4 Series(hacked, bootleg)</td><td></td></tr>
 <tr><td>7</td><td>AOROM</td><td>Battle Toads, Time Lord</td></tr>
 <tr><td>8</td><td>FFE F3 Series(hacked, bootleg)</td><td></td></tr>
 <tr><td>9</td><td>Nintendo MMC2</td><td>Punchout!</td></tr>
 <tr><td>10</td><td>Nintendo MMC4</td><td>Fire Emblem, Fire Emblem Gaiden</td></tr>
 <tr><td>11</td><td>Color Dreams</td><td>Crystal Mines, Bible Adventures</td></tr>
 <tr><td>12</td><td>??</td><td>Dragon Ball Z 5 ("bootleg" original)</td></tr>
 <tr><td>13</td><td>CPROM</td><td>Videomation</td></tr>
 <tr><td>15</td><td>Multi-cart(bootleg)</td><td>100-in-1: Contra Function 16</td></tr>
 <tr><td>16</td><td>Bandai ??</td><td>Dragon Ball Z, SD Gundam Gaiden **EEPROM is not emulated</td></tr>
 <tr><td>17</td><td>FFE F8 Series(hacked, bootleg)</td><td></td></tr>
 <tr><td>18</td><td>Jaleco SS806</td><td>Pizza Pop, Plasma Ball</td></tr>
 <tr><td>19</td><td>Namco 106</td><td>Splatter House, Mappy Kids</td></tr>
 <tr><td>21</td><td>Konami VRC4 2A</td><td>WaiWai World 2, Ganbare Goemon Gaiden 2</td></tr>
 <tr><td>22</td><td>Konami VRC4 1B</td><td>Twinbee 3</td></tr>
 <tr><td>23</td><td>Konami VRC2B</td><td>WaiWai World, Crisis Force</td></tr>
 <tr><td>24</td><td>Konami VRC6</td><td>Akumajou Densetsu</td></tr>
 <tr><td>25</td><td>Konami VRC4</td><td>Gradius 2, Bio Miracle:Boku tte Upa</td></tr>
 <tr><td>26</td><td>Konami VRC6 A0-A1 Swap</td><td>Esper Dream 2, Madara</td></tr>
 <tr><td>32</td><td>IREM G-101</td><td>Image Fight 2, Perman</td></tr>
 <tr><td>33</td><td>Taito TC0190/TC0350</td><td>Don Doko Don</td></tr>
 <tr><td>34</td><td>NINA-001 and BNROM</td><td>Impossible Mission 2, Deadly Towers, Bug Honey</td></tr>
 <tr><td>40</td><td>(bootleg)</td><td>Super Mario Bros. 2</td></tr>
 <tr><td>41</td><td>Caltron 6-in-1</td><td>Caltron 6-in-1</td></tr>
 <tr><td>42</td><td>(bootleg)</td><td>Mario Baby</td></tr>
 <tr><td>44</td><td>Multi-cart(bootleg)</td><td>Super HiK 7 in 1</td></tr>
 <tr><td>45</td><td>Multi-cart(bootleg)</td><td>Super 1000000 in 1</td></tr>
 <tr><td>46</td><td>Game Station</td><td>Rumble Station</td></tr>
 <tr><td>47</td><td>NES-QJ</td><td>Nintendo World Cup/Super Spike V-Ball</td></tr>
 <tr><td>48</td><td>Taito TC190V</td><td>Flintstones</td></tr>
 <tr><td>49</td><td>Multi-cart(bootleg)</td><td>Super HiK 4 in 1</td></tr>
 <tr><td>50</td><td>(bootleg)</td><td>Super Mario Bros. 2</td></tr>
 <tr><td>51</td><td>Multi-cart(bootleg)</td><td>11 in 1 Ball Games</td></tr>
 <tr><td>52</td><td>Multi-cart(bootleg)</td><td>Mario Party 7 in 1</td></tr>
 <tr><td>57</td><td>Multi-cart(bootleg)</td><td>Game Star GK-54</td></tr>
 <tr><td>58</td><td>Multi-cart(bootleg)</td><td>68-in-1 Game Star HKX5268</td></tr>
 <tr><td>60</td><td>Multi-cart(bootleg)</td><td>4 in 1(Reset-selected)</td></tr>
 <tr><td>61</td><td>Multi-cart(bootleg)</td><td>20 in 1</td></tr>
 <tr><td>62</td><td>Multi-cart(bootleg)</td><td>Super 700 in 1</td></tr>
 <tr><td>64</td><td>Tengen RAMBO 1</td><td>Klax, Rolling Thunder, Skull and Crossbones</td></tr>
 <tr><td>65</td><td>IREM H-3001</td><td>Daiku no Gensan 2</td></tr>
 <tr><td>66</td><td>GNROM</td><td>SMB/Duck Hunt</td></tr>
 <tr><td>67</td><td>Sunsoft ??</td><td>Fantasy Zone 2</td></tr>
 <tr><td>68</td><td>Sunsoft ??</td><td>After Burner 2, Nantetta Baseball</td></tr>
 <tr><td>69</td><td>Sunsoft FME-7</td><td>Batman: Return of the Joker, Hebereke</td>
 <tr><td>70</td><td>??</td><td>Kamen Rider Club</td></tr>
 <tr><td>71</td><td>Camerica</td><td>Fire Hawk, Linus Spacehead</td></tr>
 <tr><td>72</td><td>Jaleco ??</td><td>Pinball Quest</td></tr>
 <tr><td>73</td><td>Konami VRC3</td><td>Salamander</td></tr>
 <tr><td>74</td><td>Taiwanese MMC3 CHR ROM w/ VRAM</td><td>Super Robot Wars 2</td></tr>
 <tr><td>75</td><td>Jaleco SS8805/Konami VRC1</td><td>Tetsuwan Atom, King Kong 2</td></tr>
 <tr><td>76</td><td>Namco 109</td><td>Megami Tensei</td></tr>
 <tr><td>77</td><td>IREM ??</td><td>Napoleon Senki</td></tr>
 <tr><td>78</td><td>Irem 74HC161/32</td><td>Holy Diver</td></tr>
 <tr><td>79</td><td>NINA-06/NINA-03</td><td>F15 City War, Krazy Kreatures, Tiles of Fate</td></tr>
 <tr><td>80</td><td>Taito X-005</td><td>Minelvation Saga</td></tr>
 <tr><td>82</td><td>Taito ??</td><td>Kyuukyoku Harikiri Stadium - Heisei Gannen Ban</td><tr/>
 <tr><td>85</td><td>Konami VRC7</td><td>Lagrange Point</td></tr>
 <tr><td>86</td><td>Jaleco JF-13</td><td>More Pro Baseball</td></tr>
 <tr><td>87</td><td>??</td><td>Argus</td></tr>
 <tr><td>88</td><td>Namco 118</td><td>Dragon Spirit</td></tr>
 <tr><td>89</td><td>Sunsoft ??</td><td>Mito Koumon</td></tr>
 <tr><td>90</td><td>??</td><td>Super Mario World, Mortal Kombat 3, Tekken 2</td></tr>
 <tr><td>91</td><td>??</td><td>Mari Street Fighter 3 Turbo</td></tr>
 <tr><td>92</td><td>Jaleco ??</td><td>MOERO Pro Soccer</td></tr>
 <tr><td>93</td><td>??</td><td>Fantasy Zone</td></tr>
 <tr><td>94</td><td>??</td><td>Senjou no Ookami</td></tr>
 <tr><td>95</td><td>Namco 118</td><td>Dragon Buster</td></tr>
 <tr><td>96</td><td>Bandai ??</td><td>Oeka Kids</td></tr>
 <tr><td>97</td><td>??</td><td>Kaiketsu Yanchamaru</td></tr>
 <tr><td>99</td><td>VS System 8KB VROM Switch</td><td>VS SMB, VS Excite Bike</td></tr>
 <tr><td>105</td><td>NES-EVENT</td><td>Nintendo World Championships</td></tr>
 <tr><td>107</td><td>??</td><td>Magic Dragon</td></tr>
 <tr><td>112</td><td>Asder</td><td>Sango Fighter, Hwang Di</td></tr>
 <tr><td>113</td><td>MB-91</td><td>Deathbots</td></tr>
 <tr><td>114</td><td>??</td><td>The Lion King</td></tr>
 <tr><td>115</td><td>??</td><td>Yuu Yuu Hakusho Final</td></tr>
 <tr><td>117</td><td>??</td><td>San Guo Zhi 4</td></tr>
 <tr><td>118</td><td>MMC3-TLSROM/TKSROM Board</td><td>Ys 3, Goal! 2, NES Play Action Football</td></tr>
 <tr><td>119</td><td>MMC3-TQROM Board</td><td>High Speed, Pin*Bot</td></tr>
 <tr><td>140</td><td>Jaleco ??</td><td>Bio Senshi Dan</td></tr>
 <tr><td>144</td><td>??</td><td>Death Race</td></tr>
 <tr><td>151</td><td>Konami VS System Expansion</td><td>VS The Goonies, VS Gradius</td></tr>
 <tr><td>152</td><td>??</td><td>Arkanoid 2, Saint Seiya Ougon Densetsu</td></tr>
 <tr><td>153</td><td>Bandai ??</td><td>Famicom Jump 2</td></tr>
 <tr><td>154</td><td>Namco ??</td><td>Devil Man</td></tr>
 <tr><td>155</td><td>MMC1 w/o normal WRAM disable</td><td>The Money Game, Tatakae!! Rahmen Man</td></tr>
 <tr><td>156</td><td>??</td><td>Buzz and Waldog</td></tr>
 <tr><td>157</td><td>Bandai Datach ??</td><td>Datach DBZ, Datach SD Gundam Wars, **EEPROM is not emulated</td></tr>
 <tr><td>158</td><td nowrap>RAMBO 1 Derivative</td><td>Alien Syndrome</td></tr>
 <tr><td>160</td><td>(same as mapper 90)</td><td>(same as mapper 90)</td></tr>
 <tr><td>180</td><td>??</td><td>Crazy Climber</td></tr>
 <tr><td>182</td><td>??</td><td>Super Donkey Kong</td></tr>
 <tr><td>184</td><td>??</td><td>Wing of Madoola, The</td></tr>
 <tr><td>185</td><td>CNROM w/ CHR ROM unmapping</td><td>Banana, Mighty Bomb Jack, Spy vs Spy(Japanese)</td></tr>
 <tr><td>189</td><td>??</td><td>Thunder Warrior, Street Fighter 2 (Yoko)</td></tr>
 <tr><td>193</td><td>Mega Soft</td><td>Fighting Hero</td></tr>
 <tr><td>200</td><td>Multi-cart(bootleg)</td><td>1200-in-1</td></tr>
 <tr><td>201</td><td>Multi-cart(bootleg)</td><td>21-in-1</td></tr>
 <tr><td>202</td><td>Multi-cart(bootleg)</td><td>150 in 1</td></tr>
 <tr><td>203</td><td>Multi-cart(bootleg)</td><td>35 in 1</td></tr>
 <tr><td>206</td><td>DEIROM</td><td>Karnov</td></tr>
 <tr><td>207</td><td>Taito ??</td><td>Fudou Myouou Den</td></tr>
 <tr><td>208</td><td>??</td><td>Street Fighter IV (by Gouder)</td></tr>
 <tr><td>209</td><td>(mapper 90 w/ name-table control mode enabled)</td><td>Shin Samurai Spirits 2, Power Rangers III</td></tr>
 <tr><td>210</td><td>Namco ??</td><td>Famista '92, Famista '93, Wagyan Land 2</td></tr>
 <tr><td>225</td><td>Multi-cart(bootleg)</td><td>58-in-1/110-in-1/52 Games</td></tr>
 <tr><td>226</td><td>Multi-cart(bootleg)</td><td>76-in-1</td></tr>
 <tr><td>227</td><td>Multi-cart(bootleg)</td><td>1200-in-1</td></tr>
 <tr><td>228</td><td>Action 52</td><td>Action 52, Cheetahmen 2</td></tr>
 <tr><td>229</td><td>Multi-cart(bootleg)</td><td>31-in-1</td></tr>
 <tr><td>230</td><td>Multi-cart(bootleg)</td><td>22 Games</td></tr>
 <tr><td>231</td><td>Multi-cart(bootleg)</td><td>20-in-1</td></tr>
 <tr><td>232</td><td>BIC-48</td><td>Quattro Arcade, Quattro Sports</td></tr>
 <tr><td>234</td><td>Multi-cart ??</td><td>Maxi-15</td></tr>
 <tr><td>235</td><td>Multi-cart(bootleg)</td><td>Golden Game 150 in 1</td></tr>
 <tr><td>240</td><td>??</td><td>Gen Ke Le Zhuan, Shen Huo Le Zhuan</td></tr>
 <tr><td>242</td><td>??</td><td>Wai Xing Zhan Shi</td></tr>
 <tr><td>244</td><td>??</td><td>Decathalon</td></tr>
 <tr><td>246</td><td>??</td><td>Fong Shen Ban</td></tr>
 <tr><td>248</td><td>??</td><td>Bao Qing Tian</td></tr>
 <tr><td>249</td><td>Waixing ??</td><td>??</td></tr>
 <tr><td>250</td><td>??</td><td>Time Diver Avenger</td></tr>
 <tr><td>255</td><td>Multi-cart(bootleg)</td><td>115 in 1</td></tr>
 </table>
 </p>
 <p>
 <h4><a id="features-expansion-unif">UNIF</a></h4>
 </p> 
 <p>
 FCE Ultra supports the following UNIF boards.  The prefixes HVC-, NES-, BTL-, and BMC- are omitted, since they are currently ignored in FCE Ultra's UNIF loader.
 </p>
 <p>
 <table border width="100%">
 <tr><th colspan="2">Group:</th></tr>
 <tr><th>Name:</th><th>Game Examples:</th></tr>
 <tr><th colspan="2">Bootleg:</th></tr>
 <tr><td>MARIO1-MALEE2</td><td>Super Mario Bros. Malee 2</td></tr>
 <tr><td>NovelDiamond9999999in1</td><td>Novel Diamond 999999 in 1</td></tr>
 <tr><td>Super24in1SC03</td><td>Super 24 in 1</td></tr>
 <tr><td>Supervision16in1</td><td>Supervision 16-in-1</td></tr>
 <tr><th colspan="2">Unlicensed:</th></tr>
 <tr><td>Sachen-8259A</td><td>Super Cartridge Version 1</td></tr>
 <tr><td>Sachen-8259B</td><td>Silver Eagle</td></tr>
 <tr><td>Sachen-74LS374N</td><td>Auto Upturn</td></tr>
 <tr><td>SA-016-1M</td><td>Master Chu and the Drunkard Hu</td></tr>
 <tr><td>SA-72007</td><td>Sidewinder</td></tr>
 <tr><td>SA-72008</td><td>Jovial Race</td></tr>
 <tr><td>SA-0036</td><td>Mahjong 16</td></tr>
 <tr><td>SA-0037</td><td>Mahjong Trap</td></tr>
 <tr><td>TC-U01-1.5M</td><td>Challenge of the Dragon</td></tr>
 <tr><td>8237</td><td>Pocahontas Part 2</td></tr>
 <tr><th colspan="2">MMC1:</th></tr>
 <tr><td>SAROM</td><td>Dragon Warrior</td></tr>
 <tr><td>SBROM</td><td>Dance Aerobics</td></tr>
 <tr><td>SCROM</td><td>Orb 3D</td></tr>
 <tr><td>SEROM</td><td>Boulderdash</td></tr>
 <tr><td>SGROM</td><td>Defender of the Crown</td></tr>
 <tr><td>SKROM</td><td>Dungeon Magic</td></tr>
 <tr><td>SLROM</td><td>Castlevania 2</td></tr>
 <tr><td>SL1ROM</td><td>Sky Shark</td></tr>
 <tr><td>SNROM</td><td>Shingen the Ruler</td></tr>
 <tr><td>SOROM</td><td>Nobunaga's Ambition</td></tr>
 <tr><th colspan="2">MMC3:</th></tr>
 <tr><td>TFROM</td><td>Legacy of the Wizard</td></tr>
 <tr><td>TGROM</td><td>Megaman 4</td></tr>
 <tr><td>TKROM</td><td>Kirby's Adventure</td></tr>
 <tr><td>TKSROM</td><td>Ys 3</td></tr>
 <tr><td>TLROM</td><td>Super Spike V'Ball</td></tr>
 <tr><td>TLSROM</td><td>Goal! 2</td></tr>
 <tr><td>TR1ROM</td><td>Gauntlet</td></tr>
 <tr><td>TQROM</td><td>Pinbot</td></tr>
 <tr><td>TSROM</td><td>Super Mario Bros. 3</td></tr>
 <tr><td>TVROM</td><td>Rad Racer 2</td></tr>
 <tr><th colspan="2">MMC5:</th></tr>
 <tr><td>EKROM</td><td>Gemfire</td></tr>
 <tr><td>ELROM</td><td>Castlevania 3</td></tr>
 <tr><td>ETROM</td><td>Nobunaga's Ambition 2</td></tr>
 <tr><td>EWROM</td><td>Romance of the Three Kingdoms 2</td></tr>
 <tr><th colspan="2">MMC6:</th></tr>
 <tr><td>HKROM</td><td>Star Tropics</td></tr>
 <tr><th colspan="2">Nintendo Discrete Logic:</th></tr>
 <tr><td>CNROM</td><td>Gotcha</td></tr>
 <tr><td>CPROM</td><td>Videomation</td></tr>
 <tr><td>GNROM</td><td>Super Mario Bros./Duck Hunt</td></tr>
 <tr><td>MHROM</td><td></td></tr>
 <tr><td>NROM-128</td><td>Mario Bros.</td></tr>
 <tr><td>NROM-256</td><td>Super Mario Bros.</td></tr>
 <tr><td>RROM-128</td><td></td></tr>
 <tr><td>UNROM</td><td>Megaman</td></tr>
 </table>
 </p>
 <h4><a id="features-expansion-fds">Famicom Disk System</a></h4>
 <p>
        You will need the FDS BIOS ROM image in the base FCE Ultra directory.
        It must be named "disksys.rom".  FCE Ultra will not load FDS games
	without this file.
 </p>
        Two types of FDS disk images are supported:  disk images with the
        FWNES-style header, and disk images with no header.  The number
	of sides on headerless disk images is calculated by the total file
	size, so don't put extraneous data at the end of the file.
 <p>
	If a loaded disk image is written to during emulation, FCE Ultra will store the modified 
	disk image in the save games directory, which is by default "sav" under the base 
	directory.
 </p>
 <h4><a id="features-expansion-genie">Game Genie</a></h4>
 <p>
        The Game Genie ROM image is loaded from the file "gg.rom" in the
        base directory the first time Game Genie emulation is enabled and
        a ROM image is loaded since the time FCE Ultra has run.
</p><p>
        The ROM image may either be the 24592 byte iNES-format image, or
        the 4352 byte raw ROM image.
</p><p>
        Remember that enabling/disabling Game Genie emulation will not take
        effect until a new game is loaded(this statement shouldn't concern
        any of the "run once" command-line driven builds).
 </p>
 <h4><a id="features-expansion-vs">VS Unisystem</a></h4>
 <p>
FCE Ultra currently only supports VS Unisystem ROM images in the
iNES format.  DIP switches and coin insertion are both emulated.  
The following games are supported, and have palettes provided(though not 
necessarily 100% accurate or complete):
	<ul>
	 <li>Battle City</li>
         <li>Castlevania</li>
	 <li>Clu Clu Land</li>
	 <li>Dr. Mario</li>
	 <li>Duck Hunt</li>
	 <li>Excitebike</li>
	 <li>Excitebike (Japanese)</li>
	 <li>Freedom Force</li>
         <li>Goonies, The</li>
         <li>Gradius</li>
         <li>Gumshoe</li>
	 <li>Hogan's Alley</li>
	 <li>Ice Climber</li>
	 <li>Ladies Golf</li>
	 <li>Mach Rider</li>
	 <li>Mach Rider (Japanese)</li>
	 <li>Mighty Bomb Jack	(Japanese)</li>
	 <li>Ninja Jajamaru Kun (Japanese)</li>
	 <li>Pinball</li>
	 <li>Pinball (Japanese)</li>
	 <li>Platoon</li>
	 <li>RBI Baseball</li>
         <li>Slalom</li>
	 <li>Soccer</li>
	 <li>Star Luster</li>
         <li>Stroke and Match Golf</li>
         <li>Stroke and Match Golf - Ladies</li>
         <li>Stroke and Match Golf (Japanese)</li>
         <li>Super Mario Bros.</li>
	 <li>Super Sky Kid</li>
	 <li>Super Xevious</li>
	 <li>Tetris</li>
         <li>TKO Boxing</li>
	 <li>Top Gun</li>
	</ul>
 </p>
 <h3><a id="features-ips">Automatic IPS Patching</a></h3>
 <p>
        Place the IPS file in the same directory as the file to load,
        and name it filename.ips.
</p>
	<pre>
        Examples:       Boat.nes - Boat.nes.ips
                        Boat.zip - Boat.zip.ips
                        Boat.nes.gz - Boat.nes.gz.ips
                        Boat     - Boat.ips
        </pre>
	<p>
        Some operating systems and environments will hide file extensions.
        Keep this in mind if you are having trouble.
	</p>
	<p>
        Patching is supported for all supported formats(iNES, FDS, UNIF, and
        NSF), but it will probably only be useful for the iNES and FDS formats.
        UNIF files can't be patched well with the IPS format because they are chunk-based 
	with no fixed offsets.
 </p>
 <hr />
 <h2><a id="using">Using FCE Ultra</a></h2>
 <p>
 
 </p>
 <h3><a id="using-keys">Key Assignments</a></h3>
 <p>
 <table border>
 <tr><th>Key(s):</th><th>Action:</th></tr>
 <tr><td>F5</td><td>Save state.</td></tr>
 <tr><td>F7</td><td>Load state.</td></tr>
 <tr><td>0-9</td><td>Select save state slot.</td></tr>
 <tr><td>Shift + F5</td><td>Record movie.</td></tr>
 <tr><td>Shift + F7</td><td>Play movie.</td></tr>
 <tr><td>Shift + 0-9</td><td>Select movie slot.</td></tr>
 <tr><td>F9</td><td>Save screen snapshot.</td></tr>
 <tr><td>F4</td><td>Hide sprites(toggle).</td></tr>
 <tr><td>Shift + F4</td><td>Hide background data with overscan color(toggle).</td></tr>
 <tr><td>Alt + Enter</td><td>Toggle fullscreen mode.</td></tr>
 <tr><td>~</td><td>Temporarily disable speed throttling.</td></tr>
 <tr><td>F10</td><td>Reset.</td></tr>
 <tr><td>F11</td><td>Hard reset(toggle power switch).</td></tr>
 <tr><td>F12</td><td>Exit.</td></tr>
 </table>
 </p>
 <a id="using-keys-vs"><h4>VS Unisystem</h4></a>
 <p>  
 <table border>
 <tr><th>Key:</th><th>Action:</th></tr>
 <tr><td>F8</td><td>Insert coin.</td></tr>
 <tr><td>F6</td><td>Show/Hide dip switches.</td></tr>
 <tr><td>1-8</td><td>Toggle dip switches(when dip switches are shown).</td></tr>
 </table>
 </p> 
 <a id="using-keys-fds"><h4>Famicom Disk System</h4></a>
 <p>
 <table border>
 <tr><th>Key:</th><th>Action:</th></tr>
 <tr><td>F6</td><td>Select disk and disk side.</td></tr>
 <tr><td>F8</td><td>Eject or Insert disk.</td></tr>
 </table>
 </p>
 <a id="using-keys-barcode"><h4>Barcode Readers</h4></a>
 <p>
 <table border>
 <tr><th>Key:</th><th>Action:</th></tr>
 <tr><td>0-9</td><td>Barcode digits(after activating barcode input).</td></tr>
 <tr><td>F8</td><td>Activate barcode input/scan barcode.</td></tr>
 </table>
 </p>
 <a id="using-keys-gamepad"><h4>Game Pad</h4></a>
<p>
<table border>
<tr><td colspan="2">
 These default Game Pad key assignments do not apply to the Win32-native port.
 See the table below this for the Win32-native port's default game pad mappings.
 </td></tr>
 <tr><th>Key:</th><th nowrap>Button on Emulated Gamepad:</th></tr>
 <tr><td>Keypad 2</td><td>B</td></tr>
 <tr><td>Keypad 3</td><td>A</td></tr>
 <tr><td>Enter/Return</td><td>Start</td></tr>
 <tr><td>Tab</td><td>Select</td></tr>
 <tr><td>Z</td><td>Down</td></tr>
 <tr><td>W</td><td>Up</td></tr>
 <tr><td>A</td><td>Left</td></tr>
 <tr><td>S</td><td>Right</td></tr>
 </table>
 </p> 
 <p>
 <table border>
 <tr><th colspan="2">Win32-native Port</th></tr>
 <tr><th>Key:</th><th nowrap>Button on Emulated Gamepad:</th></tr>
 <tr><td>Left Control</td><td>B</td></tr>
 <tr><td>Left Alt</td><td>A</td></tr>
 <tr><td>Enter/Return</td><td>Start</td></tr>
 <tr><td>Tab</td><td>Select</td></tr>
 <tr><td>Cursor Down</td><td>Down</td></tr>
 <tr><td>Cursor Up</td><td>Up</td></tr>
 <tr><td>Cursor Left</td><td>Left</td></tr>
 <tr><td>Cursor Right</td><td>Right</td></tr>
 </table>
 </p>
 <a id="using-keys-powerpad"><h4>Power Pad</h4></a>
 <p>  
 <table border>
  <tr><th colspan="4">Side B</th></tr>
  <tr><td width="25%">O</td><td width="25%">P</td>
	<td width="25%">[</td><td width="25%">]</td></tr>
  <tr><td>K</td><td>L</td><td>;</td><td>'</td></tr>
  <tr><td>M</td><td>,</td><td>.</td><td>/</td></tr>
 </table>
 <br />
 <table border>
  <tr><th colspan="4">Side A</th></tr>           
  <tr><td width="25%"></td><td width="25%">P</td>
        <td width="25%">[</td><td width="25%"></td></tr>
  <tr><td>K</td><td>L</td><td>;</td><td>'</td></tr>
  <tr><td></td><td>,</td><td>.</td><td></td></tr>
 </table>
 </p> 
 <a id="using-keys-fkb"><h4>Family Keyboard</h4></a>
 <p>  
         All emulated keys are mapped to the closest open key on the PC
          keyboard, with a few exceptions.  The emulated "@" key is
          mapped to the "`"(grave) key, and the emulated "kana" key
          is mapped to the "Insert" key(in the 3x2 key block above the
          cursor keys).
 </p> 
 <p>
 To enable or disable Family Keyboard input, press the "Scroll Lock" key.
 When Family Keyboard input is enabled, FCE Ultra will also attempt
 to prevent any key presses from being passed to the GUI or system.
 </p>
 <a id="using-keys-hypershot"><h4>HyperShot Controller</h4></a>
 <p>
 <table border>
  <tr><td></td><th>Run</th><th>Jump</th></tr>
  <tr><th>Controller I</th><td>Q</td><td>W</td></tr>
  <tr><th>Controller II</th><td>E</td><td>R</td></tr>
 </table>
 </p>

 <a id="using-keys-mahjong"><h4>Mahjong Controller</h4></a>
 <p>
  <table border>
   <tr><th>Emulated Mahjong Controller:</th><td>A</td><td>B</td><td>C</td><td>D</td><td>E</td><td>F</td><td>G</td><td>H</td><td>I</td><td>J</td><td>K</td><td>L</td><td>M</td><td>N</td></tr>
   <tr><th>PC Keyboard:</th><td>Q</td><td>W</td><td>E</td><td>R</td><td>T</td><td>A</td><td>S</td><td>D</td><td>F</td><td>G</td><td>H</td><td>J</td><td>K</td><td>L</td></tr>
  </table>
  <br />
  <table border>
   <tr><th>Emulated Mahjong Controller:</th><td>SEL</td><td>ST</td><td>?</td><td>?</td><td>?</td><td>?</td><td>?</td></tr>
   <tr><th>PC Keyboard:</th><td>Z</td><td>X</td><td>C</td><td>V</td><td>B</td><td>N</td><td>M</td>
  </table>
 </p>
 <a id="using-keys-quiz"><h4>Quiz King Controller</h4></a>
 <p>
  <table border>
   <tr><th>Emulated Buzzer:</th><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td><td>6</td></tr>
   <tr><th>PC Keyboard:</th><td>Q</td><td>W</td><td>E</td><td>R</td><td>T</td><td>Y</td></tr>
  </table>
 </p>
 <a id="using-cli"><h3>Command-line</h3></a>
 <p>
  FCE Ultra supports arguments passed on the command line.   Arguments
  are taken in the form of "-parameter value".  Some arguments are valueless.
  Arguments that have both a parameter and a value will be saved in the
  configuration file, with the exception being the network-play arguments.
  <b>Please note that most of these arguments are currently not recognized on the Win32-native port.</b>
  </p>
  <p>
  <table border>
   <tr><th>Argument:</th><th>Value Type:</th><th>Default value:</th><th>Description:</th></tr>
   <tr><td>-cpalette x</td><td>string</td><td>0</td><td>Load a custom global palette from file "x".  Specifying "0" will cause FCE Ultra to stop using the custom global palette.</td></tr>
   <tr><td>-ntsccol x</td><td>boolean</td><td>0</td><td>If value is true, enable automatic generation and use of an NTSC NES' colors.</td></tr>
   <tr><td>-pal x</td><td>boolean</td><td>0</td><td>If value is true, emulate a PAL NES.  Otherwise emulate an NTSC NES.</td></tr>
   <tr><td>-sound x</td><td>boolean</td><td>1</td><td>If value is true, enable sound emulation and output.</td></tr>
   <tr><td>-soundrate x</td><td>integer</td><td>48000</td><td>Specifies the sound playback rate, in frames per second("Hz").</td></tr>
   <tr><td>-soundvol x</td><td>integer</td><td>100</td><td>Sound volume.</td></tr>
   <tr><td>-soundq x</td><td>boolean</td><td>0</td><td>If value is true, enable high-quality sound emulation.</td></tr>
   <tr><td>-soundbufsize x</td><td>integer</td><td>24(52 for Win32)</td><td>Specifies the desired size of the sound buffer, in milliseconds.</td></tr>
   <tr><td>-inputcfg x</td><td>string</td><td></td><td>Configure mapping of physical device inputs to a virtual device.  Valid values are "gamepad1", "gamepad2", "gamepad3", "gamepad4", "powerpad1", "powerpad2".<p>During configuration, pressing the same button twice in a row will cause configuration to move to the next button on the virtual device.  A maximum of 4 physical buttons may be assigned to one virtual button.</td></tr>
   <tr><td>-input1 x <br /><i>and</i><br />-input2 x</td><td>string</td><td>gamepad</td><td>Select input device for input port 1 or 2.  Valid strings are "none", "gamepad", "zapper", "powerpada", and "powerpadb".</td></tr>
   <tr><td>-fcexp x</td><td>string</td><td>none</td><td>Select Famicom expansion port device.  Valid strings are "none", "shadow", "arkanoid", "4player", and "fkb".</td></tr>
   <tr><td>-nofs x</td><td>boolean</td><td>0</td><td>If value is true, disable four-score emulation.</td></tr>
   <tr><td>-gg x</td><td>boolean</td><td>0</td><td>Enable Game Genie emulation.</td></tr>
   <tr><td nowrap>-snapname x</td><td>boolean</td><td>0</td><td>If value is true, use an alternate naming scheme(file base and numeric) for screen snapshots.</td></tr>
   <tr><td>-frameskip x</td><td>integer</td><td>0</td><td>Specifies the number of frames to skip(emulation and drawing) for each frame emulated and drawn.  The frameskipping code will not work properly with light-gun games, and it may break other games as well, so use it with this knowledge in mind!</td></tr>
   <tr><td nowrap>-nothrottle x</td><td>boolean</td><td>0</td><td>If value is true, disable the speed throttling that is used when sound emulation is disabled.</td></tr>
   <tr><td>-clipsides x</td><td>boolean</td><td>0</td><td>If value is true, clip leftmost and rightmost 8 columns of pixels of the video output.</td></tr>
   <tr><td>-slstart x</td><td>integer</td><td>8</td>
	<td>First scanline to be rendered in NTSC emulation mode(when PAL emulation is disabled).</td></tr>
   <tr><td>-slend x</td><td>integer</td><td>231</td>
	<td>Last scanline to be rendered in NTSC emulation mode.</td></tr>
     <tr><td nowrap>-slstartp x</td><td>integer</td><td>0</td>
        <td>First scanline to be rendered in PAL emulation mode.</td></tr>
   <tr><td>-slendp x</td><td>integer</td><td>239</td>
        <td>Last scanline to be rendered in PAL emulation mode.</td></tr>
  <tr><th>Argument:</th><th>Value Type:</th><th>Default value:</th><th>Description:</th></tr>
  <tr><td>-opengl x</td><td>boolean</td><td>1</td><td>Enable OpenGL support(if the support has been compiled in).</td></tr>
  <tr><td>-openglip x</td><td>boolean</td><td>1</td><td>Use bilinear interpolation when using OpenGL.</td></tr>
  <tr><td>-special(-specialfs) x</td><td>integer</td><td>0(0)</td><td>Use special video scaling filters.
<ul>
<li>1 = hq2x</li>
<li>2 = Scale2x</li>
<li>3 = hq3x</li>
<li>4 = Scale3x</li>
</ul>
</td></tr>
  <tr><td>-stretchx/-stretchy x</td><td>boolean</td><td>1/0</td><td>Stretch to fill surface on x or y axis(fullscreen, only with OpenGL).</td></tr>
  <tr><td>-doublebuf x</td><td>boolean</td><td>0</td><td>Request double buffering(note that double buffering or sync-to-vblank may be forcibly enabled by your video drivers).</td></tr>
  <tr><td>-xscale(-xscalefs)/<br/>-yscale(-yscalefs)</td><td>real</td><td>2.50(2)/2(2)</td><td>Specify the scaling factor for each axis.  Factors will be truncated to integers when not using OpenGL.</td></tr>
  <tr><td>-xres x/-yres y</td><td>integer</td><td>640/480</td><td>Set the desired horizontal/vertical resolution when in fullscreen mode.</td></tr>
  <tr><td>-efx(fs) x</td><td>integer</td><td>0</td><td>Specify simple special effects, represented by logically ORing constants.  1=scanlines, 2=TV Blur</td></tr>
  <tr><td>-fs</td><td>boolean</td><td>0</td><td>Full screen mode.</td></tr>
 </table>
 </p>
 <hr />
 <h2><a id="hacks"><h2>Game-specific Emulation Hacks</a></h2>
 <p>
 <table border width="100%">
  <tr><th>Title:</th><th>Description:</th><th>Source code files affected:</th></tr>
  <tr><td>KickMaster</td>
  <td>KickMaster relies on the low-level behavior of the MMC3(PPU A12 low->high transition)
      to work properly in certain parts.  If an IRQ begins at the "normal" time on the
      last visible scanline(239), the game will crash after beating the second boss and retrieving
      the item.  The hack is simple, to clock the IRQ counter twice on scanline 238.
  </td>
  <td>mbshare/mmc3.c</td></tr>
  <tr><td nowrap>Shougi Meikan '92<br>Shougin Meikan '93</td>
  <td>The hack for these games is identical to the hack for KickMaster.</td>
  <td>mbshare/mmc3.c</td></tr>
  <tr><td>Star Wars (PAL/European Version)</td>
  <td>This game probably has the same(or similar) problem on FCE Ultra as KickMaster.  The
   hack is to clock the IRQ counter twice on the "dummy" scanline(scanline before the first
   visible scanline).</td><td>mbshare/mmc3.c</td></tr>
  </table>
 </p>
 <hr />
 <h2><a id="credits">Credits</a></h2>
 <p>
 <table border width="100%">
  <tr><th>Name:</th><th>Contribution(s):</th></tr>
  <tr><td>\Firebug\</td><td>High-level mapper information.</td></tr>
  <tr><td>Andrea Mazzoleni</td><td>Scale2x/Scale3x scalers included in FCE Ultra.</td></tr>
  <tr><td>Bero</td><td>Original FCE source code.</td></tr>
  <tr><td>Brad Taylor</td><td>NES sound information.</td></tr>
  <tr><td>EFX</td><td>DC PasoFami NES packs, testing.</td></tr>
  <tr><td>Fredrik Olson</td><td>NES four-player adapter information.</td></tr>
  <tr><td>Gilles Vollant</td><td>PKZIP file loading functions.</td></tr>
  <tr><td>goroh</td><td>Various documents.</td></tr>
  <tr><td>Info-ZIP</td><td><a href="http://www.gzip.org/zlib/">ZLIB</a></td></tr>
  <tr><td>Jeremy Chadwick</td><td>General NES information.</td></tr>
  <tr><td>Justin Smith</td><td>Good stuff.</td></tr>
  <tr><td>kevtris</td><td>Low-level NES information and sound information.</td></tr>
  <tr><td>Ki</td><td>Various technical information.</td></tr>
  <tr><td>Mark Knibbs</td><td>Various NES information.</td></tr>
  <tr><td>Marat Fayzullin</td><td>General NES information.</td></tr>
  <tr><td>Matthew Conte</td><td>Sound information.</td></tr>
  <tr><td>Maxim Stepin</td><td>hq2x and hq3x scalers included in FCE Ultra.</td></tr>
  <tr><td>MindRape</td><td>DC PasoFami NES packs.</td></tr>
  <tr><Td>Mitsutaka</td><td>YM2413 emulator.</td></tr>
  <tr><td>nori</td><td>FDS sound information.</td></tr>
  <tr><td>Quietust</td><td>VRC7 sound translation code by The Quietust (quietust at ircN dort org).</td></tr>
  <tr><td>rahga</td><td>Famicom four-player adapter information.</td></tr>
  <tr><td>Sean Whalen</td><td>Node 99</td></tr>
  <tr><td>TheRedEye</td><td>ROM images, testing.</td></tr>
  <tr><td>TyphoonZ</td><td>Archaic Ruins.</td></tr>
  <tr><th colspan="2" align="right">...and everyone whose name my mind has misplaced.</th></tr>
 </table>
 </p>

<?php include("footer.php"); ?>
