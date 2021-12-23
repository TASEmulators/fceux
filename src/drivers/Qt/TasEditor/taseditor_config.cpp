/* ---------------------------------------------------------------------------------
Implementation file of TASEDITOR_CONFIG class
Copyright (c) 2011-2013 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Config - Current settings
[Single instance]

* stores current state of all TAS Editor settings
* all TAS Editor modules can get or set any data within Config
* when launching FCEUX, the emulator writes data from fceux.cfg file to the Config, when exiting it reads the data back to fceux.cfg
* stores resources: default values of all settings, min/max values of settings
------------------------------------------------------------------------------------ */

#include "Qt/fceuWrapper.h"
#include "Qt/TasEditor/recorder.h"
#include "Qt/TasEditor/inputlog.h"
#include "Qt/TasEditor/taseditor_config.h"

TASEDITOR_CONFIG::TASEDITOR_CONFIG(void)
{
	// set default values
	windowX = 0;
	windowY = 0;
	windowWidth = 0;
	windowHeight = 0;
	savedWindowX = 0;
	savedWindowY = 0;
	savedWindowWidth = 0;
	savedWindowHeight = 0;
	windowIsMaximized = false;

	findnoteWindowX = 0;
	findnoteWindowY = 0;
	findnoteMatchCase = false;
	findnoteSearchUp = false;

	followPlaybackCursor = true;
	turboSeek = false;
	autoRestoreLastPlaybackPosition = false;
	superimpose = SUPERIMPOSE_UNCHECKED;
	recordingUsePattern = false;
	enableLuaAutoFunction = true;

	displayBranchesTree = false;
	displayBranchScreenshots = true;
	displayBranchDescriptions = true;
	enableHotChanges = true;
	followUndoContext = true;
	followMarkerNoteContext = true;

	greenzoneCapacity = GREENZONE_CAPACITY_DEFAULT;
	maxUndoLevels = UNDO_LEVELS_DEFAULT;
	enableGreenzoning = true;
	autofirePatternSkipsLag = true;
	autoAdjustInputAccordingToLag = true;
	drawInputByDragging = true;
	combineConsecutiveRecordingsAndDraws = false;
	use1PKeysForAllSingleRecordings = true;
	useInputKeysForColumnSet = false;
	bindMarkersToInput = true;
	emptyNewMarkerNotes = true;
	oldControlSchemeForBranching = false;
	branchesRestoreEntireMovie = true;
	HUDInBranchScreenshots = true;
	autopauseAtTheEndOfMovie = true;

	lastExportedInputType = INPUT_TYPE_1P;
	lastExportedSubtitlesStatus = false;

	projectSavingOptions_SaveInBinary = true;
	projectSavingOptions_SaveMarkers = true;
	projectSavingOptions_SaveBookmarks = true;
	projectSavingOptions_SaveHistory = true;
	projectSavingOptions_SavePianoRoll = true;
	projectSavingOptions_SaveSelection = true;
	projectSavingOptions_GreenzoneSavingMode = GREENZONE_SAVING_MODE_ALL;

	saveCompact_SaveInBinary = true;
	saveCompact_SaveMarkers = true;
	saveCompact_SaveBookmarks = true;
	saveCompact_SaveHistory = false;
	saveCompact_SavePianoRoll = true;
	saveCompact_SaveSelection = false;
	saveCompact_GreenzoneSavingMode = GREENZONE_SAVING_MODE_NO;

	autosaveEnabled = true;
	autosavePeriod = AUTOSAVE_PERIOD_DEFAULT;
	autosaveSilent = true;

	tooltipsEnabled = true;

	currentPattern = 0;
	lastAuthorName[0] = 0;			// empty name

}

void TASEDITOR_CONFIG::load(void)
{
	std::string s;
	g_config->getOption("SDL.TasAutoSaveEnabled"                         , &autosaveEnabled );
	g_config->getOption("SDL.TasAutoSavePeriod"                          , &autosavePeriod  );
	g_config->getOption("SDL.TasAutoSaveSilent"                          , &autosaveSilent  );
	g_config->getOption("SDL.TasTooltipsEnabled"                         , &tooltipsEnabled );
	g_config->getOption("SDL.TasCurrentPattern"                          , &currentPattern  );
	g_config->getOption("SDL.TasFollowPlaybackCursor"                    , &followPlaybackCursor  );
	g_config->getOption("SDL.TasTurboSeek"                               , &turboSeek  );
	g_config->getOption("SDL.TasAutoRestoreLastPlaybackPosition"         , &autoRestoreLastPlaybackPosition  );
	g_config->getOption("SDL.TasSuperImpose"                             , &superimpose  );
	g_config->getOption("SDL.TasRecordingUsePattern"                     , &recordingUsePattern  );
	g_config->getOption("SDL.TasEnableLuaAutoFunction"                   , &enableLuaAutoFunction  );
	g_config->getOption("SDL.TasDisplayBranchesTree"                     , &displayBranchesTree  );
	g_config->getOption("SDL.TasDisplayBranchScreenshots"                , &displayBranchScreenshots  );
	g_config->getOption("SDL.TasDisplayBranchDescriptions"               , &displayBranchDescriptions  );
	g_config->getOption("SDL.TasEnableHotChanges"                        , &enableHotChanges  );
	g_config->getOption("SDL.TasFollowUndoContext"                       , &followUndoContext  );
	g_config->getOption("SDL.TasFollowMarkerNoteContext"                 , &followMarkerNoteContext  );
	g_config->getOption("SDL.TasGreenzoneCapacity"                       , &greenzoneCapacity  );
	g_config->getOption("SDL.TasMaxUndoLevels"                           , &maxUndoLevels  );
	g_config->getOption("SDL.TasEnableGreenzoning"                       , &enableGreenzoning  );
	g_config->getOption("SDL.TasAutofirePatternSkipsLag"                 , &autofirePatternSkipsLag  );
	g_config->getOption("SDL.TasAutoAdjustInputAccordingToLag"           , &autoAdjustInputAccordingToLag  );
	g_config->getOption("SDL.TasDrawInputByDragging"                     , &drawInputByDragging  );
	g_config->getOption("SDL.TasCombineConsecutiveRecordingsAndDraws"    , &combineConsecutiveRecordingsAndDraws  );
	g_config->getOption("SDL.TasUse1PKeysForAllSingleRecordings"         , &use1PKeysForAllSingleRecordings  );
	g_config->getOption("SDL.TasUseInputKeysForColumnSet"                , &useInputKeysForColumnSet  );
	g_config->getOption("SDL.TasBindMarkersToInput"                      , &bindMarkersToInput  );
	g_config->getOption("SDL.TasEmptyNewMarkerNotes"                     , &emptyNewMarkerNotes  );
	g_config->getOption("SDL.TasOldControlSchemeForBranching"            , &oldControlSchemeForBranching  );
	g_config->getOption("SDL.TasBranchesRestoreEntireMovie"              , &branchesRestoreEntireMovie  );
	g_config->getOption("SDL.TasHUDInBranchScreenshots"                  , &HUDInBranchScreenshots  );
	g_config->getOption("SDL.TasAutopauseAtTheEndOfMovie"                , &autopauseAtTheEndOfMovie  );
	g_config->getOption("SDL.TasLastExportedInputType"                   , &lastExportedInputType  );
	g_config->getOption("SDL.TasLastExportedSubtitlesStatus"             , &lastExportedSubtitlesStatus  );
	g_config->getOption("SDL.TasProjectSavingOptions_SaveInBinary"       , &projectSavingOptions_SaveInBinary  );
	g_config->getOption("SDL.TasProjectSavingOptions_SaveMarkers"        , &projectSavingOptions_SaveMarkers  );
	g_config->getOption("SDL.TasProjectSavingOptions_SaveBookmarks"      , &projectSavingOptions_SaveBookmarks  );
	g_config->getOption("SDL.TasProjectSavingOptions_SaveHistory"        , &projectSavingOptions_SaveHistory  );
	g_config->getOption("SDL.TasProjectSavingOptions_SavePianoRoll"      , &projectSavingOptions_SavePianoRoll  );
	g_config->getOption("SDL.TasProjectSavingOptions_SaveSelection"      , &projectSavingOptions_SaveSelection  );
	g_config->getOption("SDL.TasProjectSavingOptions_GreenzoneSavingMode", &projectSavingOptions_GreenzoneSavingMode  );
	g_config->getOption("SDL.TasSaveCompact_SaveInBinary"                , &saveCompact_SaveInBinary  );
	g_config->getOption("SDL.TasSaveCompact_SaveMarkers"                 , &saveCompact_SaveMarkers  );
	g_config->getOption("SDL.TasSaveCompact_SaveBookmarks"               , &saveCompact_SaveBookmarks  );
	g_config->getOption("SDL.TasSaveCompact_SaveHistory"                 , &saveCompact_SaveHistory  );
	g_config->getOption("SDL.TasSaveCompact_SavePianoRoll"               , &saveCompact_SavePianoRoll  );
	g_config->getOption("SDL.TasSaveCompact_SaveSelection"               , &saveCompact_SaveSelection  );
	g_config->getOption("SDL.TasSaveCompact_GreenzoneSavingMode"         , &saveCompact_GreenzoneSavingMode  );
	g_config->getOption("SDL.TasLastAuthorName" , &s);

	strncpy( lastAuthorName, s.c_str(), AUTHOR_NAME_MAX_LEN );
	lastAuthorName[AUTHOR_NAME_MAX_LEN-1] = 0;
}

void TASEDITOR_CONFIG::save(void)
{
	g_config->setOption("SDL.TasAutoSaveEnabled"                         , autosaveEnabled );
	g_config->setOption("SDL.TasAutoSavePeriod"                          , autosavePeriod  );
	g_config->setOption("SDL.TasAutoSaveSilent"                          , autosaveSilent  );
	g_config->setOption("SDL.TasTooltipsEnabled"                         , tooltipsEnabled );
	g_config->setOption("SDL.TasCurrentPattern"                          , currentPattern  );
	g_config->setOption("SDL.TasFollowPlaybackCursor"                    , followPlaybackCursor  );
	g_config->setOption("SDL.TasTurboSeek"                               , turboSeek  );
	g_config->setOption("SDL.TasAutoRestoreLastPlaybackPosition"         , autoRestoreLastPlaybackPosition  );
	g_config->setOption("SDL.TasSuperImpose"                             , superimpose  );
	g_config->setOption("SDL.TasRecordingUsePattern"                     , recordingUsePattern  );
	g_config->setOption("SDL.TasEnableLuaAutoFunction"                   , enableLuaAutoFunction  );
	g_config->setOption("SDL.TasDisplayBranchesTree"                     , displayBranchesTree  );
	g_config->setOption("SDL.TasDisplayBranchScreenshots"                , displayBranchScreenshots  );
	g_config->setOption("SDL.TasDisplayBranchDescriptions"               , displayBranchDescriptions  );
	g_config->setOption("SDL.TasEnableHotChanges"                        , enableHotChanges  );
	g_config->setOption("SDL.TasFollowUndoContext"                       , followUndoContext  );
	g_config->setOption("SDL.TasFollowMarkerNoteContext"                 , followMarkerNoteContext  );
	g_config->setOption("SDL.TasGreenzoneCapacity"                       , greenzoneCapacity  );
	g_config->setOption("SDL.TasMaxUndoLevels"                           , maxUndoLevels  );
	g_config->setOption("SDL.TasEnableGreenzoning"                       , enableGreenzoning  );
	g_config->setOption("SDL.TasAutofirePatternSkipsLag"                 , autofirePatternSkipsLag  );
	g_config->setOption("SDL.TasAutoAdjustInputAccordingToLag"           , autoAdjustInputAccordingToLag  );
	g_config->setOption("SDL.TasDrawInputByDragging"                     , drawInputByDragging  );
	g_config->setOption("SDL.TasCombineConsecutiveRecordingsAndDraws"    , combineConsecutiveRecordingsAndDraws  );
	g_config->setOption("SDL.TasUse1PKeysForAllSingleRecordings"         , use1PKeysForAllSingleRecordings  );
	g_config->setOption("SDL.TasUseInputKeysForColumnSet"                , useInputKeysForColumnSet  );
	g_config->setOption("SDL.TasBindMarkersToInput"                      , bindMarkersToInput  );
	g_config->setOption("SDL.TasEmptyNewMarkerNotes"                     , emptyNewMarkerNotes  );
	g_config->setOption("SDL.TasOldControlSchemeForBranching"            , oldControlSchemeForBranching  );
	g_config->setOption("SDL.TasBranchesRestoreEntireMovie"              , branchesRestoreEntireMovie  );
	g_config->setOption("SDL.TasHUDInBranchScreenshots"                  , HUDInBranchScreenshots  );
	g_config->setOption("SDL.TasAutopauseAtTheEndOfMovie"                , autopauseAtTheEndOfMovie  );
	g_config->setOption("SDL.TasLastExportedInputType"                   , lastExportedInputType  );
	g_config->setOption("SDL.TasLastExportedSubtitlesStatus"             , lastExportedSubtitlesStatus  );
	g_config->setOption("SDL.TasProjectSavingOptions_SaveInBinary"       , projectSavingOptions_SaveInBinary  );
	g_config->setOption("SDL.TasProjectSavingOptions_SaveMarkers"        , projectSavingOptions_SaveMarkers  );
	g_config->setOption("SDL.TasProjectSavingOptions_SaveBookmarks"      , projectSavingOptions_SaveBookmarks  );
	g_config->setOption("SDL.TasProjectSavingOptions_SaveHistory"        , projectSavingOptions_SaveHistory  );
	g_config->setOption("SDL.TasProjectSavingOptions_SavePianoRoll"      , projectSavingOptions_SavePianoRoll  );
	g_config->setOption("SDL.TasProjectSavingOptions_SaveSelection"      , projectSavingOptions_SaveSelection  );
	g_config->setOption("SDL.TasProjectSavingOptions_GreenzoneSavingMode", projectSavingOptions_GreenzoneSavingMode  );
	g_config->setOption("SDL.TasSaveCompact_SaveInBinary"                , saveCompact_SaveInBinary  );
	g_config->setOption("SDL.TasSaveCompact_SaveMarkers"                 , saveCompact_SaveMarkers  );
	g_config->setOption("SDL.TasSaveCompact_SaveBookmarks"               , saveCompact_SaveBookmarks  );
	g_config->setOption("SDL.TasSaveCompact_SaveHistory"                 , saveCompact_SaveHistory  );
	g_config->setOption("SDL.TasSaveCompact_SavePianoRoll"               , saveCompact_SavePianoRoll  );
	g_config->setOption("SDL.TasSaveCompact_SaveSelection"               , saveCompact_SaveSelection  );
	g_config->setOption("SDL.TasSaveCompact_GreenzoneSavingMode"         , saveCompact_GreenzoneSavingMode  );
	g_config->setOption("SDL.TasLastAuthorName" , lastAuthorName);

	g_config->save();
}


