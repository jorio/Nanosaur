#include <iostream>

#ifdef _WIN32
#include <windows.h> // for SysBeep :)
#endif

#include "Pomme.h"
#include "PommeInternal.h"

//-----------------------------------------------------------------------------
// Our own utils

char* Pascal2C(const char* pstr)
{
	static char cstr[256];
	memcpy(cstr, &pstr[1], pstr[0]);
	cstr[pstr[0]] = '\0';
	return cstr;
}

//-----------------------------------------------------------------------------
// Misc

void ExitToShell() {
	exit(0);
}

void SysBeep(short duration)
{
#ifdef _WIN32
	MessageBeep(0);
#else
	TODOMINOR();
#endif
}

void FlushEvents(short, short) {
	TODOMINOR();
}

void NumToString(long theNum, Str255& theString)
{
	std::stringstream ss;
	ss << theNum;
	auto str = ss.str();
	theString = Str255(str.c_str());
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

void Pomme::Init(const char* applName)
{
	Pomme::Time::Init();
	Pomme::Files::Init(applName);
	Pomme::Graphics::Init(applName);
	Pomme::Sound::Init();
	Pomme::Input::Init();
	std::cout << "Pomme initialized\n";
}
