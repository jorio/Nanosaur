#include "PommeInternal.h"
#include <SDL.h>
#include <iostream>
#include <thread>
#include <Quesa.h>

#if _WIN32
#include <windows.h>
#endif

SDL_Window*					gSDLWindow = nullptr;
TQ3ViewObject				gView = nullptr;
std::thread					gameThread;

static Pomme::Graphics::Pixmap	gCoverWindowPixmap(640, 480);
// bare minimum from Windows.c to satisfy externs in game code
WindowPtr				gCoverWindow;
UInt32*	                gCoverWindowPixPtr		= (UInt32*)gCoverWindowPixmap.data.data();

#define SDL_ENSURE(X) { \
	if (!(X)) { \
		std::cerr << #X << " --- " << SDL_GetError() << "\n"; \
		exit(1); \
	} \
}

void GameMain(void);
void RegisterUnpackableTypes(void);

void AppMain()
{
    Pomme::Init("Nanosaur\u2122");
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
	TQ3Status		qd3dStatus;

	SDL_ENSURE(0 == SDL_Init(SDL_INIT_VIDEO));

	gSDLWindow = SDL_CreateWindow("SDLNano",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		480,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

	SDL_ENSURE(gSDLWindow);

	// the sdl gl context is now obtained by quesa
	//SDL_ENSURE(gGLCtx = SDL_GL_CreateContext(gSDLWindow));

	// Initialise ourselves
	qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
		return 1;

	// Install error handlers.
	//Q3Error_Register(errorCallback, 0);
	//Q3Warning_Register(warningCallback, 0);
	//Q3Notice_Register(noticeCallback, 0);

	// Watch for leaks
//	Q3Memory_StartRecording();

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
	qd3dStatus = Q3Exit();

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

