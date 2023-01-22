/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
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
// keyscan.cpp
//

#include <QtCore>
#include <SDL.h>

#include "Qt/keyscan.h"

using namespace Qt;

/* scan code to virtual keys */
struct _KeyValue
{
  int vkey, key;
};

/* for readability */
typedef int NativeScanCode;

/* mapt to convert keyboard native scan code to qt keys */
static QMap<NativeScanCode, _KeyValue> s_nativeScanCodesMap =
#if defined(WIN32)
{
  {1, {27, 16777216}},
  {2, {49, 49}},
  {3, {50, 50}},
  {4, {51, 51}},
  {5, {52, 52}},
  {6, {53, 53}},
  {7, {54, 54}},
  {8, {55, 55}},
  {9, {56, 56}},
  {10, {57, 57}},
  {11, {48, 48}},
  {12, {189, 45}},
  {13, {187, 61}},
  {14, {8, 16777219}},
  {15, {9, 16777217}},
  {16, {81, 81}},
  {17, {87, 87}},
  {18, {69, 69}},
  {19, {82, 82}},
  {20, {84, 84}},
  {21, {89, 89}},
  {22, {85, 85}},
  {23, {73, 73}},
  {24, {79, 79}},
  {25, {80, 80}},
  {26, {219, 91}},
  {27, {221, 93}},
  {28, {13, 16777220}},
  {29, {17, 16777249}},
  {30, {65, 65}},
  {31, {83, 83}},
  {32, {68, 68}},
  {33, {70, 70}},
  {34, {71, 71}},
  {35, {72, 72}},
  {36, {74, 74}},
  {37, {75, 75}},
  {38, {76, 76}},
  {39, {186, 59}},
  {40, {222, 39}},
  {41, {192, 96}},
  {42, {16, 16777248}},
  {43, {220, 92}},
  {44, {90, 90}},
  {45, {88, 88}},
  {46, {67, 67}},
  {47, {86, 86}},
  {48, {66, 66}},
  {49, {78, 78}},
  {50, {77, 77}},
  {51, {188, 44}},
  {52, {190, 46}},
  {53, {191, 47}},
  {54, {16, 16777248}},
  {55, {106, 42}},
  {56, {18, 16777251}},
  {58, {20, 16777252}},
  {59, {112, 16777264}},
  {60, {113, 16777265}},
  {61, {114, 16777266}},
  {62, {115, 16777267}},
  {63, {116, 16777268}},
  {64, {117, 16777269}},
  {65, {118, 16777270}},
  {66, {119, 16777271}},
  {67, {120, 16777272}},
  {68, {121, 16777273}},
  {69, {19, 16777224}},
  {70, {145, 16777254}},
  {71, {36, 16777232}},
  {74, {109, 45}},
  {76, {12, 16777227}},
  {78, {107, 43}},
  {79, {35, 16777233}},
  {82, {45, 16777222}},
  {83, {46, 16777223}},
  {87, {122, 16777274}},
  {88, {123, 16777275}},
  {256, {176, 16777347}},
  {284, {13, 16777221}},
  {285, {17, 16777249}},
  {309, {111, 47}},
  {312, {18, 16777251}},
  {325, {144, 16777253}},
  {338, {45, 16777222}},
  {339, {46, 16777223}},
  {347, {91, 16777250}},
  {348, {92, 16777250}},
  {349, {93, 16777301}},
};
#else
{
  {10, {49, 49}},
  {11, {50, 50}},
  {12, {51, 51}},
  {13, {52, 52}},
  {14, {53, 53}},
  {15, {54, 54}},
  {16, {55, 55}},
  {17, {56, 56}},
  {18, {57, 57}},
  {19, {48, 48}},
  {20, {45, 45}},
  {21, {61, 61}},
  {22, {65288, 16777219}},
  {23, {65289, 16777217}},
  {24, {113, 81}},
  {25, {119, 87}},
  {26, {101, 69}},
  {27, {114, 82}},
  {28, {116, 84}},
  {29, {121, 89}},
  {30, {117, 85}},
  {31, {105, 73}},
  {32, {111, 79}},
  {33, {112, 80}},
  {34, {91, 91}},
  {35, {93, 93}},
  {36, {65293, 16777220}},
  {37, {65507, 16777249}},
  {38, {97, 65}},
  {39, {115, 83}},
  {40, {68, 68}},
  {41, {70, 70}},
  {42, {71, 71}},
  {43, {72, 72}},
  {44, {74, 74}},
  {45, {107, 75}},
  {46, {108, 76}},
  {47, {59, 59}},
  {48, {39, 39}},
  {49, {96, 96}},
  {50, {65505, 16777248}},
  {51, {92, 92}},
  {52, {90, 90}},
  {53, {88, 88}},
  {54, {67, 67}},
  {55, {118, 86}},
  {56, {98, 66}},
  {57, {78, 78}},
  {58, {77, 77}},
  {59, {44, 44}},
  {60, {46, 46}},
  {61, {47, 47}},
  {62, {65506, 16777248}},
  {63, {65450, 42}},
  {64, {65513, 16777251}},
  {66, {65509, 16777252}},
  {67, {65470, 16777264}},
  {68, {65471, 16777265}},
  {69, {65472, 16777266}},
  {70, {65473, 16777267}},
  {71, {65474, 16777268}},
  {72, {65475, 16777269}},
  {73, {65476, 16777270}},
  {74, {65477, 16777271}},
  {75, {65478, 16777272}},
  {76, {65479, 16777273}},
  {77, {65407, 16777253}},
  {78, {65300, 16777254}},
  {79, {65463, 55}},
  {80, {65464, 56}},
  {81, {65465, 57}},
  {82, {65453, 45}},
  {83, {65460, 52}},
  {84, {65461, 53}},
  {85, {65462, 54}},
  {86, {65451, 43}},
  {87, {65457, 49}},
  {88, {65458, 50}},
  {89, {65459, 51}},
  {90, {65456, 48}},
  {91, {65454, 46}},
  {95, {65480, 16777274}},
  {96, {65481, 16777275}},
  {104, {65421, 16777221}},
  {105, {65508, 16777249}},
  {106, {65455, 47}},
  {108, {65027, 16781571}},
  {118, {65379, 16777222}},
  {119, {65535, 16777223}},
  {127, {65299, 16777224}},
};
#endif

#if defined(WIN32)

#include <windows.h>
#include <winuser.h>
static uint32_t ShiftKeyCodeR = VK_RSHIFT;
static uint32_t CtrlKeyCodeR  = VK_RCONTROL;
static uint32_t AltKeyCodeR   = VK_RMENU;
static uint32_t MetaKeyCodeR  = VK_RWIN;
static BYTE keyBuf[256];

#elif  defined(__linux__)

   #if  defined(_HAS_XKB)
	#include <xkbcommon/xkbcommon.h>
	static uint32_t ShiftKeyCodeR = XKB_KEY_Shift_R;
	static uint32_t CtrlKeyCodeR  = XKB_KEY_Control_R;
	static uint32_t AltKeyCodeR   = XKB_KEY_Alt_R;
	static uint32_t MetaKeyCodeR  = XKB_KEY_Meta_R;
   #elif  defined(_HAS_X11)
        #include <X11/keysym.h>
	static uint32_t ShiftKeyCodeR = XK_Shift_R;
	static uint32_t CtrlKeyCodeR  = XK_Control_R;
	static uint32_t AltKeyCodeR   = XK_Alt_R;
	static uint32_t MetaKeyCodeR  = XK_Meta_R;
   #else
	static uint32_t ShiftKeyCodeR = 0xffe2;
	static uint32_t CtrlKeyCodeR  = 0xffe4;
	static uint32_t AltKeyCodeR   = 0xffea;
	static uint32_t MetaKeyCodeR  = 0xffe8;
   #endif

#elif  defined(__APPLE__)
static uint32_t ShiftKeyCodeR = 0x003C;
static uint32_t CtrlKeyCodeR  = 0x003E;
static uint32_t AltKeyCodeR   = 0x003D;
static uint32_t MetaKeyCodeR  = 0x0036;

#else
static uint32_t ShiftKeyCodeR = 0xffe2;
static uint32_t CtrlKeyCodeR  = 0xffe4;
static uint32_t AltKeyCodeR   = 0xffea;
static uint32_t MetaKeyCodeR  = 0xffe8;
#endif

SDL_Scancode convQtKey2SDLScanCode(Qt::Key q, uint32_t nativeVirtualKey)
{
	SDL_Scancode s = SDL_SCANCODE_UNKNOWN;

	switch (q)
	{
	case Key_Escape:
		s = SDL_SCANCODE_ESCAPE;
		break;
	case Key_Tab:
	case Key_Backtab:
		s = SDL_SCANCODE_TAB;
		break;
	case Key_Backspace:
		s = SDL_SCANCODE_BACKSPACE;
		break;
	case Key_Return:
		s = SDL_SCANCODE_RETURN;
		break;
	case Key_Enter:
		s = SDL_SCANCODE_RETURN;
		break;
	case Key_Insert:
		s = SDL_SCANCODE_INSERT;
		break;
	case Key_Delete:
		s = SDL_SCANCODE_DELETE;
		break;
	case Key_Pause:
		s = SDL_SCANCODE_PAUSE;
		break;
	case Key_Print:
	case Key_SysReq:
		s = SDL_SCANCODE_PRINTSCREEN;
		break;
	case Key_Clear:
		s = SDL_SCANCODE_CLEAR;
		break;
	case Key_Home:
		s = SDL_SCANCODE_HOME;
		break;
	case Key_End:
		s = SDL_SCANCODE_END;
		break;
	case Key_Left:
		s = SDL_SCANCODE_LEFT;
		break;
	case Key_Up:
		s = SDL_SCANCODE_UP;
		break;
	case Key_Right:
		s = SDL_SCANCODE_RIGHT;
		break;
	case Key_Down:
		s = SDL_SCANCODE_DOWN;
		break;
	case Key_PageUp:
		s = SDL_SCANCODE_PAGEUP;
		break;
	case Key_PageDown:
		s = SDL_SCANCODE_PAGEDOWN;
		break;
	case Key_Shift:
		#if defined(WIN32)
		if ( keyBuf[ShiftKeyCodeR] & 0x80 )
		{
			s = SDL_SCANCODE_RSHIFT;
		}
		else
		{
			s = SDL_SCANCODE_LSHIFT;
		}
		#else
		if ( nativeVirtualKey == ShiftKeyCodeR )
		{
			s = SDL_SCANCODE_RSHIFT;
		}
		else
		{
			s = SDL_SCANCODE_LSHIFT;
		}
		#endif
		break;
	case Key_Control:
		#ifdef  __APPLE__
		if ( nativeVirtualKey == MetaKeyCodeR )
		{
			s = SDL_SCANCODE_RGUI;
		}
		else
		{
			s = SDL_SCANCODE_LGUI;
		}
		#elif defined(WIN32)
		if ( keyBuf[CtrlKeyCodeR] & 0x80 )
		{
			s = SDL_SCANCODE_RCTRL;
		}
		else
		{
			s = SDL_SCANCODE_LCTRL;
		}
		#else
		if ( nativeVirtualKey == CtrlKeyCodeR )
		{
			s = SDL_SCANCODE_RCTRL;
		}
		else
		{
			s = SDL_SCANCODE_LCTRL;
		}
		#endif
		break;
	case Key_Meta:
		#ifdef  __APPLE__
		if ( nativeVirtualKey == CtrlKeyCodeR )
		{
			s = SDL_SCANCODE_RCTRL;
		}
		else
		{
			s = SDL_SCANCODE_LCTRL;
		}
		#elif defined(WIN32)
		if ( keyBuf[MetaKeyCodeR] & 0x80 )
		{
			s = SDL_SCANCODE_RGUI;
		}
		else
		{
			s = SDL_SCANCODE_LGUI;
		}
		#else
		if ( nativeVirtualKey == MetaKeyCodeR )
		{
			s = SDL_SCANCODE_RGUI;
		}
		else
		{
			s = SDL_SCANCODE_LGUI;
		}
		#endif
		break;
	case Key_Alt:
		#if defined(WIN32)
		if ( keyBuf[AltKeyCodeR] & 0x80 )
		{
			s = SDL_SCANCODE_RALT;
		}
		else
		{
			s = SDL_SCANCODE_LALT;
		}
		#else
		if ( nativeVirtualKey == AltKeyCodeR )
		{
			s = SDL_SCANCODE_RALT;
		}
		else
		{
			s = SDL_SCANCODE_LALT;
		}
		#endif
		break;
	case Key_CapsLock:
		s = SDL_SCANCODE_CAPSLOCK;
		break;
	case Key_NumLock:
		s = SDL_SCANCODE_NUMLOCKCLEAR;
		break;
	case Key_ScrollLock:
		s = SDL_SCANCODE_SCROLLLOCK;
		break;
	case Key_F1:
		s = SDL_SCANCODE_F1;
		break;
	case Key_F2:
		s = SDL_SCANCODE_F2;
		break;
	case Key_F3:
		s = SDL_SCANCODE_F3;
		break;
	case Key_F4:
		s = SDL_SCANCODE_F4;
		break;
	case Key_F5:
		s = SDL_SCANCODE_F5;
		break;
	case Key_F6:
		s = SDL_SCANCODE_F6;
		break;
	case Key_F7:
		s = SDL_SCANCODE_F7;
		break;
	case Key_F8:
		s = SDL_SCANCODE_F8;
		break;
	case Key_F9:
		s = SDL_SCANCODE_F9;
		break;
	case Key_F10:
		s = SDL_SCANCODE_F10;
		break;
	case Key_F11:
		s = SDL_SCANCODE_F11;
		break;
	case Key_F12:
		s = SDL_SCANCODE_F12;
		break;
	case Key_F13:
		s = SDL_SCANCODE_F13;
		break;
	case Key_F14:
		s = SDL_SCANCODE_F14;
		break;
	case Key_F15:
		s = SDL_SCANCODE_F15;
		break;
	case Key_F16:
		s = SDL_SCANCODE_F16;
		break;
	case Key_F17:
		s = SDL_SCANCODE_F17;
		break;
	case Key_F18:
		s = SDL_SCANCODE_F18;
		break;
	case Key_F19:
		s = SDL_SCANCODE_F19;
		break;
	case Key_F20:
		s = SDL_SCANCODE_F20;
		break;
	case Key_F21:
		s = SDL_SCANCODE_F21;
		break;
	case Key_F22:
		s = SDL_SCANCODE_F22;
		break;
	case Key_F23:
		s = SDL_SCANCODE_F23;
		break;
	case Key_F24:
		s = SDL_SCANCODE_F24;
		break;
		//case  Key_F25:                // F25 .. F35 only on X11
		//case  Key_F26:
		//case  Key_F27:
		//case  Key_F28:
		//case  Key_F29:
		//case  Key_F30:
		//case  Key_F31:
		//case  Key_F32:
		//case  Key_F33:
		//case  Key_F34:
		//case  Key_F35:
		//	s = SDL_SCANCODE_UNKNOWN;
		//break;
	case Key_Super_L:
	case Key_Super_R:
		s = SDL_SCANCODE_UNKNOWN;
		break;
	case Key_Menu:
		s = SDL_SCANCODE_MENU;
		break;
	case Key_Hyper_L:
	case Key_Hyper_R:
		s = SDL_SCANCODE_UNKNOWN;
		break;
	case Key_Help:
		s = SDL_SCANCODE_HELP;
		break;
	case Key_Direction_L:
	case Key_Direction_R:
		s = SDL_SCANCODE_UNKNOWN;
		break;

	case Key_Space:
		//case  Key_Any:
		s = SDL_SCANCODE_SPACE;
		break;

	case Key_Exclam:
		s = SDL_SCANCODE_1;
		break;
	case Key_QuoteDbl:
		s = SDL_SCANCODE_APOSTROPHE;
		break;
	case Key_NumberSign:
		s = SDL_SCANCODE_3;
		break;
	case Key_Dollar:
		s = SDL_SCANCODE_4;
		break;
	case Key_Percent:
		s = SDL_SCANCODE_5;
		break;
	case Key_Ampersand:
		s = SDL_SCANCODE_7;
		break;
	case Key_Apostrophe:
		s = SDL_SCANCODE_APOSTROPHE;
		break;
	case Key_ParenLeft:
		s = SDL_SCANCODE_9;
		break;
	case Key_ParenRight:
		s = SDL_SCANCODE_0;
		break;
	case Key_Asterisk:
		s = SDL_SCANCODE_8;
		break;
	case Key_Plus:
		s = SDL_SCANCODE_EQUALS;
		break;
	case Key_Comma:
		s = SDL_SCANCODE_COMMA;
		break;
	case Key_Minus:
		s = SDL_SCANCODE_MINUS;
		break;
	case Key_Period:
		s = SDL_SCANCODE_PERIOD;
		break;
	case Key_Slash:
		s = SDL_SCANCODE_SLASH;
		break;
	case Key_0:
		s = SDL_SCANCODE_0;
		break;
	case Key_1:
		s = SDL_SCANCODE_1;
		break;
	case Key_2:
		s = SDL_SCANCODE_2;
		break;
	case Key_3:
		s = SDL_SCANCODE_3;
		break;
	case Key_4:
		s = SDL_SCANCODE_4;
		break;
	case Key_5:
		s = SDL_SCANCODE_5;
		break;
	case Key_6:
		s = SDL_SCANCODE_6;
		break;
	case Key_7:
		s = SDL_SCANCODE_7;
		break;
	case Key_8:
		s = SDL_SCANCODE_8;
		break;
	case Key_9:
		s = SDL_SCANCODE_9;
		break;
	case Key_Colon:
	case Key_Semicolon:
		s = SDL_SCANCODE_SEMICOLON;
		break;
	case Key_Less:
		s = SDL_SCANCODE_COMMA;
		break;
	case Key_Equal:
		s = SDL_SCANCODE_EQUALS;
		break;
	case Key_Greater:
		s = SDL_SCANCODE_PERIOD;
		break;
	case Key_Question:
		s = SDL_SCANCODE_SLASH;
		break;
	case Key_At:
		s = SDL_SCANCODE_2;
		break;
	case Key_A:
		s = SDL_SCANCODE_A;
		break;
	case Key_B:
		s = SDL_SCANCODE_B;
		break;
	case Key_C:
		s = SDL_SCANCODE_C;
		break;
	case Key_D:
		s = SDL_SCANCODE_D;
		break;
	case Key_E:
		s = SDL_SCANCODE_E;
		break;
	case Key_F:
		s = SDL_SCANCODE_F;
		break;
	case Key_G:
		s = SDL_SCANCODE_G;
		break;
	case Key_H:
		s = SDL_SCANCODE_H;
		break;
	case Key_I:
		s = SDL_SCANCODE_I;
		break;
	case Key_J:
		s = SDL_SCANCODE_J;
		break;
	case Key_K:
		s = SDL_SCANCODE_K;
		break;
	case Key_L:
		s = SDL_SCANCODE_L;
		break;
	case Key_M:
		s = SDL_SCANCODE_M;
		break;
	case Key_N:
		s = SDL_SCANCODE_N;
		break;
	case Key_O:
		s = SDL_SCANCODE_O;
		break;
	case Key_P:
		s = SDL_SCANCODE_P;
		break;
	case Key_Q:
		s = SDL_SCANCODE_Q;
		break;
	case Key_R:
		s = SDL_SCANCODE_R;
		break;
	case Key_S:
		s = SDL_SCANCODE_S;
		break;
	case Key_T:
		s = SDL_SCANCODE_T;
		break;
	case Key_U:
		s = SDL_SCANCODE_U;
		break;
	case Key_V:
		s = SDL_SCANCODE_V;
		break;
	case Key_W:
		s = SDL_SCANCODE_W;
		break;
	case Key_X:
		s = SDL_SCANCODE_X;
		break;
	case Key_Y:
		s = SDL_SCANCODE_Y;
		break;
	case Key_Z:
		s = SDL_SCANCODE_Z;
		break;
	case Key_BracketLeft:
		s = SDL_SCANCODE_LEFTBRACKET;
		break;
	case Key_Backslash:
		s = SDL_SCANCODE_BACKSLASH;
		break;
	case Key_BracketRight:
		s = SDL_SCANCODE_RIGHTBRACKET;
		break;
	case  Key_AsciiCircum:
		s = SDL_SCANCODE_6;
		break;
	case Key_Underscore:
		s = SDL_SCANCODE_MINUS;
		break;
	case Key_QuoteLeft:
		s = SDL_SCANCODE_GRAVE;
		break;
	case Key_BraceLeft:
		s = SDL_SCANCODE_LEFTBRACKET;
		break;
	case Key_Bar:
		s = SDL_SCANCODE_BACKSLASH;
		break;
	case Key_BraceRight:
		s = SDL_SCANCODE_RIGHTBRACKET;
		break;
	case Key_AsciiTilde:
		s = SDL_SCANCODE_GRAVE;
		break;

	//case  Key_nobreakspace:
	//case  Key_exclamdown:
	//case  Key_cent:
	//case  Key_sterling:
	//case  Key_currency:
	//case  Key_yen:
	//case  Key_brokenbar:
	//case  Key_section:
	//case  Key_diaeresis:
	//case  Key_copyright:
	//case  Key_ordfeminine:
	//case  Key_guillemotleft:
	//case  Key_notsign:
	//case  Key_hyphen:
	//case  Key_registered:
	//case  Key_macron:
	//case  Key_degree:
	//case  Key_plusminus:
	//case  Key_twosuperior:
	//case  Key_threesuperior:
	//case  Key_acute:
	//case  Key_mu:
	//case  Key_paragraph:
	//case  Key_periodcentered:
	//case  Key_cedilla:
	//case  Key_onesuperior:
	//case  Key_masculine:
	//case  Key_guillemotright:
	//case  Key_onequarter:
	//case  Key_onehalf:
	//case  Key_threequarters:
	//case  Key_questiondown:
	//case  Key_Agrave:
	//case  Key_Aacute:
	//case  Key_Acircumflex:
	//case  Key_Atilde:
	//case  Key_Adiaeresis:
	//case  Key_Aring:
	//case  Key_AE:
	//case  Key_Ccedilla:
	//case  Key_Egrave:
	//case  Key_Eacute:
	//case  Key_Ecircumflex:
	//case  Key_Ediaeresis:
	//case  Key_Igrave:
	//case  Key_Iacute:
	//case  Key_Icircumflex:
	//case  Key_Idiaeresis:
	//case  Key_ETH:
	//case  Key_Ntilde:
	//case  Key_Ograve:
	//case  Key_Oacute:
	//case  Key_Ocircumflex:
	//case  Key_Otilde:
	//case  Key_Odiaeresis:
	//case  Key_multiply:
	//case  Key_Ooblique:
	//case  Key_Ugrave:
	//case  Key_Uacute:
	//case  Key_Ucircumflex:
	//case  Key_Udiaeresis:
	//case  Key_Yacute:
	//case  Key_THORN:
	//case  Key_ssharp:
	//case  Key_division:
	//case  Key_ydiaeresis:
	//	s = SDL_SCANCODE_UNKNOWN;
	//break;
	default:
		s = SDL_SCANCODE_UNKNOWN;
		break;
	}
	return s;
}

SDL_Keycode convQtKey2SDLKeyCode(Qt::Key q, uint32_t nativeVirtualKey)
{
	SDL_Keycode s = SDLK_UNKNOWN;

	switch (q)
	{
	case Key_Escape:
		s = SDLK_ESCAPE;
		break;
	case Key_Tab:
	case Key_Backtab:
		s = SDLK_TAB;
		break;
	case Key_Backspace:
		s = SDLK_BACKSPACE;
		break;
	case Key_Return:
		s = SDLK_RETURN;
		break;
	case Key_Enter:
		s = SDLK_KP_ENTER;
		break;
	case Key_Insert:
		s = SDLK_INSERT;
		break;
	case Key_Delete:
		s = SDLK_DELETE;
		break;
	case Key_Pause:
		s = SDLK_PAUSE;
		break;
	case Key_Print:
		s = SDLK_PRINTSCREEN;
		break;
	case Key_SysReq:
		s = SDLK_SYSREQ;
		break;
	case Key_Clear:
		s = SDLK_CLEAR;
		break;
	case Key_Home:
		s = SDLK_HOME;
		break;
	case Key_End:
		s = SDLK_END;
		break;
	case Key_Left:
		s = SDLK_LEFT;
		break;
	case Key_Up:
		s = SDLK_UP;
		break;
	case Key_Right:
		s = SDLK_RIGHT;
		break;
	case Key_Down:
		s = SDLK_DOWN;
		break;
	case Key_PageUp:
		s = SDLK_PAGEUP;
		break;
	case Key_PageDown:
		s = SDLK_PAGEDOWN;
		break;
	case Key_Shift:
		#if defined(WIN32)
		if ( keyBuf[ShiftKeyCodeR] & 0x80)
		{
			s = SDLK_RSHIFT;
		}
		else
		{
			s = SDLK_LSHIFT;
		}
		#elif defined(WIN32)
		if ( keyBuf[ShiftKeyCodeR] & 0x80 )
		{
			s = SDLK_RSHIFT;
		}
		else
		{
			s = SDLK_LSHIFT;
		}
		#else
		if ( nativeVirtualKey == ShiftKeyCodeR )
		{
			s = SDLK_RSHIFT;
		}
		else
		{
			s = SDLK_LSHIFT;
		}
		#endif
		break;
	case Key_Control:
		#ifdef  __APPLE__
		if ( nativeVirtualKey == MetaKeyCodeR )
		{
			s = SDLK_RGUI;
		}
		else
		{
			s = SDLK_LGUI;
		}
		#elif defined(WIN32)
		if ( keyBuf[CtrlKeyCodeR] & 0x80 )
		{
			s = SDLK_RCTRL;
		}
		else
		{
			s = SDLK_LCTRL;
		}
		#else
		if ( nativeVirtualKey == CtrlKeyCodeR )
		{
			s = SDLK_RCTRL;
		}
		else
		{
			s = SDLK_LCTRL;
		}
		#endif
		break;
	case Key_Meta:
		#ifdef  __APPLE__
		if ( nativeVirtualKey == CtrlKeyCodeR )
		{
			s = SDLK_RCTRL;
		}
		else
		{
			s = SDLK_LCTRL;
		}
		#elif defined(WIN32)
		if ( keyBuf[MetaKeyCodeR] & 0x80 )
		{
			s = SDLK_RGUI;
		}
		else
		{
			s = SDLK_LGUI;
		}
		#else
		if ( nativeVirtualKey == MetaKeyCodeR )
		{
			s = SDLK_RGUI;
		}
		else
		{
			s = SDLK_LGUI;
		}
		#endif
		break;
	case Key_Alt:
		#if defined(WIN32)
		if ( keyBuf[AltKeyCodeR] & 0x80 )
		{
			s = SDLK_RALT;
		}
		else
		{
			s = SDLK_LALT;
		}
		#else
		if ( nativeVirtualKey == AltKeyCodeR )
		{
			s = SDLK_RALT;
		}
		else
		{
			s = SDLK_LALT;
		}
		#endif
		break;
	case Key_CapsLock:
		s = SDLK_CAPSLOCK;
		break;
	case Key_NumLock:
		s = SDLK_NUMLOCKCLEAR;
		break;
	case Key_ScrollLock:
		s = SDLK_SCROLLLOCK;
		break;
	case Key_F1:
		s = SDLK_F1;
		break;
	case Key_F2:
		s = SDLK_F2;
		break;
	case Key_F3:
		s = SDLK_F3;
		break;
	case Key_F4:
		s = SDLK_F4;
		break;
	case Key_F5:
		s = SDLK_F5;
		break;
	case Key_F6:
		s = SDLK_F6;
		break;
	case Key_F7:
		s = SDLK_F7;
		break;
	case Key_F8:
		s = SDLK_F8;
		break;
	case Key_F9:
		s = SDLK_F9;
		break;
	case Key_F10:
		s = SDLK_F10;
		break;
	case Key_F11:
		s = SDLK_F11;
		break;
	case Key_F12:
		s = SDLK_F12;
		break;
	case Key_F13:
		s = SDLK_F13;
		break;
	case Key_F14:
		s = SDLK_F14;
		break;
	case Key_F15:
		s = SDLK_F15;
		break;
	case Key_F16:
		s = SDLK_F16;
		break;
	case Key_F17:
		s = SDLK_F17;
		break;
	case Key_F18:
		s = SDLK_F18;
		break;
	case Key_F19:
		s = SDLK_F19;
		break;
	case Key_F20:
		s = SDLK_F20;
		break;
	case Key_F21:
		s = SDLK_F21;
		break;
	case Key_F22:
		s = SDLK_F22;
		break;
	case Key_F23:
		s = SDLK_F23;
		break;
	case Key_F24:
		s = SDLK_F24;
		break;
		//case  Key_F25:                // F25 .. F35 only on X11
		//case  Key_F26:
		//case  Key_F27:
		//case  Key_F28:
		//case  Key_F29:
		//case  Key_F30:
		//case  Key_F31:
		//case  Key_F32:
		//case  Key_F33:
		//case  Key_F34:
		//case  Key_F35:
		//	s = SDL_SCANCODE_UNKNOWN;
		//break;
	case Key_Super_L:
	case Key_Super_R:
		s = SDLK_UNKNOWN;
		break;
	case Key_Menu:
		s = SDLK_MENU;
		break;
	case Key_Hyper_L:
	case Key_Hyper_R:
		s = SDLK_UNKNOWN;
		break;
	case Key_Help:
		s = SDLK_HELP;
		break;
	case Key_Direction_L:
	case Key_Direction_R:
		s = SDLK_UNKNOWN;
		break;

	case Key_Space:
		//case  Key_Any:
		s = SDLK_SPACE;
		break;

	case Key_Exclam:
		s = SDLK_EXCLAIM;
		break;
	case Key_QuoteDbl:
		s = SDLK_QUOTEDBL;
		break;
	case Key_NumberSign:
		s = SDLK_HASH;
		break;
	case Key_Dollar:
		s = SDLK_DOLLAR;
		break;
	case Key_Percent:
		s = SDLK_PERCENT;
		break;
	case Key_Ampersand:
		s = SDLK_AMPERSAND;
		break;
	case Key_Apostrophe:
		s = SDLK_QUOTE;
		break;
	case Key_ParenLeft:
		s = SDLK_LEFTPAREN;
		break;
	case Key_ParenRight:
		s = SDLK_RIGHTPAREN;
		break;
	case Key_Asterisk:
		s = SDLK_ASTERISK;
		break;
	case Key_Plus:
		s = SDLK_PLUS;
		break;
	case Key_Comma:
		s = SDLK_COMMA;
		break;
	case Key_Minus:
		s = SDLK_MINUS;
		break;
	case Key_Period:
		s = SDLK_PERIOD;
		break;
	case Key_Slash:
		s = SDLK_SLASH;
		break;
	case Key_0:
		s = SDLK_0;
		break;
	case Key_1:
		s = SDLK_1;
		break;
	case Key_2:
		s = SDLK_2;
		break;
	case Key_3:
		s = SDLK_3;
		break;
	case Key_4:
		s = SDLK_4;
		break;
	case Key_5:
		s = SDLK_5;
		break;
	case Key_6:
		s = SDLK_6;
		break;
	case Key_7:
		s = SDLK_7;
		break;
	case Key_8:
		s = SDLK_8;
		break;
	case Key_9:
		s = SDLK_9;
		break;
	case Key_Colon:
		s = SDLK_COLON;
		break;
	case Key_Semicolon:
		s = SDLK_SEMICOLON;
		break;
	case Key_Less:
		s = SDLK_LESS;
		break;
	case Key_Equal:
		s = SDLK_EQUALS;
		break;
	case Key_Greater:
		s = SDLK_GREATER;
		break;
	case Key_Question:
		s = SDLK_QUESTION;
		break;
	case Key_At:
		s = SDLK_AT;
		break;
	case Key_A:
		s = SDLK_a;
		break;
	case Key_B:
		s = SDLK_b;
		break;
	case Key_C:
		s = SDLK_c;
		break;
	case Key_D:
		s = SDLK_d;
		break;
	case Key_E:
		s = SDLK_e;
		break;
	case Key_F:
		s = SDLK_f;
		break;
	case Key_G:
		s = SDLK_g;
		break;
	case Key_H:
		s = SDLK_h;
		break;
	case Key_I:
		s = SDLK_i;
		break;
	case Key_J:
		s = SDLK_j;
		break;
	case Key_K:
		s = SDLK_k;
		break;
	case Key_L:
		s = SDLK_l;
		break;
	case Key_M:
		s = SDLK_m;
		break;
	case Key_N:
		s = SDLK_n;
		break;
	case Key_O:
		s = SDLK_o;
		break;
	case Key_P:
		s = SDLK_p;
		break;
	case Key_Q:
		s = SDLK_q;
		break;
	case Key_R:
		s = SDLK_r;
		break;
	case Key_S:
		s = SDLK_s;
		break;
	case Key_T:
		s = SDLK_t;
		break;
	case Key_U:
		s = SDLK_u;
		break;
	case Key_V:
		s = SDLK_v;
		break;
	case Key_W:
		s = SDLK_w;
		break;
	case Key_X:
		s = SDLK_x;
		break;
	case Key_Y:
		s = SDLK_y;
		break;
	case Key_Z:
		s = SDLK_z;
		break;
	case Key_BracketLeft:
		s = SDLK_LEFTBRACKET;
		break;
	case Key_Backslash:
		s = SDLK_BACKSLASH;
		break;
	case Key_BracketRight:
		s = SDLK_RIGHTBRACKET;
		break;
	case  Key_AsciiCircum:
		s = SDLK_CARET;
	break;
	case Key_Underscore:
		s = SDLK_UNDERSCORE;
		break;
	case Key_QuoteLeft:
		s = SDLK_BACKQUOTE;
		break;
	case Key_BraceLeft:
		s = SDLK_LEFTBRACKET;
		break;
	case Key_Bar:
		s = SDLK_BACKSLASH;
		break;
	case Key_BraceRight:
		s = SDLK_RIGHTBRACKET;
		break;
	case Key_AsciiTilde:
		s = SDLK_BACKQUOTE;
		break;

	//case  Key_nobreakspace:
	//case  Key_exclamdown:
	//case  Key_cent:
	//case  Key_sterling:
	//case  Key_currency:
	//case  Key_yen:
	//case  Key_brokenbar:
	//case  Key_section:
	//case  Key_diaeresis:
	//case  Key_copyright:
	//case  Key_ordfeminine:
	//case  Key_guillemotleft:
	//case  Key_notsign:
	//case  Key_hyphen:
	//case  Key_registered:
	//case  Key_macron:
	//case  Key_degree:
	//case  Key_plusminus:
	//case  Key_twosuperior:
	//case  Key_threesuperior:
	//case  Key_acute:
	//case  Key_mu:
	//case  Key_paragraph:
	//case  Key_periodcentered:
	//case  Key_cedilla:
	//case  Key_onesuperior:
	//case  Key_masculine:
	//case  Key_guillemotright:
	//case  Key_onequarter:
	//case  Key_onehalf:
	//case  Key_threequarters:
	//case  Key_questiondown:
	//case  Key_Agrave:
	//case  Key_Aacute:
	//case  Key_Acircumflex:
	//case  Key_Atilde:
	//case  Key_Adiaeresis:
	//case  Key_Aring:
	//case  Key_AE:
	//case  Key_Ccedilla:
	//case  Key_Egrave:
	//case  Key_Eacute:
	//case  Key_Ecircumflex:
	//case  Key_Ediaeresis:
	//case  Key_Igrave:
	//case  Key_Iacute:
	//case  Key_Icircumflex:
	//case  Key_Idiaeresis:
	//case  Key_ETH:
	//case  Key_Ntilde:
	//case  Key_Ograve:
	//case  Key_Oacute:
	//case  Key_Ocircumflex:
	//case  Key_Otilde:
	//case  Key_Odiaeresis:
	//case  Key_multiply:
	//case  Key_Ooblique:
	//case  Key_Ugrave:
	//case  Key_Uacute:
	//case  Key_Ucircumflex:
	//case  Key_Udiaeresis:
	//case  Key_Yacute:
	//case  Key_THORN:
	//case  Key_ssharp:
	//case  Key_division:
	//case  Key_ydiaeresis:
	//	s = SDL_SCANCODE_UNKNOWN;
	//break;
	default:
		s = SDLK_UNKNOWN;
		break;
	}
	return s;
}

SDL_Keymod convQtKey2SDLModifier(Qt::KeyboardModifiers m)
{
	int s = 0;

	if (m != Qt::NoModifier)
	{
		if (m & Qt::ShiftModifier)
		{
			s |= (KMOD_LSHIFT | KMOD_RSHIFT);
		}
		if (m & Qt::AltModifier)
		{
			s |= (KMOD_LALT | KMOD_RALT);
		}
		if (m & Qt::ControlModifier)
		{
			s |= (KMOD_LCTRL | KMOD_RCTRL);
		}
	}
	return (SDL_Keymod)s;
}

int convKeyEvent2Sequence( QKeyEvent *event )
{
	int k, m;

	k = event->key();
	m = event->modifiers();

	//printf("Key: %x  Modifier: %x \n", k, m );

	switch ( k )
	{
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
		case Qt::Key_Space:
		case Qt::Key_Exclam:
		case Qt::Key_QuoteDbl:
		case Qt::Key_NumberSign:
		case Qt::Key_Dollar:
		case Qt::Key_Percent:
		case Qt::Key_Ampersand:
		case Qt::Key_Apostrophe:
		case Qt::Key_ParenLeft:
		case Qt::Key_ParenRight:
		case Qt::Key_Asterisk:
		case Qt::Key_Plus:
		case Qt::Key_Comma:
		case Qt::Key_Minus:
		case Qt::Key_Period:
		case Qt::Key_Slash:
		case Qt::Key_0:
		case Qt::Key_1:
		case Qt::Key_2:
		case Qt::Key_3:
		case Qt::Key_4:
		case Qt::Key_5:
		case Qt::Key_6:
		case Qt::Key_7:
		case Qt::Key_8:
		case Qt::Key_9:
		case Qt::Key_Colon:
		case Qt::Key_Semicolon:
		case Qt::Key_Less:
		case Qt::Key_Equal:
		case Qt::Key_Greater:
		case Qt::Key_Question:
		case Qt::Key_At:
		case Qt::Key_BracketLeft:
		case Qt::Key_Backslash:
		case Qt::Key_BracketRight:
		case Qt::Key_AsciiCircum:
		case Qt::Key_Underscore:
		case Qt::Key_QuoteLeft:
		case Qt::Key_BraceLeft:
		case Qt::Key_Bar:
		case Qt::Key_BraceRight:
		case Qt::Key_AsciiTilde:
			// With these keys, the shift modifier can be implied and 
			// should not be included in the output. Doing so may create
			// an impossible double shifted key sequence.
			m &= ~Qt::ShiftModifier;
		break;
		default:
			// Leave Modifier Unchanged
		break;
	}
	return (m | k);
}

#ifdef WIN32
uint8_t win32GetKeyState( unsigned int vkey )
{
	uint8_t state = 0;

	switch ( vkey )
	{
		case SDL_SCANCODE_LSHIFT:
			state = (keyBuf[VK_LSHIFT] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_RSHIFT:
			state = (keyBuf[VK_RSHIFT] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_LALT:
			state = (keyBuf[VK_LMENU] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_RALT:
			state = (keyBuf[VK_RMENU] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_LCTRL:
			state = (keyBuf[VK_LCONTROL] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_RCTRL:
			state = (keyBuf[VK_RCONTROL] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_LGUI:
			state = (keyBuf[VK_LWIN] & 0x80) ? 1 : 0;
		break;
		case SDL_SCANCODE_RGUI:
			state = (keyBuf[VK_RWIN] & 0x80) ? 1 : 0;
		break;
		default:
			state = 0;
		break;
	}
	return state;
}
#endif

int pushKeyEvent(QKeyEvent *event, int pressDown)
{
	SDL_Event sdlev;
	uint32_t vkey;

	if (pressDown)
	{
		sdlev.type = SDL_KEYDOWN;
		sdlev.key.state = SDL_PRESSED;
	}
	else
	{
		sdlev.type = SDL_KEYUP;
		sdlev.key.state = SDL_RELEASED;
	}

#ifdef WIN32
	GetKeyboardState( keyBuf );
#endif

	vkey = event->nativeVirtualKey();

//	auto nsc  = event->nativeScanCode();
//	qDebug() << __PRETTY_FUNCTION__ << nsc << vkey << event->key();

	sdlev.key.keysym.sym = convQtKey2SDLKeyCode((Qt::Key)event->key(), vkey);

	sdlev.key.keysym.scancode = SDL_GetScancodeFromKey(sdlev.key.keysym.sym);

	//printf("Key %s: x%08X  %i\n", (sdlev.type == SDL_KEYUP) ? "UP" : "DOWN", event->key(), event->key() );
	//printf("   Native ScanCode: x%08X  %i \n", event->nativeScanCode(), event->nativeScanCode() );
	//printf("   Virtual ScanCode: x%08X  %i \n", event->nativeVirtualKey(), event->nativeVirtualKey() );
	//printf("   Scancode: 0x%08X  %i \n", sdlev.key.keysym.scancode, sdlev.key.keysym.scancode );

	// SDL Docs say this code should never happen, but it does... 
	// so force it to alternative scancode algorithm if it occurs.
	if ( (sdlev.key.keysym.scancode == SDL_SCANCODE_NONUSHASH      ) ||
	     (sdlev.key.keysym.scancode == SDL_SCANCODE_NONUSBACKSLASH ) )
	{
		sdlev.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
	}

	if ( sdlev.key.keysym.scancode == SDL_SCANCODE_UNKNOWN )
	{	// If scancode is unknown, the key may be dual function via the shift key.

		sdlev.key.keysym.scancode = convQtKey2SDLScanCode( (Qt::Key)event->key(), vkey );

		//printf("Dual Scancode: 0x%08X  %i \n", sdlev.key.keysym.scancode, sdlev.key.keysym.scancode );
	}

	sdlev.key.keysym.mod = convQtKey2SDLModifier(event->modifiers());
	sdlev.key.repeat = 0;

	//printf("Modifiers: %08X -> %08X \n", event->modifiers(), sdlev.key.keysym.mod );

	/* when we're unable to convert qt keys to sdl, we do keyboard native scan code convertation */
	if (sdlev.key.keysym.scancode == SDL_SCANCODE_UNKNOWN)
	  {
	    int nativeKey = event->nativeScanCode();
	    auto value	  = s_nativeScanCodesMap.value (nativeKey, _KeyValue {0, 0});
	    if (value.key != 0 && value.vkey != 0)
	      {
		sdlev.key.keysym.sym	  = convQtKey2SDLKeyCode    ((Qt::Key)value.key, value.vkey);
		sdlev.key.keysym.scancode = SDL_GetScancodeFromKey  (sdlev.key.keysym.sym);
	      }
	  }

	if (sdlev.key.keysym.scancode != SDL_SCANCODE_UNKNOWN)
	{
		SDL_PushEvent(&sdlev);
	}

	return 0;
}
