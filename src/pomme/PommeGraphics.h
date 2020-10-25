#pragma once

#include "PommeTypes.h"
#include <istream>
#include <vector>

namespace Pomme::Graphics
{
	struct Color
	{
		UInt8 a, r, g, b;

		Color(UInt8 red, UInt8 green, UInt8 blue);

		Color(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha);

		Color();
	};

	struct ARGBPixmap
	{
		int width;
		int height;
		std::vector<Byte> data;

		ARGBPixmap();

		ARGBPixmap(int w, int h);

		ARGBPixmap(ARGBPixmap&& other) noexcept;

		ARGBPixmap& operator=(ARGBPixmap&& other) noexcept;

		void Fill(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha = 0xFF);

		void Plot(int x, int y, UInt32 color);

		void WriteTGA(const char* path) const;

		inline UInt32* GetPtr(int x, int y)
		{ return (UInt32*) &data.data()[4 * (y * width + x)]; }
	};

	void Init(const char* windowTitle, int windowWidth, int windowHeight);

	void Shutdown();

	ARGBPixmap ReadPICT(std::istream& f, bool skip512 = true);

	void DumpTGA(const char* path, short width, short height, const char* argbData);

	void DrawARGBPixmap(int left, int top, ARGBPixmap& p);

	CGrafPtr GetScreenPort(void);

	void SetWindowIconFromIcl8Resource(short i);

	inline int Width(const Rect& r)
	{ return r.right - r.left; }

	inline int Height(const Rect& r)
	{ return r.bottom - r.top; }

	extern const uint32_t clut8[256];
	extern const uint32_t clut4[16];
}
