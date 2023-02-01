#include "game.h"

#define MAX_CHOICES 16

static const uint32_t kBGColor		= 0x000000;
static const uint32_t kFGColor		= 0xa0a0a0;
static const uint32_t kMutedColor	= 0x404040;
static const uint32_t kTitleColor	= 0x606060;
static const uint32_t kAccentColor	= 0x108020;

static const int kColumnX[] = { 128, 300, 400, 500 };

static int selectedEntry = 0;

static int controlsHighlightedRow = 0;
static int controlsHighlightedColumn = 0;
static const int kNumKeybindingRows = NUM_REMAPPABLE_NEEDS + 2;  // +2 extra rows for Reset to defaults & Done
static const int kKeybindingRow_Reset = NUM_REMAPPABLE_NEEDS + 0;
static const int kKeybindingRow_Done = NUM_REMAPPABLE_NEEDS + 1;

static bool gShowAntialiasingWarning = false;


enum
{
	kSettingsState_Off,
	kSettingsState_FadeIn,
	kSettingsState_MainPage,
	kSettingsState_ControlsPage,
	kSettingsState_ControlsPage_AwaitingPress,
};

static int gSettingsState = kSettingsState_Off;

typedef struct SettingEntry
{
	Byte*			valuePtr;
	const char*		label;
	void			(*callback)(void);
	unsigned int	numChoices;
	const char*		choices[MAX_CHOICES];
} SettingEntry;


static void Callback_EnterControls(void);
static void Callback_Difficulty(void);
static void Callback_Music(void);
static void Callback_Fullscreen(void);
static void Callback_VSync(void);
static void Callback_Antialiasing(void);
static void Callback_DebugInfo(void);
static void Callback_Done(void);

static char gMonitorNameBuffers[MAX_CHOICES][64];

static SettingEntry gSettingEntries[] =
{
	{nil							, "Configure Controls"	, Callback_EnterControls,	0,	{ NULL } },
	{nil							, nil					, nil,						0,  { NULL } },
	{&gGamePrefs.extreme			, "Game Difficulty"		, Callback_Difficulty,		2,	{ "Easy", "EXTREME!" } },
	{nil							, nil					, nil,						0,  { NULL } },
	{&gGamePrefs.music				, "Music"				, Callback_Music,			2,	{ "NO", "YES" }, },
	{&gGamePrefs.ambientSounds		, "Ambient Sounds"		, nil,						2,	{ "NO", "YES" }, },
	{nil							, nil					, nil,						0,  { NULL } },
	{&gGamePrefs.fullscreen			, "Fullscreen"			, Callback_Fullscreen,		2,	{ "NO", "YES" }, },
	{&gGamePrefs.vsync				, "V-Sync"				, Callback_VSync,			2,	{ "NO", "YES" }, },
	{&gGamePrefs.preferredDisplay	, "Preferred Display"	, Callback_Fullscreen,		1,	{ "Default" }, },
#if !(__APPLE__)
	{&gGamePrefs.antialiasingLevel	, "Antialiasing"		, Callback_Antialiasing,	4,	{ "NO", "MSAA 2x", "MSAA 4x", "MSAA 8x" }, },
#endif
	{nil							, nil					, nil,						0,  { NULL } },
	{&gGamePrefs.highQualityTextures, "Texture Filtering"	, nil,						2,	{ "NO", "YES" }, },
	{&gGamePrefs.canDoFog			, "Fog"					, nil,						2,	{ "NO", "YES" }, },
	{&gGamePrefs.nanosaurTeethFix	, "Nano's Dentist Is"	, nil				,		2,	{ "Extinct", "Alive" } },
//	{&gGamePrefs.shadows			, "Shadow Decals"		, nil,						2,	{ "NO", "YES" }, },
//	{&gGamePrefs.dust				, "Dust"				, nil,						2,	{ "NO", "YES" }, },
	{nil							, nil					, nil,						0,  { NULL } },
	{&gGamePrefs.mainMenuHelp		, "Show Help in Main Menu"		, nil,				2,	{ "NO", "YES" }, },
	{&gGamePrefs.debugInfoInTitleBar, "Debug Info in Title Bar",	Callback_DebugInfo,	2,  { "NO", "YES" } },
	{nil							, nil					, nil,						0,  { NULL } },
	{nil							, nil					, nil,						0,  { NULL } },
	{nil							, nil					, nil,						0,  { NULL } },
	{nil							, nil					, nil,						0,  { NULL } },
	{nil							, "Done"				, Callback_Done,			0,  { NULL } },
};

static const char* kInputNeedCaptions[NUM_CONTROL_NEEDS] =
{
	[kNeed_Forward		] = "Go Forward",
	[kNeed_Backward		] = "Go Backwards",
	[kNeed_TurnLeft		] = "Turn Left",
	[kNeed_TurnRight	] = "Turn Right",
	[kNeed_JetUp		] = "Jetpack Up",
	[kNeed_JetDown		] = "Jetpack Down",
//[kNeed_PrevWeapon	] = "Previous Weapon",
	[kNeed_AttackMode	] = "Next Weapon",
	[kNeed_Attack		] = "Shoot",
	[kNeed_PickUp		] = "Pick Up, Throw",
	[kNeed_Jump			] = "Jump, Double-Jump",
	[kNeed_CameraMode	] = "Camera Mode",
	[kNeed_CameraLeft	] = "Swivel Camera Left",
	[kNeed_CameraRight	] = "Swivel Camera Right",
	[kNeed_ZoomIn		] = "Zoom In",
	[kNeed_ZoomOut		] = "Zoom Out",
	[kNeed_ToggleGPS	] = "Toggle GPS",
	[kNeed_ToggleMusic	] = "Toggle Music",
	[kNeed_ToggleAmbient] = "Toggle Ambient Sounds",
	[kNeed_ToggleFullscreen] = "Toggle Fullscreen",
	[kNeed_UIUp			] = nil,
	[kNeed_UIDown		] = nil,
	[kNeed_UILeft		] = nil,
	[kNeed_UIRight		] = nil,
	[kNeed_UIConfirm	] = nil,
	[kNeed_UIBack		] = nil,
	[kNeed_UIPause		] = nil,
};

static const int numSettingEntries = sizeof(gSettingEntries) / sizeof(SettingEntry);

static bool needFullRender = false;



static unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}


static void Cycle(SettingEntry* entry, int delta)
{
	if (entry->valuePtr)
	{
		unsigned int value = (unsigned int) *entry->valuePtr;
		value = PositiveModulo(value + delta, entry->numChoices);
		*entry->valuePtr = value;
	}

	if (entry->callback)
	{
		entry->callback();
	}
}

static void Callback_EnterControls(void)
{
	needFullRender = true;
	gSettingsState = kSettingsState_ControlsPage;
}

static void Callback_Difficulty(void)
{
	SetProModeSettings(gGamePrefs.extreme);
}

static void Callback_Music(void)
{
	if (!gGamePrefs.music)
		KillSong();
	else
		PlaySong(1, true);
}

static void Callback_Fullscreen(void)
{
	SetFullscreenMode(true);
}

static void Callback_VSync(void)
{
	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);
}

static void Callback_Antialiasing(void)
{
	gShowAntialiasingWarning = true;
}

static void Callback_DebugInfo(void)
{
	if (!gGamePrefs.debugInfoInTitleBar)
		Callback_Difficulty();  // hack - this will set the window title
}

static void Callback_Done(void)
{
	needFullRender = true;
	switch (gSettingsState)
	{
		case kSettingsState_MainPage:
			gSettingsState = kSettingsState_Off;
			break;

		case kSettingsState_ControlsPage:
			gSettingsState = kSettingsState_MainPage;
			break;

		case kSettingsState_ControlsPage_AwaitingPress:
			gSettingsState = kSettingsState_ControlsPage;
			break;

		default:
			gSettingsState = kSettingsState_Off;
			break;
	}
}

static void DrawDebugInfo(void)
{
	char text[256];

	RGBForeColor2(kMutedColor);

	snprintf(text, sizeof(text), "Game version: %s", PROJECT_VERSION);
	MoveTo(640 - 8 - TextWidthC(text), 480 - 12 - 14 * 2);
	DrawStringC(text);

	snprintf(text, sizeof(text), "OpenGL %s", glGetString(GL_VERSION));
	MoveTo(640 - 8 - TextWidthC(text), 480 - 12 - 14 * 1);
	DrawStringC(text);

	snprintf(text, sizeof(text), "%s", glGetString(GL_RENDERER));
	MoveTo(640 - 8 - TextWidthC(text), 480 - 12 - 14 * 0);
	DrawStringC(text);
}

static void DrawRow(
		int y,
		bool rowIsSelected,
		float fluc,
		int numCols,
		const char** text,
		int selMask
)
{
	Rect rowRect;
	rowRect.top    = y;
	rowRect.bottom = y + 16;
	rowRect.left   = 0;
	rowRect.right  = GAME_VIEW_WIDTH;

	if (!needFullRender)
	{
		EraseRect(&rowRect);
	}

	if (numCols > 1)
	{
		RGBForeColor2(kMutedColor);
		MoveTo(kColumnX[0], rowRect.top + 12 - 1);
		LineTo(kColumnX[numCols-1] /*+ xOffset - 5*/, rowRect.top + 12 - 1);
	}

	for (int col = 0; col < numCols; col++)
	{
		bool isSelectedCol = rowIsSelected && (selMask & (1<<col));

		if (isSelectedCol)
		{
			int xOffset = 5.0f * fabsf(sinf(fluc));
			MoveTo(kColumnX[col] + xOffset, rowRect.top + 12);
			RGBForeColor2(kAccentColor);
			if (col > 0) DrawStringC("[ ");
			DrawStringC(text[col]);
			if (col > 0) DrawStringC(" ]");
		}
		else
		{
			MoveTo(kColumnX[col], rowRect.top + 12);
			RGBForeColor2(kFGColor);
			DrawStringC(text[col]);
		}
	}
}

static void DrawSettingsPage(void)
{
	static float fluc = 0.0;

	fluc += gFramesPerSecondFrac * 8;
	if (fluc > PI2) fluc = 0.0;

	SetPort(gCoverWindow);

	RGBBackColor2(kBGColor);

	if (needFullRender)
	{
		Rect r = gCoverWindow->portRect;
		EraseRect(&r);

		DrawDebugInfo();

		const char* title = PRO_MODE ? "NANOSAUR EXTREME SETTINGS" : "NANOSAUR SETTINGS";
		MoveTo(kColumnX[0], 50);
		RGBForeColor2(kTitleColor);
		DrawStringC(title);

		if (gShowAntialiasingWarning)
		{
			RGBForeColor2(0xFFFF9900);
			MoveTo(8, 480 - 12 - 14 * 1); DrawStringC("The new antialiasing level will take");
			MoveTo(8, 480 - 12 - 14 * 0); DrawStringC("effect when you restart the game.");
		}
	}

	for (int i = 0; i < numSettingEntries; i++)
	{
		bool isSelectedRow = (int) i == selectedEntry;
		if (!isSelectedRow && !needFullRender)
			continue;

		SettingEntry* entry = &gSettingEntries[i];
		if (!entry->label)
			continue;

		static const char* columnText[2];

		columnText[0] = entry->label;
		columnText[1] = "";

		if (entry->valuePtr)
		{
			unsigned int settingByte = (unsigned int) *entry->valuePtr;
			if (settingByte >= entry->numChoices)
				columnText[1] = "???";
			else
				columnText[1] = entry->choices[settingByte];
		}

		DrawRow(
				75 + i*16,
				isSelectedRow,
				fluc,
				entry->valuePtr? 2: 1,
				columnText,
				entry->valuePtr? 2: 1
				);
	}
}

static void DrawControlsPage(void)
{
	static const char* columnText[4] = {"", "", "", ""};


	bool blinkFlux = SDL_GetTicks() % 500 < 300;
	static float fluc = 0.0;

	fluc += gFramesPerSecondFrac * 8;
	if (fluc > PI2) fluc = 0.0;

	SetPort(gCoverWindow);

	RGBBackColor2(kBGColor);

	if (needFullRender)
	{
		Rect r = gCoverWindow->portRect;
		EraseRect(&r);

		const char* title = "CONFIGURE CONTROLS";
		MoveTo(kColumnX[0], 50);
		RGBForeColor2(kTitleColor);
		DrawStringC(title);
	}


	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		bool isSelectedRow = (int) i == controlsHighlightedRow;
		if (!isSelectedRow && !needFullRender)
			continue;

		KeyBinding* kb = &gGamePrefs.keys[i];

		columnText[0] = kInputNeedCaptions[i];
		for (int j = 0; j < 2; j++)
		{
			if (gSettingsState == kSettingsState_ControlsPage_AwaitingPress && isSelectedRow && controlsHighlightedColumn == j)
				columnText[j+1] = blinkFlux ? "PRESS!" : "           ";
			else if (kb->key[j])
				columnText[j+1] = SDL_GetScancodeName(kb->key[j]);
			else
				columnText[j+1] = "---";
		}

		DrawRow(
				75 + i*16,
				isSelectedRow,
				fluc,
				4,
				columnText,
				(1 << (1+ controlsHighlightedColumn))
		);
	}


	columnText[0] = "Reset to defaults";
	DrawRow(
			75 + (1 + NUM_REMAPPABLE_NEEDS+0)*16,
			controlsHighlightedRow == NUM_REMAPPABLE_NEEDS+0,
			fluc,
			1,
			columnText,
			0xFF
	);


	columnText[0] = "Done";
	DrawRow(
			75 + (1 + NUM_REMAPPABLE_NEEDS+1)*16,
			controlsHighlightedRow == NUM_REMAPPABLE_NEEDS+1,
			fluc,
			1,
			columnText,
			0xFF
	);
}

static void NavigateSettingEntriesVertically(int delta)
{
	do
	{
		selectedEntry += delta;
		selectedEntry = PositiveModulo(selectedEntry, (unsigned int)numSettingEntries);
	} while (nil == gSettingEntries[selectedEntry].label);
	needFullRender = true;
	PlayEffect(EFFECT_SELECT);
}

static void NavigateSettingsPage(void)
{
	if (GetNewNeedState(kNeed_UIBack))
		Callback_Done();

	if (GetNewNeedState(kNeed_UIUp))
		NavigateSettingEntriesVertically(-1);

	if (GetNewNeedState(kNeed_UIDown))
		NavigateSettingEntriesVertically(1);

	if (GetNewNeedState(kNeed_UIConfirm) ||
		(gSettingEntries[selectedEntry].valuePtr && (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIRight))))
	{
		int delta = GetNewNeedState(kNeed_UILeft) ? -1 : 1;
		Cycle(&gSettingEntries[selectedEntry], delta);
		PlayEffect(EFFECT_BLASTER);
		needFullRender = true;
	}
}


static int16_t* GetSelectedKeybindingKeyPtr(void)
{
	GAME_ASSERT(controlsHighlightedRow >= 0 && controlsHighlightedRow < NUM_REMAPPABLE_NEEDS);
	GAME_ASSERT(controlsHighlightedColumn >= 0 && controlsHighlightedColumn < KEYBINDING_MAX_KEYS);
	KeyBinding* kb = &gGamePrefs.keys[controlsHighlightedRow];
	return &kb->key[controlsHighlightedColumn];
}

static void UnbindScancodeFromAllRemappableInputNeeds(int16_t sdlScancode)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			if (gGamePrefs.keys[i].key[j] == sdlScancode)
				gGamePrefs.keys[i].key[j] = 0;
		}
	}
}


static void NavigateControlsPage(void)
{
	if (GetNewNeedState(kNeed_UIBack))
		Callback_Done();

	if (GetNewNeedState(kNeed_UIUp))
	{
		controlsHighlightedRow = PositiveModulo(controlsHighlightedRow - 1, (unsigned int)kNumKeybindingRows);
		PlayEffect(EFFECT_SELECT);
		needFullRender = true;
	}

	if (GetNewNeedState(kNeed_UIDown))
	{
		controlsHighlightedRow = PositiveModulo(controlsHighlightedRow + 1, (unsigned int)kNumKeybindingRows);
		PlayEffect(EFFECT_SELECT);
		needFullRender = true;
	}

	if (GetNewNeedState(kNeed_UILeft))
	{
		controlsHighlightedColumn = PositiveModulo(controlsHighlightedColumn - 1, 2);
		PlayEffect(EFFECT_SELECT);
		needFullRender = true;
	}

	if (GetNewNeedState(kNeed_UIRight))
	{
		controlsHighlightedColumn = PositiveModulo(controlsHighlightedColumn + 1, 2);
		PlayEffect(EFFECT_SELECT);
		needFullRender = true;
	}

	if (GetNewSDLKeyState(SDL_SCANCODE_DELETE) || GetNewSDLKeyState(SDL_SCANCODE_BACKSPACE))
	{
		*GetSelectedKeybindingKeyPtr() = 0;
		PlayEffect(EFFECT_CRUNCH);
		needFullRender = true;
	}

	if (GetNewSDLKeyState(SDL_SCANCODE_RETURN) && controlsHighlightedRow < NUM_REMAPPABLE_NEEDS)
	{
		gSettingsState = kSettingsState_ControlsPage_AwaitingPress;
	}

	if (GetNewNeedState(kNeed_UIConfirm))
	{
		if (controlsHighlightedRow == kKeybindingRow_Reset)
		{
			memcpy(gGamePrefs.keys, kDefaultKeyBindings, sizeof(kDefaultKeyBindings));
			_Static_assert(sizeof(kDefaultKeyBindings) == sizeof(gGamePrefs.keys), "size mismatch: default keybindings / prefs keybindings");
			PlayEffect_Parms(EFFECT_CRYSTAL, FULL_CHANNEL_VOLUME/2, kMiddleC-6);
			needFullRender = true;
		}
		else if (controlsHighlightedRow == kKeybindingRow_Done)
		{
			Callback_Done();
		}
	}
}



static void NavigateControlsPage_AwaitingPress(void)
{
	if (GetNewNeedState(kNeed_UIBack))
	{
		Callback_Done();
		return;
	}

	for (int i = 0; i < SDL_NUM_SCANCODES; i++)
	{
		if (GetNewSDLKeyState(i))
		{
			UnbindScancodeFromAllRemappableInputNeeds(i);
			*GetSelectedKeybindingKeyPtr() = i;
			gSettingsState = kSettingsState_ControlsPage;
			PlayEffect(EFFECT_POWPICKUP);
			needFullRender = true;
			break;
		}
	}
}

static void InitDisplayPref(void)
{
	// Init display pref
	for (size_t i = 0; i < sizeof(gSettingEntries)/sizeof(gSettingEntries[0]); i++)
	{
		SettingEntry* entry = &gSettingEntries[i];
		if (entry->valuePtr == &gGamePrefs.preferredDisplay)
		{
			entry->numChoices = SDL_GetNumVideoDisplays();

			if (entry->numChoices > MAX_CHOICES)
			{
				entry->numChoices = MAX_CHOICES;
			}

			for (size_t j = 0; j < entry->numChoices; j++)
			{
				snprintf(gMonitorNameBuffers[j], 64, "Monitor #%d", (int) j+1);
				entry->choices[j] = gMonitorNameBuffers[j];
			}

			for (size_t j = entry->numChoices; j < MAX_CHOICES; j++)
			{
				snprintf(gMonitorNameBuffers[j], 64, "Disconnected monitor #%d", (int) j+1);
			}

			break;
		}
	}
}

void DoSettingsScreen(void)
{
	Render_InitState();
	Render_Alloc2DCover(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);
	glClearColor(((kBGColor >> 16) & 0xFF) / 255.0f, ((kBGColor >> 8) & 0xFF) / 255.0f, (kBGColor & 0xFF) / 255.0f, 1.0f);

	needFullRender = true;
	gSettingsState = kSettingsState_FadeIn;

	InitDisplayPref();

	float gamma = 0;

	QD3D_CalcFramesPerSecond();

	while (gSettingsState != kSettingsState_Off)
	{
		UpdateInput();
		switch (gSettingsState)
		{
			case kSettingsState_FadeIn:
#if ALLOW_FADE
				gamma += gFramesPerSecondFrac * 100.0f / 0.5f;
				if (gamma > 100)
				{
					gamma = 100;
					gSettingsState = kSettingsState_MainPage;
				}
				Render_SetWindowGamma(gamma);
#else
				Render_SetWindowGamma(100);
				gSettingsState = kSettingsState_MainPage;
#endif
				DrawSettingsPage();
				needFullRender = false;
				break;

			case kSettingsState_MainPage:
				DrawSettingsPage();
				needFullRender = false;
				NavigateSettingsPage();
				break;

			case kSettingsState_ControlsPage:
				DrawControlsPage();
				needFullRender = false;
				NavigateControlsPage();
				break;

			case kSettingsState_ControlsPage_AwaitingPress:
				DrawControlsPage();
				needFullRender = false;
				NavigateControlsPage_AwaitingPress();
				break;

			default:
				break;
		}

		QD3D_CalcFramesPerSecond();
		DoSoundMaintenance();

		Render_StartFrame();
		Render_Draw2DCover(kCoverQuadFit);
		Render_EndFrame();

		SDL_GL_SwapWindow(gSDLWindow);
	}

	SavePrefs(&gGamePrefs);

	Render_FreezeFrameFadeOut();
	Render_Dispose2DCover();
}
