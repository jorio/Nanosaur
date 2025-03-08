/****************************/
/*     INPUT.C			    */
/* By Brian Greenstone      */
/* (c)1997 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

static SDL_Gamepad	*gSDLGamepad = NULL;

static KeyState		gRawKeyboardState[SDL_SCANCODE_COUNT];
bool				gAnyNewKeysPressed = false;
char				gTextInput[64];

static KeyState		gNeedStates[NUM_CONTROL_NEEDS];


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_PRESSED		= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};

#define JOYSTICK_DEAD_ZONE .33f
#define JOYSTICK_DEAD_ZONE_SQUARED (JOYSTICK_DEAD_ZONE*JOYSTICK_DEAD_ZONE)

#define JOYSTICK_FAKEDIGITAL_DEAD_ZONE .66f

#if __APPLE__
#define DEFAULTKB1_JUMP		SDL_SCANCODE_LGUI
#define DEFAULTKB2_JUMP		SDL_SCANCODE_RGUI
#define DEFAULTKB1_PICKUP	SDL_SCANCODE_LALT
#define DEFAULTKB2_PICKUP	SDL_SCANCODE_RALT
#else
#define DEFAULTKB1_JUMP		SDL_SCANCODE_LALT
#define DEFAULTKB2_JUMP		SDL_SCANCODE_RALT
#define DEFAULTKB1_PICKUP	SDL_SCANCODE_LCTRL
#define DEFAULTKB2_PICKUP	SDL_SCANCODE_RCTRL
#endif

const KeyBinding kDefaultKeyBindings[NUM_CONTROL_NEEDS] =
{
//Need------------------    Keys--------------------------------------------- MouseBtn-------  MWheel-  GamepadButtons---------------------------------------------------  GamepadAxis----------  GamepadAxisSign
[kNeed_Forward			] = {{SDL_SCANCODE_UP,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_UP,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTY,			-1,	},
[kNeed_Backward			] = {{SDL_SCANCODE_DOWN,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_DOWN,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTY,			+1,	},
[kNeed_TurnLeft			] = {{SDL_SCANCODE_LEFT,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_LEFT,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTX,			-1,	},
[kNeed_TurnRight		] = {{SDL_SCANCODE_RIGHT,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_RIGHT,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTX,			+1,	},
[kNeed_JetUp			] = {{SDL_SCANCODE_A,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,	+1,	},
[kNeed_JetDown			] = {{SDL_SCANCODE_Z,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,	+1,	},
[kNeed_PrevWeapon		] = {{SDL_SCANCODE_RSHIFT,	SDL_SCANCODE_LEFTBRACKET}, 0,					+1,	{SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,	SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_NextWeapon		] = {{SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RIGHTBRACKET}, 0,					-1,	{SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,	SDL_GAMEPAD_BUTTON_NORTH,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_Attack			] = {{SDL_SCANCODE_SPACE,	0,						}, SDL_BUTTON_LEFT,		0,	{SDL_GAMEPAD_BUTTON_WEST,			SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_PickUp			] = {{DEFAULTKB1_PICKUP,	DEFAULTKB2_PICKUP,		}, SDL_BUTTON_MIDDLE,	0,	{SDL_GAMEPAD_BUTTON_EAST,			SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_Jump				] = {{DEFAULTKB1_JUMP,		DEFAULTKB2_JUMP,		}, SDL_BUTTON_RIGHT,	0,	{SDL_GAMEPAD_BUTTON_SOUTH,			SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_CameraMode		] = {{SDL_SCANCODE_TAB,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_RIGHT_STICK,	SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_CameraLeft		] = {{SDL_SCANCODE_COMMA,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_RIGHTX,			-1,	},
[kNeed_CameraRight		] = {{SDL_SCANCODE_PERIOD,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_RIGHTX,			+1,	},
[kNeed_ZoomIn			] = {{SDL_SCANCODE_2,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_RIGHTY,			-1,	},
[kNeed_ZoomOut			] = {{SDL_SCANCODE_1,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_RIGHTY,			+1,	},
[kNeed_ToggleGPS		] = {{SDL_SCANCODE_G,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_BACK,			SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_UIUp				] = {{SDL_SCANCODE_UP,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_UP,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTY,			-1,	},
[kNeed_UIDown			] = {{SDL_SCANCODE_DOWN,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_DOWN,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTY,			+1,	},
[kNeed_UILeft			] = {{SDL_SCANCODE_LEFT,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_LEFT,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTX,			-1,	},
[kNeed_UIRight			] = {{SDL_SCANCODE_RIGHT,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_DPAD_RIGHT,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_LEFTX,			+1,	},
[kNeed_UIConfirm		] = {{SDL_SCANCODE_RETURN,	SDL_SCANCODE_SPACE,		}, 0,					0,	{SDL_GAMEPAD_BUTTON_START,			SDL_GAMEPAD_BUTTON_SOUTH,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_UIBack			] = {{SDL_SCANCODE_ESCAPE,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_BACK,			SDL_GAMEPAD_BUTTON_EAST,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_UIPause			] = {{SDL_SCANCODE_ESCAPE,	0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_START,			SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_ToggleMusic		] = {{SDL_SCANCODE_M,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
[kNeed_ToggleAmbient	] = {{SDL_SCANCODE_B,		0,						}, 0,					0,	{SDL_GAMEPAD_BUTTON_INVALID,		SDL_GAMEPAD_BUTTON_INVALID,	}, SDL_GAMEPAD_AXIS_INVALID,		0,	},
};


/**********************/
/*     VARIABLES      */
/**********************/


/**********************/
/* STATIC FUNCTIONS   */
/**********************/

static inline void UpdateKeyState(KeyState* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;

		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

/************************* INIT INPUT *********************************/

void InitInput(void)
{
}


void UpdateInput(void)
{

	gTextInput[0] = '\0';


	/**********************/
	/* DO SDL MAINTENANCE */
	/**********************/

//	MouseSmoothing_StartFrame();

	int mouseWheelDelta = 0;

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_EVENT_WINDOW_RESIZED:
				QD3D_OnWindowResized();
				break;

/*
			case SDL_WINDOWEVENT_FOCUS_LOST:
#if __APPLE__
				// On Mac, always restore system mouse accel if cmd-tabbing away from the game
				RestoreMacMouseAcceleration();
#endif
				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED:
#if __APPLE__
				// On Mac, kill mouse accel when focus is regained only if the game has captured the mouse
				if (SDL_GetRelativeMouseMode())
					KillMacMouseAcceleration();
#endif
				break;

			case SDL_MOUSEMOTION:
				if (!gEatMouse)
				{
					MouseSmoothing_OnMouseMotion(&event.motion);
				}
				break;
*/

			case SDL_EVENT_TEXT_INPUT:
				SDL_snprintf(gTextInput, sizeof(gTextInput), "%s", event.text.text);
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				TryOpenGamepad(false);
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				OnJoystickRemoved(event.gdevice.which);
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				mouseWheelDelta += event.wheel.y;
				mouseWheelDelta += event.wheel.x;
				break;
		}
	}

	int numkeys = 0;
	const bool* keystate = SDL_GetKeyboardState(&numkeys);
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	gAnyNewKeysPressed = false;

	{
		int minNumKeys = numkeys < SDL_SCANCODE_COUNT ? numkeys : SDL_SCANCODE_COUNT;

		for (int i = 0; i < minNumKeys; i++)
		{
			UpdateKeyState(&gRawKeyboardState[i], keystate[i]);
			if (gRawKeyboardState[i] == KEYSTATE_PRESSED)
				gAnyNewKeysPressed = true;
		}

		// fill out the rest
		for (int i = minNumKeys; i < SDL_SCANCODE_COUNT; i++)
			UpdateKeyState(&gRawKeyboardState[i], false);
	}

	// --------------------------------------------
	// Intercept system key chords

	if (GetNewSDLKeyState(SDL_SCANCODE_RETURN)
		&& (GetSDLKeyState(SDL_SCANCODE_LALT) || GetSDLKeyState(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode(false);

		gRawKeyboardState[SDL_SCANCODE_RETURN] = KEYSTATE_IGNOREHELD;
	}

	if ((!gTerrainPtr || gGamePaused) && IsCmdQPressed())
	{
		CleanQuit();
		return;
	}

	// --------------------------------------------
	// Update need states

	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const KeyBinding* kb = &gGamePrefs.keys[i];

		bool downNow = false;

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
			if (kb->key[j] && kb->key[j] < numkeys)
				downNow |= gRawKeyboardState[kb->key[j]] & KEYSTATE_ACTIVE_BIT;

		if (kb->mouseButton)
			downNow |= 0 != (mouseButtons & SDL_BUTTON_MASK(kb->mouseButton));

		if ((kb->mouseWheelDelta > 0 && mouseWheelDelta > 0) || (kb->mouseWheelDelta < 0 && mouseWheelDelta < 0))
			downNow |= true;

		if (gSDLGamepad)
		{
			for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
				if (kb->gamepadButton[j] != SDL_GAMEPAD_BUTTON_INVALID)
					downNow |= 0 != SDL_GetGamepadButton(gSDLGamepad, kb->gamepadButton[j]);

			if (kb->gamepadAxis != SDL_GAMEPAD_AXIS_INVALID)
			{
				int16_t rawValue = SDL_GetGamepadAxis(gSDLGamepad, kb->gamepadAxis);
				downNow |= (kb->gamepadAxisSign > 0 && rawValue > (int16_t)(JOYSTICK_FAKEDIGITAL_DEAD_ZONE * 32767.0f))
						|| (kb->gamepadAxisSign < 0 && rawValue < (int16_t)(JOYSTICK_FAKEDIGITAL_DEAD_ZONE * -32768.0f));
			}
		}

		UpdateKeyState(&gNeedStates[i], downNow);
	}
}


/******* DID USER PRESS CMD+Q (MAC ONLY) *******/

bool IsCmdQPressed(void)
{
#if __APPLE__
	return (GetSDLKeyState(SDL_SCANCODE_LGUI) || GetSDLKeyState(SDL_SCANCODE_RGUI))
		&& GetNewSDLKeyState(SDL_GetScancodeFromKey(SDLK_Q, NULL));
#else
	return false;
#endif
}


/************************ GET SKIP KEY STATE ***************************/

bool UserWantsOut(void)
{
	return GetNewNeedState(kNeed_UIConfirm) || GetNewNeedState(kNeed_UIBack);
}


#pragma mark -


bool GetNewSDLKeyState(unsigned short sdlScanCode)
{
	return gRawKeyboardState[sdlScanCode] == KEYSTATE_PRESSED;
}

bool GetSDLKeyState(unsigned short sdlScanCode)
{
	return 0 != (gRawKeyboardState[sdlScanCode] & KEYSTATE_ACTIVE_BIT);
}

bool AreAnyNewKeysPressed(void)
{
	return gAnyNewKeysPressed;
}

bool GetNeedState(int needID)
{
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
	return 0 != (gNeedStates[needID] & KEYSTATE_ACTIVE_BIT);
}

bool GetNewNeedState(int needID)
{
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
	return gNeedStates[needID] == KEYSTATE_PRESSED;
}


#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

SDL_Gamepad* TryOpenGamepad(bool showMessage)
{
	if (gSDLGamepad)
	{
		SDL_Log("Already have a valid gamepad.");
		return gSDLGamepad;
	}

	int numJoysticks = 0;

	SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
	SDL_Gamepad* newGamepad = NULL;

	for (int i = 0; i < numJoysticks; ++i)
	{
		SDL_JoystickID joystickID = joysticks[i];

		// Usable as an SDL_Gamepad?
		if (!SDL_IsGamepad(joystickID))
		{
			continue;
		}

		// Use this one
		newGamepad = SDL_OpenGamepad(joystickID);
		if (newGamepad)
		{
			break;
		}
	}

	if (newGamepad)
	{
		// OK
	}
	else if (numJoysticks == 0)
	{
		// No-op
	}
	else
	{
		SDL_Log("%d joysticks found, but none is suitable as an SDL_Gamepad.", numJoysticks);
		if (showMessage)
		{
			char messageBuf[1024];
			SDL_snprintf(messageBuf, sizeof(messageBuf),
						 "The game does not support your controller yet (\"%s\").\n\n"
						 "You can play with the keyboard and mouse instead. Sorry!",
						 SDL_GetJoystickNameForID(joysticks[0]));
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
		}
	}

	SDL_free(joysticks);

	return newGamepad;
}


void OnJoystickRemoved(SDL_JoystickID which)
{
	if (!gSDLGamepad)		// don't care, I didn't open any gamepad
		return;

	if (which != SDL_GetGamepadID(gSDLGamepad))	// don't care, this isn't the joystick I'm using
		return;

	SDL_Log("Current joystick was removed: %d", which);

	// Nuke reference to this controller+joystick
	SDL_CloseGamepad(gSDLGamepad);
	gSDLGamepad = NULL;

	// Try to open another joystick if any is connected.
	TryOpenGamepad(false);
}

/*
static TQ3Vector2D GetThumbStickVector(bool rightStick)
{
	Sint16 dxRaw = SDL_GamepadGetAxis(gSDLGamepad, rightStick ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GamepadGetAxis(gSDLGamepad, rightStick ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY);

	float dx = dxRaw / 32767.0f;
	float dy = dyRaw / 32767.0f;

	float magnitudeSquared = dx*dx + dy*dy;
	if (magnitudeSquared < JOYSTICK_DEAD_ZONE_SQUARED)
		return (TQ3Vector2D) { 0, 0 };
	else
		return (TQ3Vector2D) { dx, dy };
}
*/

