#include "Pomme.h"
#include "PommeInternal.h"

#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>

std::chrono::time_point<std::chrono::steady_clock> bootTP;

//-----------------------------------------------------------------------------
// Time Manager

void Pomme::InitTimeManager() {
	bootTP = std::chrono::high_resolution_clock::now();
}

void GetDateTime(unsigned long* secs) {
	TODOMINOR();
}

void Microseconds(UnsignedWide* usecs) {
	auto now = std::chrono::high_resolution_clock::now();
	auto usecs1 = std::chrono::duration_cast<std::chrono::microseconds>(now - bootTP);
	auto usecs2 = usecs1.count();
	usecs->lo = usecs2 & 0xFFFFFFFFL;
	usecs->hi = (usecs2 >> 32) & 0xFFFFFFFFL;
}

UInt32 TickCount() {
	TODO();
	return 0;
}
