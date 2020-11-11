#include "Pomme.h"

extern "C" {
#include "structs.h"
#include "input.h"
#include "file.h"
#include "sound2.h"
#include "qd3d_support.h"
}

#include <functional>
#include <vector>
#include "GamePatches.h"
#include "version.h"

extern "C" {
extern	WindowPtr				gCoverWindow;
extern	KeyMap					gNewKeys_Real;
extern	PrefsType				gGamePrefs;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	SDL_Window*				gSDLWindow;
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
		{&gGamePrefs.fullscreen         , "Fullscreen"        , SetFullscreenMode },
		{&gGamePrefs.vsync              , "V-Sync"            , []() { SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0); } },
		{&gGamePrefs.highQualityTextures, "Texture Filtering"   },
		{&gGamePrefs.canDoFog           , "Fog"                 },
		{&gGamePrefs.shadows            , "Shadow Decals"       },
//		{&gGamePrefs.dust               , "Dust"                },
		{&gGamePrefs.allowGammaFade     , "Allow Gamma Fade"    },
		{&gGamePrefs.softerLighting     , "Softer Lighting"     },
		{&gGamePrefs.interpolationStyle , "Face Shading"      , nullptr, {"Flat", "Per-Pixel"} },
		{&gGamePrefs.opaqueWater        , "Water Alpha"       , nullptr, {"Translucent", "Opaque"}},
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
			const char* title = "NANOSAUR SETTINGS";
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

	ExclusiveOpenGLMode_Begin();

	while (1)
	{
		ReadKeyboard();

		if (GetNewKeyState(kKey_Pause)) break;
		
		if (GetNewKeyState(kKey_Forward))   { selectedEntry--; needFullRender = true; PlayEffect(EFFECT_SELECT); }
		if (GetNewKeyState(kKey_Backward))  { selectedEntry++; needFullRender = true; PlayEffect(EFFECT_SELECT); }
		selectedEntry = PositiveModulo(selectedEntry, (unsigned int)settings.size());

		if (GetNewKeyState_Real(KEY_RETURN) || GetNewKeyState(kKey_Attack) || GetNewKeyState(kKey_TurnRight) || GetNewKeyState(kKey_TurnLeft))
		{
			settings[selectedEntry].Cycle(GetNewKeyState(kKey_TurnLeft) ? -1 : 1);
			PlayEffect(EFFECT_BLASTER);
		}
		RenderQualityDialog();

		QD3D_CalcFramesPerSecond();
		DoSoundMaintenance();
		RenderBackdropQuad(BACKDROP_FIT);

		DoSDLMaintenance();
	}

	SavePrefs(&gGamePrefs);

	ExclusiveOpenGLMode_End();
}
