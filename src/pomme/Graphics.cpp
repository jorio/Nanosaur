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

struct GrafPortImpl
{
	GrafPort port;
	ARGBPixmap* pixels;
	SDL_Window* window;
	bool dirty;

	PixMap macpm;
	PixMap* macpmPtr;

	GrafPortImpl(const Rect boundsRect, SDL_Window* sdlWindow)
	{
		const int width = boundsRect.right - boundsRect.left;
		const int height = boundsRect.bottom - boundsRect.top;

		port = {};
		port._impl = this;
		port.portRect = boundsRect;

		pixels = new ARGBPixmap(width, height);
		window = sdlWindow;
		dirty = false;

		macpm = {};
		macpm.bounds = boundsRect;
		macpm.pixelSize = 32;
		//macpm.baseAddr
		macpm._impl = (Ptr)pixels;

		macpmPtr = &macpm;
	}

	~GrafPortImpl()
	{
		delete pixels;
		pixels = nullptr;
		macpm._impl = nullptr;
	}
};

// ---------------------------------------------------------------------------- -
// Internal State

static GrafPortImpl* screenPort = nullptr;
static GrafPortImpl* curPort = nullptr;
static UInt32 penFG = 0xFF'FF'00'FF;
static UInt32 penBG = 0xFF'00'00'FF;
static short penX = 0;
static short penY = 0;

// ---------------------------------------------------------------------------- -
// Globals

SDL_Window* gSDLWindow = nullptr;

// ---------------------------------------------------------------------------- -
// Initialization

#define SDL_ENSURE(X) { \
	if (!(X)) { \
		std::cerr << #X << " --- " << SDL_GetError() << "\n"; \
		exit(1); \
	} \
}

CGrafPtr Pomme::Graphics::GetScreenPort(void)
{
	return &screenPort->port;
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

	screenPort = new GrafPortImpl({ 0,0,480,640}, gSDLWindow);
	curPort = screenPort;

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
	ARGBPixmap pm = ReadPICT(substream, false);
	ReleaseResource(rawResource);

	// Tack the data onto the end of the Picture struct,
	// so that DisposeHandle frees both the Picture and the data.
	PicHandle ph = (PicHandle)NewHandle(int(sizeof(Picture) + pm.data.size()));

	Picture& pic = **ph;
	Ptr pixels = (Ptr)*ph + sizeof(Picture);

	pic.picFrame = Rect{ 0, 0, (SInt16)pm.height, (SInt16)pm.width };
	pic.picSize = -1;
	pic.__pomme_pixelsARGB32 = pixels;

	memcpy(pic.__pomme_pixelsARGB32, pm.data.data(), pm.data.size());

	return ph;
}

// ---------------------------------------------------------------------------- -
// Rect

void SetRect(Rect* r, short left, short top, short right, short bottom)
{
	r->left = left;
	r->top = top;
	r->right = right;
	r->bottom = bottom;
}

// ---------------------------------------------------------------------------- -
// GWorld

static constexpr GrafPortImpl& GetImpl(GWorldPtr offscreenGWorld)
{
	return *(GrafPortImpl*)offscreenGWorld->_impl;
}

static constexpr ARGBPixmap& GetImpl(PixMapPtr pixMap)
{
	return *(ARGBPixmap*)pixMap->_impl;
}

OSErr NewGWorld(GWorldPtr* offscreenGWorld, short pixelDepth, const Rect* boundsRect, void* junk1, void* junk2, long junk3)
{
	GrafPortImpl* impl = new GrafPortImpl(*boundsRect, nullptr);
	*offscreenGWorld = &impl->port;
	return noErr;
}

void DisposeGWorld(GWorldPtr offscreenGWorld)
{
	delete &GetImpl(offscreenGWorld);
	offscreenGWorld->_impl = nullptr;
}

void GetGWorld(CGrafPtr* port, GDHandle* gdh)
{
	*port = &curPort->port;
	*gdh = nil;
}

void SetGWorld(CGrafPtr port, GDHandle gdh)
{
	SetPort(port);
}

PixMapHandle GetGWorldPixMap(GWorldPtr offscreenGWorld)
{
	return &GetImpl(offscreenGWorld).macpmPtr;
}

Ptr GetPixBaseAddr(PixMapHandle pm)
{
	return (Ptr)GetImpl(*pm).data.data();
}

Boolean LockPixels(PixMapHandle pm)
{
	return true;
}

void UnlockPixels(PixMapHandle pm)
{
	// no-op
}

// ---------------------------------------------------------------------------- -
// Port

void SetPort(GrafPtr port)
{
	curPort = &GetImpl(port);
}

void GetPort(GrafPtr* outPort)
{
	*outPort = &curPort->port;
}

Boolean Pomme_IsPortDirty(void)
{
	return curPort->dirty;
}

void Pomme_SetPortDirty(Boolean dirty)
{
	curPort->dirty = dirty;
}

void Pomme_DumpPortTGA(const char* outPath)
{
	curPort->pixels->WriteTGA(outPath);
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

void BackColor(long color)
{
	penBG = GetEightColorPaletteValue(color);
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

static void _FillRect(const struct Rect* r, UInt32 fillColor)
{
	if (!curPort) {
		throw std::exception(__FUNCTION__ ": no port set");
	}

	fillColor = ToBE(fillColor);

	UInt32* dst = (UInt32*)curPort->pixels->data.data();
	dst += r->left;
	dst += r->top * curPort->pixels->width;

	for (int y = r->top; y < r->bottom; y++) {
		for (int x = 0; x < r->right - r->left; x++) {
			dst[x] = fillColor;
		}
		dst += curPort->pixels->width;
	}

	curPort->dirty = true;
}

void PaintRect(const struct Rect* r)
{
	_FillRect(r, penFG);
}

void EraseRect(const struct Rect* r)
{
	_FillRect(r, penBG);
}

void LineTo(short x1, short y1)
{
	auto color = ToBE(penFG);

	Point off = curPort->port.portRect.topLeft;

	int x0 = penX;
	int y0 = penY;
	int dx = std::abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -std::abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	while (1) {
		curPort->pixels->Plot(x0 - off.h, y0 - off.v, color);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
	penX = x0;
	penY = y0;
	curPort->dirty = true;
}

void FrameRect(const Rect* r)
{
	auto color = ToBE(penFG);
	auto& pm = *curPort->pixels;
	Point off = curPort->port.portRect.topLeft;

	for (int x = r->left; x < r->right; x++) pm.Plot(x            - off.h, r->top        - off.v, color);
	for (int x = r->left; x < r->right; x++) pm.Plot(x            - off.h, r->bottom - 1 - off.v, color);
	for (int y = r->top; y < r->bottom; y++) pm.Plot(r->left      - off.h, y             - off.v, color);
	for (int y = r->top; y < r->bottom; y++) pm.Plot(r->right - 1 - off.h, y             - off.v, color);

	curPort->dirty = true;
}

void Pomme_DumpPict(int left, int top, ARGBPixmap& p)
{
	if (!curPort) {
		throw std::exception(__FUNCTION__ ": no port set");
	}

	left -= curPort->port.portRect.left;
	top -= curPort->port.portRect.top;

	UInt32* src = (UInt32*)p.data.data();
	UInt32* dst = (UInt32*)curPort->pixels->data.data();
	dst += left;
	dst += top * curPort->pixels->width;

	int bottom = std::min(curPort->pixels->height, top + p.height);
	int right = std::min(curPort->pixels->width, top + p.width);

	for (int y = top; y < bottom; y++)
	{
		for (int x = left; x < right; x++)
		{
			dst[x] = src[x - left];
		}
		dst += curPort->pixels->width;
		src += p.width;
	}

	curPort->dirty = true;
}

void DrawPicture(PicHandle myPicture, const Rect* dstRect)
{
	auto& pic = **myPicture;

	UInt32* srcPixels = (UInt32*)pic.__pomme_pixelsARGB32;

	int dstWidth = Width(*dstRect);
	int dstHeight = Height(*dstRect);
	int srcWidth = Width(pic.picFrame);
	int srcHeight = Height(pic.picFrame);

	if (srcWidth != dstWidth || srcHeight != dstHeight)
		TODOFATAL2("we only support dstRect with the same width/height as the source picture");
	
	for (int y = 0; y < dstHeight; y++)
	{
		memcpy(
			curPort->pixels->GetPtr(dstRect->left, dstRect->top + y),
			srcPixels + y * srcWidth,
			4 * dstWidth);
	}
}

void CopyBits(
	const PixMap* srcBits,
	PixMap* dstBits,
	const Rect* srcRect,
	const Rect* dstRect,
	short mode,
	void* maskRgn
)
{
	auto& srcPM = GetImpl((PixMapPtr)srcBits);
	auto& dstPM = GetImpl(dstBits);

	const auto& srcBounds = srcBits->bounds;
	const auto& dstBounds = dstBits->bounds;

	int srcRectWidth = Width(*srcRect);
	int srcRectHeight = Height(*srcRect);
	int dstRectWidth = Width(*dstRect);
	int dstRectHeight = Height(*dstRect);

	if (srcRectWidth != dstRectWidth || srcRectHeight != dstRectHeight)
		TODOFATAL2("can only copy between rects of same dimensions");
	
	for (int y = 0; y < srcRectHeight; y++)
	//for (int x = 0; x < srcRectWidth; x++)
	{
		memcpy(
			dstPM.GetPtr(dstRect->left - dstBounds.left, dstRect->top - dstBounds.top + y),
			srcPM.GetPtr(srcRect->left - srcBounds.left, srcRect->top - srcBounds.top + y),
			4 * srcRectWidth
		);
	}
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
