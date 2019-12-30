#include "PommeInternal.h"

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr				gCoverWindow;
UInt16*                 gCoverWindowPixPtr;
UInt32                  gCoverWindowRowBytes2;

void GameMain(void);

int main(int argc, char** argv) {
    Pomme::Init();
    GameMain();
    return 0;
}
