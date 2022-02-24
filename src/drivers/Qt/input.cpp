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

#include <QFileDialog>
#include <QInputDialog>
#include <QApplication>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/CheatsConf.h"
#include "Qt/TasEditor/TasEditorWindow.h"

#include "Qt/sdl.h"
#include "Qt/sdl-video.h"
#include "Qt/sdl-joystick.h"

#include "common/cheat.h"
#include "../../movie.h"
#include "../../fceu.h"
#include "../../driver.h"
#include "../../state.h"
#include "../../utils/xstring.h"
#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

#include <cstring>
#include <cstdio>

/** GLOBALS **/
int NoWaiting = 0;
extern Config *g_config;
extern bool bindSavestate, frameAdvanceLagSkip, lagCounterDisplay;
unsigned int frameAdvHoldTimer = 0;

/* UsrInputType[] is user-specified.  CurInputType[] is current
        (game loading can override user settings)
*/
static int UsrInputType[NUM_INPUT_DEVICES] = {SI_GAMEPAD, SI_GAMEPAD, SI_NONE};
static int CurInputType[NUM_INPUT_DEVICES] = {SI_GAMEPAD, SI_GAMEPAD, SI_NONE};
static int cspec = 0;
static int buttonConfigInProgress = 0;

extern int gametype;
static int DTestButton(ButtConfig *bc);

//std::list<gamepad_function_key_t *> gpKeySeqList;

/**
 * Necessary for proper GUI functioning (configuring when a game isn't loaded).
 */
void InputUserActiveFix()
{
	int x;
	for (x = 0; x < 3; x++)
	{
		CurInputType[x] = UsrInputType[x];
	}
}

/**
 * Parse game information and configure the input devices accordingly.
 */
void ParseGIInput(FCEUGI *gi)
{
	gametype = gi->type;

	CurInputType[0] = UsrInputType[0];
	CurInputType[1] = UsrInputType[1];
	CurInputType[2] = UsrInputType[2];

	if (gi->input[0] >= 0)
	{
		CurInputType[0] = gi->input[0];
	}
	if (gi->input[1] >= 0)
	{
		CurInputType[1] = gi->input[1];
	}
	if (gi->inputfc >= 0)
	{
		CurInputType[2] = gi->inputfc;
	}
	cspec = gi->cspecial;
}

int getInputSelection(int port, int *cur, int *usr)
{
	if ((port >= 0) && (port < 3))
	{
		if (cur)
		{
			*cur = CurInputType[port];
		}
		if (usr)
		{
			*usr = UsrInputType[port];
		}
	}
	return 0;
}

static uint8 QuizKingData = 0;
static uint8 HyperShotData = 0;
static uint32 MahjongData = 0;
static uint32 FTrainerData = 0;
static uint8 TopRiderData = 0;
static uint8 BWorldData[1 + 13 + 1];

static void UpdateFKB(void);
static void UpdateGamepad(void);
static void UpdateQuizKing(void);
static void UpdateHyperShot(void);
static void UpdateMahjong(void);
static void UpdateFTrainer(void);
static void UpdateTopRider(void);

static uint32 JSreturn = 0;

#include "keyscan.h"
static uint8 g_keyState[SDL_NUM_SCANCODES];
static int keyModifier = 0;
//static int DIPS = 0;

//static uint8 keyonce[SDL_NUM_SCANCODES];
//#define KEY(__a) g_keyState[MKK(__a)]

int getKeyState(int k)
{
	k = SDL_GetScancodeFromKey(k);
	if ((k >= 0) && (k < SDL_NUM_SCANCODES))
	{
		return g_keyState[k];
	}
	return 0;
}

//static int
//_keyonly(int a)
//{
//	int sc;
//
//	if (a < 0)
//	{
//		return 0;
//	}
//
//	sc = SDL_GetScancodeFromKey(a);
//
//	// check for valid key
//	if (sc >= SDL_NUM_SCANCODES || sc < 0)
//	{
//		return 0;
//	}
//
//	if (g_keyState[sc])
//	{
//		if (!keyonce[sc])
//		{
//			keyonce[sc] = 1;
//			return 1;
//		}
//	}
//	else
//	{
//		keyonce[sc] = 0;
//	}
//	return 0;
//}

uint32 GetGamepadPressedImmediate(void)
{
	//printf("JSreturn: 0x%08X\n", JSreturn);
	return JSreturn;
}

#define keyonly(__a) _keyonly(MKK(__a))

static bool g_fkbEnabled = false;

// this function loads the sdl hotkeys from the config file into the
// global scope.  this elimates the need for accessing the config file

class hotkey_t Hotkeys[HK_MAX];

hotkey_t::hotkey_t(void)
{
	sdl.value = 0;
	sdl.modifier = 0;
	qkey.value = 0;
	qkey.modifier = 0;
	prevState = 0;
	shortcut = nullptr;
	act = nullptr;
	configName = "";
}

int hotkey_t::init(QWidget *parent)
{
	std::string keyText;
	std::string prefix = "SDL.Hotkeys.";

	g_config->getOption(prefix + configName, &keyText);

	//printf("Initializing: '%s' = '%s'\n", configName, keyText.c_str() );

	shortcut = new QShortcut(QKeySequence(QString::fromStdString(keyText)), parent);

	//printf("ShortCut: '%s' = '%s'\n", configName, shortcut->key().toString().toStdString().c_str() );

	conv2SDL();
	return 0;
}

int hotkey_t::readConfig(void)
{
	std::string keyText;
	std::string prefix = "SDL.Hotkeys.";

	g_config->getOption(prefix + configName, &keyText);

	//printf("Config: '%s' = '%s'\n", configName, keyText.c_str() );
	keySeq = QKeySequence( QString::fromStdString(keyText) );

	if (shortcut)
	{
		shortcut->setKey(QString::fromStdString(keyText));

		//printf("ShortCut: '%s' = '%s'\n", configName, shortcut->key().toString().toStdString().c_str() );

		if (act)
		{
			act->setText(actText + "\t" + shortcut->key().toString());
		}
	}

	conv2SDL();
	return 0;
}

void hotkey_t::conv2SDL(void)
{
	if (shortcut == nullptr)
		return;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	qkey.value    = shortcut->key()[0].key();
	qkey.modifier = shortcut->key()[0].keyboardModifiers();

	SDL_Keycode k = convQtKey2SDLKeyCode(shortcut->key()[0].key());
	SDL_Keymod m = convQtKey2SDLModifier(shortcut->key()[0].keyboardModifiers());
#else
	qkey.value    = (Qt::Key)(shortcut->key()[0] & 0x01FFFFFF);
	qkey.modifier = (Qt::KeyboardModifier)(shortcut->key()[0] & 0xFE000000);

	SDL_Keycode k = convQtKey2SDLKeyCode((Qt::Key)(shortcut->key()[0] & 0x01FFFFFF));
	SDL_Keymod m = convQtKey2SDLModifier((Qt::KeyboardModifier)(shortcut->key()[0] & 0xFE000000));
#endif

	//printf("Key: '%s'  0x%08x\n", shortcut->key().toString().toStdString().c_str(), shortcut->key()[0] );

	sdl.value = k;
	sdl.modifier = m;

	//printf("Key: SDL: '%s' \n", SDL_GetKeyName(sdl.value) );
}

void hotkey_t::setConfigName(const char *cName)
{
	configName = cName;
}

void hotkey_t::setAction(QAction *actIn)
{
	act = actIn;

	actText = act->text();

	act->setText(actText + "\t" + shortcut->key().toString());
}

QShortcut *hotkey_t::getShortcut(void)
{
	return shortcut;
}

const char *hotkey_t::getConfigName(void)
{
	return configName;
}

int hotkey_t::getString(char *s)
{
	s[0] = 0;

	if (shortcut)
	{
		strcpy(s, shortcut->key().toString().toStdString().c_str());
	}
	//if ( sdl.modifier != 0 )
	//{
	//	if ( sdl.modifier & (KMOD_LSHIFT | KMOD_RSHIFT) )
	//	{
	//		strcat( s, "Shift+" );
	//	}

	//	if ( sdl.modifier & (KMOD_LALT | KMOD_RALT) )
	//	{
	//		strcat( s, "Alt+" );
	//	}

	//	if ( sdl.modifier & (KMOD_LCTRL | KMOD_RCTRL) )
	//	{
	//		strcat( s, "Ctrl+" );
	//	}
	//}

	//strcat( s, SDL_GetKeyName(sdl.value) );

	return 0;
}

int hotkey_t::getState(void)
{
	int k;

	if (sdl.modifier != 0)
	{
		if (sdl.modifier & (KMOD_LSHIFT | KMOD_RSHIFT))
		{
			if (!g_keyState[SDL_SCANCODE_LSHIFT] && !g_keyState[SDL_SCANCODE_RSHIFT])
			{
				return 0;
			}
		}

		if (sdl.modifier & (KMOD_LALT | KMOD_RALT))
		{
			if (!g_keyState[SDL_SCANCODE_LALT] && !g_keyState[SDL_SCANCODE_RALT])
			{
				return 0;
			}
		}

		if (sdl.modifier & (KMOD_LCTRL | KMOD_RCTRL))
		{
			if (!g_keyState[SDL_SCANCODE_LCTRL] && !g_keyState[SDL_SCANCODE_RCTRL])
			{
				return 0;
			}
		}
	}
	else
	{
		if (keyModifier != 0)
		{
			return 0;
		}
	}

	k = SDL_GetScancodeFromKey(sdl.value);
	if ((k >= 0) && (k < SDL_NUM_SCANCODES))
	{
		return g_keyState[k];
	}
	return 0;
}

int hotkey_t::getRisingEdge(void)
{
	if (sdl.value < 0)
	{
		return 0;
	}

	if (getState())
	{
		if (!prevState)
		{
			prevState = 1;
			return 1;
		}
	}
	else
	{
		prevState = 0;
	}
	return 0;
}

void hotkey_t::setModifierFromString(const char *s)
{
	int i, j;
	char id[128];

	i = 0;
	j = 0;
	sdl.modifier = 0;

	while (s[i] != 0)
	{
		j = 0;
		while (isalnum(s[i]) || (s[i] == '_'))
		{
			id[j] = tolower(s[i]);
			j++;
			i++;
		}
		id[j] = 0;

		if (j == 0)
			break;

		if (strcmp(id, "ctrl") == 0)
		{
			sdl.modifier |= (KMOD_LCTRL | KMOD_RCTRL);
		}
		else if (strcmp(id, "alt") == 0)
		{
			sdl.modifier |= (KMOD_LALT | KMOD_RALT);
		}
		else if (strcmp(id, "shift") == 0)
		{
			sdl.modifier |= (KMOD_LSHIFT | KMOD_RSHIFT);
		}

		if ((s[i] == '+') || (s[i] == '|'))
		{
			i++;
		}
		else
		{
			break;
		}
	}
}

// on every cycle of keyboardinput()
void setHotKeys(void)
{
	for (int i = 0; i < HK_MAX; i++)
	{
		Hotkeys[i].readConfig();
	}

	if ( tasWin != NULL )
	{
		tasWin->initHotKeys();
	}
	return;
}

gamepad_function_key_t::gamepad_function_key_t(void)
{
	for (int i = 0; i < 2; i++)
	{
		keySeq[i].key = 0;
		keySeq[i].modifier = Qt::NoModifier;
		keyRelReq[i] = 0;
	}
	for (int i = 0; i < 2; i++)
	{
		hk[i] = -1;
	}
	for (int i = 0; i < 2; i++)
	{
		bmap[i].ButtType = -1;
		bmap[i].DeviceNum = -1;
		bmap[i].ButtonNum = -1;
		bmap[i].state = 0;
	}
}

gamepad_function_key_t::~gamepad_function_key_t(void)
{
}

void gamepad_function_key_t::sendKeyPressEvent(int idx)
{
	bool hasShortcut = false;

	// If the hot key has a shortcut associated with it, 
	// activate shortcut directly instead of attempting to send key sequence events.
	if ( (hk[idx] >= 0) && (hk[idx] < HK_MAX) )
	{
		QShortcut *s = Hotkeys[ hk[idx] ].getShortcut();

		if ( s && s->isEnabled() )
		{
			emit s->activated();
			hasShortcut = true;
		}
	}

	if ( !hasShortcut && (keySeq[idx].key > 0) )
	{
		QKeyEvent *k = new QKeyEvent(QEvent::KeyPress, keySeq[idx].key, (Qt::KeyboardModifiers)keySeq[idx].modifier);

		qApp->postEvent((QObject *)consoleWindow, (QEvent *)k);

		keyRelReq[idx] = 1;
	}
}

void gamepad_function_key_t::sendKeyReleaseEvent(int idx)
{
	if ( keyRelReq[idx] )
	{
		QKeyEvent *k = new QKeyEvent(QEvent::KeyRelease, keySeq[idx].key, (Qt::KeyboardModifiers)keySeq[idx].modifier);

		qApp->postEvent((QObject *)consoleWindow, (QEvent *)k);

		keyRelReq[idx] = 0;
	}
}

void gamepad_function_key_t::updateStatus(void)
{
	int state_lp[2], state[2];

	state_lp[0] = bmap[0].state;
	state_lp[1] = bmap[1].state;

	state[0] = DTestButton(&bmap[0]);
	state[1] = DTestButton(&bmap[1]);

	if ((bmap[0].ButtonNum >= 0) && (bmap[1].ButtonNum >= 0))
	{
		int s, lp;

		s = state[0] && state[1];
		lp = state_lp[0] && state_lp[1];

		if (s && !lp)
		{
			sendKeyPressEvent(0);
		}
		else if (!s && lp)
		{
			sendKeyReleaseEvent(0);
			sendKeyPressEvent(1);
			sendKeyReleaseEvent(1);
		}
	}
	else if (bmap[1].ButtonNum >= 0)
	{
		if (state[1] && !state_lp[1])
		{
			sendKeyPressEvent(0);
		}
		else if (!state[1] && state_lp[1])
		{
			sendKeyReleaseEvent(0);
			sendKeyPressEvent(1);
			sendKeyReleaseEvent(1);
		}
	}
}

/***
  * This function is a wrapper for FCEUI_ToggleEmulationPause that handles
  * releasing/capturing mouse pointer during pause toggles
  * */
void TogglePause(void)
{
	FCEUI_ToggleEmulationPause();

	int no_cursor;
	g_config->getOption("SDL.NoFullscreenCursor", &no_cursor);
	int fullscreen;
	g_config->getOption("SDL.Fullscreen", &fullscreen);

	return;
}

/*** 
 * This function opens a file chooser dialog and returns the filename the 
 * user selected.
 * */
static std::string GetFilename(const char *title, int mode, const char *filter)
{
	int ret, useNativeFileDialogVal;
	QFileDialog dialog(consoleWindow, title);
	std::string initPath;
	QList<QUrl> urls;

	//if (FCEUI_EmulationPaused () == 0)
	//	FCEUI_ToggleEmulationPause ();
	std::string fname = "";

	urls << QUrl::fromLocalFile(QDir::rootPath());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile(QDir(FCEUI_GetBaseDirectory()).absolutePath());

	initPath.assign(FCEUI_GetBaseDirectory());

	switch (mode)
	{
	case 0: // Save State
		dialog.setLabelText(QFileDialog::Accept, dialog.tr("Save"));
		dialog.setFileMode(QFileDialog::AnyFile);
		initPath += "/fcs";
		break;
	default:
	case 1: // Load State
		dialog.setLabelText(QFileDialog::Accept, dialog.tr("Load"));
		dialog.setFileMode(QFileDialog::ExistingFile);
		initPath += "/fcs";
		break;
	case 2: // Record Movie To
		dialog.setLabelText(QFileDialog::Accept, dialog.tr("Record"));
		dialog.setFileMode(QFileDialog::AnyFile);
		initPath += "/movies";
		break;
	case 3: // Load Lua Script
		dialog.setLabelText(QFileDialog::Accept, dialog.tr("Load"));
		dialog.setFileMode(QFileDialog::ExistingFile);
		//initPath += "/fcs";
		break;
	}
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setDirectory(dialog.tr(initPath.c_str()));

	// Check config option to use native file dialog or not
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if (ret)
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if (fileList.size() > 0)
		{
			fname = fileList[0].toStdString();
		}
	}

	//FCEUI_ToggleEmulationPause ();
	return fname;
}

/**
* Lets the user start a new .fm2 movie file
**/
void FCEUD_MovieRecordTo(void)
{
	//bool ok = false;
	//std::string fname = GetFilename("Save FM2 movie for recording", 2, "FM2 movies|*.fm2");
	//if (!fname.size())
	//{
	//	return;													// no filename selected, quit the whole thing
	//}
	//std::string s = QInputDialog::getText( consoleWindow, QObject::tr("Movie Recording"), 
	//			QObject::tr("Enter Author Name"), QLineEdit::Normal, QObject::tr(""), &ok ).toStdString();

	//std::wstring author (s.begin (), s.end ());

	//FCEUI_SaveMovie(fname.c_str(), MOVIE_FLAG_FROM_POWERON, author);
	if ( consoleWindow )
	{
		consoleWindow->recordMovie();
	}
}

/**
* Lets the user save a savestate to a specific file
**/
void FCEUD_SaveStateAs(void)
{
	std::string fname = GetFilename("Save State As...", 0, "Save States|*.fc0");
	if (!fname.size())
		return; // no filename selected, quit the whole thing

	FCEUI_SaveState(fname.c_str());
}

/**
* Lets the user load a savestate from a specific file
*/
void FCEUD_LoadStateFrom(void)
{
	std::string fname = GetFilename("Load State From...", 1, "Save States|*.fc?");
	if (!fname.size())
		return; // no filename selected, quit the whole thing

	FCEUI_LoadState(fname.c_str());
}

/**
* Hook for transformer board
*/
unsigned int *GetKeyboard(void)
{
	int size = 256;

	Uint8 *keystate = (Uint8 *)SDL_GetKeyboardState(&size);

	return (unsigned int *)(keystate);
}

static void FKB_CheckShortcutConflicts(void)
{
	bool fkbActv;
	QShortcut *shortcut;

	fkbActv = g_fkbEnabled && (CurInputType[2] == SIFC_FKB);

	for (int i = 0; i < HK_MAX; i++)
	{
		shortcut = Hotkeys[i].getShortcut();

		if (i == HK_FKB_ENABLE)
		{
			if (shortcut)
			{
				shortcut->setEnabled(true);
			}
			continue;
		}

		if (shortcut)
		{
			shortcut->setEnabled(!fkbActv);
		}
	}
}

void toggleFamilyKeyboardFunc(void)
{
	if (CurInputType[2] == SIFC_FKB)
	{
		g_fkbEnabled = !g_fkbEnabled;

		FCEUI_DispMessage("Family Keyboard %sabled.", 0,
						  g_fkbEnabled ? "En" : "Dis");
	}
	else
	{
		FCEUI_DispMessage("Family Keyboard Not Active", 0);
		g_fkbEnabled = false;
	}
	FKB_CheckShortcutConflicts();
}

bool isFamilyKeyboardActv(void)
{
	return ((CurInputType[2] == SIFC_FKB) && g_fkbEnabled);
}

/**
 * Parse keyboard commands and execute accordingly.
 */
static void KeyboardCommands(void)
{
	// get the keyboard input

	// check if the family keyboard is enabled
	if (CurInputType[2] == SIFC_FKB)
	{
		if (g_fkbEnabled)
		{
			return;
		}
	}

	//if (g_keyState[SDL_SCANCODE_LSHIFT] || g_keyState[SDL_SCANCODE_RSHIFT])
	//{
	//	is_shift = 1;
	//}
	//else
	//{
	//	is_shift = 0;
	//}

	//if (g_keyState[SDL_SCANCODE_LALT] || g_keyState[SDL_SCANCODE_RALT])
	//{
	//	is_alt = 1;
	//}
	//else
	//{
	//	is_alt = 0;
	//}

	//if ( Hotkeys[HK_TOGGLE_BG].getRisingEdge() )
	//{
	//	bool fgOn, bgOn;

	//	FCEUI_GetRenderPlanes( fgOn, bgOn );

	//	FCEUI_SetRenderPlanes( fgOn, !bgOn );
	//}

	// Alt-Enter to toggle full-screen
	// This is already handled by Qt Menu Actions
	// So only process if menu is hidden or disabled.
	//if ( is_alt )
	//{
	//	if (keyonly (ENTER))
	//	{
	//		if ( consoleWindow )
	//		{
	//			if ( !consoleWindow->menuBar()->isVisible() )
	//			{
	//				consoleWindow->toggleFullscreen();
	//			}
	//		}
	//	}
	//}

	// Alt-M to toggle Main Menu Visibility
	//if ( is_alt )
	//{
	//	if (keyonly (SLASH))
	//	{
	//		if ( consoleWindow )
	//		{
	//			consoleWindow->toggleMenuVis();
	//		}
	//	}
	//}

	// Toggle Movie auto-backup
	//if ( is_shift )
	//{
	//	if (keyonly (M))
	//	{
	//		autoMovieBackup ^= 1;
	//		FCEUI_DispMessage ("Automatic movie backup %sabled.", 0,
	//			 autoMovieBackup ? "en" : "dis");
	//	}
	//}

	//if ( is_alt )
	//{
	//	// Start recording an FM2 movie on Alt+R
	//	if (keyonly (R))
	//	{
	//		FCEUD_MovieRecordTo ();
	//	}
	//	// Save a state from a file
	//	if (keyonly (S))
	//	{
	//		FCEUD_SaveStateAs ();
	//	}
	//	// Load a state from a file
	//	if (keyonly (L))
	//	{
	//		FCEUD_LoadStateFrom ();
	//	}
	//}

	// Famicom disk-system games
	//	if (gametype == GIT_FDS)
	//	{
	//		if ( Hotkeys[HK_FDS_SELECT].getRisingEdge() )
	//		{
	//			FCEUI_FDSSelect ();
	//		}
	//		if ( Hotkeys[HK_FDS_EJECT].getRisingEdge() )
	//		{
	//			FCEUI_FDSInsert ();
	//		}
	//	}
	//
	//	if ( Hotkeys[HK_SCREENSHOT].getRisingEdge() )
	//	{
	//		FCEUI_SaveSnapshot ();
	//	}

	// if not NES Sound Format
	//	if (gametype != GIT_NSF)
	//	{
	//		if ( Hotkeys[HK_CHEAT_MENU].getRisingEdge() )
	//		{
	//			openCheatDialog( consoleWindow );
	//		}
	//
	//		// f5 (default) save key, hold shift to save movie
	//		if ( Hotkeys[HK_SAVE_STATE].getRisingEdge() )
	//		{
	//			if (is_shift)
	//			{
	//				std::string movie_fname = FCEU_MakeFName (FCEUMKF_MOVIE, 0, 0);
	//				FCEUI_printf ("Recording movie to %s\n", movie_fname.c_str() );
	//				FCEUI_SaveMovie(movie_fname.c_str() , MOVIE_FLAG_NONE, L"");
	//			}
	//			else
	//			{
	//				FCEUI_SaveState (NULL);
	//			}
	//		}
	//
	//		// f7 to load state, Shift-f7 to load movie
	//		if ( Hotkeys[HK_LOAD_STATE].getRisingEdge() )
	//		{
	//			if (is_shift)
	//			{
	//				FCEUI_StopMovie ();
	//				std::string fname;
	//				fname =
	//				GetFilename ("Open FM2 movie for playback...", false,
	//								"FM2 movies|*.fm2");
	//				if (fname != "")
	//				{
	//					if (fname.find (".fm2") != std::string::npos
	//					|| fname.find (".fm3") != std::string::npos)
	//					{
	//						FCEUI_printf ("Playing back movie located at %s\n",
	//										fname.c_str ());
	//						FCEUI_LoadMovie (fname.c_str (), false, false);
	//					}
	//					else
	//					{
	//						FCEUI_printf
	//							("Only .fm2 and .fm3 movies are supported.\n");
	//					}
	//				}
	//			}
	//			else
	//			{
	//				FCEUI_LoadState(NULL);
	//			}
	//		}
	//	}

	//if ( Hotkeys[HK_DECREASE_SPEED].getRisingEdge() )
	//{
	//	DecreaseEmulationSpeed();
	//}

	//if ( Hotkeys[HK_INCREASE_SPEED].getRisingEdge() )
	//{
	//	IncreaseEmulationSpeed ();
	//}

	//if ( Hotkeys[HK_TOGGLE_FRAME_DISPLAY].getRisingEdge() )
	//{
	//	FCEUI_MovieToggleFrameDisplay ();
	//}

	//if ( Hotkeys[HK_TOGGLE_INPUT_DISPLAY].getRisingEdge() )
	//{
	//	FCEUI_ToggleInputDisplay ();
	//	extern int input_display;
	//	g_config->setOption ("SDL.InputDisplay", input_display);
	//}

	//if ( Hotkeys[HK_MOVIE_TOGGLE_RW].getRisingEdge() )
	//{
	//	FCEUI_SetMovieToggleReadOnly (!FCEUI_GetMovieToggleReadOnly ());
	//}

#ifdef CREATE_AVI
	if (Hotkeys[HK_MUTE_CAPTURE].getRisingEdge())
	{
		extern int mutecapture;
		mutecapture ^= 1;
	}
#endif

	//if ( Hotkeys[HK_PAUSE].getRisingEdge() )
	//{
	//	//FCEUI_ToggleEmulationPause();
	//	// use the wrapper function instead of the fceui function directly
	//	// so we can handle cursor grabbage
	//	TogglePause ();
	//}

	// Toggle throttling
	if (Hotkeys[HK_TURBO].getState())
	{
		NoWaiting |= 0x01;
	}
	else
	{
		NoWaiting &= 0x02;
		//printf("NoWaiting: 0x%04x\n", NoWaiting );
	}

	static bool frameAdvancing = false;
	if (Hotkeys[HK_FRAME_ADVANCE].getState())
	{
		if (frameAdvancing == false)
		{
			frameAdvHoldTimer = 0;
			FCEUI_FrameAdvance();
			frameAdvancing = true;
			//printf("Frame Advance Start\n");
		}
		if ( consoleWindow )
		{
			frameAdvHoldTimer += consoleWindow->getPeriodicInterval();
		}
	}
	else
	{
		if (frameAdvancing)
		{
			FCEUI_FrameAdvanceEnd();
			frameAdvancing = false;
			frameAdvHoldTimer = 0;
			//printf("Frame Advance End\n");
		}
	}

	//if ( Hotkeys[HK_RESET].getRisingEdge() )
	//{
	//	FCEUI_ResetNES ();
	//}
	//if( Hotkeys[HK_POWER].getRisingEdge() )
	//{
	//    FCEUI_PowerNES();
	//}
	//	if ( Hotkeys[HK_QUIT].getRisingEdge() )
	//	{
	//		CloseGame();
	//		FCEUI_Kill();
	//		SDL_Quit();
	//		exit(0);
	//	}
	//	else
	//#ifdef _S9XLUA_H
	//	if ( Hotkeys[HK_LOAD_LUA].getRisingEdge() )
	//	{
	//		std::string fname;
	//		fname = GetFilename ("Open LUA script...", 3, "Lua scripts|*.lua");
	//		if (fname != "")
	//		FCEU_LoadLuaCode (fname.c_str ());
	//	}
	//#endif

	//for (int i = 0; i < 10; i++)
	//{
	//	if ( Hotkeys[HK_SELECT_STATE_0 + i].getRisingEdge() )
	//	{
	//		FCEUI_SelectState (i, 1);
	//	}
	//}

	//if ( Hotkeys[HK_SELECT_STATE_NEXT].getRisingEdge() )
	//{
	//	FCEUI_SelectStateNext (1);
	//}

	//if ( Hotkeys[HK_SELECT_STATE_PREV].getRisingEdge() )
	//{
	//	FCEUI_SelectStateNext (-1);
	//}

	//if ( Hotkeys[HK_BIND_STATE].getRisingEdge() )
	//{
	//	bindSavestate ^= 1;
	//	FCEUI_DispMessage ("Savestate binding to movie %sabled.", 0,
	//	bindSavestate ? "en" : "dis");
	//}

	//if ( Hotkeys[HK_FA_LAG_SKIP].getRisingEdge() )
	//{
	//	frameAdvanceLagSkip ^= 1;
	//	FCEUI_DispMessage ("Skipping lag in Frame Advance %sabled.", 0,
	//	frameAdvanceLagSkip ? "en" : "dis");
	//}

	//if ( Hotkeys[HK_LAG_COUNTER_DISPLAY].getRisingEdge() )
	//{
	//	lagCounterDisplay ^= 1;
	//}

	//if ( Hotkeys[HK_TOGGLE_SUBTITLE].getRisingEdge() )
	//{
	//	movieSubtitles = !movieSubtitles;
	//	FCEUI_DispMessage ("Movie subtitles o%s.", 0,
	//	movieSubtitles ? "n" : "ff");
	//}

	//if ( Hotkeys[HK_VOLUME_DOWN].getRisingEdge() )
	//{
	//	FCEUD_SoundVolumeAdjust(-1);
	//}

	//if ( Hotkeys[HK_VOLUME_UP].getRisingEdge() )
	//{
	//	FCEUD_SoundVolumeAdjust(1);
	//}

	// VS Unisystem games
	//	if (gametype == GIT_VSUNI)
	//	{
	//		// insert coin
	//		if ( Hotkeys[HK_VS_INSERT_COIN].getRisingEdge() )
	//		{
	//			FCEUI_VSUniCoin ();
	//		}
	//
	//		// toggle dipswitch display
	//		if ( Hotkeys[HK_VS_TOGGLE_DIPSWITCH].getRisingEdge() )
	//		{
	//			DIPS ^= 1;
	//			FCEUI_VSUniToggleDIPView ();
	//		}
	//		if (!(DIPS & 1))
	//			goto DIPSless;
	//
	//		// toggle the various dipswitches
	//		for(int i=1; i<=8;i++)
	//		{
	//			if(keyonly(i))
	//				FCEUI_VSUniToggleDIP(i-1);
	//		}
	//	}
	//	else
	//	{
	//		static uint8 bbuf[32];
	//		static int bbuft;
	//		static int barcoder = 0;
	//
	//		if (keyonly (H))
	//			FCEUI_NTSCSELHUE ();
	//		if (keyonly (T))
	//			FCEUI_NTSCSELTINT ();
	//
	//		if (Hotkeys[HK_DECREASE_SPEED].getRisingEdge())
	//		{
	//			FCEUI_NTSCDEC ();
	//		}
	//		if (Hotkeys[HK_INCREASE_SPEED].getRisingEdge())
	//		{
	//			FCEUI_NTSCINC ();
	//		}
	//
	//		if ((CurInputType[2] == SIFC_BWORLD) || (cspec == SIS_DATACH))
	//		{
	//			if (keyonly (F8))
	//			{
	//				barcoder ^= 1;
	//				if (!barcoder)
	//				{
	//					if (CurInputType[2] == SIFC_BWORLD)
	//					{
	//						strcpy ((char *) &BWorldData[1], (char *) bbuf);
	//						BWorldData[0] = 1;
	//					}
	//					else
	//					{
	//						FCEUI_DatachSet (bbuf);
	//					}
	//					FCEUI_DispMessage ("Barcode Entered", 0);
	//				}
	//				else
	//				{
	//					bbuft = 0;
	//					FCEUI_DispMessage ("Enter Barcode", 0);
	//				}
	//			}
	//		}
	//		else
	//		{
	//			barcoder = 0;
	//		}
	/*
#define SSM(x)
do {                                              \
	if(barcoder) {                                \
		if(bbuft < 13) {                          \
			bbuf[bbuft++] = '0' + x;              \
			bbuf[bbuft] = 0;                      \
		}                                         \
		FCEUI_DispMessage("Barcode: %s",0, bbuf); \
	}                                             \
} while(0)

		DIPSless:
		for(int i=0; i<10;i++)
		{
			if (keyonly (i))
				SSM (i);
		}
#undef SSM
*/
	//	}
}

/**
 * Return the state of the mouse buttons.  Input 'd' is an array of 3
 * integers that store <x, y, button state>.
 */
void GetMouseData(uint32 (&d)[3])
{
	uint32 t, b;
	double nx = 0.0, ny = 0.0;

	b = 0; // map mouse buttons

	if (consoleWindow->viewport_SDL)
	{
		consoleWindow->viewport_SDL->getNormalizedCursorPos(nx, ny);

		if (consoleWindow->viewport_SDL->getMouseButtonState(Qt::LeftButton))
		{
			b |= 0x01;
		}
		if (consoleWindow->viewport_SDL->getMouseButtonState(Qt::RightButton))
		{
			b |= 0x02;
		}
	}
	else if (consoleWindow->viewport_GL)
	{
		consoleWindow->viewport_GL->getNormalizedCursorPos(nx, ny);

		if (consoleWindow->viewport_GL->getMouseButtonState(Qt::LeftButton))
		{
			b |= 0x01;
		}
		if (consoleWindow->viewport_GL->getMouseButtonState(Qt::RightButton))
		{
			b |= 0x02;
		}
	}

	t = PtoV(nx, ny);
	d[2] = b;
	d[0] = t & 0xFFFF;
	d[1] = (t >> 16) & 0xFFFF;

	//printf("mouse %d %d %d\n", d[0], d[1], d[2]);
}

void GetMouseRelative(int32 (&d)[3])
{
	// converts absolute mouse positions to relative ones for input devices that require this

	// The windows version additionally in fullscreen will constantly return the mouse to center screen
	// after reading it, so that the user can endlessly keep moving the mouse.
	// The same should eventually be implemented here, but this version should minimally provide
	// the necessary relative input, piggybacking on the already implemented GetMouseData.

	static int cx = -1;
	static int cy = -1;

	uint32 md[3];
	GetMouseData(md);

	if (cx < 0 || cy < 0)
	{
		cx = md[0];
		cy = md[1];
	}

	int dx = md[0] - cx;
	int dy = md[1] - cy;

	d[0] = dx;
	d[1] = dy;
	d[2] = md[2]; // buttons
}

//static void checkKeyBoardState( int scanCode )
//{
//	printf("Key State is: %i \n", g_keyState[ scanCode ] );
//}
/**
 * Handles outstanding SDL events.
 */
static void
UpdatePhysicalInput()
{
	SDL_Event event;

	// loop, handling all pending events
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			CloseGame();
			puts("Quit");
			break;
		case SDL_FCEU_HOTKEY_EVENT:
			switch (event.user.code)
			{
			case HK_PAUSE:
				TogglePause();
				break;
			default:
				FCEU_printf("Warning: unknown hotkey event %d\n",
							event.user.code);
			}
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			//printf("SDL_Event.type: %i  Keysym: %i  ScanCode: %i\n",
			//		event.type, event.key.keysym.sym, event.key.keysym.scancode );

			keyModifier = event.key.keysym.mod;
			g_keyState[SDL_SCANCODE_LSHIFT] = (event.key.keysym.mod & KMOD_LSHIFT) ? 1 : 0;
			g_keyState[SDL_SCANCODE_RSHIFT] = (event.key.keysym.mod & KMOD_RSHIFT) ? 1 : 0;
			g_keyState[SDL_SCANCODE_LALT] = (event.key.keysym.mod & KMOD_LALT) ? 1 : 0;
			g_keyState[SDL_SCANCODE_RALT] = (event.key.keysym.mod & KMOD_RALT) ? 1 : 0;
			g_keyState[SDL_SCANCODE_LCTRL] = (event.key.keysym.mod & KMOD_LCTRL) ? 1 : 0;
			g_keyState[SDL_SCANCODE_RCTRL] = (event.key.keysym.mod & KMOD_RCTRL) ? 1 : 0;

			g_keyState[event.key.keysym.scancode] = (event.type == SDL_KEYDOWN) ? 1 : 0;

			KeyboardCommands();

			break;
		case SDL_JOYDEVICEADDED:
			AddJoystick(event.jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			RemoveJoystick(event.jdevice.which);
			break;
		default:
			break;
		}
	}
	//SDL_PumpEvents();
}

/**
 *  Begin configuring the buttons by placing the video and joystick
 *  subsystems into a well-known state.  Button configuration really
 *  needs to be cleaned up after the new config system is in place.
 */
int ButtonConfigBegin()
{
	// initialize the joystick subsystem (if not already inited)
	InitJoysticks();

	buttonConfigInProgress = 1;

	return 1;
}

/**
 *  Finish configuring the buttons by reverting the video and joystick
 *  subsystems to their previous state.  Button configuration really
 *  needs to be cleaned up after the new config system is in place.
 */
void ButtonConfigEnd()
{
	buttonConfigInProgress = 0;
}

/**
 * Tests to see if a specified button is currently pressed.
 */
static int
DTestButton(ButtConfig *bc)
{

	if (bc->ButtType == BUTTC_KEYBOARD)
	{
		if (g_keyState[SDL_GetScancodeFromKey(bc->ButtonNum)])
		{
			bc->state = 1;
			return 1;
		}
		else
		{
			bc->state = 0;
		}
	}
	else if (bc->ButtType == BUTTC_JOYSTICK)
	{
		if (DTestButtonJoy(bc))
		{
			return 1;
		}
	}
	return 0;
}

#define MK(x)                     \
	{                             \
		BUTTC_KEYBOARD, 0, (x), 0 \
	}
//#define MK2(x1,x2)  {BUTTC_KEYBOARD,0,MKK(x1)}
#define MKZ()       \
	{               \
		0, 0, -1, 0 \
	}
#define GPZ()                      \
	{                              \
		MKZ(), MKZ(), MKZ(), MKZ() \
	}

//ButtConfig GamePadConfig[ GAMEPAD_NUM_DEVICES ][ GAMEPAD_NUM_BUTTONS ] =
//{
///* Gamepad 1 */
//	{MK (KP_3), MK (KP_2), MK (SLASH), MK (ENTER),
//	MK (w), MK (z), MK (a), MK (s), MKZ (), MKZ ()},
//
//	/* Gamepad 2 */
//	GPZ (),
//
//	/* Gamepad 3 */
//	GPZ (),
//
//	/* Gamepad 4 */
//	GPZ ()
//};

/**
 * Update the status of the gamepad input devices.
 */
static void
UpdateGamepad(void)
{
	// don't update during movie playback
	if (FCEUMOV_Mode(MOVIEMODE_PLAY))
	{
		return;
	}

	uint32 JS = 0;
	int x,c;
	int wg;
	bool fire;
	char btns[GAMEPAD_NUM_BUTTONS];

	int opposite_dirs;
	g_config->getOption("SDL.Input.EnableOppositeDirectionals", &opposite_dirs);

	// go through each of the four game pads
	for (wg = 0; wg < 4; wg++)
	{
		bool left = false;
		bool up = false;
		memset( btns, 0, sizeof(btns) );

		for (c = 0; c < GamePad_t::NUM_CONFIG; c++)
		{
			// a, b, select, start, up, down, left, right
			for (x = 0; x < 8; x++)
			{
				if (DTestButton(&GamePad[wg].bmap[c][x]))
				{
					btns[x] = 1;
					//printf("GamePad%i Button Hit: %i \n", wg, x );
					if (opposite_dirs == 0)
					{
						// test for left+right and up+down
						if (x == 4)
						{
							up = true;
						}
						if ((x == 5) && (up == true))
						{
							continue;
						}
						if (x == 6)
						{
							left = true;
						}
						if ((x == 7) && (left == true))
						{
							continue;
						}
					}
					JS |= (1 << x) << (wg << 3);
				}
			}

			int four_button_exit;
			g_config->getOption("SDL.ABStartSelectExit", &four_button_exit);
			// if a+b+start+select is pressed, exit
			if (four_button_exit && JS == 15)
			{
				FCEUI_printf("all buttons pressed, exiting\n");
				CloseGame();
				FCEUI_Kill();
				exit(0);
			}

			// rapid-fire a, rapid-fire b
			for (x = 0; x < 2; x++)
			{
				if (DTestButton(&GamePad[wg].bmap[c][8 + x]))
				{
					fire = GetAutoFireState(x);

					if (fire)
					{
						JS |= (1 << x) << (wg << 3);
					}
					btns[8+x] = 1;
				}
			}
		}

		for (x = 0; x < GAMEPAD_NUM_BUTTONS; x++)
		{
			GamePad[wg].bmapState[x] = btns[x];
		}
	}

	//  for(x=0;x<32;x+=8)      /* Now, test to see if anything weird(up+down at same time)
	//                     is happening, and correct */
	//  {
	//   if((JS & (0xC0<<x) ) == (0xC0<<x) ) JS&=~(0xC0<<x);
	//   if((JS & (0x30<<x) ) == (0x30<<x) ) JS&=~(0x30<<x);
	//  }

	JSreturn = JS;
}

static ButtConfig powerpadsc[2][12] = {
	{MK(SDLK_O), MK(SDLK_P), MK(SDLK_BRACKET_LEFT),
	 MK(SDLK_BRACKET_RIGHT), MK(SDLK_K), MK(SDLK_L), MK(SDLK_SEMICOLON),
	 MK(SDLK_APOSTROPHE),
	 MK(SDLK_M), MK(SDLK_COMMA), MK(SDLK_PERIOD), MK(SDLK_SLASH)},
	{MK(SDLK_O), MK(SDLK_P), MK(SDLK_BRACKET_LEFT),
	 MK(SDLK_BRACKET_RIGHT), MK(SDLK_K), MK(SDLK_L), MK(SDLK_SEMICOLON),
	 MK(SDLK_APOSTROPHE),
	 MK(SDLK_M), MK(SDLK_COMMA), MK(SDLK_PERIOD), MK(SDLK_SLASH)}};

static uint32 powerpadbuf[2] = {0, 0};

/**
 * Update the status of the power pad input device.
 */
static uint32
UpdatePPadData(int w)
{
	// don't update if a movie is playing
	if (FCEUMOV_Mode(MOVIEMODE_PLAY))
	{
		return 0;
	}

	uint32 r = 0;
	ButtConfig *ppadtsc = powerpadsc[w];
	int x;

	// update each of the 12 buttons
	for (x = 0; x < 12; x++)
	{
		if (DTestButton(&ppadtsc[x]))
		{
			r |= 1 << x;
		}
	}

	return r;
}

static uint32 MouseData[3] = {0, 0, 0};
static int32 MouseRelative[3] = {0, 0, 0};

static uint8 fkbkeys[0x48];

static void updateGamePadKeyMappings(void)
{
	std::list<gamepad_function_key_t *>::iterator it;

	for (int i=0; i<4; i++)
	{
		if (GamePad[i].gpKeySeqList.size() == 0)
		{
			continue;
		}

		for (it = GamePad[i].gpKeySeqList.begin(); it != GamePad[i].gpKeySeqList.end(); it++)
		{
			(*it)->updateStatus();
		}
	}
}

/**
 * Update all of the input devices required for the active game.
 */
void FCEUD_UpdateInput(void)
{
	int x;
	int t = 0;

	if (buttonConfigInProgress)
	{
		return;
	}

	updateGamePadKeyMappings();

	UpdatePhysicalInput();
	KeyboardCommands();

	for (x = 0; x < 2; x++)
	{
		switch (CurInputType[x])
		{
		case SI_GAMEPAD:
		case SI_SNES:
			t |= 1;
			break;
		case SI_ARKANOID:
			t |= 2;
			break;
		case SI_ZAPPER:
			t |= 2;
			break;
		case SI_POWERPADA:
		case SI_POWERPADB:
			powerpadbuf[x] = UpdatePPadData(x);
			break;
		case SI_MOUSE:
		case SI_SNES_MOUSE:
			t |= 4;
			break;
		}
	}

	switch (CurInputType[2])
	{
	case SIFC_ARKANOID:
		t |= 2;
		break;
	case SIFC_SHADOW:
		t |= 2;
		break;
	case SIFC_FKB:
		if (g_fkbEnabled)
		{
			UpdateFKB();
		}
		break;
	case SIFC_HYPERSHOT:
		UpdateHyperShot();
		break;
	case SIFC_MAHJONG:
		UpdateMahjong();
		break;
	case SIFC_QUIZKING:
		UpdateQuizKing();
		break;
	case SIFC_FTRAINERB:
	case SIFC_FTRAINERA:
		UpdateFTrainer();
		break;
	case SIFC_TOPRIDER:
		UpdateTopRider();
		break;
	case SIFC_OEKAKIDS:
		t |= 2;
		break;
	}

	if (t & 1)
	{
		UpdateGamepad();
	}

	// Don't get input when a movie is playing back
	if (!FCEUMOV_Mode(MOVIEMODE_PLAY))
	{
		if (t & 2)
		{
			GetMouseData(MouseData);
		}
		if (t & 4)
		{
			GetMouseRelative(MouseRelative);
		}
	}
}

void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1,
					ESIFC fcexp)
{
	eoptions &= ~EO_FOURSCORE;
	if (fourscore)
	{ // Four Score emulation, only support gamepads, nothing else
		eoptions |= EO_FOURSCORE;
		CurInputType[0] = SI_GAMEPAD; // Controllers 1 and 3
		CurInputType[1] = SI_GAMEPAD; // Controllers 2 and 4
		CurInputType[2] = SIFC_NONE;  // No extension
	}
	else
	{
		// no Four Core emulation, check the config/movie file for controller types
		CurInputType[0] = port0;
		CurInputType[1] = port1;
		CurInputType[2] = fcexp;
	}

	replaceP2StartWithMicrophone = microphone;

	InitInputInterface();
}

/**
 * Initialize the input device interface between the emulation and the driver.
 */
void InitInputInterface()
{
	void *InputDPtr;

	int t;
	int x;
	int attrib;

	memset(g_keyState, 0, sizeof(g_keyState));

	for (t = 0, x = 0; x < 2; x++)
	{
		attrib = 0;
		InputDPtr = 0;

		switch (CurInputType[x])
		{
		case SI_POWERPADA:
		case SI_POWERPADB:
			InputDPtr = &powerpadbuf[x];
			break;
		case SI_GAMEPAD:
		case SI_SNES:
			InputDPtr = &JSreturn;
			break;
		case SI_ARKANOID:
			InputDPtr = MouseData;
			t |= 1;
			break;
		case SI_ZAPPER:
			InputDPtr = MouseData;
			t |= 1;
			attrib = 1;
			break;
		case SI_MOUSE:
		case SI_SNES_MOUSE:
			InputDPtr = MouseRelative;
			t |= 1;
			break;
		}
		FCEUI_SetInput(x, (ESI)CurInputType[x], InputDPtr, attrib);
	}

	attrib = 0;
	InputDPtr = 0;
	switch (CurInputType[2])
	{
	case SIFC_SHADOW:
		InputDPtr = MouseData;
		t |= 1;
		attrib = 1;
		break;
	case SIFC_OEKAKIDS:
		InputDPtr = MouseData;
		t |= 1;
		attrib = 1;
		break;
	case SIFC_ARKANOID:
		InputDPtr = MouseData;
		t |= 1;
		break;
	case SIFC_FKB:
		InputDPtr = fkbkeys;
		break;
	case SIFC_HYPERSHOT:
		InputDPtr = &HyperShotData;
		break;
	case SIFC_MAHJONG:
		InputDPtr = &MahjongData;
		break;
	case SIFC_QUIZKING:
		InputDPtr = &QuizKingData;
		break;
	case SIFC_TOPRIDER:
		InputDPtr = &TopRiderData;
		break;
	case SIFC_BWORLD:
		InputDPtr = BWorldData;
		break;
	case SIFC_FTRAINERA:
	case SIFC_FTRAINERB:
		InputDPtr = &FTrainerData;
		break;
	}

	FCEUI_SetInputFC((ESIFC)CurInputType[2], InputDPtr, attrib);
	FCEUI_SetInputFourscore((eoptions & EO_FOURSCORE) != 0);
}

static ButtConfig fkbmap[0x48] = {
	/*  0 */ MK(SDLK_F1), MK(SDLK_F2), MK(SDLK_F3), MK(SDLK_F4), MK(SDLK_F5), MK(SDLK_F6), MK(SDLK_F7), MK(SDLK_F8),
	/*  8 */ MK(SDLK_1), MK(SDLK_2), MK(SDLK_3), MK(SDLK_4), MK(SDLK_5), MK(SDLK_6), MK(SDLK_7), MK(SDLK_8), MK(SDLK_9),
	/* 17 */ MK(SDLK_0),
	/* 18 */ MK(SDLK_MINUS), MK(SDLK_EQUAL), MK(SDLK_BACKSLASH), MK(SDLK_BACKSPACE),
	/* 22 */ MK(SDLK_ESCAPE), MK(SDLK_Q), MK(SDLK_W), MK(SDLK_E), MK(SDLK_R), MK(SDLK_T), MK(SDLK_Y), MK(SDLK_U), MK(SDLK_I),
	/* 31 */ MK(SDLK_O),
	/* 32 */ MK(SDLK_P), MK(SDLK_GRAVE), MK(SDLK_BRACKET_LEFT), MK(SDLK_ENTER),
	/* 36 */ MK(SDLK_LEFTCONTROL), MK(SDLK_A), MK(SDLK_S), MK(SDLK_D), MK(SDLK_F), MK(SDLK_G), MK(SDLK_H), MK(SDLK_J),
	/* 44 */ MK(SDLK_K),
	/* 45 */ MK(SDLK_L), MK(SDLK_SEMICOLON), MK(SDLK_APOSTROPHE), MK(SDLK_BRACKET_RIGHT), MK(SDLK_INSERT),
	/* 50 */ MK(SDLK_LEFTSHIFT), MK(SDLK_Z), MK(SDLK_X), MK(SDLK_C), MK(SDLK_V), MK(SDLK_B), MK(SDLK_N), MK(SDLK_M),
	/* 58 */ MK(SDLK_COMMA),
	/* 59 */ MK(SDLK_PERIOD), MK(SDLK_SLASH), MK(SDLK_RIGHTALT), MK(SDLK_RIGHTSHIFT), MK(SDLK_LEFTALT),
	/* 64 */ MK(SDLK_SPACE),
	/* 65 */ MK(SDLK_DELETE), MK(SDLK_END), MK(SDLK_PAGEDOWN),
	/* 68 */ MK(SDLK_CURSORUP), MK(SDLK_CURSORLEFT), MK(SDLK_CURSORRIGHT), MK(SDLK_CURSORDOWN)};

/**
 * Update the status of the Family KeyBoard.
 */
static void UpdateFKB()
{
	int x;
	char leftShiftDown;
	//static char lp[0x48];

	leftShiftDown = DTestButton(&fkbmap[50]);

	for (x = 0; x < 0x48; x++)
	{
		if ( leftShiftDown && (x == 62) )
		{	// Family BASIC appears to not like when both shift keys are pressed at the
			// same time. Since Qt key events do not differentiate between left and right
			// shift this GUI sets both left and right shift scancodes when a shift modifier
			// is detected. So to avoid having the FKB see both shift keys pressed at once,
			// always skip the right shift key here if the left key is already detected down.
			fkbkeys[x] = 0;
			continue;
		}

		if (DTestButton(&fkbmap[x]))
		{
			fkbkeys[x] = 1;

			//if ( !lp[x] )
			//{
			//	printf("FKB Key %i Down\n", x );
			//}
		}
		else
		{
			fkbkeys[x] = 0;

			//if ( lp[x] )
			//{
			//	printf("FKB Key %i Up\n", x );
			//}
		}
		//lp[x] = fkbkeys[x];
	}
}

static ButtConfig HyperShotButtons[4] = {
	MK(SDLK_Q), MK(SDLK_W), MK(SDLK_E), MK(SDLK_R)};

/**
 * Update the status of the HyperShot input device.
 */
static void
UpdateHyperShot()
{
	int x;

	HyperShotData = 0;
	for (x = 0; x < 0x4; x++)
	{
		if (DTestButton(&HyperShotButtons[x]))
		{
			HyperShotData |= 1 << x;
		}
	}
}

static ButtConfig MahjongButtons[21] = {
	MK(SDLK_Q), MK(SDLK_W), MK(SDLK_E), MK(SDLK_R), MK(SDLK_T),
	MK(SDLK_A), MK(SDLK_S), MK(SDLK_D), MK(SDLK_F), MK(SDLK_G), MK(SDLK_H), MK(SDLK_J), MK(SDLK_K), MK(SDLK_L),
	MK(SDLK_Z), MK(SDLK_X), MK(SDLK_C), MK(SDLK_V), MK(SDLK_B), MK(SDLK_N), MK(SDLK_M)};

/**
 * Update the status of for the Mahjong input device.
 */
static void
UpdateMahjong()
{
	int x;

	MahjongData = 0;
	for (x = 0; x < 21; x++)
	{
		if (DTestButton(&MahjongButtons[x]))
		{
			MahjongData |= 1 << x;
		}
	}
}

static ButtConfig QuizKingButtons[6] = {
	MK(SDLK_Q), MK(SDLK_W), MK(SDLK_E), MK(SDLK_R), MK(SDLK_T), MK(SDLK_Y)};

/**
 * Update the status of the QuizKing input device.
 */
static void
UpdateQuizKing()
{
	int x;

	QuizKingData = 0;

	for (x = 0; x < 6; x++)
	{
		if (DTestButton(&QuizKingButtons[x]))
		{
			QuizKingData |= 1 << x;
		}
	}
}

static ButtConfig TopRiderButtons[8] = {
	MK(SDLK_Q), MK(SDLK_W), MK(SDLK_E), MK(SDLK_R), MK(SDLK_T), MK(SDLK_Y), MK(SDLK_U), MK(SDLK_I)};

/**
 * Update the status of the TopRider input device.
 */
static void
UpdateTopRider()
{
	int x;
	TopRiderData = 0;
	for (x = 0; x < 8; x++)
	{
		if (DTestButton(&TopRiderButtons[x]))
		{
			TopRiderData |= (1 << x);
		}
	}
}

static ButtConfig FTrainerButtons[12] = {
	MK(SDLK_O), MK(SDLK_P), MK(SDLK_BRACKET_LEFT),
	MK(SDLK_BRACKET_RIGHT), MK(SDLK_K), MK(SDLK_L), MK(SDLK_SEMICOLON),
	MK(SDLK_APOSTROPHE),
	MK(SDLK_M), MK(SDLK_COMMA), MK(SDLK_PERIOD), MK(SDLK_SLASH)};

/**
 * Update the status of the FTrainer input device.
 */
static void
UpdateFTrainer()
{
	int x;
	FTrainerData = 0;

	for (x = 0; x < 12; x++)
	{
		if (DTestButton(&FTrainerButtons[x]))
		{
			FTrainerData |= (1 << x);
		}
	}
}

/**
 * Get the display name of the key or joystick button mapped to a specific 
 * NES gamepad button.
 * @param bc the NES gamepad's button config
 * @param which the index of the button
 */
const char *ButtonName(const ButtConfig *bc)
{
	static char name[256];

	name[0] = 0;

	if (bc->ButtonNum == -1)
	{
		return name;
	}
	switch (bc->ButtType)
	{
	case BUTTC_KEYBOARD:
		return SDL_GetKeyName(bc->ButtonNum);
		break;
	case BUTTC_JOYSTICK:
	{
		int joyNum, inputNum;
		const char *inputType, *inputDirection;
		char direction[128] = "";

		joyNum = bc->DeviceNum;

		if (bc->ButtonNum & 0x8000)
		{
			inputType = "Axis";
			inputNum = bc->ButtonNum & 0x3FFF;
			inputDirection = bc->ButtonNum & 0x4000 ? "-" : "+";
		}
		else if (bc->ButtonNum & 0x2000)
		{
			int inputValue;

			inputType = "Hat";
			inputNum = (bc->ButtonNum >> 8) & 0x1F;
			inputValue = bc->ButtonNum & 0xF;

			if (inputValue & SDL_HAT_UP)
				strncat(direction, "Up ", sizeof(direction) - 1);
			if (inputValue & SDL_HAT_DOWN)
				strncat(direction, "Down ", sizeof(direction) - 1);
			if (inputValue & SDL_HAT_LEFT)
				strncat(direction, "Left ", sizeof(direction) - 1);
			if (inputValue & SDL_HAT_RIGHT)
				strncat(direction, "Right ", sizeof(direction) - 1);

			if (direction[0])
				inputDirection = direction;
			else
				inputDirection = "Center";
		}
		else
		{
			inputType = "Button";
			inputNum = bc->ButtonNum;
			inputDirection = "";
		}
		sprintf(name, "js%i:%s%i%s", joyNum, inputType, inputNum, inputDirection);
	}
	break;
	}

	return name;
}

/**
 * Waits for a button input and returns the information as to which
 * button was pressed.  Used in button configuration.
 */
int DWaitButton(const uint8_t *text, ButtConfig *bc, int *buttonConfigStatus)
{
	SDL_Event event;
	static int32 LastAx[64][64];
	int x, y;
	int timeout_ms = 10000;

	if (text)
	{
		std::string title = "Press a key for ";
		title += (const char *)text;
		// TODO - SDL2
		//SDL_WM_SetCaption (title.c_str (), 0);
		puts((const char *)text);
	}

	for (x = 0; x < 64; x++)
	{
		for (y = 0; y < 64; y++)
		{
			LastAx[x][y] = 0x100000;
		}
	}

	// Purge all pending events, so that this next button press
	// will be the one we want.
	while (SDL_PollEvent(&event))
	{
	}

	while (1)
	{
		int done = 0;

		SDL_Delay(10);
		timeout_ms -= 10;

		if (timeout_ms <= 0)
		{
			break;
		}

		QCoreApplication::processEvents();

		while (SDL_PollEvent(&event))
		{
			done++;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				//printf("SDL KeyDown:%i \n", event.key.keysym.sym );
				bc->ButtType = BUTTC_KEYBOARD;
				bc->DeviceNum = 0;
				bc->ButtonNum = event.key.keysym.sym;
				return (1);
			case SDL_JOYBUTTONDOWN:
				bc->ButtType = BUTTC_JOYSTICK;
				bc->DeviceNum = FindJoystickByInstanceID(event.jbutton.which);
				bc->ButtonNum = event.jbutton.button;
				return (1);
			case SDL_JOYHATMOTION:
				if (event.jhat.value == SDL_HAT_CENTERED)
					done--;
				else
				{
					bc->ButtType = BUTTC_JOYSTICK;
					bc->DeviceNum = FindJoystickByInstanceID(event.jhat.which);
					bc->ButtonNum =
						(0x2000 | ((event.jhat.hat & 0x1F) << 8) | event.jhat.value);
					return (1);
				}
				break;
			case SDL_JOYAXISMOTION:
				if (LastAx[event.jaxis.which][event.jaxis.axis] == 0x100000)
				{
					if (abs(event.jaxis.value) < 1000)
					{
						LastAx[event.jaxis.which][event.jaxis.axis] =
							event.jaxis.value;
					}
					done--;
				}
				else
				{
					if (abs(LastAx[event.jaxis.which][event.jaxis.axis] -
							event.jaxis.value) >= 8192)
					{
						bc->ButtType = BUTTC_JOYSTICK;
						bc->DeviceNum = FindJoystickByInstanceID(event.jaxis.which);
						bc->ButtonNum = (0x8000 | event.jaxis.axis |
										 ((event.jaxis.value < 0)
											  ? 0x4000
											  : 0));
						return (1);
					}
					else
						done--;
				}
				break;
			default:
				done--;
			}
		}
		if (done)
			break;

		// If the button config window is Closed,
		// get out of loop.
		if (buttonConfigStatus != NULL)
		{
			if (*buttonConfigStatus == 0)
			{
				break;
			}
		}
	}

	return (0);
}

extern Config *g_config;

static const char *stdPortInputEncode(int v)
{
	const char *s;

	switch (v)
	{
	default:
	case SI_NONE:
		s = "SI_NONE";
		break;
	case SI_GAMEPAD:
		s = "SI_GAMEPAD";
		break;
	case SI_ZAPPER:
		s = "SI_ZAPPER";
		break;
	case SI_POWERPADA:
		s = "SI_POWERPADA";
		break;
	case SI_POWERPADB:
		s = "SI_POWERPADB";
		break;
	case SI_ARKANOID:
		s = "SI_ARKANOID";
		break;
	}
	return s;
}

static int stdPortInputDecode(const char *s)
{
	int ret = SI_NONE;

	if (s[0] == 0)
	{
		return ret;
	}

	if (isdigit(s[0]))
	{
		ret = atoi(s);
	}
	else
	{
		if (strcmp(s, "SI_GAMEPAD") == 0)
		{
			ret = SI_GAMEPAD;
		}
		else if (strcmp(s, "SI_ZAPPER") == 0)
		{
			ret = SI_ZAPPER;
		}
		else if (strcmp(s, "SI_POWERPADA") == 0)
		{
			ret = SI_POWERPADA;
		}
		else if (strcmp(s, "SI_POWERPADB") == 0)
		{
			ret = SI_POWERPADB;
		}
		else if (strcmp(s, "SI_ARKANOID") == 0)
		{
			ret = SI_ARKANOID;
		}
	}

	return ret;
}

static const char *expPortInputEncode(int v)
{
	const char *s;

	switch (v)
	{
	default:
	case SIFC_NONE:
		s = "SIFC_NONE";
		break;
	case SIFC_ARKANOID:
		s = "SIFC_ARKANOID";
		break;
	case SIFC_SHADOW:
		s = "SIFC_SHADOW";
		break;
	case SIFC_HYPERSHOT:
		s = "SIFC_HYPERSHOT";
		break;
	case SIFC_FKB:
		s = "SIFC_FKB";
		break;
	case SIFC_MAHJONG:
		s = "SIFC_MAHJONG";
		break;
	case SIFC_QUIZKING:
		s = "SIFC_QUIZKING";
		break;
	case SIFC_FTRAINERA:
		s = "SIFC_FTRAINERA";
		break;
	case SIFC_FTRAINERB:
		s = "SIFC_FTRAINERB";
		break;
	case SIFC_OEKAKIDS:
		s = "SIFC_OEKAKIDS";
		break;
	case SIFC_TOPRIDER:
		s = "SIFC_TOPRIDER";
		break;
	}
	return s;
}

static int expPortInputDecode(const char *s)
{
	int ret = SIFC_NONE;

	if (s[0] == 0)
	{
		return ret;
	}

	if (isdigit(s[0]))
	{
		ret = atoi(s);
	}
	else
	{
		if (strcmp(s, "SIFC_ARKANOID") == 0)
		{
			ret = SIFC_ARKANOID;
		}
		else if (strcmp(s, "SIFC_SHADOW") == 0)
		{
			ret = SIFC_SHADOW;
		}
		else if (strcmp(s, "SIFC_HYPERSHOT") == 0)
		{
			ret = SIFC_HYPERSHOT;
		}
		else if (strcmp(s, "SIFC_FKB") == 0)
		{
			ret = SIFC_FKB;
		}
		else if (strcmp(s, "SIFC_MAHJONG") == 0)
		{
			ret = SIFC_MAHJONG;
		}
		else if (strcmp(s, "SIFC_QUIZKING") == 0)
		{
			ret = SIFC_QUIZKING;
		}
		else if (strcmp(s, "SIFC_FTRAINERA") == 0)
		{
			ret = SIFC_FTRAINERA;
		}
		else if (strcmp(s, "SIFC_FTRAINERB") == 0)
		{
			ret = SIFC_FTRAINERB;
		}
		else if (strcmp(s, "SIFC_OEKAKIDS") == 0)
		{
			ret = SIFC_OEKAKIDS;
		}
		else if (strcmp(s, "SIFC_TOPRIDER") == 0)
		{
			ret = SIFC_TOPRIDER;
		}
	}

	return ret;
}

static bool boolDecode(const char *s)
{
	bool ret = false;

	if (isdigit(s[0]))
	{
		ret = atoi(s) != 0;
	}
	else if (strcasecmp(s, "true") == 0)
	{
		ret = true;
	}
	return ret;
}

int saveInputSettingsToFile(const char *filename)
{
	QDir dir;
	std::string path;
	const char *baseDir = FCEUI_GetBaseDirectory();
	char base[256];

	path = std::string(baseDir) + "/input/presets/";

	dir.mkpath(QString::fromStdString(path));

	if (filename != NULL)
	{
		getFileBaseName(filename, base, NULL);

		path += std::string(base) + ".pre";
	}
	else
	{
		const char *romFile = getRomFile();

		if (romFile == NULL)
		{
			return -1;
		}
		getFileBaseName(romFile, base, NULL);

		path += std::string(base) + ".pre";
	}

	FILE *fp = fopen(path.c_str(), "w");

	if (fp == NULL)
	{
		return -1;
	}
	fprintf(fp, "# Input Port Settings\n");
	fprintf(fp, "InputTypePort1   = %s \n", stdPortInputEncode(CurInputType[0]));
	fprintf(fp, "InputTypePort2   = %s \n", stdPortInputEncode(CurInputType[1]));
	fprintf(fp, "InputTypeExpPort = %s \n", expPortInputEncode(CurInputType[2]));
	fprintf(fp, "Enable4Score     = %i \n", (eoptions & EO_FOURSCORE) ? 1 : 0);
	fprintf(fp, "EnableMicPort2   = %i \n", replaceP2StartWithMicrophone);

	fclose(fp);

	return 0;
}

int loadInputSettingsFromFile(const char *filename)
{
	QDir dir;
	std::string path;
	const char *baseDir = FCEUI_GetBaseDirectory();
	char base[256], line[256];
	char id[128], val[128];
	int i, j;

	path = std::string(baseDir) + "/input/presets/";

	dir.mkpath(QString::fromStdString(path));

	if (filename != NULL)
	{
		getFileBaseName(filename, base, NULL);

		path += std::string(base) + ".pre";
	}
	else
	{
		const char *romFile = getRomFile();

		if (romFile == NULL)
		{
			return -1;
		}
		getFileBaseName(romFile, base, NULL);

		path += std::string(base) + ".pre";
	}

	FILE *fp = fopen(path.c_str(), "r");

	if (fp == NULL)
	{
		return -1;
	}

	while (fgets(line, sizeof(line) - 1, fp) != 0)
	{
		i = 0;
		while (line[i] != 0)
		{
			if (line[i] == '#')
			{
				line[i] = 0;
				break;
			}
			i++;
		}

		i = 0;
		while (isspace(line[i]))
			i++;

		j = 0;
		while (isalnum(line[i]) || (line[i] == '_'))
		{
			id[j] = line[i];
			i++;
			j++;
		}
		id[j] = 0;

		if (j == 0)
			continue;

		while (isspace(line[i]))
			i++;

		if (line[i] == '=')
			i++;

		while (isspace(line[i]))
			i++;

		j = 0;
		while (line[i] && !isspace(line[i]))
		{
			val[j] = line[i];
			i++;
			j++;
		}
		val[j] = 0;

		if (j == 0)
		{
			printf("Warning: No Value Specified for Token ID: '%s'\n", id);
			continue;
		}
		//printf("ID: '%s'   Val: '%s' \n", id, val );

		if (strcmp(id, "InputTypePort1") == 0)
		{
			CurInputType[0] = UsrInputType[0] = stdPortInputDecode(val);
		}
		else if (strcmp(id, "InputTypePort2") == 0)
		{
			CurInputType[1] = UsrInputType[1] = stdPortInputDecode(val);
		}
		else if (strcmp(id, "InputTypeExpPort") == 0)
		{
			CurInputType[2] = UsrInputType[2] = expPortInputDecode(val);
		}
		else if (strcmp(id, "Enable4Score") == 0)
		{
			if (boolDecode(val))
			{
				eoptions &= ~EO_FOURSCORE;
			}
			else
			{
				eoptions |= EO_FOURSCORE;
			}
		}
		else if (strcmp(id, "EnableMicPort2") == 0)
		{
			replaceP2StartWithMicrophone = boolDecode(val);
		}
	}

	fclose(fp);

	return 0;
}

/**
 * Hack to map the new configuration onto the existing button
 * configuration management.  Will probably want to change this in the
 * future - soules.
 */
void UpdateInput(Config *config)
{
	char buf[64];
	std::string device, prefix, guid, mapping;

	InitJoysticks();

	for (unsigned int i = 0; i < 3; i++)
	{
		snprintf(buf, 64, "SDL.Input.%u", i);
		config->getOption(buf, &device);

		if (device == "None")
		{
			UsrInputType[i] = SI_NONE;
		}
		else if (device.find("GamePad") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_GAMEPAD : (int)SIFC_NONE;
		}
		else if (device.find("PowerPad.0") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_POWERPADA : (int)SIFC_NONE;
		}
		else if (device.find("PowerPad.1") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_POWERPADB : (int)SIFC_NONE;
		}
		else if (device.find("QuizKing") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_QUIZKING;
		}
		else if (device.find("HyperShot") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_HYPERSHOT;
		}
		else if (device.find("Mahjong") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_MAHJONG;
		}
		else if (device.find("TopRider") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_TOPRIDER;
		}
		else if (device.find("FTrainer") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_FTRAINERA;
		}
		else if (device.find("FamilyKeyBoard") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_FKB;
		}
		else if (device.find("OekaKids") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_OEKAKIDS;
		}
		else if (device.find("Arkanoid") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_ARKANOID : (int)SIFC_ARKANOID;
		}
		else if (device.find("Shadow") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_SHADOW;
		}
		else if (device.find("Zapper") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_ZAPPER : (int)SIFC_NONE;
		}
		else if (device.find("BWorld") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_BWORLD;
		}
		else if (device.find("4Player") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_4PLAYER;
		}
		else
		{
			// Unknown device
			UsrInputType[i] = SI_NONE;
		}
	}

	// update each of the devices' configuration structure
	// XXX soules - this is temporary until this file is cleaned up to
	//              simplify the interface between configuration and
	//              structure data.  This will likely include the
	//              removal of multiple input buttons for a single
	//              input device key.
	int type, devnum, button;

	// gamepad 0 - 3
	for (unsigned int i = 0; i < GAMEPAD_NUM_DEVICES; i++)
	{
		char buf[64];
		snprintf(buf, sizeof(buf) - 1, "SDL.Input.GamePad.%u.", i);
		prefix = buf;

		config->getOption(prefix + "DeviceType", &device);
		config->getOption(prefix + "DeviceGUID", &guid);
		config->getOption(prefix + "Profile", &mapping);

		GamePad[i].init(i, guid.c_str(), mapping.c_str());
	}

	// PowerPad 0 - 1
	for (unsigned int i = 0; i < POWERPAD_NUM_DEVICES; i++)
	{
		char buf[64];
		snprintf(buf, sizeof(buf) - 1, "SDL.Input.PowerPad.%u.", i);
		prefix = buf;

		config->getOption(prefix + "DeviceType", &device);
		if (device.find("Keyboard") != std::string::npos)
		{
			type = BUTTC_KEYBOARD;
		}
		else if (device.find("Joystick") != std::string::npos)
		{
			type = BUTTC_JOYSTICK;
		}
		else
		{
			type = 0;
		}

		config->getOption(prefix + "DeviceNum", &devnum);
		for (unsigned int j = 0; j < POWERPAD_NUM_BUTTONS; j++)
		{
			config->getOption(prefix + PowerPadNames[j], &button);

			powerpadsc[i][j].ButtType = type;
			powerpadsc[i][j].DeviceNum = devnum;
			powerpadsc[i][j].ButtonNum = button;
		}
	}

	// QuizKing
	prefix = "SDL.Input.QuizKing.";
	config->getOption(prefix + "DeviceType", &device);
	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption(prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < QUIZKING_NUM_BUTTONS; j++)
	{
		config->getOption(prefix + QuizKingNames[j], &button);

		QuizKingButtons[j].ButtType = type;
		QuizKingButtons[j].DeviceNum = devnum;
		QuizKingButtons[j].ButtonNum = button;
	}

	// HyperShot
	prefix = "SDL.Input.HyperShot.";
	config->getOption(prefix + "DeviceType", &device);
	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption(prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < HYPERSHOT_NUM_BUTTONS; j++)
	{
		config->getOption(prefix + HyperShotNames[j], &button);

		HyperShotButtons[j].ButtType = type;
		HyperShotButtons[j].DeviceNum = devnum;
		HyperShotButtons[j].ButtonNum = button;
	}

	// Mahjong
	prefix = "SDL.Input.Mahjong.";
	config->getOption(prefix + "DeviceType", &device);
	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption(prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < MAHJONG_NUM_BUTTONS; j++)
	{
		config->getOption(prefix + MahjongNames[j], &button);

		MahjongButtons[j].ButtType = type;
		MahjongButtons[j].DeviceNum = devnum;
		MahjongButtons[j].ButtonNum = button;
	}

	// TopRider
	prefix = "SDL.Input.TopRider.";
	config->getOption(prefix + "DeviceType", &device);
	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption(prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < TOPRIDER_NUM_BUTTONS; j++)
	{
		config->getOption(prefix + TopRiderNames[j], &button);

		TopRiderButtons[j].ButtType = type;
		TopRiderButtons[j].DeviceNum = devnum;
		TopRiderButtons[j].ButtonNum = button;
	}

	// FTrainer
	prefix = "SDL.Input.FTrainer.";
	config->getOption(prefix + "DeviceType", &device);
	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption(prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < FTRAINER_NUM_BUTTONS; j++)
	{
		config->getOption(prefix + FTrainerNames[j], &button);

		FTrainerButtons[j].ButtType = type;
		FTrainerButtons[j].DeviceNum = devnum;
		FTrainerButtons[j].ButtonNum = button;
	}

	// FamilyKeyBoard
	prefix = "SDL.Input.FamilyKeyBoard.";
	config->getOption(prefix + "DeviceType", &device);
	if (device.find("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption(prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++)
	{
		config->getOption(prefix + FamilyKeyBoardNames[j], &button);

		fkbmap[j].ButtType = type;
		fkbmap[j].DeviceNum = devnum;
		fkbmap[j].ButtonNum = button;
	}
}

// Definitions from main.h:
// GamePad defaults
const char *GamePadNames[GAMEPAD_NUM_BUTTONS] = {"A", "B", "Select", "Start",
						 "Up", "Down", "Left", "Right", "TurboA", "TurboB"};
const char *DefaultGamePadDevice[GAMEPAD_NUM_DEVICES] =
	{"Keyboard", "None", "None", "None"};
const int DefaultGamePad[GAMEPAD_NUM_DEVICES][GAMEPAD_NUM_BUTTONS] =
	{{SDLK_f, SDLK_d, SDLK_s, SDLK_RETURN,
	  SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, -1, -1},
	 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

// PowerPad defaults
const char *PowerPadNames[POWERPAD_NUM_BUTTONS] =
	{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B"};
const char *DefaultPowerPadDevice[POWERPAD_NUM_DEVICES] =
	{"Keyboard", "None"};
const int DefaultPowerPad[POWERPAD_NUM_DEVICES][POWERPAD_NUM_BUTTONS] =
	{{SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
	  SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
	  SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH},
	 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

// QuizKing defaults
const char *QuizKingNames[QUIZKING_NUM_BUTTONS] =
	{"0", "1", "2", "3", "4", "5"};
const char *DefaultQuizKingDevice = "Keyboard";
const int DefaultQuizKing[QUIZKING_NUM_BUTTONS] =
	{SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y};

// HyperShot defaults
const char *HyperShotNames[HYPERSHOT_NUM_BUTTONS] = {"0", "1", "2", "3"};

const char *DefaultHyperShotDevice = "Keyboard";
const int DefaultHyperShot[HYPERSHOT_NUM_BUTTONS] =
	{SDLK_q, SDLK_w, SDLK_e, SDLK_r};

// Mahjong defaults
const char *MahjongNames[MAHJONG_NUM_BUTTONS] =
	{"00", "01", "02", "03", "04", "05", "06", "07",
	 "08", "09", "10", "11", "12", "13", "14", "15",
	 "16", "17", "18", "19", "20"};

const char *DefaultMahjongDevice = "Keyboard";
const int DefaultMahjong[MAHJONG_NUM_BUTTONS] =
	{SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_a, SDLK_s, SDLK_d,
	 SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_z, SDLK_x,
	 SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m};

// TopRider defaults
const char *TopRiderNames[TOPRIDER_NUM_BUTTONS] =
	{"0", "1", "2", "3", "4", "5", "6", "7"};
const char *DefaultTopRiderDevice = "Keyboard";
const int DefaultTopRider[TOPRIDER_NUM_BUTTONS] =
	{SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i};

// FTrainer defaults
const char *FTrainerNames[FTRAINER_NUM_BUTTONS] =
	{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B"};
const char *DefaultFTrainerDevice = "Keyboard";
const int DefaultFTrainer[FTRAINER_NUM_BUTTONS] =
	{SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
	 SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
	 SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH};

// FamilyKeyBoard defaults
const char *FamilyKeyBoardNames[FAMILYKEYBOARD_NUM_BUTTONS] =
	{"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
	 "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	 "MINUS", "EQUAL", "BACKSLASH", "BACKSPACE",
	 "ESCAPE", "Q", "W", "E", "R", "T", "Y", "U", "I", "O",
	 "P", "GRAVE", "BRACKET_LEFT", "ENTER",
	 "LEFTCONTROL", "A", "S", "D", "F", "G", "H", "J", "K",
	 "L", "SEMICOLON", "APOSTROPHE", "BRACKET_RIGHT", "INSERT",
	 "LEFTSHIFT", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
	 "PERIOD", "SLASH", "RIGHTALT", "RIGHTSHIFT", "LEFTALT", "SPACE",
	 "DELETE", "END", "PAGEDOWN",
	 "CURSORUP", "CURSORLEFT", "CURSORRIGHT", "CURSORDOWN"};

const char *DefaultFamilyKeyBoardDevice = "Keyboard";
const int DefaultFamilyKeyBoard[FAMILYKEYBOARD_NUM_BUTTONS] =
	{SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8,
	 SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
	 SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0,
	 SDLK_MINUS, SDLK_EQUALS, SDLK_BACKSLASH, SDLK_BACKSPACE,
	 SDLK_ESCAPE, SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u,
	 SDLK_i, SDLK_o, SDLK_p, SDLK_BACKQUOTE, SDLK_LEFTBRACKET, SDLK_RETURN,
	 SDLK_LCTRL, SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j,
	 SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_RIGHTBRACKET,
	 SDLK_INSERT, SDLK_LSHIFT, SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b,
	 SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RALT,
	 SDLK_RSHIFT, SDLK_LALT, SDLK_SPACE, SDLK_DELETE, SDLK_END, SDLK_PAGEDOWN,
	 SDLK_UP, SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN};
