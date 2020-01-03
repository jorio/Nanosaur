#include "Pomme.h"

//-----------------------------------------------------------------------------
// Input

int WinKeyToMacKey(WORD w) {
	switch (w) {
	case '0': return kVK_ANSI_0;
	case '1': return kVK_ANSI_1;
	case '2': return kVK_ANSI_2;
	case '3': return kVK_ANSI_3;
	case '4': return kVK_ANSI_4;
	case '5': return kVK_ANSI_5;
	case '6': return kVK_ANSI_6;
	case '7': return kVK_ANSI_7;
	case '8': return kVK_ANSI_8;
	case '9': return kVK_ANSI_9;
	case 'A': return kVK_ANSI_A;
	case 'B': return kVK_ANSI_B;
	case 'C': return kVK_ANSI_C;
	case 'D': return kVK_ANSI_D;
	case 'E': return kVK_ANSI_E;
	case 'F': return kVK_ANSI_F;
	case 'G': return kVK_ANSI_G;
	case 'H': return kVK_ANSI_H;
	case 'I': return kVK_ANSI_I;
	case 'J': return kVK_ANSI_J;
	case 'K': return kVK_ANSI_K;
	case 'L': return kVK_ANSI_L;
	case 'M': return kVK_ANSI_M;
	case 'N': return kVK_ANSI_N;
	case 'O': return kVK_ANSI_O;
	case 'P': return kVK_ANSI_P;
	case 'Q': return kVK_ANSI_Q;
	case 'R': return kVK_ANSI_R;
	case 'S': return kVK_ANSI_S;
	case 'T': return kVK_ANSI_T;
	case 'U': return kVK_ANSI_U;
	case 'V': return kVK_ANSI_V;
	case 'W': return kVK_ANSI_W;
	case 'X': return kVK_ANSI_X;
	case 'Y': return kVK_ANSI_Y;
	case 'Z': return kVK_ANSI_Z;
	case VK_BACK: return kVK_Delete;
	case VK_DELETE: return kVK_ForwardDelete;
	case VK_TAB: return kVK_Tab;
	case VK_SPACE: return kVK_Space;
	case VK_ESCAPE: return kVK_Escape;
	case VK_SHIFT: return kVK_Shift;
	case VK_CONTROL: return kVK_Control;
	case VK_LEFT: return kVK_LeftArrow;
	case VK_RIGHT: return kVK_RightArrow;
	case VK_UP: return kVK_UpArrow;
	case VK_DOWN: return kVK_DownArrow;
	case VK_HOME: return kVK_Home;
	case VK_END: return kVK_End;
	case VK_RETURN: return kVK_Return;
	default: return -1;
	}
}

void GetKeys(KeyMap km) {
	km[0] = 0;
	km[1] = 0;
	km[2] = 0;
	km[3] = 0;

	for (int i = 0; i < 256; i++) {
		int vk = WinKeyToMacKey(i);
		if (vk < 0)
			continue;
		if (!(GetAsyncKeyState(i) & 0x8000))
			continue;
		int byteNo = vk >> 5; // =vk/32
		int bitNo = vk & 31;
		km[byteNo] |= 1 << bitNo;
	}
}

Boolean Button(void) {
	TODOMINOR();
	return false;
}
