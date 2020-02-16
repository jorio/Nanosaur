#include "PommeInternal.h"
#include <iostream>

using namespace Pomme::Graphics;

//-----------------------------------------------------------------------------
// Pixmap

ARGBPixmap::ARGBPixmap() :
	width(0),
	height(0),
	data(0)
{
}

ARGBPixmap::ARGBPixmap(int w, int h) :
	width(w),
	height(h),
	data(w* h * 4, 0xAA)
{
	Fill(255, 0, 255);
}

void ARGBPixmap::Fill(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha)
{
	for (int i = 0; i < width * height * 4; i += 4) {
		data[i + 0] = alpha;
		data[i + 1] = red;
		data[i + 2] = green;
		data[i + 3] = blue;
	}
}

void ARGBPixmap::Plot(int x, int y, UInt32 color)
{
	if (x < 0 || y < 0 || x >= width || y >= height) {
		TODO2("OOB " << x << ", " << y);
	}
	else {
		*(UInt32*)&data[4 * (y * width + x)] = color;
	}
}

void ARGBPixmap::WriteTGA(const char* path) const
{
	DumpTGA(path, width, height, (const char*)data.data());
}

