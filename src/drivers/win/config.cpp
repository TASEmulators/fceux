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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "ramwatch.h"
#include "debugger.h"
#include "taseditor/taseditor_config.h"

#include "../../state.h"	//adelikat: For bool backupSavestates

extern CFGSTRUCT NetplayConfig[];
extern CFGSTRUCT InputConfig[];
extern CFGSTRUCT HotkeyConfig[];
extern int autoHoldKey, autoHoldClearKey;
extern int EnableAutosave, AutosaveQty, AutosaveFrequency;
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
extern bool fullscreenByDoubleclick;
extern int CurrentState;
extern bool pauseWhileActive; //adelikat: Cheats dialog
extern bool enableHUDrecording;
extern bool disableMovieMessages;
extern bool replaceP2StartWithMicrophone;
extern bool SingleInstanceOnly;
extern bool Show_FPS;
extern bool oldInputDisplay;
extern bool fullSaveStateLoads;
extern int frameSkipAmt;

extern TASEDITOR_CONFIG taseditor_config;
extern char* recent_projects[];
// Hacky fix for taseditor_config.last_author
char* taseditor_config_last_author;

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
extern int logging_options;
extern int log_lines_option;
extern int Tracer_wndx, Tracer_wndy;
extern int CDLogger_wndx, CDLogger_wndy;
extern int GGConv_wndx, GGConv_wndy;
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

//adelikat:  Hacky fix for Ram Watch recent menu
char* ramWatchRecent[] = {0, 0, 0, 0, 0};

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

	ACS(ramWatchRecent[0]),
	ACS(ramWatchRecent[1]),
	ACS(ramWatchRecent[2]),
	ACS(ramWatchRecent[3]),
	ACS(ramWatchRecent[4]),

	ACS(recent_projects[0]),
	ACS(recent_projects[1]),
	ACS(recent_projects[2]),
	ACS(recent_projects[3]),
	ACS(recent_projects[4]),
	ACS(recent_projects[5]),
	ACS(recent_projects[6]),
	ACS(recent_projects[7]),
	ACS(recent_projects[8]),
	ACS(recent_projects[9]),

	AC(gNoBGFillColor),
	AC(ntsccol),AC(ntsctint),AC(ntschue),
	AC(force_grayscale),

	NAC("palyo",pal_emulation),
	NAC("genie",genie),
	NAC("fs",fullscreen),
	NAC("vgamode",vmod),
	NAC("sound",soundo),
	NAC("sicon",status_icon),

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
	AC(NTSCwinspecial),
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
	AC(rerecord_display),
	AC(input_display),
	ACS(MemWatchDir),
	AC(EnableBackgroundInput),
	AC(MemWatchLoadOnStart),
	AC(MemWatchLoadFileOnStart),
	AC(MemWCollapsed),
	AC(BindToMain),
	AC(EnableAutosave),
	AC(AutosaveQty),
	AC(AutosaveFrequency),
	AC(frameAdvanceLagSkip),
	AC(debuggerAutoload),
	AC(allowUDLR),
	AC(debuggerSaveLoadDEBFiles),
	AC(debuggerDisplayROMoffsets),
	AC(fullSaveStateLoads),
	AC(frameSkipAmt),

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
	AC(logging_options),
	AC(log_lines_option),
	AC(Tracer_wndx),
	AC(Tracer_wndy),
	AC(CDLogger_wndx),
	AC(CDLogger_wndy),
	AC(GGConv_wndx),
	AC(GGConv_wndy),
	AC(TextHookerPosX),
	AC(TextHookerPosY),
	AC(MetaPosX),
	AC(MetaPosY),
	AC(MLogPosX),
	AC(MLogPosY),

	AC(pauseAfterPlayback),
	AC(closeFinishedMovie),
	AC(suggestReadOnlyReplay),
	AC(AFon),
	AC(AFoff),
	AC(AutoFireOffset),
	AC(DesynchAutoFire),
	AC(taseditor_config.wndx),
	AC(taseditor_config.wndy),
	AC(taseditor_config.wndwidth),
	AC(taseditor_config.wndheight),
	AC(taseditor_config.saved_wndx),
	AC(taseditor_config.saved_wndy),
	AC(taseditor_config.saved_wndwidth),
	AC(taseditor_config.saved_wndheight),
	AC(taseditor_config.wndmaximized),
	AC(taseditor_config.findnote_wndx),
	AC(taseditor_config.findnote_wndy),
	AC(taseditor_config.follow_playback),
	AC(taseditor_config.turbo_seek),
	AC(taseditor_config.show_branch_screenshots),
	AC(taseditor_config.show_branch_descr),
	AC(taseditor_config.bind_markers),
	AC(taseditor_config.empty_marker_notes),
	AC(taseditor_config.combine_consecutive),
	AC(taseditor_config.use_1p_rec),
	AC(taseditor_config.columnset_by_keys),
	AC(taseditor_config.branch_full_movie),
	AC(taseditor_config.old_branching_controls),
	AC(taseditor_config.view_branches_tree),
	AC(taseditor_config.branch_scr_hud),
	AC(taseditor_config.restore_position),
	AC(taseditor_config.adjust_input_due_to_lag),
	AC(taseditor_config.superimpose),
	AC(taseditor_config.enable_auto_function),
	AC(taseditor_config.enable_hot_changes),
	AC(taseditor_config.greenzone_capacity),
	AC(taseditor_config.undo_levels),
	AC(taseditor_config.autosave_period),
	AC(taseditor_config.jump_to_undo),
	AC(taseditor_config.follow_note_context),
	AC(taseditor_config.last_export_type),
	AC(taseditor_config.last_export_subtitles),
	AC(taseditor_config.savecompact_binary),
	AC(taseditor_config.savecompact_markers),
	AC(taseditor_config.savecompact_bookmarks),
	AC(taseditor_config.savecompact_greenzone),
	AC(taseditor_config.savecompact_history),
	AC(taseditor_config.savecompact_piano_roll),
	AC(taseditor_config.savecompact_selection),
	AC(taseditor_config.findnote_matchcase),
	AC(taseditor_config.findnote_search_up),
	AC(taseditor_config.draw_input),
	AC(taseditor_config.enable_greenzoning),
	AC(taseditor_config.silent_autosave),
	AC(taseditor_config.autopause_at_finish),
	AC(taseditor_config.tooltips),
	AC(taseditor_config.current_pattern),
	AC(taseditor_config.pattern_skips_lag),
	AC(taseditor_config.pattern_recording),
	ACS(taseditor_config_last_author),
	AC(lagCounterDisplay),
	AC(oldInputDisplay),
	AC(movieSubtitles),
	AC(subtitlesOnAVI),
	AC(bindSavestate),
	AC(autoMovieBackup),
	AC(ClipSidesOffset),
	AC(PPUViewRefresh),
	AC(NTViewRefresh),
	AC(rightClickEnabled),
	AC(fullscreenByDoubleclick),
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

	AC(AutoRWLoad),
	AC(RWSaveWindowPos),
	AC(ramw_x),
	AC(ramw_y),

	AC(backupSavestates),
	AC(compressSavestates),
	AC(pauseWhileActive),
	AC(enableHUDrecording),
	AC(disableMovieMessages),
	AC(replaceP2StartWithMicrophone),
	AC(SingleInstanceOnly),
	AC(Show_FPS),

	ENDCFGSTRUCT
};

void SaveConfig(const char *filename)
{
	//adelikat: Hacky fix for Ram Watch recent menu
	for (int x = 0; x < 5; x++)
	{
		ramWatchRecent[x] = rw_recent_files[x];
	}
	// Hacky fix for taseditor_config.last_author
	taseditor_config_last_author = taseditor_config.last_author;
	//-----------------------------------

	SaveFCEUConfig(filename,fceuconfig);
}

void LoadConfig(const char *filename)
{
	FCEUI_GetNTSCTH(&ntsctint, &ntschue);

	LoadFCEUConfig(filename, fceuconfig);

	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);

	//adelikat:Hacky fix for Ram Watch recent menu
	for (int x = 0; x < 5; x++)
	{
		if(ramWatchRecent[x])
		{
			strncpy(rw_recent_files[x], ramWatchRecent[x], 1024);
			free(ramWatchRecent[x]);
			ramWatchRecent[x] = 0;
		}
		else
		{
			rw_recent_files[x][0] = 0;
		}
	}
	// Hacky fix for taseditor_config.last_author
	if (taseditor_config_last_author)
		strncpy(taseditor_config.last_author, taseditor_config_last_author, AUTHOR_MAX_LEN-1);
	else
		taseditor_config.last_author[0] = 0;
	//-----------------------------------
}

