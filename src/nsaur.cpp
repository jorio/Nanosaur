#include "PommeInternal.h"
#include <SDL.h>
#include <iostream>
#include <Quesa.h>

#if _WIN32
#include <windows.h>
#endif

TQ3ViewObject				gView = nullptr;

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr gCoverWindow = nullptr;
UInt32* gCoverWindowPixPtr = nullptr;

void GameMain(void);
void RegisterUnpackableTypes(void);

int CommonMain(int argc, const char** argv)
{
	// Start our "machine"
	Pomme::Init("Nanosaur\u2122");

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*)GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Register format strings to unpack the structs
	RegisterUnpackableTypes();

	// Start the game
	GameMain();

	// Clean up
	if (gView != NULL)
		Q3Object_Dispose(gView);

	// TODO: dispose SDL gl context

//	if (gDC != NULL)
//		ReleaseDC((HWND)gWindow, gDC);

//	DestroyWindow((HWND)gWindow);

	// Terminate Quesa
	Q3Exit();
	
	//Pomme::Shutdown();

	return 0;
}

int main(int argc, const char** argv)
{
#ifdef _WIN32
	AllocConsole();
	FILE* junk;
	freopen_s(&junk, "conin$", "r", stdin);
	freopen_s(&junk, "conout$", "w", stdout);
	freopen_s(&junk, "conout$", "w", stderr);

	DWORD outMode = 0;
	HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleMode(stdoutHandle, &outMode)) exit(GetLastError());
	// Enable ANSI escape codes
	outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(stdoutHandle, outMode)) exit(GetLastError());
#endif

	std::string uncaught;

	try {
		return CommonMain(argc, argv);
	}
	catch (const std::exception & ex) {
		uncaught = ex.what();
	}
	catch (...) {
		uncaught = "unknown";
	}

	if (!uncaught.empty()) {
		std::cerr << "Uncaught exception: " << uncaught << "\n";
		SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
		exit(1);
	}
}
