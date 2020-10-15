#include <iostream>
#include <cstring>

#include "Pomme.h"
#include "PommeInternal.h"

//-----------------------------------------------------------------------------
// Our own utils

const char* Pomme::QuitRequest::what() const noexcept
{
	return "the user has requested to quit the application";
}

//-----------------------------------------------------------------------------
// Misc

void ExitToShell()
{
	throw Pomme::QuitRequest();
}

void SysBeep(short duration)
{
#ifdef _WIN32
	Pomme::Platform::Windows::SysBeep();
#else
	TODOMINOR();
#endif
}

void FlushEvents(short, short) {
	TODOMINOR();
}

void NumToStringC(long theNum, Str255 theString)
{
	snprintf(theString, 256, "%ld", theNum);
}

//-----------------------------------------------------------------------------
// Mouse cursor

void InitCursor() {
	TODOMINOR();
}

void HideCursor() {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Our own init

void Pomme::Init(const char* windowName)
{
	Pomme::Time::Init();
	Pomme::Files::Init();
	Pomme::Graphics::Init(windowName, 640, 480);
	Pomme::Sound::Init();
	Pomme::Input::Init();
	std::cout << "Pomme initialized\n";
}

void Pomme::Shutdown()
{
	Pomme::Sound::Shutdown();
	Pomme::Graphics::Shutdown();
	std::cout << "Pomme shut down\n";
}
