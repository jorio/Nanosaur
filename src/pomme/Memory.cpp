#include <iostream>
#include <vector>
#include <queue>

#include "PommeInternal.h"
#include "FixedPool.h"

#define LOG POMME_GENLOG(POMME_DEBUG_MEMORY, "MEMO")

//-----------------------------------------------------------------------------
// Implementation-specific stuff

static struct BlockDescriptor {
	Ptr buf;
	Size size;
};

// these must not move around
static Pomme::FixedPool<BlockDescriptor, UInt16, 1000> blocks;

static BlockDescriptor* HandleToBlock(Handle h) {
	return (BlockDescriptor*)h;
}

//-----------------------------------------------------------------------------
// Memory: Handle

Handle NewHandle(Size s) {
	if (s < 0) throw std::invalid_argument("trying to alloc negative size handle");

	BlockDescriptor* block = blocks.Alloc();
	block->buf = new char[s];
	block->size = s;

	if ((Ptr)&block->buf != (Ptr)block)
		throw std::exception("buffer address mismatches block address");

	LOG << (void*)block->buf << ", size " << s << "\n";

	return &block->buf;
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
	return HandleToBlock(h)->size;
}

void SetHandleSize(Handle handle, Size byteCount) {
	TODOFATAL();
}

void DisposeHandle(Handle h) {
	LOG << (void*)*h << "\n";
	BlockDescriptor* b = HandleToBlock(h);
	delete[] b->buf;
	b->buf = 0;
	b->size = -1;
	blocks.Dispose(b);
}

//-----------------------------------------------------------------------------
// Memory: Ptr

Ptr NewPtr(Size byteCount) {
	if (byteCount < 0) throw std::invalid_argument("trying to NewPtr negative size");
	return new char[byteCount];
}

Ptr NewPtrSys(Size byteCount) {
	if (byteCount < 0) throw std::invalid_argument("trying to NewPtrSys negative size");
	return new char[byteCount];
}

void DisposePtr(Ptr p) {
	delete[] p;
}

//-----------------------------------------------------------------------------
// Memory: BlockMove

void BlockMove(const void* srcPtr, void* destPtr, Size byteCount) {
	memcpy(destPtr, srcPtr, byteCount);
}

void BlockMoveData(const void* srcPtr, void* destPtr, Size byteCount) {
	TODOFATAL();
}
