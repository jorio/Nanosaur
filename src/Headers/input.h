//
// input.h
//

#pragma once

#include <SDL.h>

typedef struct KeyBinding
{
	int16_t		key1;
	int16_t		key2;
	int16_t		mouseButton;
	int16_t		mouseWheelDelta;
	int16_t		gamepadButton1;
	int16_t		gamepadButton2;
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

	// ^^^ REMAPPABLE
	// --------------------------------------------------------
	//              NON-REMAPPABLE vvv

	kNeed_UIUp,
	kNeed_UIDown,
	kNeed_UILeft,
	kNeed_UIRight,
	kNeed_UIConfirm,
	kNeed_UIBack,
	kNeed_UIPause,
	kNeed_ToggleMusic,
	kNeed_ToggleAmbient,
	kNeed_ToggleFullscreen,
	kNeed_RaiseVolume,
	kNeed_LowerVolume,
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
