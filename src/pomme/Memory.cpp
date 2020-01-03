#include <iostream>
#include <vector>
#include <queue>
#include "PommeInternal.h"

#ifdef POMME_DEBUG_MEMORY
static std::ostream& LOG = std::cout;
#else
static std::ostringstream LOG;
#endif

//-----------------------------------------------------------------------------
// Implementation-specific stuff

struct BlockDescriptor {
	Ptr buf;
	Size size;
	int index;
};

// these must not move around
Pomme::Pool<BlockDescriptor, long> blocks;

BlockDescriptor& HandleToBlock(Handle h) {
	return *(BlockDescriptor*)h;
}

//-----------------------------------------------------------------------------
// Memory: Handle

Handle NewHandle(Size s) {
	long index;
	BlockDescriptor& block = blocks.Alloc(&index);
	block.index = index;
	block.buf = new char[s];
	block.size = s;

	if ((void*)&block.buf != (void*)&block)
		throw std::exception("buffer address mismatches block address");

	LOG << "NewHandle " << (void*)block.buf << " size " << s << "\n";

	return &block.buf;
}

Handle NewHandleClear(Size s) {
	Handle h = NewHandle(s);
	memset(*h, 0, s);
	return h;
}

Handle TempNewHandle(Size s, OSErr* err) {
	Handle h = NewHandle(s);
	*err = noErr;
	return h;
}

Size GetHandleSize(Handle h) {
	return HandleToBlock(h).size;
}

void SetHandleSize(Handle handle, Size byteCount) {
	TODOFATAL();
}

void DisposeHandle(Handle h) {
	LOG << "DisposeHandle " << (void*)*h << "\n";

	BlockDescriptor b = HandleToBlock(h);
	delete[] b.buf;
	b.buf = 0;
	b.size = -1;
	blocks.Dispose(b.index);
}

//-----------------------------------------------------------------------------
// Memory: Ptr

Ptr NewPtr(Size byteCount) {
	return new char[byteCount];
}

Ptr NewPtrSys(Size byteCount) {
	return new char[byteCount];
}

void DisposePtr(Ptr p) {
	delete[] p;
}
