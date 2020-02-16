#include "PommeInternal.h"
#include <SDL.h>
#include <iostream>
#include <thread>
#include <Quesa.h>

#if _WIN32
#include <windows.h>
#endif

TQ3ViewObject				gView = nullptr;
std::thread					gameThread;

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr gCoverWindow = nullptr;
UInt32* gCoverWindowPixPtr = nullptr;

void GameMain(void);
void RegisterUnpackableTypes(void);

void AppMain()
{
    Pomme::Init("Nanosaur\u2122");

	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*)GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));
	
    RegisterUnpackableTypes();
    GameMain();
}

void WrapAppMain()
{
    std::string uncaught;

    try {
        AppMain();
    }
    catch (const std::exception & ex) {
        uncaught = ex.what();
    }
    catch (const std::string & ex) {
        uncaught = ex;
    }
    catch (const char* ex) {
        uncaught = ex;
    }
    catch (...) {
        uncaught = "unknown";
    }

    if (!uncaught.empty()) {
        SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
    }
}

int CommonMain(int argc, const char** argv)
{
	// Start the game
	gameThread = std::thread(WrapAppMain);

	// SDL event loop
	SDL_Event e;
	while (0 != SDL_WaitEvent(&e)) {
	}

	// Clean up

	if (gView != NULL)
		Q3Object_Dispose(gView);

	// TODO: dispose SDL gl context

//	if (gDC != NULL)
//		ReleaseDC((HWND)gWindow, gDC);

//	DestroyWindow((HWND)gWindow);

	// Terminate Quesa
	Q3Exit();

	return 0;
}

#ifdef _WIN32
void WindowsConsoleInit()
{
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
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WindowsConsoleInit();
	return CommonMain(0, nullptr);
}
#else
int main(int argc, const char** argv)
{
	return CommonMain(argc, argv);
}
#endif

