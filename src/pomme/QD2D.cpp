#include "Pomme.h"
#include "PommeInternal.h"
#include <strstream>
#include <iostream>

using namespace Pomme;

//-----------------------------------------------------------------------------
// Point & Rect extensions

Rect::Rect() :
	top(0),
	left(0),
	bottom(0),
	right(0)
{
}

Rect::Rect(SInt16 t, SInt16 l, SInt16 b, SInt16 r) :
	top(t),
	left(l),
	bottom(b),
	right(r)
{

}

int Rect::width() const {
	return right - left;
}

int Rect::height() const {
	return bottom - top;
}

// ---------------------------------------------------------------------------- -
// PICT resources

PicHandle GetPicture(short PICTresourceID) {
	Handle rawResource = GetResource('PICT', PICTresourceID);
	if (rawResource == nil)
		return nil;
	std::istrstream substream(*rawResource, GetHandleSize(rawResource));
	Pixmap pm = Pomme::ReadPICT(substream, false);
	ReleaseResource(rawResource);

	// Tack the data onto the end of the Picture struct,
	// so that DisposeHandle frees both the Picture and the data.
	PicHandle ph = (PicHandle)NewHandle(sizeof(Picture) + pm.data.size());

	Picture& pic = **ph;
	Ptr pixels = ((Ptr)*ph) + sizeof(Picture);

	pic.picFrame = Rect(0, 0, pm.width, pm.height);
	pic.picSize = -1;
	pic.__pomme_pixelsARGB32 = pixels;

	memcpy(pic.__pomme_pixelsARGB32, pm.data.data(), pm.data.size());

	return ph;
}
