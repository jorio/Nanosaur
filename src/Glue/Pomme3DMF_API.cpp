#include "Pomme.h"
#include "PommeFiles.h"
#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"

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

template<typename T>
static T* __Q3Alloc(size_t bytes, uint32_t cookie)
{
	uint8_t* data = (uint8_t*) calloc(16 + bytes, 1);

	*(uint32_t*) data = cookie;

	return (T*) (data + 16);
}

static void __Q3Dispose(void* object, uint32_t expectedCookie)
{
	if (!object)
		return;

	uint8_t* memoryChunk = ((uint8_t*) object) - 16;

	uint32_t* memoryCookiePtr = (uint32_t*) memoryChunk;

	if (expectedCookie != *memoryCookiePtr)
		throw std::runtime_error("Dispose: incorrect cookie");

	*memoryCookiePtr = 'DEAD';

	free(memoryChunk);
}

Pomme3DMF_FileHandle Pomme3DMF_LoadModelFile(const FSSpec* spec)
{
	short refNum;
	OSErr err;

	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
		return nullptr;

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

void Q3TriMeshData_Dispose(TQ3TriMeshData* triMeshData)
{
	__Q3Dispose(triMeshData, 'TMSH');
}
