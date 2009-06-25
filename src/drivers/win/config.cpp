/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/****************************************************************/
/*			FCE Ultra				*/
/*								*/
/*	This file contains code to interface to the standard    */
/*	FCE Ultra configuration file saving/loading code.	*/
/*								*/
/****************************************************************/

#include "config.h"
#include "common.h"
#include "main.h"
#include "window.h"
#include "video.h"
#include "memwatch.h"
#include "fceu.h"
#include "file.h"
#include "texthook.h"
#include "movieoptions.h"

extern CFGSTRUCT NetplayConfig[];
extern CFGSTRUCT InputConfig[];
extern CFGSTRUCT HotkeyConfig[];
extern int autoHoldKey, autoHoldClearKey;
extern int EnableAutosave;
extern int AFon, AFoff, AutoFireOffset;
extern int DesynchAutoFire;
extern bool lagCounterDisplay;
extern bool frameAdvanceLagSkip;
extern int ClipSidesOffset;
extern bool movieSubtitles;
extern bool subtitlesOnAVI;
extern bool autoMovieBackup;
extern bool bindSavestate;
extern int PPUViewRefresh;
extern int NTViewRefresh;
extern uint8 gNoBGFillColor;
extern bool rightClickEnabled;
extern int CurrentState;

//window positions and sizes:
extern int ChtPosX,ChtPosY;
extern int DbgPosX,DbgPosY;
extern int DbgSizeX,DbgSizeY;
extern int MemViewSizeX,MemViewSizeY;
extern int MemView_wndx, MemView_wndy;
extern int MemFind_wndx, MemFind_wndy;
extern int NTViewPosX,NTViewPosY;
extern int PPUViewPosX,PPUViewPosY;
extern int MainWindow_wndx, MainWindow_wndy;
extern int MemWatch_wndx, MemWatch_wndy;
extern int Monitor_wndx, Monitor_wndy;
extern int Tracer_wndx, Tracer_wndy;
extern int CDLogger_wndx, CDLogger_wndy;
extern int GGConv_wndx, GGConv_wndy;
extern int TasEdit_wndx, TasEdit_wndy;
extern int MetaPosX,MetaPosY;
extern int MLogPosX,MLogPosY;

extern int HexRowHeightBorder;
extern int HexBackColorR;
extern int HexBackColorG;
extern int HexBackColorB;
extern int HexForeColorR;
extern int HexForeColorG;
extern int HexForeColorB;
extern int HexFreezeColorR;
extern int HexFreezeColorG;
extern int HexFreezeColorB;
extern int RomFreezeColorR;
extern int RomFreezeColorG;
extern int RomFreezeColorB;

//Structure that contains configuration information
static CFGSTRUCT fceuconfig[] = {

	ACS(recent_files[0]),
	ACS(recent_files[1]),
	ACS(recent_files[2]),
	ACS(recent_files[3]),
	ACS(recent_files[4]),
	ACS(recent_files[5]),
	ACS(recent_files[6]),
	ACS(recent_files[7]),
	ACS(recent_files[8]),
	ACS(recent_files[9]),

	ACS(memw_recent_files[0]),
	ACS(memw_recent_files[1]),
	ACS(memw_recent_files[2]),
	ACS(memw_recent_files[3]),
	ACS(memw_recent_files[4]),

	ACS(recent_lua[0]),
	ACS(recent_lua[1]),
	ACS(recent_lua[2]),
	ACS(recent_lua[3]),
	ACS(recent_lua[4]),

	ACS(recent_movie[0]),
	ACS(recent_movie[1]),
	ACS(recent_movie[2]),
	ACS(recent_movie[3]),
	ACS(recent_movie[4]),

	AC(gNoBGFillColor),
	AC(ntsccol),AC(ntsctint),AC(ntschue),

	NAC("palyo",pal_emulation),
	NAC("genie",genie),
	NAC("fs",fullscreen),
	NAC("vgamode",vmod),
	NAC("sound",soundo),
	NAC("sicon",status_icon),
	//adelikat - we want this behvaior //ch4: cause newppu mode fixation after changing through config file lol
    AC(newppu),

	NACS("odroms",directory_names[0]),
	NACS("odnonvol",directory_names[1]),
	NACS("odstates",directory_names[2]),
	NACS("odfdsrom",directory_names[3]),
	NACS("odsnaps",directory_names[4]),
	NACS("odcheats",directory_names[5]),
	NACS("odmovies",directory_names[6]),
	NACS("odmemwatch",directory_names[7]),
	NACS("odmacro",directory_names[9]),
	NACS("odinput",directory_names[10]),
	NACS("odlua",directory_names[11]),
	NACS("odavi",directory_names[12]),
	NACS("odbase",directory_names[13]),

	AC(winspecial),
	AC(winsizemulx),
	AC(winsizemuly),
	NAC("saspectw987",saspectw),
	NAC("saspecth987",saspecth),

	AC(soundrate),
	AC(soundbuftime),
	AC(soundoptions),
	AC(soundquality),
	AC(soundvolume),
	AC(soundTrianglevol),
	AC(soundSquare1vol),
	AC(soundSquare2vol),
	AC(soundNoisevol),
	AC(soundPCMvol),
	AC(muteTurbo),

	AC(goptions),
	NAC("eoptions",eoptions),
	NACA("cpalette",cpalette),

	NACA("InputType",InputType),

	NAC("vmcx",vmodes[0].x),
	NAC("vmcy",vmodes[0].y),
	NAC("vmcb",vmodes[0].bpp),
	NAC("vmcf",vmodes[0].flags),
	NAC("vmcxs",vmodes[0].xscale),
	NAC("vmcys",vmodes[0].yscale),
	NAC("vmspecial",vmodes[0].special),

	NAC("srendline",srendlinen),
	NAC("erendline",erendlinen),
	NAC("srendlinep",srendlinep),
	NAC("erendlinep",erendlinep),

	AC(disvaccel),
	AC(winsync),
	NAC("988fssync",fssync),

	AC(ismaximized),
	AC(maxconbskip),
	AC(ffbskip),

	ADDCFGSTRUCT(NetplayConfig),
	ADDCFGSTRUCT(InputConfig),
	ADDCFGSTRUCT(HotkeyConfig),

	AC(autoHoldKey),
	AC(autoHoldClearKey),
	AC(frame_display),
	AC(input_display),
	ACS(MemWatchDir),
	AC(EnableBackgroundInput),
	AC(MemWatchLoadOnStart),
	AC(MemWatchLoadFileOnStart),
	AC(MemWCollapsed),
	AC(BindToMain),
	AC(EnableAutosave),
	AC(frameAdvanceLagSkip),

	AC(allowUDLR),

	//window positions
	AC(ChtPosX),
	AC(ChtPosY),
	AC(DbgPosX),
	AC(DbgPosY),
	AC(DbgSizeX),
	AC(DbgSizeY),
	AC(MemViewSizeX),
	AC(MemViewSizeY),
	AC(MemView_wndx),
	AC(MemView_wndy),
	AC(MemFind_wndx), 
	AC(MemFind_wndy),
	AC(NTViewPosX),
	AC(NTViewPosY),
	AC(PPUViewPosX),
	AC(PPUViewPosY),
	AC(MainWindow_wndx),
	AC(MainWindow_wndy),
	AC(MemWatch_wndx),
	AC(MemWatch_wndy),
	AC(Monitor_wndx),
	AC(Monitor_wndy),
	AC(Tracer_wndx),
	AC(Tracer_wndy),
	AC(CDLogger_wndx),
	AC(CDLogger_wndy),
	AC(GGConv_wndx),
	AC(GGConv_wndy),
	AC(TasEdit_wndx),
	AC(TasEdit_wndy),
	AC(TextHookerPosX),
	AC(TextHookerPosY),
	AC(MetaPosX),
	AC(MetaPosY),
	AC(MLogPosX),
	AC(MLogPosY),

	AC(pauseAfterPlayback),
	AC(AFon),
	AC(AFoff),
	AC(AutoFireOffset),
	AC(DesynchAutoFire),
	AC(lagCounterDisplay),
	AC(movieSubtitles),
	AC(subtitlesOnAVI),
	AC(bindSavestate),
	AC(autoMovieBackup),
	AC(ClipSidesOffset),
	AC(PPUViewRefresh),
	AC(NTViewRefresh),
	AC(rightClickEnabled),
	AC(CurrentState),
	AC(HexRowHeightBorder),
	AC(HexBackColorR),
	AC(HexBackColorG),
	AC(HexBackColorB),
	AC(HexForeColorR),
	AC(HexForeColorG),
	AC(HexForeColorB),
	AC(HexFreezeColorR),
	AC(HexFreezeColorG),
	AC(HexFreezeColorB),
	AC(RomFreezeColorR),
	AC(RomFreezeColorG),
	AC(RomFreezeColorB),
	//ACS(memwLastfile[2048]),
	ENDCFGSTRUCT
};

void SaveConfig(const char *filename)
{
	SaveFCEUConfig(filename,fceuconfig);
}

void LoadConfig(const char *filename)
{
	FCEUI_GetNTSCTH(&ntsctint, &ntschue);

	LoadFCEUConfig(filename, fceuconfig);

	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
}

