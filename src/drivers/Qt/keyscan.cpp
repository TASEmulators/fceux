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
  #if  defined(_HAS_XKB)
  {  9, { XKB_KEY_Escape           , Qt::Key_Escape       }},
  { 10, { XKB_KEY_1                , Qt::Key_1            }},
  { 11, { XKB_KEY_2                , Qt::Key_2            }},
  { 12, { XKB_KEY_3                , Qt::Key_3            }},
  { 13, { XKB_KEY_4                , Qt::Key_4            }},
  { 14, { XKB_KEY_5                , Qt::Key_5            }},
  { 15, { XKB_KEY_6                , Qt::Key_6            }},
  { 16, { XKB_KEY_7                , Qt::Key_7            }},
  { 17, { XKB_KEY_8                , Qt::Key_8            }},
  { 18, { XKB_KEY_9                , Qt::Key_9            }},
  { 19, { XKB_KEY_0                , Qt::Key_0            }},
  { 20, { XKB_KEY_minus            , Qt::Key_Minus        }},
  { 21, { XKB_KEY_equal            , Qt::Key_Equal        }},
  { 22, { XKB_KEY_BackSpace        , Qt::Key_Backspace    }},
  { 23, { XKB_KEY_Tab              , Qt::Key_Tab          }},
  { 24, { XKB_KEY_q                , Qt::Key_Q            }},
  { 25, { XKB_KEY_w                , Qt::Key_W            }},
  { 26, { XKB_KEY_e                , Qt::Key_E            }},
  { 27, { XKB_KEY_r                , Qt::Key_R            }},
  { 28, { XKB_KEY_t                , Qt::Key_T            }},
  { 29, { XKB_KEY_y                , Qt::Key_Y            }},
  { 30, { XKB_KEY_u                , Qt::Key_U            }},
  { 31, { XKB_KEY_i                , Qt::Key_I            }},
  { 32, { XKB_KEY_o                , Qt::Key_O            }},
  { 33, { XKB_KEY_p                , Qt::Key_P            }},
  { 34, { XKB_KEY_bracketleft      , Qt::Key_BracketLeft  }},
  { 35, { XKB_KEY_bracketright     , Qt::Key_BracketRight }},
  { 36, { XKB_KEY_Return           , Qt::Key_Return       }},
  { 37, { XKB_KEY_Control_L        , Qt::Key_Control      }},
  { 38, { XKB_KEY_a                , Qt::Key_A            }},
  { 39, { XKB_KEY_s                , Qt::Key_S            }},
  { 40, { XKB_KEY_d                , Qt::Key_D            }},
  { 41, { XKB_KEY_f                , Qt::Key_F            }},
  { 42, { XKB_KEY_g                , Qt::Key_G            }},
  { 43, { XKB_KEY_h                , Qt::Key_H            }},
  { 44, { XKB_KEY_j                , Qt::Key_J            }},
  { 45, { XKB_KEY_k                , Qt::Key_K            }},
  { 46, { XKB_KEY_l                , Qt::Key_L            }},
  { 47, { XKB_KEY_semicolon        , Qt::Key_Semicolon    }},
  { 48, { XKB_KEY_apostrophe       , Qt::Key_Apostrophe   }},
  { 49, { XKB_KEY_grave            , Qt::Key_QuoteLeft    }},
  { 50, { XKB_KEY_Shift_L          , Qt::Key_Shift        }},
  { 51, { XKB_KEY_backslash        , Qt::Key_Backslash    }},
  { 52, { XKB_KEY_z                , Qt::Key_Z            }},
  { 53, { XKB_KEY_x                , Qt::Key_X            }},
  { 54, { XKB_KEY_c                , Qt::Key_C            }},
  { 55, { XKB_KEY_v                , Qt::Key_V            }},
  { 56, { XKB_KEY_b                , Qt::Key_B            }},
  { 57, { XKB_KEY_n                , Qt::Key_N            }},
  { 58, { XKB_KEY_m                , Qt::Key_M            }},
  { 59, { XKB_KEY_comma            , Qt::Key_Comma        }},
  { 60, { XKB_KEY_period           , Qt::Key_Period       }},
  { 61, { XKB_KEY_slash            , Qt::Key_Slash        }},
  { 62, { XKB_KEY_Shift_R          , Qt::Key_Shift        }},
  { 63, { XKB_KEY_KP_Multiply      , Qt::Key_Asterisk     }},
  { 64, { XKB_KEY_Alt_L            , Qt::Key_Alt          }},
  { 66, { XKB_KEY_Caps_Lock        , Qt::Key_CapsLock     }},
  { 67, { XKB_KEY_F1               , Qt::Key_F1           }},
  { 68, { XKB_KEY_F2               , Qt::Key_F2           }},
  { 69, { XKB_KEY_F3               , Qt::Key_F3           }},
  { 70, { XKB_KEY_F4               , Qt::Key_F4           }},
  { 71, { XKB_KEY_F5               , Qt::Key_F5           }},
  { 72, { XKB_KEY_F6               , Qt::Key_F6           }},
  { 73, { XKB_KEY_F7               , Qt::Key_F7           }},
  { 74, { XKB_KEY_F8               , Qt::Key_F8           }},
  { 75, { XKB_KEY_F9               , Qt::Key_F9           }},
  { 76, { XKB_KEY_F10              , Qt::Key_F10          }},
  { 77, { XKB_KEY_Num_Lock         , Qt::Key_NumLock      }},
  { 78, { XKB_KEY_Scroll_Lock      , Qt::Key_ScrollLock   }},
  { 79, { XKB_KEY_KP_7             , Qt::Key_7            }},
  { 80, { XKB_KEY_KP_8             , Qt::Key_8            }},
  { 81, { XKB_KEY_KP_9             , Qt::Key_9            }},
  { 82, { XKB_KEY_KP_Subtract      , Qt::Key_Minus        }},
  { 83, { XKB_KEY_KP_4             , Qt::Key_4            }},
  { 84, { XKB_KEY_KP_5             , Qt::Key_5            }},
  { 85, { XKB_KEY_KP_6             , Qt::Key_6            }},
  { 86, { XKB_KEY_KP_Add           , Qt::Key_Plus         }},
  { 87, { XKB_KEY_KP_1             , Qt::Key_1            }},
  { 88, { XKB_KEY_KP_2             , Qt::Key_2            }},
  { 89, { XKB_KEY_KP_3             , Qt::Key_3            }},
  { 90, { XKB_KEY_KP_0             , Qt::Key_0            }},
  { 91, { XKB_KEY_KP_Decimal       , Qt::Key_Period       }},
  { 95, { XKB_KEY_F11              , Qt::Key_F11          }},
  { 96, { XKB_KEY_F12              , Qt::Key_F12          }},
  {104, { XKB_KEY_KP_Enter         , Qt::Key_Enter        }},
  {105, { XKB_KEY_Control_R        , Qt::Key_Control      }},
  {106, { XKB_KEY_KP_Divide        , Qt::Key_Slash        }},
  {108, { XKB_KEY_Alt_R            , Qt::Key_Alt          }},
  {110, { XKB_KEY_Home             , Qt::Key_Home         }},
  {111, { XKB_KEY_Up               , Qt::Key_Up           }},
  {112, { XKB_KEY_Page_Up          , Qt::Key_PageUp       }},
  {113, { XKB_KEY_Left             , Qt::Key_Left         }},
  {114, { XKB_KEY_Right            , Qt::Key_Right        }},
  {115, { XKB_KEY_End              , Qt::Key_End          }},
  {116, { XKB_KEY_Down             , Qt::Key_Down         }},
  {117, { XKB_KEY_Page_Down        , Qt::Key_PageDown     }},
  {118, { XKB_KEY_Insert           , Qt::Key_Insert       }},
  {119, { XKB_KEY_Delete           , Qt::Key_Delete       }},
  {127, { XKB_KEY_Pause            , Qt::Key_Pause        }},
  {134, { XKB_KEY_Super_R          , Qt::Key_Meta         }},
  #else
  {  9, { 0xff1b   , Qt::Key_Escape       }},
  { 10, { 0x0031   , Qt::Key_1            }},
  { 11, { 0x0032   , Qt::Key_2            }},
  { 12, { 0x0033   , Qt::Key_3            }},
  { 13, { 0x0034   , Qt::Key_4            }},
  { 14, { 0x0035   , Qt::Key_5            }},
  { 15, { 0x0036   , Qt::Key_6            }},
  { 16, { 0x0037   , Qt::Key_7            }},
  { 17, { 0x0038   , Qt::Key_8            }},
  { 18, { 0x0039   , Qt::Key_9            }},
  { 19, { 0x0030   , Qt::Key_0            }},
  { 20, { 0x002d   , Qt::Key_Minus        }},
  { 21, { 0x003d   , Qt::Key_Equal        }},
  { 22, { 0xff08   , Qt::Key_Backspace    }},
  { 23, { 0xff09   , Qt::Key_Tab          }},
  { 24, { 0x0071   , Qt::Key_Q            }},
  { 25, { 0x0077   , Qt::Key_W            }},
  { 26, { 0x0065   , Qt::Key_E            }},
  { 27, { 0x0072   , Qt::Key_R            }},
  { 28, { 0x0074   , Qt::Key_T            }},
  { 29, { 0x0079   , Qt::Key_Y            }},
  { 30, { 0x0075   , Qt::Key_U            }},
  { 31, { 0x0069   , Qt::Key_I            }},
  { 32, { 0x006f   , Qt::Key_O            }},
  { 33, { 0x0070   , Qt::Key_P            }},
  { 34, { 0x005b   , Qt::Key_BracketLeft  }},
  { 35, { 0x005d   , Qt::Key_BracketRight }},
  { 36, { 0xff0d   , Qt::Key_Return       }},
  { 37, { 0xffe3   , Qt::Key_Control      }},
  { 38, { 0x0061   , Qt::Key_A            }},
  { 39, { 0x0073   , Qt::Key_S            }},
  { 40, { 0x0064   , Qt::Key_D            }},
  { 41, { 0x0066   , Qt::Key_F            }},
  { 42, { 0x0067   , Qt::Key_G            }},
  { 43, { 0x0068   , Qt::Key_H            }},
  { 44, { 0x006a   , Qt::Key_J            }},
  { 45, { 0x006b   , Qt::Key_K            }},
  { 46, { 0x006c   , Qt::Key_L            }},
  { 47, { 0x003b   , Qt::Key_Semicolon    }},
  { 48, { 0x0027   , Qt::Key_Apostrophe   }},
  { 49, { 0x0060   , Qt::Key_QuoteLeft    }},
  { 50, { 0xffe1   , Qt::Key_Shift        }},
  { 51, { 0x005c   , Qt::Key_Backslash    }},
  { 52, { 0x007a   , Qt::Key_Z            }},
  { 53, { 0x0078   , Qt::Key_X            }},
  { 54, { 0x0063   , Qt::Key_C            }},
  { 55, { 0x0076   , Qt::Key_V            }},
  { 56, { 0x0062   , Qt::Key_B            }},
  { 57, { 0x006e   , Qt::Key_N            }},
  { 58, { 0x006d   , Qt::Key_M            }},
  { 59, { 0x002c   , Qt::Key_Comma        }},
  { 60, { 0x002e   , Qt::Key_Period       }},
  { 61, { 0x002f   , Qt::Key_Slash        }},
  { 62, { 0xffe2   , Qt::Key_Shift        }},
  { 63, { 0xffaa   , Qt::Key_Asterisk     }},
  { 64, { 0xffe9   , Qt::Key_Alt          }},
  { 66, { 0xffe5   , Qt::Key_CapsLock     }},
  { 67, { 0xffbe   , Qt::Key_F1           }},
  { 68, { 0xffbf   , Qt::Key_F2           }},
  { 69, { 0xffc0   , Qt::Key_F3           }},
  { 70, { 0xffc1   , Qt::Key_F4           }},
  { 71, { 0xffc2   , Qt::Key_F5           }},
  { 72, { 0xffc3   , Qt::Key_F6           }},
  { 73, { 0xffc4   , Qt::Key_F7           }},
  { 74, { 0xffc5   , Qt::Key_F8           }},
  { 75, { 0xffc6   , Qt::Key_F9           }},
  { 76, { 0xffc7   , Qt::Key_F10          }},
  { 77, { 0xff7f   , Qt::Key_NumLock      }},
  { 78, { 0xff14   , Qt::Key_ScrollLock   }},
  { 79, { 0xffb7   , Qt::Key_7            }},
  { 80, { 0xffb8   , Qt::Key_8            }},
  { 81, { 0xffb9   , Qt::Key_9            }},
  { 82, { 0xffad   , Qt::Key_Minus        }},
  { 83, { 0xffb4   , Qt::Key_4            }},
  { 84, { 0xffb5   , Qt::Key_5            }},
  { 85, { 0xffb6   , Qt::Key_6            }},
  { 86, { 0xffab   , Qt::Key_Plus         }},
  { 87, { 0xffb1   , Qt::Key_1            }},
  { 88, { 0xffb2   , Qt::Key_2            }},
  { 89, { 0xffb3   , Qt::Key_3            }},
  { 90, { 0xffb0   , Qt::Key_0            }},
  { 91, { 0xffae   , Qt::Key_Period       }},
  { 95, { 0xffc8   , Qt::Key_F11          }},
  { 96, { 0xffc9   , Qt::Key_F12          }},
  {104, { 0xff8d   , Qt::Key_Enter        }},
  {105, { 0xffe4   , Qt::Key_Control      }},
  {106, { 0xffaf   , Qt::Key_Slash        }},
  {108, { 0xffea   , Qt::Key_Alt          }},
  {110, { 0xff50   , Qt::Key_Home         }},
  {111, { 0xff52   , Qt::Key_Up           }},
  {112, { 0xff55   , Qt::Key_PageUp       }},
  {113, { 0xff51   , Qt::Key_Left         }},
  {114, { 0xff53   , Qt::Key_Right        }},
  {115, { 0xff57   , Qt::Key_End          }},
  {116, { 0xff54   , Qt::Key_Down         }},
  {117, { 0xff56   , Qt::Key_PageDown     }},
  {118, { 0xff63   , Qt::Key_Insert       }},
  {119, { 0xffff   , Qt::Key_Delete       }},
  {127, { 0xff13   , Qt::Key_Pause        }},
  {134, { 0xffec   , Qt::Key_Meta         }},
  #endif
};
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

	printf("Key %s: x%08X  %i\n", (sdlev.type == SDL_KEYUP) ? "UP" : "DOWN", event->key(), event->key() );
	printf("   Native ScanCode: x%08X  %i \n", event->nativeScanCode(), event->nativeScanCode() );
	printf("   Virtual ScanCode: x%08X  %i \n", event->nativeVirtualKey(), event->nativeVirtualKey() );
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

	/* when we're unable to convert qt keys to sdl, we do keyboard native scan code conversion */
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
