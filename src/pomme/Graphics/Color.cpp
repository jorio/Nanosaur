#include "PommeGraphics.h"

using namespace Pomme::Graphics;

//-----------------------------------------------------------------------------
// Color

Color::Color(uint8_t r_, uint8_t g_, uint8_t b_) :
	a(0xFF), r(r_), g(g_), b(b_)
{}

Color::Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) :
	a(a_), r(r_), g(g_), b(b_)
{}

Color::Color() :
	a(0xFF), r(0xFF), g(0x00), b(0xFF)
{}
