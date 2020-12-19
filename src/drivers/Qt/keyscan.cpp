// keyscan.cpp
//

#include <QtCore>
#include <SDL.h>

#include "Qt/keyscan.h"

using namespace Qt;

SDL_Scancode convQtKey2SDLScanCode( Qt::Key q )
{
	SDL_Scancode s = SDL_SCANCODE_UNKNOWN;

	switch (q)
	{
		case  Key_Escape:
			s = SDL_SCANCODE_ESCAPE;
		break;
		case  Key_Tab:
		case  Key_Backtab:
			s = SDL_SCANCODE_TAB;
		break;
		case  Key_Backspace:
			s = SDL_SCANCODE_BACKSPACE;
		break;
		case  Key_Return:
			s = SDL_SCANCODE_RETURN;
		break;
		case  Key_Enter:
			s = SDL_SCANCODE_RETURN;
		break;
		case  Key_Insert:
			s = SDL_SCANCODE_INSERT;
		break;
		case  Key_Delete:
			s = SDL_SCANCODE_DELETE;
		break;
		case  Key_Pause:
			s = SDL_SCANCODE_PAUSE;
		break;
		case  Key_Print:
		case  Key_SysReq:
			s = SDL_SCANCODE_PRINTSCREEN;
		break;
		case  Key_Clear:
			s = SDL_SCANCODE_CLEAR;
		break;
		case  Key_Home:
			s = SDL_SCANCODE_HOME;
		break;
		case  Key_End:
			s = SDL_SCANCODE_END;
		break;
		case  Key_Left:
			s = SDL_SCANCODE_LEFT;
		break;
		case  Key_Up:
			s = SDL_SCANCODE_UP;
		break;
		case  Key_Right:
			s = SDL_SCANCODE_RIGHT;
		break;
		case  Key_Down:
			s = SDL_SCANCODE_DOWN;
		break;
		case  Key_PageUp:
			s = SDL_SCANCODE_PAGEUP;
		break;
		case  Key_PageDown:
			s = SDL_SCANCODE_PAGEDOWN;
		break;
		case  Key_Shift:
			s = SDL_SCANCODE_LSHIFT;
		break;
		case  Key_Control:
			s = SDL_SCANCODE_LCTRL;
		break;
		case  Key_Meta:
			s = SDL_SCANCODE_LGUI;
		break;
		case  Key_Alt:
			s = SDL_SCANCODE_LALT;
		break;
		case  Key_CapsLock:
			s = SDL_SCANCODE_CAPSLOCK;
		break;
		case  Key_NumLock:
			s = SDL_SCANCODE_NUMLOCKCLEAR;
		break;
		case  Key_ScrollLock:
			s = SDL_SCANCODE_SCROLLLOCK;
		break;
		case  Key_F1:
			s = SDL_SCANCODE_F1;
		break;
      case  Key_F2:
			s = SDL_SCANCODE_F2;
		break;
      case  Key_F3:
			s = SDL_SCANCODE_F3;
		break;
      case  Key_F4:
			s = SDL_SCANCODE_F4;
		break;
      case  Key_F5:
			s = SDL_SCANCODE_F5;
		break;
      case  Key_F6:
			s = SDL_SCANCODE_F6;
		break;
      case  Key_F7:
			s = SDL_SCANCODE_F7;
		break;
      case  Key_F8:
			s = SDL_SCANCODE_F8;
		break;
      case  Key_F9:
			s = SDL_SCANCODE_F9;
		break;
		case  Key_F10:
			s = SDL_SCANCODE_F10;
		break;
      case  Key_F11:
			s = SDL_SCANCODE_F11;
		break;
      case  Key_F12:
			s = SDL_SCANCODE_F12;
		break;
      case  Key_F13:
			s = SDL_SCANCODE_F13;
		break;
      case  Key_F14:
			s = SDL_SCANCODE_F14;
		break;
      case  Key_F15:
			s = SDL_SCANCODE_F15;
		break;
      case  Key_F16:
			s = SDL_SCANCODE_F16;
		break;
      case  Key_F17:
			s = SDL_SCANCODE_F17;
		break;
      case  Key_F18:
			s = SDL_SCANCODE_F18;
		break;
      case  Key_F19:
			s = SDL_SCANCODE_F19;
		break;
      case  Key_F20:
			s = SDL_SCANCODE_F20;
		break;
      case  Key_F21:
			s = SDL_SCANCODE_F21;
		break;
      case  Key_F22:
			s = SDL_SCANCODE_F22;
		break;
      case  Key_F23:
			s = SDL_SCANCODE_F23;
		break;
      case  Key_F24:
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
		case  Key_Super_L:
		case  Key_Super_R:
			s = SDL_SCANCODE_UNKNOWN;
		break;
		case  Key_Menu:
			s = SDL_SCANCODE_MENU;
		break;
		case  Key_Hyper_L:
		case  Key_Hyper_R:
			s = SDL_SCANCODE_UNKNOWN;
		break;
		case  Key_Help:
			s = SDL_SCANCODE_HELP;
		break;
		case  Key_Direction_L:
		case  Key_Direction_R:
			s = SDL_SCANCODE_UNKNOWN;
		break;

		case  Key_Space:
		//case  Key_Any:
			s = SDL_SCANCODE_SPACE;
		break;

		case  Key_Exclam:
			s = SDL_SCANCODE_1;
		break;
		case  Key_QuoteDbl:
			s = SDL_SCANCODE_APOSTROPHE;
		break;
		case  Key_NumberSign:
			s = SDL_SCANCODE_3;
		break;
		case  Key_Dollar:
			s = SDL_SCANCODE_4;
		break;
		case  Key_Percent:
			s = SDL_SCANCODE_5;
		break;
		case  Key_Ampersand:
			s = SDL_SCANCODE_7;
		break;
		case  Key_Apostrophe:
			s = SDL_SCANCODE_APOSTROPHE;
		break;
		case  Key_ParenLeft:
			s = SDL_SCANCODE_9;
		break;
		case  Key_ParenRight:
			s = SDL_SCANCODE_0;
		break;
		case  Key_Asterisk:
			s = SDL_SCANCODE_8;
		break;
		case  Key_Plus:
			s = SDL_SCANCODE_EQUALS;
		break;
		case  Key_Comma:
			s = SDL_SCANCODE_COMMA;
		break;
		case  Key_Minus:
			s = SDL_SCANCODE_MINUS;
		break;
		case  Key_Period:
			s = SDL_SCANCODE_PERIOD;
		break;
		case  Key_Slash:
			s = SDL_SCANCODE_SLASH;
		break;
		case  Key_0:
			s = SDL_SCANCODE_0;
		break;
      case  Key_1:
			s = SDL_SCANCODE_1;
		break;
      case  Key_2:
			s = SDL_SCANCODE_2;
		break;
      case  Key_3:
			s = SDL_SCANCODE_3;
		break;
      case  Key_4:
			s = SDL_SCANCODE_4;
		break;
      case  Key_5:
			s = SDL_SCANCODE_5;
		break;
      case  Key_6:
			s = SDL_SCANCODE_6;
		break;
      case  Key_7:
			s = SDL_SCANCODE_7;
		break;
      case  Key_8:
			s = SDL_SCANCODE_8;
		break;
      case  Key_9:
			s = SDL_SCANCODE_9;
		break;
		case  Key_Colon:
		case  Key_Semicolon:
			s = SDL_SCANCODE_SEMICOLON;
		break;
		case  Key_Less:
			s = SDL_SCANCODE_COMMA;
		break;
		case  Key_Equal:
			s = SDL_SCANCODE_EQUALS;
		break;
		case  Key_Greater:
			s = SDL_SCANCODE_PERIOD;
		break;
		case  Key_Question:
			s = SDL_SCANCODE_SLASH;
		break;
		case  Key_At:
			s = SDL_SCANCODE_2;
		break;
		break;
		case  Key_A:
			s = SDL_SCANCODE_A;
		break;
      case  Key_B:
			s = SDL_SCANCODE_B;
		break;
      case  Key_C:
			s = SDL_SCANCODE_C;
		break;
      case  Key_D:
			s = SDL_SCANCODE_D;
		break;
      case  Key_E:
			s = SDL_SCANCODE_E;
		break;
      case  Key_F:
			s = SDL_SCANCODE_F;
		break;
      case  Key_G:
			s = SDL_SCANCODE_G;
		break;
      case  Key_H:
			s = SDL_SCANCODE_H;
		break;
      case  Key_I:
			s = SDL_SCANCODE_I;
		break;
      case  Key_J:
			s = SDL_SCANCODE_J;
		break;
      case  Key_K:
			s = SDL_SCANCODE_K;
		break;
      case  Key_L:
			s = SDL_SCANCODE_L;
		break;
      case  Key_M:
			s = SDL_SCANCODE_M;
		break;
      case  Key_N:
			s = SDL_SCANCODE_N;
		break;
      case  Key_O:
			s = SDL_SCANCODE_O;
		break;
      case  Key_P:
			s = SDL_SCANCODE_P;
		break;
      case  Key_Q:
			s = SDL_SCANCODE_Q;
		break;
      case  Key_R:
			s = SDL_SCANCODE_R;
		break;
      case  Key_S:
			s = SDL_SCANCODE_S;
		break;
      case  Key_T:
			s = SDL_SCANCODE_T;
		break;
      case  Key_U:
			s = SDL_SCANCODE_U;
		break;
      case  Key_V:
			s = SDL_SCANCODE_V;
		break;
      case  Key_W:
			s = SDL_SCANCODE_W;
		break;
      case  Key_X:
			s = SDL_SCANCODE_X;
		break;
      case  Key_Y:
			s = SDL_SCANCODE_Y;
		break;
      case  Key_Z:
			s = SDL_SCANCODE_Z;
		break;
		case  Key_BracketLeft:
			s = SDL_SCANCODE_LEFTBRACKET;
		break;
		case  Key_Backslash:
			s = SDL_SCANCODE_BACKSLASH;
		break;
		case  Key_BracketRight:
			s = SDL_SCANCODE_RIGHTBRACKET;
		break;
		//case  Key_AsciiCircum:
		//	s = SDL_SCANCODE_UNKNOWN;
		//break;
		case  Key_Underscore:
			s = SDL_SCANCODE_MINUS;
		break;
		case  Key_QuoteLeft:
			s = SDL_SCANCODE_GRAVE;
		break;
		case  Key_BraceLeft:
			s = SDL_SCANCODE_LEFTBRACKET;
		break;
		case  Key_Bar:
			s = SDL_SCANCODE_BACKSLASH;
		break;
		case  Key_BraceRight:
			s = SDL_SCANCODE_RIGHTBRACKET;
		break;
		case  Key_AsciiTilde:
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

SDL_Keycode convQtKey2SDLKeyCode( Qt::Key q )
{
	SDL_Keycode s = SDLK_UNKNOWN;

	switch (q)
	{
		case  Key_Escape:
			s = SDLK_ESCAPE;
		break;
		case  Key_Tab:
		case  Key_Backtab:
			s = SDLK_TAB;
		break;
		case  Key_Backspace:
			s = SDLK_BACKSPACE;
		break;
		case  Key_Return:
			s = SDLK_RETURN;
		break;
		case  Key_Enter:
			s = SDLK_KP_ENTER;
		break;
		case  Key_Insert:
			s = SDLK_INSERT;
		break;
		case  Key_Delete:
			s = SDLK_DELETE;
		break;
		case  Key_Pause:
			s = SDLK_PAUSE;
		break;
		case  Key_Print:
			s = SDLK_PRINTSCREEN;
		break;
		case  Key_SysReq:
			s = SDLK_SYSREQ;
		break;
		case  Key_Clear:
			s = SDLK_CLEAR;
		break;
		case  Key_Home:
			s = SDLK_HOME;
		break;
		case  Key_End:
			s = SDLK_END;
		break;
		case  Key_Left:
			s = SDLK_LEFT;
		break;
		case  Key_Up:
			s = SDLK_UP;
		break;
		case  Key_Right:
			s = SDLK_RIGHT;
		break;
		case  Key_Down:
			s = SDLK_DOWN;
		break;
		case  Key_PageUp:
			s = SDLK_PAGEUP;
		break;
		case  Key_PageDown:
			s = SDLK_PAGEDOWN;
		break;
		case  Key_Shift:
			s = SDLK_LSHIFT;
		break;
		case  Key_Control:
			s = SDLK_LCTRL;
		break;
		case  Key_Meta:
			s = SDLK_LGUI;
		break;
		case  Key_Alt:
			s = SDLK_LALT;
		break;
		case  Key_CapsLock:
			s = SDLK_CAPSLOCK;
		break;
		case  Key_NumLock:
			s = SDLK_NUMLOCKCLEAR;
		break;
		case  Key_ScrollLock:
			s = SDLK_SCROLLLOCK;
		break;
		case  Key_F1:
			s = SDLK_F1;
		break;
      case  Key_F2:
			s = SDLK_F2;
		break;
      case  Key_F3:
			s = SDLK_F3;
		break;
      case  Key_F4:
			s = SDLK_F4;
		break;
      case  Key_F5:
			s = SDLK_F5;
		break;
      case  Key_F6:
			s = SDLK_F6;
		break;
      case  Key_F7:
			s = SDLK_F7;
		break;
      case  Key_F8:
			s = SDLK_F8;
		break;
      case  Key_F9:
			s = SDLK_F9;
		break;
		case  Key_F10:
			s = SDLK_F10;
		break;
      case  Key_F11:
			s = SDLK_F11;
		break;
      case  Key_F12:
			s = SDLK_F12;
		break;
      case  Key_F13:
			s = SDLK_F13;
		break;
      case  Key_F14:
			s = SDLK_F14;
		break;
      case  Key_F15:
			s = SDLK_F15;
		break;
      case  Key_F16:
			s = SDLK_F16;
		break;
      case  Key_F17:
			s = SDLK_F17;
		break;
      case  Key_F18:
			s = SDLK_F18;
		break;
      case  Key_F19:
			s = SDLK_F19;
		break;
      case  Key_F20:
			s = SDLK_F20;
		break;
      case  Key_F21:
			s = SDLK_F21;
		break;
      case  Key_F22:
			s = SDLK_F22;
		break;
      case  Key_F23:
			s = SDLK_F23;
		break;
      case  Key_F24:
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
		case  Key_Super_L:
		case  Key_Super_R:
			s = SDLK_UNKNOWN;
		break;
		case  Key_Menu:
			s = SDLK_MENU;
		break;
		case  Key_Hyper_L:
		case  Key_Hyper_R:
			s = SDLK_UNKNOWN;
		break;
		case  Key_Help:
			s = SDLK_HELP;
		break;
		case  Key_Direction_L:
		case  Key_Direction_R:
			s = SDLK_UNKNOWN;
		break;

		case  Key_Space:
		//case  Key_Any:
			s = SDLK_SPACE;
		break;

		case  Key_Exclam:
			s = SDLK_EXCLAIM;
		break;
		case  Key_QuoteDbl:
			s = SDLK_QUOTEDBL;
		break;
		case  Key_NumberSign:
			s = SDLK_HASH;
		break;
		case  Key_Dollar:
			s = SDLK_DOLLAR;
		break;
		case  Key_Percent:
			s = SDLK_PERCENT;
		break;
		case  Key_Ampersand:
			s = SDLK_AMPERSAND;
		break;
		case  Key_Apostrophe:
			s = SDLK_QUOTE;
		break;
		case  Key_ParenLeft:
			s = SDLK_LEFTPAREN;
		break;
		case  Key_ParenRight:
			s = SDLK_RIGHTPAREN;
		break;
		case  Key_Asterisk:
			s = SDLK_ASTERISK;
		break;
		case  Key_Plus:
			s = SDLK_PLUS;
		break;
		case  Key_Comma:
			s = SDLK_COMMA;
		break;
		case  Key_Minus:
			s = SDLK_MINUS;
		break;
		case  Key_Period:
			s = SDLK_PERIOD;
		break;
		case  Key_Slash:
			s = SDLK_SLASH;
		break;
		case  Key_0:
			s = SDLK_0;
		break;
      case  Key_1:
			s = SDLK_1;
		break;
      case  Key_2:
			s = SDLK_2;
		break;
      case  Key_3:
			s = SDLK_3;
		break;
      case  Key_4:
			s = SDLK_4;
		break;
      case  Key_5:
			s = SDLK_5;
		break;
      case  Key_6:
			s = SDLK_6;
		break;
      case  Key_7:
			s = SDLK_7;
		break;
      case  Key_8:
			s = SDLK_8;
		break;
      case  Key_9:
			s = SDLK_9;
		break;
		case  Key_Colon:
			s = SDLK_COLON;
		break;
		case  Key_Semicolon:
			s = SDLK_SEMICOLON;
		break;
		case  Key_Less:
			s = SDLK_LESS;
		break;
		case  Key_Equal:
			s = SDLK_EQUALS;
		break;
		case  Key_Greater:
			s = SDLK_GREATER;
		break;
		case  Key_Question:
			s = SDLK_QUESTION;
		break;
		case  Key_At:
			s = SDLK_AT;
		break;
		break;
		case  Key_A:
			s = SDLK_a;
		break;
      case  Key_B:
			s = SDLK_b;
		break;
      case  Key_C:
			s = SDLK_c;
		break;
      case  Key_D:
			s = SDLK_d;
		break;
      case  Key_E:
			s = SDLK_e;
		break;
      case  Key_F:
			s = SDLK_f;
		break;
      case  Key_G:
			s = SDLK_g;
		break;
      case  Key_H:
			s = SDLK_h;
		break;
      case  Key_I:
			s = SDLK_i;
		break;
      case  Key_J:
			s = SDLK_j;
		break;
      case  Key_K:
			s = SDLK_k;
		break;
      case  Key_L:
			s = SDLK_l;
		break;
      case  Key_M:
			s = SDLK_m;
		break;
      case  Key_N:
			s = SDLK_n;
		break;
      case  Key_O:
			s = SDLK_o;
		break;
      case  Key_P:
			s = SDLK_p;
		break;
      case  Key_Q:
			s = SDLK_q;
		break;
      case  Key_R:
			s = SDLK_r;
		break;
      case  Key_S:
			s = SDLK_s;
		break;
      case  Key_T:
			s = SDLK_t;
		break;
      case  Key_U:
			s = SDLK_u;
		break;
      case  Key_V:
			s = SDLK_v;
		break;
      case  Key_W:
			s = SDLK_w;
		break;
      case  Key_X:
			s = SDLK_x;
		break;
      case  Key_Y:
			s = SDLK_y;
		break;
      case  Key_Z:
			s = SDLK_z;
		break;
		case  Key_BracketLeft:
			s = SDLK_LEFTBRACKET;
		break;
		case  Key_Backslash:
			s = SDLK_BACKSLASH;
		break;
		case  Key_BracketRight:
			s = SDLK_RIGHTBRACKET;
		break;
		//case  Key_AsciiCircum:
		//	s = SDLK_CARET;
		//break;
		case  Key_Underscore:
			s = SDLK_UNDERSCORE;
		break;
		case  Key_QuoteLeft:
			s = SDLK_BACKQUOTE;
		break;
		case  Key_BraceLeft:
			s = SDLK_LEFTBRACKET;
		break;
		case  Key_Bar:
			s = SDLK_BACKSLASH;
		break;
		case  Key_BraceRight:
			s = SDLK_RIGHTBRACKET;
		break;
		case  Key_AsciiTilde:
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

SDL_Keymod convQtKey2SDLModifier( Qt::KeyboardModifiers m )
{
	int s = 0;

	if ( m != Qt::NoModifier )
	{
		if ( m & Qt::ShiftModifier )
		{
			s |= (KMOD_LSHIFT | KMOD_RSHIFT);
		}
		if ( m & Qt::AltModifier )
		{
			s |= (KMOD_LALT | KMOD_RALT);
		}
		if ( m & Qt::ControlModifier )
		{
			s |= (KMOD_LCTRL | KMOD_RCTRL);
		}
	}
	return (SDL_Keymod)s;
}

int  pushKeyEvent( QKeyEvent *event, int pressDown )
{
	SDL_Event sdlev;

	if ( pressDown )
	{
		sdlev.type = SDL_KEYDOWN;
		sdlev.key.state = SDL_PRESSED;
	}
	else
	{
		sdlev.type = SDL_KEYUP;
		sdlev.key.state = SDL_RELEASED;
	}

	sdlev.key.keysym.sym = convQtKey2SDLKeyCode( (Qt::Key)event->key() );

	sdlev.key.keysym.scancode = SDL_GetScancodeFromKey( sdlev.key.keysym.sym );

	sdlev.key.keysym.mod = convQtKey2SDLModifier( event->modifiers() );
	sdlev.key.repeat = 0;

	//printf("Modifiers: %08X -> %08X \n", event->modifiers(), sdlev.key.keysym.mod );

	if (sdlev.key.keysym.scancode != SDL_SCANCODE_UNKNOWN)
	{
		SDL_PushEvent (&sdlev);
	}

	return 0;
}
