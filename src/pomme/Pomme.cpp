#include <iostream>
#include <unordered_set>

#include <SDL.h>

#include "Pomme.h"
#include "PommeInternal.h"

//-----------------------------------------------------------------------------
// Our own utils

std::unordered_set<std::string> implementMeAlreadySaid;

void ImplementMe(const char* fn, std::string msg, int severity) {
	if (severity >= 0) {
		std::stringstream ss;
		ss << "TODO[" << severity << "] " << fn << "()"; 
		if (!msg.empty()) ss << ": " << msg;
		auto str = ss.str();
		if (implementMeAlreadySaid.find(str) == implementMeAlreadySaid.end()) {
			std::cerr << (severity > 0? "\x1b[31m": "\x1b[33m") << str << "\x1b[0m\n";
			implementMeAlreadySaid.insert(str);
		}
	}
	
	if (severity >= 2) {
		std::stringstream ss;
		ss << fn << "()";
		if (!msg.empty()) ss << "\n" << msg;
		
		auto str = ss.str();

		int mbflags = SDL_MESSAGEBOX_ERROR;
		if (severity == 0) mbflags = SDL_MESSAGEBOX_INFORMATION;
		if (severity == 1) mbflags = SDL_MESSAGEBOX_WARNING;

		SDL_ShowSimpleMessageBox(mbflags, "Source port TODO", str.c_str(), nullptr);
	}

	if (severity >= 2) {
		abort();
	}
}

std::string Pomme::FourCCString(FourCharCode t, char filler) {
	char b[5];
	*(ResType*)b = t;
#if !(TARGET_RT_BIGENDIAN)
	std::reverse(b, b + 4);
#endif
	// replace non-ascii with '?'
	for (int i = 0; i < 4; i++) {
		char c = b[i];
		if (c < ' ' || c > '~') b[i] = filler;
	}
	b[4] = '\0';
	return b;
}

//-----------------------------------------------------------------------------
// QuickDraw 2D

void DisposeGWorld(GWorldPtr offscreenGWorld) {
	TODO();
}

//-----------------------------------------------------------------------------
// Misc

void ExitToShell() {
	exit(0);
}

void SysBeep(short duration) {
#ifdef WIN32
	MessageBeep(0);
#else
	TODOMINOR();
#endif
}

void FlushEvents(short, short) {
	TODOMINOR();
}

void NumToString(long theNum, Str255 theString) {
	std::stringstream ss;
	ss << theNum;
	theString = Str255(ss.str().c_str());
}

//-----------------------------------------------------------------------------
// No-op memory junk

void MaxApplZone(void) {
	// No-op
}

void MoreMasters(void) {
	// No-op
}

Size CompactMem(Size) {
	// No-op
	// TODO: what should we actually return?
	return 0;
}

Size CompactMemSys(Size) {
	// No-op
	// TODO: what should we actually return?
	return 0;
}

void PurgeMem(Size) {
	// No-op
}

void PurgeMemSys(Size) {
	// No-op
}

Size MaxMem(Size*) {
	// No-op
	// TODO: what should we actually return?
	return 0;
}

void HNoPurge(Handle) {
	// No-op
}

void HLock(Handle) {
	// No-op
}

void HLockHi(Handle) {
	// No-op
}

void NoPurgePixels(PixMapHandle) {
	// No-op
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
// Fade

void MakeFadeEvent(Boolean fadeIn) {
	TODOMINOR2("fadeIn=" << fadeIn);
}

void GammaFadeOut() {
	TODOMINOR();
}

void GammaFadeIn() {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Our own init

char* Pascal2C(const char* pstr) {
	static char cstr[256];
	memcpy(cstr, &pstr[1], pstr[0]);
	cstr[pstr[0]] = '\0';
	return cstr;
}

void Pomme::Init(const char* applName)
{
	Pomme::Time::Init();
	Pomme::Files::Init(applName);
	Pomme::Sound::Init();
	std::cout << "Pomme initialized\n";
}
