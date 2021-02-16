#include "Pomme.h"

#include "structs.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include "qd3d_support.h"
#include "terrain.h"
#include "misc.h"
#include "window.h"

#include <SDL.h>
#include "version.h"

extern	WindowPtr				gCoverWindow;
extern	PrefsType				gGamePrefs;
extern	float					gFramesPerSecond,gFramesPerSecondFrac;
extern	SDL_Window*				gSDLWindow;
extern	const int				PRO_MODE;

static const int column1X = 320-256/2;
static const int column2X = column1X + 256;

static int selectedEntry = 0;

static unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

typedef struct SettingEntry
{
	Byte*			valuePtr;
	const char*		label;
	void			(*callback)(void);
	unsigned int	numChoices;
	const char*		choices[8];
} SettingEntry;


static void Cycle(SettingEntry* entry, int delta)
{
	unsigned int value = (unsigned int) *entry->valuePtr;
	value = PositiveModulo(value + delta, entry->numChoices);
	*entry->valuePtr = value;
	if (entry->callback)
	{
		entry->callback();
	}
}

static void Callback_Difficulty(void)
{
	SetProModeSettings(gGamePrefs.extreme);
}

static void Callback_VSync(void)
{
	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);
}

static SettingEntry gSettingEntries[] =
{
	{&gGamePrefs.extreme			, "Game Difficulty"		, Callback_Difficulty,	2,	{ "Easy", "EXTREME!" } },
	{&gGamePrefs.fullscreen			, "Fullscreen"			, SetFullscreenMode,	2,	{ "NO", "YES" }, },
	{&gGamePrefs.vsync				, "V-Sync"				, Callback_VSync,		2,	{ "NO", "YES" }, },
	{&gGamePrefs.highQualityTextures, "Texture Filtering"	, nil,					2,	{ "NO", "YES" }, },
	{&gGamePrefs.canDoFog			, "Fog"					, nil,					2,	{ "NO", "YES" }, },
//	{&gGamePrefs.shadows			, "Shadow Decals"		, nil,					2,	{ "NO", "YES" }, },
//	{&gGamePrefs.dust				, "Dust"				, nil,					2,	{ "NO", "YES" }, },
	{&gGamePrefs.softerLighting		, "Softer Lighting"		, nil,					2,	{ "NO", "YES" }, },
	{&gGamePrefs.mainMenuHelp		, "Main Menu Help"		, nil,					2,	{ "NO", "YES" }, },
};

static const int numSettingEntries = sizeof(gSettingEntries) / sizeof(SettingEntry);

static bool needFullRender = false;

static void RenderQualityDialog()
{
	static float fluc = 0.0;

	fluc += gFramesPerSecondFrac * 6;
	if (fluc > PI2) fluc = 0.0;

	SetPort(gCoverWindow);

	RGBBackColor2(0xA5A5A5);

	if (needFullRender)
	{
		Rect r = gCoverWindow->portRect;
		EraseRect(&r);

		RGBForeColor2(0x808080);
		MoveTo(8, 480 - 12 - 16 * 3);
		DrawStringC("Renderer: ");  DrawStringC((const char*)glGetString(GL_RENDERER));
		MoveTo(8, 480 - 12 - 16 * 2);
		DrawStringC("OpenGL: ");
		DrawStringC((const char*)glGetString(GL_VERSION));
		MoveTo(8, 480 - 12 - 16 * 1);
		DrawStringC("GLSL: ");
		DrawStringC((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

		MoveTo(8, 480 - 12 - 16 * 0);
		RGBForeColor2(0x404040);
		DrawStringC("Game version: "); DrawStringC(PROJECT_VERSION);

		ForeColor(blackColor);
		{
			const char* title = PRO_MODE ? "NANOSAUR EXTREME SETTINGS" : "NANOSAUR SETTINGS";
			short titleWidth = TextWidthC(title);
			MoveTo(320 - (titleWidth / 2), 100);
			DrawStringC(title);
		}
	}

	Rect rowRect;
	rowRect.top    = 200;
	rowRect.bottom = 200 + 16;
	rowRect.left   = column1X;
	rowRect.right  = column2X;

	for (int i = 0; i < numSettingEntries; i++)
	{
		SettingEntry* entry = &gSettingEntries[i];
		bool isSelected = (int) i == selectedEntry;

		rowRect.top    = (SInt16)(200 + i * 16);
		rowRect.bottom = (SInt16)(rowRect.top + 16);

		int xOffset = 0;

		if (isSelected)
		{
			xOffset = 10.0 * fabs(sin(fluc));
			EraseRect(&rowRect);
		}
		else if (!needFullRender)
		{
			continue;
		}

		RGBForeColor2(0x808080);
		MoveTo(xOffset + column1X, rowRect.top + 12 - 1);
		LineTo(column2X - 5, rowRect.top + 12 - 1);

		RGBForeColor2(isSelected ? 0x108020 : 0x000000);

		MoveTo(xOffset + column1X, rowRect.top + 12);
		DrawStringC(entry->label);

		unsigned int settingByte = (unsigned int) *entry->valuePtr;
		if (settingByte > entry->numChoices)
		{
			settingByte = 0;
		}

		const char* choice = entry->choices[settingByte];
		short choiceWidth = TextWidthC(choice);
		MoveTo(column2X - choiceWidth, rowRect.top + 12);
		DrawStringC(choice);
	}

	needFullRender = false;
}

void DoQualityDialog()
{
	needFullRender = true;

	SDL_GLContext glContext = SDL_GL_CreateContext(gSDLWindow);
	GAME_ASSERT(glContext);
	Render_InitState();
	Render_Alloc2DCover(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);
	glClearColor(.6471f, .6471f, .6471f, 1.0f);

	while (1)
	{
		UpdateInput();

		if (GetNewNeedState(kNeed_UIBack)) break;
		
		if (GetNewNeedState(kNeed_UIUp))   { selectedEntry--; needFullRender = true; PlayEffect(EFFECT_SELECT); }
		if (GetNewNeedState(kNeed_UIDown)) { selectedEntry++; needFullRender = true; PlayEffect(EFFECT_SELECT); }
		selectedEntry = PositiveModulo(selectedEntry, (unsigned int)numSettingEntries);

		if (GetNewNeedState(kNeed_UIConfirm) || GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIRight))
		{
			int delta = GetNewNeedState(kNeed_UILeft) ? -1 : 1;
			Cycle(&gSettingEntries[selectedEntry], delta);
			PlayEffect(EFFECT_BLASTER);
			needFullRender = true;
		}
		RenderQualityDialog();

		QD3D_CalcFramesPerSecond();
		DoSoundMaintenance();

		Render_StartFrame();
		Render_Draw2DCover(kCoverQuadFit);

		SDL_GL_SwapWindow(gSDLWindow);
	}

	SavePrefs(&gGamePrefs);

	Render_FreezeFrameFadeOut();

	Render_Dispose2DCover();
	SDL_GL_DeleteContext(glContext);
}
