#include "PommeInternal.h"
#include "Qut.h"
#include <iostream>
#include <thread>
#include <atomic>

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr				gCoverWindow;
UInt16*                 gCoverWindowPixPtr;
UInt32                  gCoverWindowRowBytes2;

void GameMain(void);

enum {
    Turn_MAIN = 0,
    Turn_GAME = 1,
};

std::atomic<int> turn(Turn_MAIN);
std::thread gameThread;

void RenderYield(TQ3ViewObject theView) {
    std::cout << "RenderYield\n";
    turn = Turn_GAME;
    while (turn != Turn_MAIN)
        std::this_thread::yield();
}

void GameYield() {
    std::cout << "GameYield\n";
    while (turn != Turn_GAME)
        std::this_thread::yield();
    turn = Turn_MAIN;
}

void AppMain() {
    std::cout << "AppMain\n";
    Pomme::Init("._Nanosaur\u2122");
    GameMain();
}

#ifndef QUT_HDR

int main(int argc, char** argv) {
    try {
        AppMain();
    }
    catch (const std::exception & ex) {
        TODOFATAL2("unhandled exception: " << ex.what());
        throw;
    }
    catch (const std::string & ex) {
        TODOFATAL2("unhandled exception: " << ex);
        throw;
    }
    catch (const char* ex) {
        TODOFATAL2("unhandled exception: " << ex);
        throw;
    }
    catch (...) {
        TODOFATAL2("unhandled exception");
        throw;
    }

    return 0;
}

#else

void App_Initialise(void) {
    AllocConsole();
    FILE* junk;
    freopen_s(&junk, "conin$",  "r", stdin);
    freopen_s(&junk, "conout$", "w", stdout);
    freopen_s(&junk, "conout$", "w", stderr);

	// Install error handlers.
	//Q3Error_Register(errorCallback, 0);
	//Q3Warning_Register(warningCallback, 0);
	//Q3Notice_Register(noticeCallback, 0);

	// Watch for leaks
	Q3Memory_StartRecording();

	// Initialise Qut
	Qut_CreateWindow("Nanosaur\u2122", 640, 480, kQ3False);

    //Qut_CreateView(Q3View_New, appConfigureView);
    //Qut_SetMouseTrackFunc(appMouseTrack);
    Qut_SetRenderFunc(RenderYield);

    turn = Turn_GAME;
    gameThread = std::thread(AppMain);
}

void App_Terminate(void) {
}

#endif
