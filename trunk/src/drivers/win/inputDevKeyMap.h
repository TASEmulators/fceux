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

#ifndef INPUTDEVKEYMAP_H
#define INPUTDEVKEYMAP_H

#define GPAD_NOCFG() {{{BtnConfig::BT_UNSET},{0},{0},0}, {{BtnConfig::BT_UNSET},{0},{0},0}, {{BtnConfig::BT_UNSET},{0},{0},0}, {{BtnConfig::BT_UNSET},{0},{0},0}}
#define MK(key) {{BtnConfig::BT_KEYBOARD},{0},{SCANCODE(key)},1} // make config from key id

#define MC(scancode) {{BtnConfig::BT_KEYBOARD},{0},{scancode},1} // make config from scan code
	// FIXME encourages use of hardcoded values, remove;
	// if the key has no ID in keyscan.h we should not use it

static BtnConfig GamePadConfig[GPAD_COUNT][GPAD_NUMKEYS] = {
	//Gamepad 1
	{
		MK(F), // A
		MK(D), // B
		MK(S), // select
		MK(ENTER), // start
		MK(BL_CURSORUP), MK(BL_CURSORDOWN),MK(BL_CURSORLEFT),MK(BL_CURSORRIGHT)
	},

	//Gamepad 2
	GPAD_NOCFG(),

	//Gamepad 3
	GPAD_NOCFG(),

	//Gamepad 4
	GPAD_NOCFG()
};

static BtnConfig PowerPadConfig[2][NUMKEYS_POWERPAD]={
	{
		MK(O),MK(P),MK(BRACKET_LEFT),MK(BRACKET_RIGHT),
		MK(K),MK(L),MK(SEMICOLON),MK(APOSTROPHE),
		MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
	},
	{
		MK(O),MK(P),MK(BRACKET_LEFT),MK(BRACKET_RIGHT),
		MK(K),MK(L),MK(SEMICOLON),MK(APOSTROPHE),
		MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
	}
};

static BtnConfig FamiKeyBoardConfig[NUMKEYS_FAMIKB]=
{
	MK(F1),MK(F2),MK(F3),MK(F4),MK(F5),MK(F6),MK(F7),MK(F8),
	MK(1),MK(2),MK(3),MK(4),MK(5),MK(6),MK(7),MK(8),MK(9),MK(0),
	MK(MINUS),MK(EQUAL),MK(BACKSLASH),MK(BACKSPACE),
	MK(ESCAPE),MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I),MK(O),
	MK(P),MK(TILDE),MK(BRACKET_LEFT),MK(ENTER),
	MK(LEFTCONTROL),MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),
	MK(L),MK(SEMICOLON),MK(APOSTROPHE),MK(BRACKET_RIGHT),MK(INSERT),
	MK(LEFTSHIFT),MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M),MK(COMMA),
	MK(PERIOD),MK(SLASH),MK(RIGHTALT),MK(RIGHTSHIFT),MK(LEFTALT),MK(SPACE),
	MK(BL_DELETE),
	MK(BL_END),
	MK(BL_PAGEDOWN),
	MK(BL_CURSORUP),MK(BL_CURSORLEFT),MK(BL_CURSORRIGHT),MK(BL_CURSORDOWN)
};

// FIXME uses codes not listed in our keyscan.h
static BtnConfig SuborKeyBoardConfig[NUMKEYS_SUBORKB]=
{
	MK(ESCAPE),MK(F1),MK(F2),MK(F3),MK(F4),MK(F5),MK(F6),MK(F7),MK(F8),MK(F9),MK(F10),MK(F11),MK(F12),MK(NUMLOCK),
	MK(TILDE),MK(1),MK(2),MK(3),MK(4),MK(5),MK(6),MK(7),MK(8),MK(9),MK(0),MK(MINUS),MK(EQUAL),MK(BACKSPACE),MC(0xd2/*INS*/),MC(0xc7/*HOME*/),MC(0xc9/*NUM9*/),MK(PAUSE),
	MC(0xb5/*NUM/*/),MK(ASTERISK),MK(KP_MINUS),
	MK(TAB),MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I),MK(O),MK(P),MK(BRACKET_LEFT),MK(BRACKET_RIGHT),MK(ENTER),MC(0xd3/*NUM.*/),MC(0xca/*???*/),MC(0xd1/*NUM3*/),MK(HOME),MK(CURSORUP),MK(PAGEUP),MK(KP_PLUS),
	MK(CAPSLOCK),MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),MK(L),MK(SEMICOLON),MK(APOSTROPHE),MK(CURSORLEFT),MK(CENTER),MK(CURSORRIGHT),
	MK(LEFTSHIFT),MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M),MK(COMMA),MK(PERIOD),MK(SLASH),MK(BACKSLASH),MC(0xc8/*NUM8*/),MK(END),MK(CURSORDOWN),MK(PAGEDOWN),
	MK(LEFTCONTROL),MK(LEFTALT),MK(SPACE),MC(0xcb/*NUM4*/),MC(0xd0/*NUM2*/),MC(0xcd/*NUM6*/),MK(INSERT),MK(KP_DELETE),MC(0x00),MC(0x00),MC(0x00),MC(0x00),MC(0x00),MC(0x00)
};

static BtnConfig MahjongConfig[NUMKEYS_MAHJONG]=
{
	MK(Q),MK(W),MK(E),MK(R),MK(T),
	MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),MK(L),
	MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M)
};

static BtnConfig PartyTapConfig[NUMKEYS_PARTYTAP]=
{
	MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y)
};

static BtnConfig TopRiderConfig[NUMKEYS_TOPRIDER]=
{
	MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I)
};

static BtnConfig FamiTrainerConfig[NUMKEYS_FAMITRAINER]=
{
	MK(O),MK(P),MK(BRACKET_LEFT),
	MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
	MK(APOSTROPHE),
	MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
};

static BtnConfig HyperShotConfig[NUMKEYS_HYPERSHOT]=
{
	MK(Q),MK(W),MK(E),MK(R)
};

#endif // INPUTDEVKEYMAP_H
