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

std::string Pomme::Pascal2Cpp(const char* pstr)
{
	return std::string(&pstr[1], pstr[0]);
}

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

void Pomme::Init(const char* windowName)
{
	Pomme::Time::Init();
	Pomme::Files::Init();
	Pomme::Graphics::Init(windowName);
	Pomme::Sound::Init();
	Pomme::Input::Init();
	std::cout << "Pomme initialized\n";
}
