#include <game/Structs.h>
#include <game/input.h>
#include <game/file.h>
#include <game/sound2.h>
#include <game/qd3d_support.h>
#include "GamePatches.h"

extern	WindowPtr				gCoverWindow;
extern	KeyMap					gNewKeys_Real;
extern	PrefsType				gGamePrefs;
extern	float			gFramesPerSecond,gFramesPerSecondFrac;

static const RGBColor backgroundColor = {0xA000,0xA000,0xA000};
static const RGBColor foregroundColor = {0x0000,0x0000,0x0000};
static const RGBColor selectedForegroundColor1 = {0x1000,0x8000,0x2000};
static const RGBColor lineColor       = {0x8000,0x8000,0x8000};
static const int column1X = 320-256/2;
static const int column2X = column1X + 256;

static int selectedEntry = 0;

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
	std::vector<const char*> choices;
	
	void Cycle(int delta)
	{
		unsigned int value = (unsigned int)*ptr;
		value = PositiveModulo(value + delta, choices.size());
		*ptr = value;
	} 
};

std::vector<SettingEntry> settings = {
		{&gGamePrefs.highQualityTextures    ,   "Texture Filtering"         ,   {"OFF", "ON"}},
		{&gGamePrefs.canDoFog               ,   "Fog"                       ,   {"OFF", "ON"}},
		{&gGamePrefs.shadows                ,   "Shadow Decals"             ,   {"OFF", "ON"}},
		{&gGamePrefs.dust                   ,   "Dust"                      ,   {"OFF", "ON"}},
		{&gGamePrefs.allowGammaFade         ,   "Allow Gamma Fade"          ,   {"OFF", "ON"}},
		{&gGamePrefs.softerLighting         ,   "Softer Lighting"           ,   {"OFF", "ON"}},
};

static void RenderQualityDialog()
{
	static float fluc = 0.0;

	fluc += gFramesPerSecondFrac * 6;
	if (fluc > PI2) fluc = 0.0;

	SetPort(gCoverWindow);

	Rect r = gCoverWindow->portRect;
	RGBBackColor(&backgroundColor);
	EraseRect(&r);

	ForeColor(blackColor);
	{
		Str255 title = "NANOSAUR SETTINGS";
		short titleWidth = TextWidth(&title[1], 0, title[0]);
		MoveTo(320 - (titleWidth / 2), 100);
		DrawString(title);
	}

	for (int i = 0; i < settings.size(); i++) {
		auto& setting = settings[i];
		int y = 200 + i * 16;
		
		int xOffset = 10.0 * fabs(sin(fluc));
		if (i != selectedEntry) xOffset = 0;
		

		RGBForeColor(&lineColor);
		MoveTo(xOffset + column1X, y-1);
		LineTo(column2X-5, y-1);
		
		RGBForeColor(selectedEntry == i? &selectedForegroundColor1: &foregroundColor);

		MoveTo(xOffset + column1X, y);
		DrawString(settings[i].label);

		int settingByte = (unsigned int)*setting.ptr;
		if (settingByte < 0) settingByte = 0;
		if (settingByte > settings[i].choices.size()) settingByte = 0;

		auto choice = setting.choices[settingByte];
		short choiceWidth = TextWidth(choice, 0, strlen(choice));
		MoveTo(column2X - choiceWidth, y);
		DrawString(choice);
	}
}

void DoQualityDialog()
{
	ExclusiveOpenGLMode_Begin();

	RenderQualityDialog();

	while(1) {
		ReadKeyboard();
		
		if (GetNewKeyState(kKey_Pause)) break;
		
		if (GetNewKeyState(kKey_Forward))   { selectedEntry--; PlayEffect(EFFECT_SELECT); }
		if (GetNewKeyState(kKey_Backward))  { selectedEntry++; PlayEffect(EFFECT_SELECT); }
		selectedEntry = PositiveModulo(selectedEntry, settings.size());
		
		if (GetNewKeyState_Real(KEY_RETURN) || GetNewKeyState(kKey_Attack) || GetNewKeyState(kKey_TurnRight) || GetNewKeyState(kKey_TurnLeft))    {
			settings[selectedEntry].Cycle(GetNewKeyState(kKey_TurnLeft)? -1: 1);
			PlayEffect(EFFECT_BLASTER);
		}
		RenderQualityDialog();

		QD3D_CalcFramesPerSecond();
		DoSoundMaintenance();
		RenderBackdropQuad();
		
		DoSDLMaintenance();
	}

	SavePrefs(&gGamePrefs);

	ExclusiveOpenGLMode_End();
}
