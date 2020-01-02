#include <iostream>
#include <vector>
#include <queue>

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
std::vector<BlockDescriptor> blocks;
std::deque<int> freeBlocks;

BlockDescriptor& HandleToBlock(Handle h) {
	return *(BlockDescriptor*)h;
}

//-----------------------------------------------------------------------------
// Memory: Handle

Handle NewHandle(Size s) {
	BlockDescriptor block;
	block.buf = new char[s];
	block.size = s;
	if (freeBlocks.empty()) {
		block.index = blocks.size();
	} else {
		block.index = freeBlocks.front();
		freeBlocks.pop_front();
	}

	if ((void*)&block.buf != (void*)&block)
		throw std::exception("buffer address mismatches block address");
	
	if (block.index == blocks.size())
		blocks.push_back(block);
	else
		blocks[block.index] = block;

	LOG << "NewHandle " << (void*)block.buf << " size " << s << "\n";

	return &blocks[block.index].buf;
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
	freeBlocks.push_back(b.index);

	while (freeBlocks.size() > 0 && freeBlocks.back() == blocks.size() - 1) {
		LOG << "Nuked block " << freeBlocks.back() << "\n";
		freeBlocks.pop_back();
		blocks.pop_back();
	}
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
