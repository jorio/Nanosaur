#include "Pomme.h"

extern "C" {
#include <game/Structs.h>
#include <game/input.h>
#include <game/file.h>
#include <game/sound2.h>
#include <game/qd3d_support.h>
}

#include <functional>
#include <vector>
#include "GamePatches.h"

extern "C" {
extern	WindowPtr				gCoverWindow;
extern	KeyMap					gNewKeys_Real;
extern	PrefsType				gGamePrefs;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;
extern	SDL_Window*				gSDLWindow;
}

static const RGBColor backgroundColor = {0xA500,0xA500,0xA500};
static const RGBColor foregroundColor = {0x0000,0x0000,0x0000};
static const RGBColor selectedForegroundColor1 = {0x1000,0x8000,0x2000};
static const RGBColor lineColor       = {0x8000,0x8000,0x8000};
static const int column1X = 320-256/2;
static const int column2X = column1X + 256;

static size_t selectedEntry = 0;

static unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int)m;
	if (mod < 0) {
		mod += m;
	}
	return mod;
}

struct SettingEntry
{
	Byte*       ptr;
	const char* label;
	std::function<void()> callback = nullptr;
	std::vector<const char*> choices = {"NO", "YES"};
	
	void Cycle(int delta)
	{
		unsigned int value = (unsigned int)*ptr;
		value = PositiveModulo(value + delta, choices.size());
		*ptr = value;
		if (callback) {
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
		{&gGamePrefs.mainMenuHelp       , "Main Menu Help"      },
};

static bool needFullRender = false;

static void RenderQualityDialog()
{
	static float fluc = 0.0;

	fluc += gFramesPerSecondFrac * 6;
	if (fluc > PI2) fluc = 0.0;

	SetPort(gCoverWindow);

	RGBBackColor(&backgroundColor);
	
	if (needFullRender) {
		Rect r = gCoverWindow->portRect;
		EraseRect(&r);

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

	for (size_t i = 0; i < settings.size(); i++) {
		auto& setting = settings[i];

		rowRect.top    = 200 + i * 16;
		rowRect.bottom = rowRect.top + 16;

		int xOffset = 0;

		if (i == selectedEntry) {
			xOffset = 10.0 * fabs(sin(fluc));
			EraseRect(&rowRect);
		}
		else if (!needFullRender) {
			continue;
		}

		RGBForeColor(&lineColor);
		MoveTo(xOffset + column1X, rowRect.top + 12 - 1);
		LineTo(column2X-5, rowRect.top + 12 - 1);
		
		RGBForeColor(selectedEntry == i? &selectedForegroundColor1: &foregroundColor);

		MoveTo(xOffset + column1X, rowRect.top + 12);
		DrawStringC(settings[i].label);

		unsigned int settingByte = (unsigned int)*setting.ptr;
		if (settingByte < 0) settingByte = 0;
		if (settingByte > settings[i].choices.size()) settingByte = 0;

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

	while(1) {
		ReadKeyboard();
		
		if (GetNewKeyState(kKey_Pause)) break;
		
		if (GetNewKeyState(kKey_Forward))   { selectedEntry--; needFullRender = true; PlayEffect(EFFECT_SELECT); }
		if (GetNewKeyState(kKey_Backward))  { selectedEntry++; needFullRender = true; PlayEffect(EFFECT_SELECT); }
		selectedEntry = PositiveModulo(selectedEntry, settings.size());
		
		if (GetNewKeyState_Real(KEY_RETURN) || GetNewKeyState(kKey_Attack) || GetNewKeyState(kKey_TurnRight) || GetNewKeyState(kKey_TurnLeft))    {
			settings[selectedEntry].Cycle(GetNewKeyState(kKey_TurnLeft)? -1: 1);
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
