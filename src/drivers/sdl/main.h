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
#ifndef __FCEU_SDL_MAIN_H
#define __FCEU_SDL_MAIN_H

#include <SDL.h>

#include "../../driver.h"
#include "../common/config.h"
#include "../common/args.h"

extern int eoptions;
#define EO_NO8LIM      1
#define EO_SUBASE      2
#define EO_CLIPSIDES   8
#define EO_SNAPNAME    16
#define EO_NOFOURSCORE	32
#define EO_NOTHROTTLE	64
#define EO_GAMEGENIE	128
#define EO_PAL		256
#define EO_LOWPASS	512
#define EO_AUTOHIDE	1024

extern int NoWaiting;

extern int _sound;
extern long soundrate;
extern long soundbufsize;

int CLImain(int argc, char *argv[]);

// Device management defaults
#define NUM_INPUT_DEVICES 3

// GamePad defaults
#define GAMEPAD_NUM_DEVICES 4
#define GAMEPAD_NUM_BUTTONS 10
static char *GamePadNames[GAMEPAD_NUM_BUTTONS] =
    {"A", "B", "Select", "Start",
     "Up", "Down", "Left", "Right", "TurboA", "TurboB"};
static char *DefaultGamePadDevice[GAMEPAD_NUM_DEVICES] =
    {"Keyboard", "None", "None", "None"};
static int DefaultGamePad[GAMEPAD_NUM_DEVICES][GAMEPAD_NUM_BUTTONS] =
    { { SDLK_KP2, SDLK_KP3, SDLK_TAB, SDLK_RETURN,
        SDLK_w, SDLK_s, SDLK_a, SDLK_d, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

// PowerPad defaults
#define POWERPAD_NUM_DEVICES 2
#define POWERPAD_NUM_BUTTONS 12
static char *PowerPadNames[POWERPAD_NUM_BUTTONS] =
    {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B"};
static char *DefaultPowerPadDevice[POWERPAD_NUM_DEVICES] =
    {"Keyboard", "None"};
static int DefaultPowerPad[POWERPAD_NUM_DEVICES][POWERPAD_NUM_BUTTONS] =
    { { SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
        SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
        SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

// QuizKing defaults
#define QUIZKING_NUM_BUTTONS 6
static char *QuizKingNames[QUIZKING_NUM_BUTTONS] =
    { "0", "1", "2", "3", "4", "5" };
static char *DefaultQuizKingDevice = "Keyboard";
static int DefaultQuizKing[QUIZKING_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y };

// HyperShot defaults
#define HYPERSHOT_NUM_BUTTONS 4
static char *HyperShotNames[HYPERSHOT_NUM_BUTTONS] =
    { "0", "1", "2", "3" };
static char *DefaultHyperShotDevice = "Keyboard";
static int DefaultHyperShot[HYPERSHOT_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r };

// Mahjong defaults
#define MAHJONG_NUM_BUTTONS 21
static char *MahjongNames[MAHJONG_NUM_BUTTONS] =
    { "00", "01", "02", "03", "04", "05", "06", "07",
      "08", "09", "10", "11", "12", "13", "14", "15",
      "16", "17", "18", "19", "20" };
static char *DefaultMahjongDevice = "Keyboard";
static int DefaultMahjong[MAHJONG_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_a, SDLK_s, SDLK_d,
      SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_z, SDLK_x,
      SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m };

// TopRider defaults
#define TOPRIDER_NUM_BUTTONS 8
static char *TopRiderNames[TOPRIDER_NUM_BUTTONS] =
    { "0", "1", "2", "3", "4", "5", "6", "7" };
static char *DefaultTopRiderDevice = "Keyboard";
static int DefaultTopRider[TOPRIDER_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i };

// FTrainer defaults
#define FTRAINER_NUM_BUTTONS 12
static char *FTrainerNames[FTRAINER_NUM_BUTTONS] =
    { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B" };
static char *DefaultFTrainerDevice = "Keyboard";
static int DefaultFTrainer[FTRAINER_NUM_BUTTONS] =
    { SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
      SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
      SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH };

// FamilyKeyBoard defaults
#define FAMILYKEYBOARD_NUM_BUTTONS 0x48
static char *FamilyKeyBoardNames[FAMILYKEYBOARD_NUM_BUTTONS] =
    { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
      "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
      "MINUS", "EQUAL", "BACKSLASH", "BACKSPACE",
      "ESCAPE", "Q", "W", "E", "R", "T", "Y", "U", "I", "O",
      "P", "GRAVE", "BRACKET_LEFT", "ENTER",
      "LEFTCONTROL", "A", "S", "D", "F", "G", "H", "J", "K",
      "L", "SEMICOLON", "APOSTROPHE", "BRACKET_RIGHT", "INSERT",
      "LEFTSHIFT", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
      "PERIOD", "SLASH", "RIGHTALT", "RIGHTSHIFT", "LEFTALT", "SPACE",
      "DELETE", "END", "PAGEDOWN",
      "CURSORUP", "CURSORLEFT", "CURSORRIGHT", "CURSORDOWN" };
static char *DefaultFamilyKeyBoardDevice = "Keyboard";
static int DefaultFamilyKeyBoard[FAMILYKEYBOARD_NUM_BUTTONS] =
    { SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8,
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
      SDLK_UP, SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN };

#endif
