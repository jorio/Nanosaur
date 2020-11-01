#include "Pomme.h"
#include "PommeInit.h"
#include "PommeGraphics.h"
#include "PommeFiles.h"
#include "pomme/Files/ArchiveVolume.h"
#include "GamePatches.h"
#include "FindGameData.h"

#include <iostream>
#include <Quesa.h>
#include <SDL.h>

extern "C"
{
// bare minimum from Windows.c to satisfy externs in game code
WindowPtr gCoverWindow = nullptr;
UInt32* gCoverWindowPixPtr = nullptr;

extern FSSpec gDataSpec;

void GameMain(void);
}

int CommonMain(int argc, const char** argv)
{
	// Start our "machine"
	Pomme::Init("Nanosaur");

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Clear window
	ExclusiveOpenGLMode_Begin();
	ClearBackdrop(0xFFA5A5A5);
	RenderBackdropQuad(BACKDROP_FILL);
	ExclusiveOpenGLMode_End();

#if EMBED_DATA
	FindEmbeddedGameData(&gDataSpec);
#else
	SetGameDataPathFromArgs(argc, argv);
	if (!FindGameData(&gDataSpec))
	{
		return 1;
	}

	Pomme::Graphics::SetWindowIconFromIcl8Resource(128);
#endif

	// Initialize Quesa
	auto qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
	{
		throw std::runtime_error("Couldn't initialize Quesa.");
	}

	// Start the game
	try
	{
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}

#if !(EMBED_DATA)
	WriteDataLocationSetting();
#endif

	// Clean up
	Pomme::Shutdown();

	return 0;
}

int main(int argc, char** argv)
{
	std::string uncaught;

	try
	{
		return CommonMain(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex)
	{
		uncaught = ex.what();
	}
	catch (...)
	{
		uncaught = "unknown";
	}

	std::cerr << "Uncaught exception: " << uncaught << "\n";
	SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
	return 1;
}
