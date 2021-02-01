#include <cstring>
#include "Pomme.h"
#include "PommeFiles.h"
#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"

#define ALLOCATOR_HEADER_BYTES 16

// TODO: use __Q3Alloc for C++ classes as well
#define CHECK_COOKIE(obj)												\
	do																	\
	{																	\
		if ( (obj).cookie != (obj).COOKIE )								\
			throw std::runtime_error("Pomme3DMF: illegal cookie");		\
	} while (0)

static void Assert(bool condition, const char* message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

struct __Q3AllocatorCookie
{
	uint32_t		classID;
	uint32_t		blockSize;		// including header cookie
};

static_assert(sizeof(__Q3AllocatorCookie) <= ALLOCATOR_HEADER_BYTES);

template<typename T>
static T* __Q3Alloc(size_t payloadBytes, uint32_t classID)
{
	size_t totalBytes = ALLOCATOR_HEADER_BYTES + payloadBytes;

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

template<typename T>
static T* __Q3Copy(const T* sourcePayload, uint32_t classID)
{
	__Q3AllocatorCookie* sourceCookie = __Q3GetCookie(sourcePayload, classID);

	uint8_t* block = (uint8_t*) calloc(sourceCookie->blockSize, 1);
	memcpy(block, sourceCookie, sourceCookie->blockSize);

	return (T*) (block + ALLOCATOR_HEADER_BYTES);
}

static void __Q3Dispose(void* object, uint32_t classID)
{
	if (!object)
		return;

	auto cookie = __Q3GetCookie(object, classID);
	cookie->classID = 'DEAD';

	free(cookie);
}

Pomme3DMF_FileHandle Pomme3DMF_LoadModelFile(const FSSpec* spec)
{
	short refNum;
	OSErr err;

	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
		return nullptr;

	printf("========== LOADING 3DMF: %s ===========\n", spec->cName);

	Q3MetaFile* metaFile = new Q3MetaFile();

	auto& fileStream = Pomme::Files::GetStream(refNum);
	Q3MetaFileParser(fileStream, *metaFile).Parse3DMF();
	FSClose(refNum);

	return (Pomme3DMF_FileHandle) metaFile;
}

void Pomme3DMF_DisposeModelFile(Pomme3DMF_FileHandle the3DMFFile)
{
	if (!the3DMFFile)
		return;

	auto metaFile = (Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(*metaFile);
	delete metaFile;
}

TQ3TriMeshFlatGroup Pomme3DMF_GetAllMeshes(Pomme3DMF_FileHandle the3DMFFile)
{
	auto& metaFile = *(Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(metaFile);

	TQ3TriMeshFlatGroup list;
	list.numMeshes		= metaFile.meshes.size();
	list.meshes			= metaFile.meshes.data();
	return list;
}

int Pomme3DMF_CountTopLevelMeshGroups(Pomme3DMF_FileHandle the3DMFFile)
{
	auto& metaFile = *(Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(metaFile);

	return metaFile.topLevelMeshGroups.size();
}

TQ3TriMeshFlatGroup Pomme3DMF_GetTopLevelMeshGroup(Pomme3DMF_FileHandle the3DMFFile, int groupNumber)
{
	auto& metaFile = *(Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(metaFile);

	auto& internalGroup = metaFile.topLevelMeshGroups.at(groupNumber);

	TQ3TriMeshFlatGroup group;
	group.numMeshes = internalGroup.size();
	group.meshes = internalGroup.data();
	return group;
}

TQ3TriMeshData* Q3TriMeshData_New(int numTriangles,	int numPoints)
{
	uint8_t* base = 0;
	TQ3TriMeshData* triMeshData = nullptr;

	for (int i = 0; i < 2; i++)
	{
#define SETPTR(lhs, n) do { if (i != 0) { (lhs) = (typeof((lhs))) base; } base += sizeof((lhs)[0]) * (n); } while(0)
		SETPTR(triMeshData,						1);
		SETPTR(triMeshData->triangles,			numTriangles);
		SETPTR(triMeshData->points,				numPoints);
		SETPTR(triMeshData->vertexNormals,		numPoints);
		SETPTR(triMeshData->vertexColors,		numPoints);
		SETPTR(triMeshData->vertexUVs,			numPoints);
#undef SETPTR

		if (i == 0)
		{
			triMeshData = __Q3Alloc<TQ3TriMeshData>((ptrdiff_t) base, 'TMSH');
			base = (uint8_t*) triMeshData;
		}
	}

	triMeshData->numTriangles = numTriangles;
	triMeshData->numPoints = numPoints;

	for (int i = 0; i < numPoints; i++)
	{
		triMeshData->vertexNormals[i] = {0, 1, 0};
		triMeshData->vertexColors[i] = {1, 1, 1, 1};
		triMeshData->vertexUVs[i] = {.5f, .5f};
	}

	return triMeshData;
}

TQ3TriMeshData* Q3TriMeshData_Duplicate(const TQ3TriMeshData* source)
{
	return __Q3Copy(source, 'TMSH');
}

void Q3TriMeshData_Dispose(TQ3TriMeshData* triMeshData)
{
	__Q3Dispose(triMeshData, 'TMSH');
}
