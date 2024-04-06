#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"

#include <SDL.h>

#include <iostream>

extern "C"
{
	// bare minimum from Window.c to satisfy externs in game code
	SDL_Window* gSDLWindow = nullptr;
	WindowPtr gCoverWindow = nullptr;
	UInt32* gBackdropPixels = nullptr;

	// Lets the game know where to find its asset files
	extern FSSpec gDataSpec;

	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
#if 0 //_WIN32
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif

	#include "game.h"
}

static fs::path FindGameData(const char* executablePath)
{
	fs::path dataPath;

	int attemptNum = 0;

#if !(__APPLE__)
	attemptNum++;		// skip macOS special case #0
#endif

	if (!executablePath)
		attemptNum = 2;

tryAgain:
	switch (attemptNum)
	{
		case 0:			// special case for macOS app bundles
			dataPath = executablePath;
			dataPath = dataPath.parent_path().parent_path() / "Resources";
			break;

		case 1:
			dataPath = executablePath;
			dataPath = dataPath.parent_path() / "Data";
			break;

		case 2:
			dataPath = "Data";
			break;

		default:
			throw std::runtime_error("Couldn't find the Data folder.");
	}

	attemptNum++;

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	FSSpec dummySpec;
	if (noErr != FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Diloph.3dmf", &dummySpec))
	{
		goto tryAgain;
	}

	return dataPath;
}

void Boot(int argc, const char** argv)
{
	(void) argc;
	(void) argv;

	// Start our "machine"
	Pomme::Init();

	// Load game prefs before starting
	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);

retry:
	// Initialize SDL video subsystem
	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't init SDL video subsystem: " + std::string(SDL_GetError()));
	}

	if (gGamePrefs.preferredDisplay >= SDL_GetNumVideoDisplays())
	{
		gGamePrefs.preferredDisplay = 0;
	}

	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	if (gGamePrefs.antialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gGamePrefs.antialiasingLevel);
	}

	// Prepare window dimensions
	int display = gGamePrefs.preferredDisplay;
	float screenFillRatio = 2.0f / 3.0f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = GAME_VIEW_WIDTH, .h = GAME_VIEW_HEIGHT };
	SDL_GetDisplayUsableBounds(display, &displayBounds);
	TQ3Vector2D fitted = FitRectKeepAR(GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, displayBounds.w, displayBounds.h);
	int initialWidth  = (int) (fitted.x * screenFillRatio);
	int initialHeight = (int) (fitted.y * screenFillRatio);

	gSDLWindow = SDL_CreateWindow(
			"Nanosaur " PROJECT_VERSION,
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
			initialWidth,
			initialHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!gSDLWindow)
	{
		if (gGamePrefs.antialiasingLevel != 0)
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create SDL window with requested MSAA level. Retrying without MSAA...");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retry;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window: " + std::string(SDL_GetError()));
		}
	}

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gBackdropPixels = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Init gDataSpec
	const char* executablePath = argc > 0 ? argv[0] : NULL;
	fs::path dataPath = FindGameData(executablePath);

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			DoAlert("Couldn't load gamecontrollerdb.txt! No big deal, but gamepads may not work.");
		}
	}
}

static void Shutdown()
{
	Pomme::Shutdown();

	SDL_Quit();
}

int main(int argc, char** argv)
{
	bool success = true;
	std::string uncaught;

#if _DEBUG
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#else
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
#endif

	try
	{
		// Start the game
		Boot(argc, const_cast<const char**>(argv));

		// Run the game
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG)  // Skip last-resort catch in debug builds so we get a stack trace
	catch (std::exception& ex)
	{
		success = false;
		uncaught = ex.what();
	}
	catch (...)
	{
		success = false;
		uncaught = "unknown";
	}
#endif

	if (success)
	{
		// Clean up
		Shutdown();
		return 0;
	}
	else
	{
		std::cerr << "Uncaught exception: " << uncaught << "\n";
		Enter2D();
		SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
		return 1;
	}
}
