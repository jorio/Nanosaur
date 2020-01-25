#include "Pomme.h"
#include "PommeInternal.h"

#include <SDL.h>

//-----------------------------------------------------------------------------
// Input

static char scancodeLookupTable[256];

static void InitScancodeLookupTable()
{
	const char NO_MAC_VK = 0xFF;

	memset(scancodeLookupTable, NO_MAC_VK, sizeof(scancodeLookupTable));
	char* T = scancodeLookupTable;

	T[0]						= 0xFF;						// 0
	T[1]						= 0xFF;
	T[2]						= 0xFF;
	T[3]						= 0xFF;
	T[SDL_SCANCODE_A]			= kVK_ANSI_A;
	T[SDL_SCANCODE_B]			= kVK_ANSI_B;
	T[SDL_SCANCODE_C]			= kVK_ANSI_C;
	T[SDL_SCANCODE_D]			= kVK_ANSI_D;
	T[SDL_SCANCODE_E]			= kVK_ANSI_E;
	T[SDL_SCANCODE_F]			= kVK_ANSI_F;
	T[SDL_SCANCODE_G]			= kVK_ANSI_G;				// 10
	T[SDL_SCANCODE_H]			= kVK_ANSI_H;
	T[SDL_SCANCODE_I]			= kVK_ANSI_I;
	T[SDL_SCANCODE_J]			= kVK_ANSI_J;
	T[SDL_SCANCODE_K]			= kVK_ANSI_K;
	T[SDL_SCANCODE_L]			= kVK_ANSI_L;
	T[SDL_SCANCODE_M]			= kVK_ANSI_M;
	T[SDL_SCANCODE_N]			= kVK_ANSI_N;
	T[SDL_SCANCODE_O]			= kVK_ANSI_O;
	T[SDL_SCANCODE_P]			= kVK_ANSI_P;
	T[SDL_SCANCODE_Q]			= kVK_ANSI_Q;				// 20
	T[SDL_SCANCODE_R]			= kVK_ANSI_R;
	T[SDL_SCANCODE_S]			= kVK_ANSI_S;
	T[SDL_SCANCODE_T]			= kVK_ANSI_T;
	T[SDL_SCANCODE_U]			= kVK_ANSI_U;
	T[SDL_SCANCODE_V]			= kVK_ANSI_V;
	T[SDL_SCANCODE_W]			= kVK_ANSI_W;
	T[SDL_SCANCODE_X]			= kVK_ANSI_X;
	T[SDL_SCANCODE_Y]			= kVK_ANSI_Y;
	T[SDL_SCANCODE_Z]			= kVK_ANSI_Z;
	T[SDL_SCANCODE_1]			= kVK_ANSI_1;				// 30
	T[SDL_SCANCODE_2]			= kVK_ANSI_2;
	T[SDL_SCANCODE_3]			= kVK_ANSI_3;
	T[SDL_SCANCODE_4]			= kVK_ANSI_4;
	T[SDL_SCANCODE_5]			= kVK_ANSI_5;
	T[SDL_SCANCODE_6]			= kVK_ANSI_6;
	T[SDL_SCANCODE_7]			= kVK_ANSI_7;
	T[SDL_SCANCODE_8]			= kVK_ANSI_8;
	T[SDL_SCANCODE_9]			= kVK_ANSI_9;
	T[SDL_SCANCODE_0]			= kVK_ANSI_0;
	T[SDL_SCANCODE_RETURN]		= kVK_Return;				// 40
	T[SDL_SCANCODE_ESCAPE]		= kVK_Escape;
	T[SDL_SCANCODE_BACKSPACE]	= kVK_Delete;
	T[SDL_SCANCODE_TAB]			= kVK_Tab;
	T[SDL_SCANCODE_SPACE]		= kVK_Space;
	T[SDL_SCANCODE_MINUS]		= kVK_ANSI_Minus;
	T[SDL_SCANCODE_EQUALS]		= kVK_ANSI_Equal;
	T[SDL_SCANCODE_LEFTBRACKET] = kVK_ANSI_LeftBracket;
	T[SDL_SCANCODE_RIGHTBRACKET]= kVK_ANSI_RightBracket;
	T[SDL_SCANCODE_BACKSLASH]	= kVK_ANSI_Backslash;
	T[SDL_SCANCODE_NONUSHASH]	= NO_MAC_VK;				// 50
	T[SDL_SCANCODE_SEMICOLON]	= kVK_ANSI_Semicolon;
	T[SDL_SCANCODE_APOSTROPHE]	= kVK_ANSI_Quote;
	T[SDL_SCANCODE_GRAVE]		= kVK_ANSI_Grave;
	T[SDL_SCANCODE_COMMA]		= kVK_ANSI_Comma;
	T[SDL_SCANCODE_PERIOD]		= kVK_ANSI_Period;
	T[SDL_SCANCODE_SLASH]		= kVK_ANSI_Slash;
	T[SDL_SCANCODE_CAPSLOCK]	= kVK_CapsLock;
	T[SDL_SCANCODE_F1]			= kVK_F1;
	T[SDL_SCANCODE_F2]			= kVK_F2;
	T[SDL_SCANCODE_F3]			= kVK_F3;					// 60
	T[SDL_SCANCODE_F4]			= kVK_F4;
	T[SDL_SCANCODE_F5]			= kVK_F5;
	T[SDL_SCANCODE_F6]			= kVK_F6;
	T[SDL_SCANCODE_F7]			= kVK_F7;
	T[SDL_SCANCODE_F8]			= kVK_F8;
	T[SDL_SCANCODE_F9]			= kVK_F9;
	T[SDL_SCANCODE_F10]			= kVK_F10;
	T[SDL_SCANCODE_F11]			= kVK_F11;
	T[SDL_SCANCODE_F12]			= kVK_F12;
	T[SDL_SCANCODE_PRINTSCREEN] = NO_MAC_VK; // could be F13	// 70
	T[SDL_SCANCODE_SCROLLLOCK]	= NO_MAC_VK; // could be F14
	T[SDL_SCANCODE_PAUSE]		= NO_MAC_VK; // could be F15
	T[SDL_SCANCODE_INSERT]		= kVK_Help; // 'help' is there on an ext'd kbd
	T[SDL_SCANCODE_HOME]		= kVK_Home;
	T[SDL_SCANCODE_PAGEUP]		= kVK_PageUp;
	T[SDL_SCANCODE_DELETE]		= kVK_ForwardDelete;
	T[SDL_SCANCODE_END]			= kVK_End;
	T[SDL_SCANCODE_PAGEDOWN]	= kVK_PageDown;
	T[SDL_SCANCODE_RIGHT]		= kVK_RightArrow;
	T[SDL_SCANCODE_LEFT]		= kVK_LeftArrow;			// 80
	T[SDL_SCANCODE_DOWN]		= kVK_DownArrow;
	T[SDL_SCANCODE_UP]			= kVK_UpArrow;
	T[SDL_SCANCODE_NUMLOCKCLEAR]= kVK_ANSI_KeypadClear;		// 83
	T[SDL_SCANCODE_KP_DIVIDE]	= kVK_ANSI_KeypadDivide;	// 84
	T[SDL_SCANCODE_KP_MULTIPLY]	= kVK_ANSI_KeypadMultiply;
	T[SDL_SCANCODE_KP_MINUS]	= kVK_ANSI_KeypadMinus;
	T[SDL_SCANCODE_KP_PLUS]		= kVK_ANSI_KeypadPlus;
	T[SDL_SCANCODE_KP_ENTER]	= kVK_ANSI_KeypadEnter;
	T[SDL_SCANCODE_KP_1]		= kVK_ANSI_Keypad1;
    T[SDL_SCANCODE_KP_2]		= kVK_ANSI_Keypad2;			// 90
    T[SDL_SCANCODE_KP_3]		= kVK_ANSI_Keypad3;
    T[SDL_SCANCODE_KP_4]		= kVK_ANSI_Keypad4;
    T[SDL_SCANCODE_KP_5]		= kVK_ANSI_Keypad5;
    T[SDL_SCANCODE_KP_6]		= kVK_ANSI_Keypad6;
    T[SDL_SCANCODE_KP_7]		= kVK_ANSI_Keypad7;
    T[SDL_SCANCODE_KP_8]		= kVK_ANSI_Keypad8;
    T[SDL_SCANCODE_KP_9]		= kVK_ANSI_Keypad9;
    T[SDL_SCANCODE_KP_0]		= kVK_ANSI_Keypad0;
    T[SDL_SCANCODE_KP_PERIOD]	= kVK_ANSI_KeypadDecimal;
	T[SDL_SCANCODE_NONUSBACKSLASH] = NO_MAC_VK;				// 100
	T[SDL_SCANCODE_APPLICATION] = NO_MAC_VK;
	T[SDL_SCANCODE_POWER]		= NO_MAC_VK;
	T[SDL_SCANCODE_KP_EQUALS]	= kVK_ANSI_KeypadEquals;	// 103
	T[SDL_SCANCODE_F13]			= kVK_F13;					// 104
	T[SDL_SCANCODE_F14]			= kVK_F14;
	T[SDL_SCANCODE_F15]			= kVK_F15;
	T[SDL_SCANCODE_F16]			= kVK_F16;
	T[SDL_SCANCODE_F17]			= kVK_F17;
	T[SDL_SCANCODE_F18]			= kVK_F18;
	T[SDL_SCANCODE_F19]			= kVK_F19;					// 110
	T[SDL_SCANCODE_F20]			= kVK_F20;					// 111
	T[SDL_SCANCODE_F21]			= NO_MAC_VK;
	T[SDL_SCANCODE_F22]			= NO_MAC_VK;
	T[SDL_SCANCODE_F23]			= NO_MAC_VK;
	T[SDL_SCANCODE_F24]			= NO_MAC_VK;				// 115
	T[SDL_SCANCODE_HELP]		= kVK_Help; // also INSERT
	
	// --snip--

	T[SDL_SCANCODE_LCTRL]		= kVK_Control;				// 224
	T[SDL_SCANCODE_LSHIFT]		= kVK_Shift;
	T[SDL_SCANCODE_LALT]		= kVK_Option;
	T[SDL_SCANCODE_LGUI]		= kVK_Command;
	T[SDL_SCANCODE_RCTRL]		= kVK_RightControl;
	T[SDL_SCANCODE_RSHIFT]		= kVK_RightShift;
	T[SDL_SCANCODE_RALT]		= kVK_RightOption;
	T[SDL_SCANCODE_RGUI]		= kVK_Command; // no right command I guess
}

void GetKeys(KeyMap km)
{
	km[0] = 0;
	km[1] = 0;
	km[2] = 0;
	km[3] = 0;

	SDL_PumpEvents();
	int numkeys = 0;
	const UInt8* keystate =  SDL_GetKeyboardState(&numkeys);

	numkeys = min(sizeof(scancodeLookupTable), numkeys);

	for (int sdlScancode = 0; sdlScancode < numkeys; sdlScancode++) {
		if (!keystate[sdlScancode])
			continue;
		int vk = scancodeLookupTable[sdlScancode];
		if (vk == 0xFF)
			continue;
		int byteNo = vk >> 5; // =vk/32
		int bitNo = vk & 31;
		km[byteNo] |= 1 << bitNo;
	}

#if POMME_DEBUG_INPUT
	if (km[0] || km[1] || km[2] || km[3])
		printf("GK %08x%08x%08x%08x\n", km[0], km[1], km[2], km[3]);
#endif
}

Boolean Button(void)
{
	TODOMINOR();
	return false;
}

void Pomme::Input::Init()
{
	InitScancodeLookupTable();
}
