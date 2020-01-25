#include "PommeInternal.h"
#include "Qut.h"
#include <SDL.h>
#include <iostream>
#include <thread>
#include <atomic>

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr				gCoverWindow;
UInt16*                 gCoverWindowPixPtr;
UInt32                  gCoverWindowRowBytes2;

void GameMain(void);
void RegisterUnpackableTypes(void);

enum {
    Turn_MAIN = 0,
    Turn_GAME = 1,
};

std::atomic<int> turn(Turn_MAIN);
std::thread gameThread;

void RenderYield(TQ3ViewObject theView) {
    turn = Turn_GAME;
    while (turn != Turn_MAIN)
        std::this_thread::yield();
}

void GameYield() {
    while (turn != Turn_GAME)
        std::this_thread::yield();
    turn = Turn_MAIN;
}

void AppMain() {
    Pomme::Init("Nanosaur\u2122");
    RegisterUnpackableTypes();
    GameMain();
}

void WrapAppMain() {
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

#ifndef QUT_HDR

int main(int argc, char** argv) {
    WrapAppMain();
    return 0;
}

#else

void App_Initialise(void) {

	// Install error handlers.
	//Q3Error_Register(errorCallback, 0);
	//Q3Warning_Register(warningCallback, 0);
	//Q3Notice_Register(noticeCallback, 0);

	// Watch for leaks
	Q3Memory_StartRecording();

	// Initialise Qut
	Qut_CreateWindow("Nanosaur\u2122", 640, 480, kQ3False);

    turn = Turn_GAME;
    gameThread = std::thread(WrapAppMain);
}

void App_Terminate(void) {
}

#endif
