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
#include "window.h" // GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT
#include "renderer.h"
#include "version.h"
}

#include "GamePatches.h"

#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "PommeVideo.h"
#include "Video/Cinepak.h"

#include <fstream>

extern "C" {
extern SDL_Window* gSDLWindow;
extern PrefsType gGamePrefs;
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

	SDL_GLContext glContext = SDL_GL_CreateContext(gSDLWindow);
	GAME_ASSERT(glContext);

	SDL_GL_SetSwapInterval(gGamePrefs.vsync ? 1 : 0);
	
	Render_InitState();
	Render_Alloc2DCover(movie.width, movie.height);
	glClearColor(0,0,0,1);

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

		Render_StartFrame();
		Render_Draw2DCover(kCoverQuadLetterbox);
		SDL_GL_SwapWindow(gSDLWindow);

		unsigned int endTicks = SDL_GetTicks();
		int diffTicks = endTicks - startTicks;
		int waitTicks = 1000 / movie.videoFrameRate - diffTicks;
		if (waitTicks > 0)
			SDL_Delay(waitTicks);

		UpdateInput();
		movieAbortedByUser = UserWantsOut();
	}

	// Freeze on last frame while audio track is still playing (Lose.mov requires this)
	while (!movieAbortedByUser && movie.audioStream.GetState() == cmixer::CM_STATE_PLAYING)
	{
		SDL_Delay(100);
		UpdateInput();
		movieAbortedByUser = UserWantsOut();
	}

	movie.audioStream.Stop();

	Render_FreezeFrameFadeOut();

	Render_Dispose2DCover();
	SDL_GL_DeleteContext(glContext);
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
