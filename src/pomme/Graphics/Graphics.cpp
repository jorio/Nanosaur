#include "Pomme.h"
#include "PommeGraphics.h"
#include "SysFont.h"
#include "Utilities/memstream.h"

#include <iostream>
#include <memory>
#include <SDL.h>

using namespace Pomme;
using namespace Pomme::Graphics;

static bool IntersectRects(const Rect* r1, Rect* r2)
{
	r2->left   = std::max(r1->left, r2->left);
	r2->right  = std::min(r1->right, r2->right);
	r2->top    = std::max(r1->top, r2->top);
	r2->bottom = std::min(r1->bottom, r2->bottom);
	return r2->left < r2->right&& r2->top < r2->bottom;
}

// ---------------------------------------------------------------------------- -
// Types

struct GrafPortImpl
{
	GrafPort port;
	ARGBPixmap pixels;
	bool dirty;
	Rect dirtyRect;
	PixMap macpm;
	PixMap* macpmPtr;

	GrafPortImpl(const Rect boundsRect)
		: port({boundsRect, this})
		, pixels(boundsRect.right - boundsRect.left, boundsRect.bottom - boundsRect.top)
		, dirty(false)
	{
		macpm = {};
		macpm.bounds = boundsRect;
		macpm.pixelSize = 32;
		macpm._impl = (Ptr)&pixels;
		macpmPtr = &macpm;
	}

	void DamageRegion(const Rect& r)
	{
		if (!dirty) {
			dirtyRect = r;
		} else {
			// Already dirty, expand existing dirty rect
			dirtyRect.top    = std::min(dirtyRect.top,    r.top);
			dirtyRect.left   = std::min(dirtyRect.left,   r.left);
			dirtyRect.bottom = std::max(dirtyRect.bottom, r.bottom);
			dirtyRect.right  = std::max(dirtyRect.right,  r.right);
		}
		dirty = true;
	}

	void DamageRegion(int x, int y, int w, int h)
	{
		Rect r = { y, x, y+h, x+w };
		DamageRegion(r);
	}

	~GrafPortImpl()
	{
		macpm._impl = nullptr;
	}
};

// ---------------------------------------------------------------------------- -
// Internal State

static std::unique_ptr<GrafPortImpl> screenPort = nullptr;
static GrafPortImpl* curPort = nullptr;
static UInt32 penFG = 0xFF'FF'00'FF;
static UInt32 penBG = 0xFF'00'00'FF;
static int penX = 0;
static int penY = 0;

// ---------------------------------------------------------------------------- -
// Globals

extern "C" {
SDL_Window* gSDLWindow = nullptr;
}

// ---------------------------------------------------------------------------- -
// Initialization

CGrafPtr Pomme::Graphics::GetScreenPort(void)
{
	return &screenPort->port;
}

void Pomme::Graphics::Init(const char* windowTitle, int windowWidth, int windowHeight)
{
	if (0 != SDL_Init(SDL_INIT_VIDEO)) {
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	gSDLWindow = SDL_CreateWindow(
		windowTitle,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowWidth,
		windowHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

	if (!gSDLWindow) {
		throw std::runtime_error("Couldn't create SDL window.");
	}

	Rect boundsRect = {0,0,480,640};
	screenPort = std::make_unique<GrafPortImpl>(boundsRect);
	curPort = screenPort.get();
}

void Pomme::Graphics::Shutdown()
{
	SDL_DestroyWindow(gSDLWindow);
	gSDLWindow = nullptr;
}

// ---------------------------------------------------------------------------- -
// Internal utils

static UInt32 GetEightColorPaletteValue(long color)
{
	switch (color) {
	case whiteColor:	return clut4[0];
	case yellowColor:	return clut4[1];
	case redColor:		return clut4[3];
	case magentaColor:	return clut4[4];
	case blackColor:	return clut4[15];
	case cyanColor:		return clut4[7];
	case greenColor:	return clut4[8]; // I'm assuming this is light green rather than dark
	case blueColor:		return clut4[6];
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
	memstream substream(*rawResource, GetHandleSize(rawResource));
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

static inline GrafPortImpl& GetImpl(GWorldPtr offscreenGWorld)
{
	return *(GrafPortImpl*)offscreenGWorld->_impl;
}

static inline ARGBPixmap& GetImpl(PixMapPtr pixMap)
{
	return *(ARGBPixmap*)pixMap->_impl;
}

OSErr NewGWorld(GWorldPtr* offscreenGWorld, short pixelDepth, const Rect* boundsRect, void* junk1, void* junk2, long junk3)
{
	GrafPortImpl* impl = new GrafPortImpl(*boundsRect);
	*offscreenGWorld = &impl->port;
	return noErr;
}

void DisposeGWorld(GWorldPtr offscreenGWorld)
{
	delete &GetImpl(offscreenGWorld);
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

Boolean IsPortDamaged(void)
{
	return curPort->dirty;
}

void GetPortDamageRegion(Rect* r)
{
	*r = curPort->dirtyRect;
}

void ClearPortDamage(void)
{
	curPort->dirty = false;
}

void DamagePortRegion(const Rect* r)
{
	curPort->DamageRegion(*r);
}

void DumpPortTGA(const char* outPath)
{
	curPort->pixels.WriteTGA(outPath);
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

void RGBBackColor(const RGBColor* color)
{
	penBG
		= 0xFF'00'00'00
		| ((color->red >> 8) << 16)
		| ((color->green >> 8) << 8)
		| (color->blue >> 8)
		;
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

static void _FillRect(const int left, const int top, const int right, const int bottom, UInt32 fillColor)
{
	if (!curPort) {
		throw std::runtime_error("_FillRect: no port set");
	}

	Rect dstRect;
	dstRect.left   = left;
	dstRect.top    = top;
	dstRect.right  = right;
	dstRect.bottom = bottom;
	Rect clippedDstRect = dstRect;
	if (!IntersectRects(&curPort->port.portRect, &clippedDstRect)) {
		return;
	}
	curPort->DamageRegion(clippedDstRect);

	fillColor = ByteswapScalar(fillColor);  // convert to big-endian

	UInt32* dst = curPort->pixels.GetPtr(clippedDstRect.left, clippedDstRect.top);

	for (int y = clippedDstRect.top; y < clippedDstRect.bottom; y++) {
		for (int x = 0; x < clippedDstRect.right - clippedDstRect.left; x++) {
			dst[x] = fillColor;
		}
		dst += curPort->pixels.width;
	}
}

void PaintRect(const struct Rect* r)
{
	_FillRect(r->left, r->top, r->right, r->bottom, penFG);
}

void EraseRect(const struct Rect* r)
{
	_FillRect(r->left, r->top, r->right, r->bottom, penBG);
}

void LineTo(short x1, short y1)
{
	auto color = ByteswapScalar(penFG);

	Point off = curPort->port.portRect.topLeft;

	int x0 = penX;
	int y0 = penY;
	int dx = std::abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -std::abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	curPort->DamageRegion(penX, penY, dx, dy);
	while (1) {
		curPort->pixels.Plot(x0 - off.h, y0 - off.v, color);
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
}

void FrameRect(const Rect* r)
{
	auto color = ByteswapScalar(penFG);
	auto& pm = curPort->pixels;
	Point off = curPort->port.portRect.topLeft;

	for (int x = r->left; x < r->right; x++) pm.Plot(x            - off.h, r->top        - off.v, color);
	for (int x = r->left; x < r->right; x++) pm.Plot(x            - off.h, r->bottom - 1 - off.v, color);
	for (int y = r->top; y < r->bottom; y++) pm.Plot(r->left      - off.h, y             - off.v, color);
	for (int y = r->top; y < r->bottom; y++) pm.Plot(r->right - 1 - off.h, y             - off.v, color);

	curPort->DamageRegion(*r);
}

void Pomme::Graphics::DrawARGBPixmap(int left, int top, ARGBPixmap& pixmap)
{
	if (!curPort) {
		throw std::runtime_error("DrawARGBPixmap: no port set");
	}

	Rect dstRect;
	dstRect.left   = left;
	dstRect.top    = top;
	dstRect.right  = left + pixmap.width;
	dstRect.bottom = top  + pixmap.height;
	Rect clippedDstRect = dstRect;
	if (!IntersectRects(&curPort->port.portRect, &clippedDstRect)) {
		return;  // wholly outside bounds
	}
	curPort->DamageRegion(clippedDstRect);

	UInt32* src = pixmap.GetPtr(clippedDstRect.left - dstRect.left, clippedDstRect.top - dstRect.top);
	UInt32* dst = curPort->pixels.GetPtr(clippedDstRect.left, clippedDstRect.top);

	for (int y = clippedDstRect.top; y < clippedDstRect.bottom; y++)
	{
		memcpy(dst, src, Width(clippedDstRect) * sizeof(UInt32));
		dst += curPort->pixels.width;
		src += pixmap.width;
	}
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
			curPort->pixels.GetPtr(dstRect->left, dstRect->top + y),
			srcPixels + y * srcWidth,
			4 * dstWidth);
	}

	curPort->DamageRegion(*dstRect);
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

short TextWidthC(const char* cstr)
{
	int totalWidth = -SysFont::charSpacing;
	for (; *cstr; cstr++) {
		totalWidth += SysFont::charSpacing;
		totalWidth += SysFont::GetGlyph(*cstr).width;
	}
	return totalWidth;
}

void DrawStringC(const char* cstr)
{
	_FillRect(
			penX,
			penY - SysFont::ascend,
			penX + TextWidthC(cstr),
			penY + SysFont::descend,
			penBG
			);

	penX -= SysFont::charSpacing;
	for (; *cstr; cstr++) {
        penX += SysFont::charSpacing;
		DrawChar(*cstr);
	}
}

void DrawChar(char c)
{
	UInt32 fg = ByteswapScalar(penFG);

	auto& glyph = SysFont::GetGlyph(c);

	// Theoretical coordinates of top-left corner of glyph (may be outside port bounds!)
	Rect dstRect;
	dstRect.left   = penX - SysFont::leftMargin;
	dstRect.top    = penY - SysFont::ascend;
	dstRect.right  = dstRect.left + SysFont::widthBits;
	dstRect.bottom = dstRect.top  + SysFont::rows;

	Rect clippedDstRect = dstRect;
	if (!IntersectRects(&curPort->port.portRect, &clippedDstRect)) {
		return;  // wholly outside bounds
	}
	curPort->DamageRegion(clippedDstRect);

	// Glyph boundaries
	int minCol = clippedDstRect.left - dstRect.left;
	int minRow = clippedDstRect.top  - dstRect.top;

	auto* dst2 = curPort->pixels.GetPtr(clippedDstRect.left, clippedDstRect.top);

	for (int glyphY = minRow; glyphY < minRow+Height(clippedDstRect); glyphY++) {
		auto rowBits = glyph.bits[glyphY];

		rowBits >>= minCol;

		auto* dstRow = dst2;

		for (int glyphX = minCol; glyphX < minCol+Width(clippedDstRect); glyphX++) {
			if (rowBits & 1) {
				*dstRow = fg;
			}
			rowBits >>= 1;
			dstRow++;
		}

		dst2 += curPort->pixels.width;
	}

	penX += glyph.width;
}

// ----------------------------------------------------------------------------
// Icons

void Pomme::Graphics::SetWindowIconFromIcl8Resource(short icl8ID)
{
	Handle macIcon = GetResource('icl8', icl8ID);
	if (1024 != GetHandleSize(macIcon)) {
		throw std::invalid_argument("icl8 resource has incorrect size");
	}

	const int width = 32;
	const int height = 32;

	SDL_Surface* icon = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	for (int y = 0; y < height; y++) {
		uint32_t *out = (uint32_t*)((char*) icon->pixels + icon->pitch * y);
		for (int x = 0; x < width; x++) {
			unsigned char pixel = (*macIcon)[y*width + x];
			*out++ = pixel == 0 ? 0 : Pomme::Graphics::clut8[pixel];
		}
	}
	SDL_SetWindowIcon(gSDLWindow, icon);
	SDL_FreeSurface(icon);
	DisposeHandle(macIcon);
}
