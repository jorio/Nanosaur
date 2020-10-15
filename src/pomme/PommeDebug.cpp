#include "PommeInternal.h"

#include <SDL.h>
#include <sstream>
#include <iostream>

void ImplementMe(const char* fn, std::string msg, int severity)
{
	if (severity >= 0) {
		std::stringstream ss;
		ss << "[TODO] \x1b[1m" << fn << "\x1b[22m";
		if (!msg.empty()) {
			ss << ": ";
			//for (int i = strlen(fn); i < 32; i++) ss << '.';
			ss << msg;
		}
		auto str = ss.str();
		std::cerr << (severity > 0 ? "\x1b[31m" : "\x1b[33m") << str << "\x1b[0m\n";
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

std::string Pomme::FourCCString(FourCharCode t, char filler)
{
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
