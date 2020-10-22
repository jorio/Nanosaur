#undef WIN32_LEAN_AND_MEAN
#undef NOUSER
#undef NOGDI

#include <windows.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include "CompilerSupport/filesystem.h"

extern "C"
{
	extern SDL_Window* gSDLWindow;
}

fs::path DoOpenDialog(const char* expectedArchiveName)
{
    const int fileLength = 2048;
    const int promptLength = 512;
	
    OPENFILENAME ofn = {};
    TCHAR file[fileLength];
    mbstowcs(file, expectedArchiveName, fileLength);

    TCHAR prompt[promptLength];
    _snwprintf_s(prompt, promptLength, L"Where is \"%s\"?", file);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(gSDLWindow, &wmInfo);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = wmInfo.info.win.window;
    ofn.lpstrFile = file;
    ofn.nMaxFile = fileLength;
    ofn.lpstrTitle = prompt;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileName(&ofn) ? fs::path(ofn.lpstrFile) : fs::path("");
}
