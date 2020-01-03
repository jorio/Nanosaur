#include "PommeInternal.h"

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr				gCoverWindow;
UInt16*                 gCoverWindowPixPtr;
UInt32                  gCoverWindowRowBytes2;

void GameMain(void);

void AppMain() {
    Pomme::Init("._Nanosaur\u2122");
    GameMain();
}

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
