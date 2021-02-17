#include "structs.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include "qd3d_support.h"
#include "misc.h"
#include "window.h"

#include <SDL.h>
#include "version.h"

extern	WindowPtr				gCoverWindow;
extern	PrefsType				gGamePrefs;
extern	float					gFramesPerSecond,gFramesPerSecondFrac;
extern	SDL_Window*				gSDLWindow;
extern	const int				PRO_MODE;
extern	const KeyBinding		kDefaultKeyBindings[NUM_CONTROL_NEEDS];

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


enum
{
	kSettingsState_Off,
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
	const char*		choices[8];
} SettingEntry;


static void Callback_EnterControls(void);
static void Callback_Difficulty(void);
static void Callback_VSync(void);
static void Callback_Done(void);

static SettingEntry gSettingEntries[] =
{
	{nil							, "Configure Controls"	, Callback_EnterControls,	0,	{} },
	{nil							, nil					, nil,						0,  {} },
	{&gGamePrefs.extreme			, "Game Difficulty"		, Callback_Difficulty,		2,	{ "Easy", "EXTREME!" } },
	{nil							, nil					, nil,						0,  {} },
	{&gGamePrefs.fullscreen			, "Fullscreen"			, SetFullscreenMode,		2,	{ "NO", "YES" }, },
	{&gGamePrefs.vsync				, "V-Sync"				, Callback_VSync,			2,	{ "NO", "YES" }, },
	{nil							, nil					, nil,						0,  {} },
	{&gGamePrefs.highQualityTextures, "Texture Filtering"	, nil,						2,	{ "NO", "YES" }, },
	{&gGamePrefs.canDoFog			, "Fog"					, nil,						2,	{ "NO", "YES" }, },
//	{&gGamePrefs.shadows			, "Shadow Decals"		, nil,						2,	{ "NO", "YES" }, },
//	{&gGamePrefs.dust				, "Dust"				, nil,						2,	{ "NO", "YES" }, },
	{&gGamePrefs.softerLighting		, "Softer Lighting"		, nil,						2,	{ "NO", "YES" }, },
	{&gGamePrefs.mainMenuHelp		, "Main Menu Help"		, nil,						2,	{ "NO", "YES" }, },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, nil					, nil,						0,  {} },
	{nil							, "Done"				, Callback_Done,			0,  {} },
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
	[kNeed_RaiseVolume	] = "Raise Volume",
	[kNeed_LowerVolume	] = "Lower Volume",
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

static void Callback_VSync(void)
{
	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);
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
	RGBForeColor2(kMutedColor);
	MoveTo(8, 480 - 12 - 16 * 3);
	DrawStringC("Renderer: ");  DrawStringC((const char*)glGetString(GL_RENDERER));
	MoveTo(8, 480 - 12 - 16 * 2);
	DrawStringC("OpenGL: ");
	DrawStringC((const char*)glGetString(GL_VERSION));
	MoveTo(8, 480 - 12 - 16 * 1);
	DrawStringC("GLSL: ");
	DrawStringC((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

	MoveTo(8, 480 - 12 - 16 * 0);
	RGBForeColor2(kFGColor);
	DrawStringC("Game version: "); DrawStringC(PROJECT_VERSION);
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

//		DrawDebugInfo();

		const char* title = PRO_MODE ? "NANOSAUR EXTREME SETTINGS" : "NANOSAUR SETTINGS";
		MoveTo(kColumnX[0], 50);
		RGBForeColor2(kTitleColor);
		DrawStringC(title);
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
			if (settingByte > entry->numChoices)
				settingByte = 0;
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

	fluc += gFramesPerSecondFrac * 6;
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
			_Static_assert(sizeof(gGamePrefs.keys) == sizeof(gGamePrefs.keys));
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

void DoSettingsScreen(void)
{
	SDL_GLContext glContext = SDL_GL_CreateContext(gSDLWindow);
	GAME_ASSERT(glContext);

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);

	Render_InitState();
	Render_Alloc2DCover(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);
	glClearColor(((kBGColor >> 16) & 0xFF) / 255.0f, ((kBGColor >> 8) & 0xFF) / 255.0f, (kBGColor & 0xFF) / 255.0f, 1.0f);

	needFullRender = true;
	gSettingsState = kSettingsState_MainPage;

	while (gSettingsState != kSettingsState_Off)
	{
		UpdateInput();
		switch (gSettingsState)
		{
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
	SDL_GL_DeleteContext(glContext);
}
