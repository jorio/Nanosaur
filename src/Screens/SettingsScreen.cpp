#include "Pomme.h"

extern "C" {
#include "structs.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include "qd3d_support.h"
#include "terrain.h"
#include "misc.h"
#include "window.h"
}

#include <SDL.h>
#include <functional>
#include <vector>
#include "version.h"

extern "C" {
extern	WindowPtr				gCoverWindow;
extern	PrefsType				gGamePrefs;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	SDL_Window*				gSDLWindow;
extern	const int				PRO_MODE;
}

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

struct SettingEntry
{
	Byte* ptr;
	const char* label;
	std::function<void()> callback = nullptr;
	std::vector<const char*> choices = {"NO", "YES"};

	void Cycle(int delta)
	{
		unsigned int value = (unsigned int) *ptr;
		value = PositiveModulo(value + delta, (unsigned int) choices.size());
		*ptr = value;
		if (callback)
		{
			callback();
		}
	}
};

std::vector<SettingEntry> settings = {
		{&gGamePrefs.extreme            , "Game Difficulty"   , []() { SetProModeSettings(gGamePrefs.extreme); }, { "Easy", "EXTREME!" } },
		{&gGamePrefs.fullscreen         , "Fullscreen"        , SetFullscreenMode },
		{&gGamePrefs.vsync              , "V-Sync"            , []() { SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0); } },
		{&gGamePrefs.highQualityTextures, "Texture Filtering"   },
		{&gGamePrefs.canDoFog           , "Fog"                 },
//		{&gGamePrefs.shadows            , "Shadow Decals"       },
//		{&gGamePrefs.dust               , "Dust"                },
		{&gGamePrefs.softerLighting     , "Softer Lighting"     },
		{&gGamePrefs.mainMenuHelp       , "Main Menu Help"      },
};

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

	for (size_t i = 0; i < settings.size(); i++)
	{
		auto& setting = settings[i];
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
		DrawStringC(settings[i].label);

		unsigned int settingByte = (unsigned int) *setting.ptr;
		if (settingByte > settings[i].choices.size())
		{
			settingByte = 0;
		}

		auto choice = setting.choices[settingByte];
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
		selectedEntry = PositiveModulo(selectedEntry, (unsigned int)settings.size());

		if (GetNewNeedState(kNeed_UIConfirm) || GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIRight))
		{
			settings[selectedEntry].Cycle(GetNewNeedState(kNeed_UILeft) ? -1 : 1);
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
