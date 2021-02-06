#pragma once

#ifdef __cplusplus

#include <vector>
#include <istream>
#include <map>
#include <cstring>
#include <QD3D.h>
#include "Utilities/BigEndianIStream.h"

class Q3MetaFileParser
{
public:
	Q3MetaFileParser(std::istream& input, TQ3MetaFile& dest);
	void Parse3DMF();

private:
	// Returns fourcc of parsed chunk
	uint32_t Parse1Chunk();

	// Parse trimesh chunk
	void Parse_tmsh(uint32_t chunkSize);

	// Parse attribute array chunk
	void Parse_atar(uint32_t chunkSize);

	// Parse mipmap texture chunk
	uint32_t Parse_txmm(uint32_t chunkSize);

	TQ3MetaFile& metaFile;
	std::istream& baseStream;
	Pomme::BigEndianIStream f;

	int currentDepth = 0;

	TQ3TriMeshData* currentMesh;

	std::map<uint32_t, uint64_t> referenceTOC;
	std::map<std::streampos, uint32_t> knownTextures;
};

//-----------------------------------------------------------------------------
// Memory allocator

#define ALLOCATOR_HEADER_BYTES 16

struct __Q3AllocatorCookie
{
	uint32_t		classID;
	uint32_t		blockSize;		// including header cookie
};

static_assert(sizeof(__Q3AllocatorCookie) <= ALLOCATOR_HEADER_BYTES);

template<typename T>
static T* __Q3Alloc(size_t count, uint32_t classID)
{
	size_t totalBytes = ALLOCATOR_HEADER_BYTES + count*sizeof(T);

	uint8_t* block = (uint8_t*) calloc(totalBytes, 1);

	__Q3AllocatorCookie* cookie = (__Q3AllocatorCookie*) block;

	cookie->classID		= classID;
	cookie->blockSize	= totalBytes;

	return (T*) (block + ALLOCATOR_HEADER_BYTES);
}

static __Q3AllocatorCookie* __Q3GetCookie(const void* sourcePayload, uint32_t classID)
{
	if (!sourcePayload)
		throw std::runtime_error("__Q3GetCookie: got null pointer");

	uint8_t* block = ((uint8_t*) sourcePayload) - ALLOCATOR_HEADER_BYTES;

	__Q3AllocatorCookie* cookie = (__Q3AllocatorCookie*) block;

	if (classID != cookie->classID)
		throw std::runtime_error("__Q3GetCookie: incorrect cookie");

	return cookie;
}

static void __Q3Dispose(void* object, uint32_t classID)
{
	if (!object)
		return;

	auto cookie = __Q3GetCookie(object, classID);

	memset(cookie, 0, cookie->blockSize);

	cookie->classID = 'DEAD';

	free(cookie);
}

template<typename T>
static T* __Q3Realloc(T* sourcePayload, size_t minCount, uint32_t classID)
{
	if (!sourcePayload)
	{
		return __Q3Alloc<T>(minCount * 2, classID);
	}

	__Q3AllocatorCookie* sourceCookie = __Q3GetCookie(sourcePayload, classID);

	size_t currentBytes = sourceCookie->blockSize - ALLOCATOR_HEADER_BYTES;
	size_t wantedBytes = minCount * sizeof(T);

	if (wantedBytes < currentBytes)
	{
		return sourcePayload;
	}

	T* newPayload = __Q3Alloc<T>(minCount * 2, classID);
	memcpy(newPayload, sourcePayload, currentBytes);

	__Q3Dispose(sourcePayload, classID);

	return newPayload;
}

template<typename T>
static void __Q3EnlargeArray(T*& sourcePayload, int& size, uint32_t classID)
{
	size++;
	sourcePayload = __Q3Realloc(sourcePayload, size, classID);
}

template<typename T>
static T* __Q3Copy(const T* sourcePayload, uint32_t classID)
{
	if (!sourcePayload)
		return nullptr;

	__Q3AllocatorCookie* sourceCookie = __Q3GetCookie(sourcePayload, classID);

	uint8_t* block = (uint8_t*) calloc(sourceCookie->blockSize, 1);
	memcpy(block, sourceCookie, sourceCookie->blockSize);

	return (T*) (block + ALLOCATOR_HEADER_BYTES);
}

template<typename T>
static void __Q3DisposeArray(T** arrayPtr, uint32_t classID)
{
	if (*arrayPtr)
	{
		__Q3Dispose(*arrayPtr, classID);
		*arrayPtr = nullptr;
	}
}

#endif // __cplusplus
