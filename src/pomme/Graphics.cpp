#include "Pomme.h"
#include "PommeInternal.h"
#include "SysFont.h"
#include <strstream>
#include <iostream>
#include <SDL.h>
#include <Quesa.h>

#if _WIN32
	#undef min
	#undef max
	#undef SetPort
#endif

using namespace Pomme;
using namespace Pomme::Graphics;

// ---------------------------------------------------------------------------- -
// Types

struct GrafPortImpl {
	Pixmap* pixels;
	SDL_Window* window;
	bool dirty;
};

// ---------------------------------------------------------------------------- -
// Internal State

static GrafPortImpl gGrafPortImpl;
static GrafPort gGrafPort;
static Pomme::Graphics::Pixmap gCoverWindowPixmap(640, 480);

static GrafPtr curPortMacStruct = &gGrafPort;
static GrafPortImpl* curPort = &gGrafPortImpl;
static UInt32 penFG = 0xFF'FF'00'FF;
static UInt32 penBG = 0xFF'00'00'FF;
static short penX = 0;
static short penY = 0;

// ---------------------------------------------------------------------------- -
// Globals

SDL_Window* gSDLWindow = nullptr;

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr gCoverWindow = &gGrafPort;
UInt32* gCoverWindowPixPtr = (UInt32*)gCoverWindowPixmap.data.data();

// ---------------------------------------------------------------------------- -
// Initialization

#define SDL_ENSURE(X) { \
	if (!(X)) { \
		std::cerr << #X << " --- " << SDL_GetError() << "\n"; \
		exit(1); \
	} \
}

void Pomme::Graphics::Init(const char* windowTitle)
{
	SDL_ENSURE(0 == SDL_Init(SDL_INIT_VIDEO));

	gSDLWindow = SDL_CreateWindow(
		windowTitle,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		480,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

	SDL_ENSURE(gSDLWindow);

	gGrafPort._impl = &gGrafPortImpl;
	gGrafPortImpl = {};
	gGrafPortImpl.pixels = &gCoverWindowPixmap;
	gGrafPortImpl.window = gSDLWindow;

	// the sdl gl context is now obtained by quesa
	//SDL_ENSURE(gGLCtx = SDL_GL_CreateContext(gSDLWindow));

	// Initialise ourselves
	auto qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
		TODOFATAL2("Couldn't init Quesa");

	// Install error handlers.
	//Q3Error_Register(errorCallback, 0);
	//Q3Warning_Register(warningCallback, 0);
	//Q3Notice_Register(noticeCallback, 0);

	// Watch for leaks
//	Q3Memory_StartRecording();
}

// ---------------------------------------------------------------------------- -
// Internal utils

// According to https://lowendmac.com/mac-16-color-4-bit-palette/
static const UInt32 fourBitPalette[16] = {
	0xFF'FF'FF'FF,	// 0 white
	0xFF'FF'FF'00,	// 1 yellow
	0xFF'FF'66'00,	// 2 orange
	0xFF'DD'00'00,	// 3 red
	0xFF'FF'00'99,	// 4 magenta
	0xFF'33'00'99,	// 5 purple
	0xFF'00'00'CC,	// 6 blue
	0xFF'00'99'FF,	// 7 cyan
	0xFF'00'AA'00,	// 8 light green
	0xFF'00'66'00,	// 9 dark green
	0xFF'66'33'00,	//10 dark brown
	0xFF'99'66'33,	//11 light brown
	0xFF'BB'BB'BB,	//12 light gray
	0xFF'88'88'88,	//13 gray
	0xFF'44'44'44,	//14 dark gray
	0xFF'00'00'00,	//15 black
};

static UInt32 GetEightColorPaletteValue(long color)
{
	switch (color) {
	case whiteColor:	return fourBitPalette[0];
	case yellowColor:	return fourBitPalette[1];
	case redColor:		return fourBitPalette[3];
	case magentaColor:	return fourBitPalette[4];
	case blackColor:	return fourBitPalette[15];
	case cyanColor:		return fourBitPalette[7];
	case greenColor:	return fourBitPalette[8]; // I'm assuming this is light green rather than dark
	case blueColor:		return fourBitPalette[6];
	default:			return 0xFF'FF'00'FF;
	}
}

// ---------------------------------------------------------------------------- -
// PICT resources

PicHandle GetPicture(short PICTresourceID)
{
	Handle rawResource = GetResource('PICT', PICTresourceID);
	if (rawResource == nil)
		return nil;
	std::istrstream substream(*rawResource, GetHandleSize(rawResource));
	Pixmap pm = ReadPICT(substream, false);
	ReleaseResource(rawResource);

	// Tack the data onto the end of the Picture struct,
	// so that DisposeHandle frees both the Picture and the data.
	PicHandle ph = (PicHandle)NewHandle(int(sizeof(Picture) + pm.data.size()));

	Picture& pic = **ph;
	Ptr pixels = (Ptr)*ph + sizeof(Picture);

	pic.picFrame = Rect{ 0, 0, (SInt16)pm.width, (SInt16)pm.height };
	pic.picSize = -1;
	pic.__pomme_pixelsARGB32 = pixels;

	memcpy(pic.__pomme_pixelsARGB32, pm.data.data(), pm.data.size());

	return ph;
}

// ---------------------------------------------------------------------------- -
// GWorld

void DisposeGWorld(GWorldPtr offscreenGWorld)
{
	TODO();
}

// ---------------------------------------------------------------------------- -
// Port

void SetPort(GrafPtr port)
{
	curPortMacStruct = port;
	curPort = (GrafPortImpl*)port->_impl;
}

void GetPort(GrafPtr* outPort)
{
	*outPort = curPortMacStruct;
}

Boolean Pomme_IsPortDirty(void)
{
	return curPort->dirty;
}

void Pomme_SetPortDirty(Boolean dirty)
{
	curPort->dirty = dirty;
}

// ---------------------------------------------------------------------------- -
// Pen state manipulation

void MoveTo(short h, short v)
{
	penX = h;
	penY = v;
}

void ForeColor(long color)
{
	penFG = GetEightColorPaletteValue(color);
}

void GetForeColor(RGBColor* rgb)
{
	rgb->red = (penFG >> 16 & 0xFF) << 8;
	rgb->green = (penFG >> 8 & 0xFF) << 8;
	rgb->blue = (penFG & 0xFF) << 8;
}

void RGBForeColor(const RGBColor* color)
{
	penFG
		= 0xFF'00'00'00
		| ((color->red >> 8) << 16)
		| ((color->green >> 8) << 8)
		| (color->blue >> 8)
		;
}

// ---------------------------------------------------------------------------- -
// Paint

void PaintRect(const struct Rect* r)
{
	if (!curPort) {
		throw std::exception(__FUNCTION__ ": no port set");
	}

	UInt32* dst = (UInt32*)curPort->pixels->data.data();
	dst += r->left;
	dst += r->top * curPort->pixels->width;

	for (int y = r->top; y <= r->bottom; y++) {
		for (int x = 0; x <= r->right - r->left; x++) {
			dst[x] = ToBE(penFG);
		}
		dst += curPort->pixels->width;
	}

	curPort->dirty = true;
}

void Pomme_DumpPict(int left, int top, Pixmap& p)
{
	if (!curPort) {
		throw std::exception(__FUNCTION__ ": no port set");
	}

	UInt32* src = (UInt32*)p.data.data();
	UInt32* dst = (UInt32*)curPort->pixels->data.data();
	dst += left;
	dst += top * curPort->pixels->width;

	int bottom = std::min(curPort->pixels->height, top + p.height) - 1;
	int right = std::min(curPort->pixels->width, top + p.width) - 1;

	for (int y = top; y <= bottom; y++)
	{
		for (int x = left; x <= right; x++)
		{
			dst[x] = src[x - left];
		}
		dst += curPort->pixels->width;
		src += p.width;
	}

	curPort->dirty = true;
}

// ---------------------------------------------------------------------------- -
// Text rendering

short TextWidth(Ptr textBuf, short firstByte, short byteCount)
{
	short totalWidth = 0;
	for (int i = firstByte; i < firstByte + byteCount; i++) {
		totalWidth += SysFont::GetGlyph(textBuf[i]).width + SysFont::charSpacing;
	}
	return totalWidth;
}

void DrawString(ConstStr255Param s)
{
	for (unsigned i = 1; i <= (unsigned int)s[0]; i++)
	{
		DrawChar(s[i]);
	}
}

void DrawChar(char c)
{
	UInt32 fg = FromBE(penFG);
	UInt32 bg = FromBE(penBG);

	auto& glyph = SysFont::GetGlyph(c);

	auto* dst2 = curPort->pixels->GetPtr(penX - SysFont::leftMargin, penY - SysFont::ascend);

	int minCol = std::max(0, -(penX - SysFont::leftMargin));
	int maxCol = std::min(SysFont::widthBits, curPort->pixels->width - (penX - SysFont::leftMargin));
	int maxRow = std::min(SysFont::rows, curPort->pixels->height - (penY - SysFont::ascend));

	for (int glyphRow = 0; glyphRow < maxRow; glyphRow++) {
		auto rowBits = glyph.bits[glyphRow];

		rowBits >>= minCol;

		for (int glyphX = minCol; glyphX < maxCol; glyphX++) {
			dst2[glyphX] = rowBits & 1 ? fg : bg;
			rowBits >>= 1;
		}

		dst2 += curPort->pixels->width;
	}

	penX += glyph.width + (int)SysFont::charSpacing;

	curPort->dirty = true;
}
