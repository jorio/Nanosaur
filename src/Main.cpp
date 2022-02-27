#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"

#include <SDL.h>

#include <iostream>

#if __APPLE__
#include <libproc.h>
#include <unistd.h>
#endif

extern "C"
{
	// bare minimum from Window.c to satisfy externs in game code
	SDL_Window* gSDLWindow = nullptr;
	WindowPtr gCoverWindow = nullptr;
	UInt32* gCoverWindowPixPtr = nullptr;

	// Lets the game know where to find its asset files
	extern FSSpec gDataSpec;

	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
#if 0 //_WIN32
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif

	#include "game.h"
}

static fs::path FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

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
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
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

	gSDLWindow = SDL_CreateWindow(
			"Nanosaur " PROJECT_VERSION,
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
			640,
			480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

	if (!gSDLWindow)
	{
		if (gGamePrefs.antialiasingLevel != 0)
		{
			printf("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...\n");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retry;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Init gDataSpec
	fs::path dataPath = FindGameData();

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Nanosaur", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
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
