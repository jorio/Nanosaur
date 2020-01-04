#include "Pomme.h"
#include "PommeInternal.h"

#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>

std::chrono::time_point<std::chrono::steady_clock> bootTP;

// timestamp (from unix epoch) of the mac epoch, Jan 1, 1904, 00:00:00
constexpr int JANUARY_1_1904 = -2'082'844'800;

//-----------------------------------------------------------------------------
// Time Manager

void Pomme::InitTimeManager() {
	bootTP = std::chrono::high_resolution_clock::now();
}

void GetDateTime(unsigned long* secs) {
	*secs = std::time(nullptr) + JANUARY_1_1904;
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
