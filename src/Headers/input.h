//
// input.h
//

#pragma once

#include <SDL.h>

#define KEYBINDING_MAX_KEYS					2
#define KEYBINDING_MAX_GAMEPAD_BUTTONS		2

typedef struct KeyBinding
{
	int16_t		key[KEYBINDING_MAX_KEYS];
	int16_t		mouseButton;
	int16_t		mouseWheelDelta;
	int16_t		gamepadButton[KEYBINDING_MAX_GAMEPAD_BUTTONS];
	int16_t		gamepadAxis;
	int16_t		gamepadAxisSign;
} KeyBinding;


enum
{
	kNeed_Forward,
	kNeed_Backward,
	kNeed_TurnLeft,
	kNeed_TurnRight,
	kNeed_Jump,
	kNeed_Attack,
	kNeed_AttackMode,
	kNeed_PickUp,
	kNeed_JetUp,
	kNeed_JetDown,
	kNeed_CameraLeft,
	kNeed_CameraRight,
	kNeed_ZoomIn,
	kNeed_ZoomOut,
	kNeed_CameraMode,
	kNeed_ToggleGPS,
	kNeed_ToggleMusic,
	kNeed_ToggleAmbient,
	kNeed_ToggleFullscreen,
	NUM_REMAPPABLE_NEEDS,

	// ^^^ REMAPPABLE
	// --------------------------------------------------------
	//              NON-REMAPPABLE vvv

	kNeed_UIUp = NUM_REMAPPABLE_NEEDS,
	kNeed_UIDown,
	kNeed_UILeft,
	kNeed_UIRight,
	kNeed_UIConfirm,
	kNeed_UIBack,
	kNeed_UIPause,
	NUM_CONTROL_NEEDS
};


//============================================================================================


void InitInput(void);
void UpdateInput(void);

bool GetNewSDLKeyState(unsigned short sdlScanCode);
bool GetSDLKeyState(unsigned short sdlScanCode);
bool UserWantsOut(void);
bool AreAnyNewKeysPressed(void);

bool GetNewNeedState(int needID);
bool GetNeedState(int needID);

SDL_GameController* TryOpenController(bool showMessageOnFailure);
void OnJoystickRemoved(SDL_JoystickID which);
