#include "Pomme.h"
#include <QD3DMath.h>
#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
#include "sound2.h"
#include "structs.h"
#include "input.h"
#include "qd3d_support.h"
#include "movie.h" // PlayAMovie
#include "misc.h" // DrawPictureToScreen
#include "windows_nano.h" // GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT
}

#include "GamePatches.h"

#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "PommeVideo.h"
#include "Video/Cinepak.h"

#include <fstream>

extern "C" {
extern long gNodesDrawn;
extern long gTrianglesDrawn;
extern long gMeshesDrawn;
extern SDL_Window* gSDLWindow;
extern float gFramesPerSecond;
extern PrefsType gGamePrefs;
extern short gPrefsFolderVRefNum;
extern long gPrefsFolderDirID;
}

OSErr MakePrefsFSSpec(const char* prefFileName, FSSpec* spec)
{
	static Boolean checkedOnce = false;
	constexpr const char* PREFS_FOLDER = "Nanosaur";

	if (!checkedOnce)
	{
		checkedOnce = true;

		OSErr iErr = FindFolder(
			kOnSystemDisk,
			kPreferencesFolderType,
			kDontCreateFolder,
			&gPrefsFolderVRefNum,
			&gPrefsFolderDirID);

		if (iErr != noErr)
			DoAlert("Warning: Cannot locate Preferences folder.");

		long createdDirID;
		DirCreate(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FOLDER, &createdDirID);
	}

	char name[256];
	snprintf(name, 256, ":%s:%s", PREFS_FOLDER, prefFileName);
	return FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, name, spec);
}

char GetTypedKey(void)
{
	static const char KCHR_US_LOWER[] = "asdfhgzxcv\0bqweryt123465=97-80]ou[ip\rlj'k;\\,/nm.\t `\x08\0\x1B\0\0\0\0\0\0\0\0\0\0\0.\0*\0+\0\0\0\0\0/\n\0-\0\0=01234567\x0089\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x7F\0\0\0\0\0\x1C\x1D\x1F\x1E";
	static const char KCHR_US_UPPER[] = "ASDFHGZXCV\0BQWERYT!@#$^%+(&_*)}OU{IP\rLJ\"K:|<?NM>\t ~\x08\0\x1B\0\0\0\0\0\0\0\0\0\0\0.\0*\0+\0\0\0\0\0/\n\0-\0\0=01234567\x0089\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x7F\0\0\0\0\0\x1C\x1D\x1F\x1E";

	ReadKeyboard_Real();

	for (size_t i = 0; i < sizeof(KCHR_US_LOWER); i++)
	{
		if (!GetNewKeyState_Real(i))
			continue;

		char c = '\0';

		if (GetKeyState_Real(kVK_Shift) || GetKeyState_Real(kVK_RightShift))
			c = KCHR_US_UPPER[i];
		else
			c = KCHR_US_LOWER[i];

		if (c == '\0')
			continue;

		return c;
	}

	return 0;
}


/**************** DRAW PICTURE TO SCREEN ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
//
//
// INPUT: myFSSpec = spec of image file
//

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y)
{
	short refNum;

	OSErr error = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (noErr != error)
	{
		TODO2("Couldn't open picture: " << spec->cName);
		return error;
	}

	auto& stream = Pomme::Files::GetStream(refNum);
	auto pict = Pomme::Graphics::ReadPICT(stream, true);
	FSClose(refNum);

	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(Pomme::Graphics::GetScreenPort());
	Pomme::Graphics::DrawARGBPixmap(x, y, pict);
	SetPort(oldPort);

	return noErr;
}


//-----------------------------------------------------------------------------
// Movie


void PlayAMovie(FSSpec* spec)
{
	KillSong();

	Pomme::Video::Movie movie;

	short refNum;
	FSpOpenDF(spec, fsRdPerm, &refNum);
	{
		auto& movieFileStream = Pomme::Files::GetStream(refNum);
		movie = Pomme::Video::ReadMoov(movieFileStream);
	}
	FSClose(refNum);

	// -----------

	CinepakContext cinepak(movie.width, movie.height);

	ExclusiveOpenGLMode_Begin();
	SetBackdropClipRegion(movie.width, movie.height);
	ClearBackdrop(0xFF000000);

	movie.audioStream.Play();

	bool movieAbortedByUser = false;

	while (!movieAbortedByUser && !movie.videoFrames.empty())
	{
		unsigned int startTicks = SDL_GetTicks();

		{
			const auto& frame = movie.videoFrames.front();
			cinepak.DecodeFrame(frame.data(), frame.size());
			movie.videoFrames.pop();
		}

		int pitch = GAME_VIEW_WIDTH;

		UInt32* backdropPix = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(Pomme::Graphics::GetScreenPort()));

		// RGB888 to ARGB8888
		for (int y = 0; y < movie.height; y++)
		{
			const uint8_t* in = cinepak.frame_data0 + cinepak.frame_linesize0 * y;
			uint8_t* out = (uint8_t*) (backdropPix + pitch * y);
			for (int x = 0; x < movie.width; x++)
			{
				*out++ = 0xFF;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
			}
		}

		Rect damage = {0, 0, (SInt16) movie.height, (SInt16) movie.width};
		DamagePortRegion(&damage);
		RenderBackdropQuad(BACKDROP_FIT | BACKDROP_CLEAR_BLACK);
		DoSDLMaintenance();

		unsigned int endTicks = SDL_GetTicks();
		int diffTicks = endTicks - startTicks;
		int waitTicks = 1000 / movie.videoFrameRate - diffTicks;
		if (waitTicks > 0)
			SDL_Delay(waitTicks);

		ReadKeyboard();
		movieAbortedByUser = GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState_Real(KEY_ESC);
	}

	// Freeze on last frame while audio track is still playing (Lose.mov requires this)
	while (!movieAbortedByUser && movie.audioStream.GetState() == cmixer::CM_STATE_PLAYING)
	{
		DoSDLMaintenance();
		SDL_Delay(100);
		ReadKeyboard();
		movieAbortedByUser = GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState_Real(KEY_ESC);
	}

	movie.audioStream.Stop();

	SetBackdropClipRegion(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);
	ExclusiveOpenGLMode_End();
}

//-----------------------------------------------------------------------------
// Fade

void DumpGLPixels(const char* outFN)
{
	int width = 640;
	int height = 480;
	SDL_GetWindowSize(gSDLWindow, &width, &height);
	auto buf = std::vector<char>(width * height * 3);
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buf.data());

	std::ofstream out(outFN, std::ios::out | std::ios::binary);
	uint16_t TGAhead[] = {0, 2, 0, 0, 0, 0, (uint16_t) width, (uint16_t) height, 24};
	out.write(reinterpret_cast<char*>(&TGAhead), sizeof(TGAhead));
	out.write(buf.data(), buf.size());

	printf("Screenshot saved to %s\n", outFN);
}

//-----------------------------------------------------------------------------
// SDL maintenance

static struct
{
	UInt32 lastUpdateAt = 0;
	const UInt32 updateInterval = 250;
	UInt32 frameAccumulator = 0;
	char titleBuffer[1024];
} debugText;

void DoSDLMaintenance()
{
	static int holdFramerateCap = 0;

	// Cap frame rate.
	if (gFramesPerSecond > 200 || holdFramerateCap > 0)
	{
		SDL_Delay(5);
		// Keep framerate cap for a while to avoid jitter in game physics
		holdFramerateCap = 10;
	}
	else
	{
		holdFramerateCap--;
	}

#if _DEBUG
	UInt32 now = SDL_GetTicks();
	UInt32 ticksElapsed = now - debugText.lastUpdateAt;
	if (ticksElapsed >= debugText.updateInterval) {
		float fps = 1000 * debugText.frameAccumulator / (float)ticksElapsed;
		snprintf(debugText.titleBuffer, 1024, "nsaur - %d fps - %ld nodes drawn - %ld tris drawn - %ld meshes drawn", (int)round(fps), gNodesDrawn, gTrianglesDrawn, gMeshesDrawn);
		SDL_SetWindowTitle(gSDLWindow, debugText.titleBuffer);
		debugText.frameAccumulator = 0;
		debugText.lastUpdateAt = now;
	}
	debugText.frameAccumulator++;
#endif

	if (GetNewKeyState(kKey_ToggleFullscreen))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode();
	}

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			throw Pomme::QuitRequest();
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
				throw Pomme::QuitRequest();
				break;

			case SDL_WINDOWEVENT_RESIZED:
				QD3D_OnWindowResized(event.window.data1, event.window.data2);
				break;
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// SDL maintenance

void SetFullscreenMode()
{
	SDL_SetWindowFullscreen(
			gSDLWindow,
			gGamePrefs.fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);

	// Ensure the clipping pane gets resized properly after switching in or out of fullscreen mode
	int width, height;
	SDL_GetWindowSize(gSDLWindow, &width, &height);
	QD3D_OnWindowResized(width, height);

	SDL_ShowCursor(gGamePrefs.fullscreen? 0: 1);
}
