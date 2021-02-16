#include "Pomme.h"
#include "PommeFiles.h"
#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"

#define EDGE_PADDING_REPEAT 8

static void Assert(bool condition, const char* message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

TQ3MetaFile* Q3MetaFile_Load3DMF(const FSSpec* spec)
{
	short refNum;
	OSErr err;

	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
		return nullptr;

	TQ3MetaFile* metaFile = __Q3Alloc<TQ3MetaFile>(1, '3DMF');

	auto& fileStream = Pomme::Files::GetStream(refNum);
	Q3MetaFileParser(fileStream, *metaFile).Parse3DMF();
	FSClose(refNum);

	return metaFile;
}

void Q3MetaFile_Dispose(TQ3MetaFile* metaFile)
{
	__Q3GetCookie(metaFile, '3DMF');

	for (int i = 0; i < metaFile->numTextures; i++)
	{
		if (metaFile->textures[i]->glTextureName)
			glDeleteTextures(1, &metaFile->textures[i]->glTextureName);
		Q3Pixmap_Dispose(metaFile->textures[i]);
	}

	for (int i = 0; i < metaFile->numMeshes; i++)
		Q3TriMeshData_Dispose(metaFile->meshes[i]);

	for (int i = 0; i < metaFile->numTopLevelGroups; i++)
		__Q3Dispose(metaFile->topLevelGroups[i].meshes, 'GMSH');

	__Q3Dispose(metaFile->textures,				'TLST');
	__Q3Dispose(metaFile->meshes,				'MLST');
	__Q3Dispose(metaFile->topLevelGroups,		'GLST');
	__Q3Dispose(metaFile,						'3DMF');
}

#pragma mark -


template<typename T>
static void _EdgePadding(
		T* const pixelData,
		const int width,
		const int height,
		const int rowBytes,
		const T alphaMask)
{
	Assert(rowBytes % sizeof(T) == 0, "EdgePadding: rowBytes is not a multiple of pixel bytesize");
	const int rowAdvance = rowBytes / sizeof(T);

	T* const firstRow = pixelData;
	T* const lastRow = firstRow + (height-1) * rowAdvance;

	for (int i = 0; i < EDGE_PADDING_REPEAT; i++)
	{
		// Dilate horizontally, row by row
		for (T* row = firstRow; row <= lastRow; row += rowAdvance)
		{
			// Expand east
			for (int x = 0; x < width-1; x++)
				if (!row[x])
					row[x] = row[x+1] & ~alphaMask;

			// Expand west
			for (int x = width-1; x > 0; x--)
				if (!row[x])
					row[x] = row[x-1] & ~alphaMask;
		}

		// Dilate vertically, column by column
		for (int x = 0; x < width; x++)
		{
			// Expand south
			for (T* row = firstRow; row < lastRow; row += rowAdvance)
				if (!row[x])
					row[x] = row[x + rowAdvance] & ~alphaMask;

			// Expand north
			for (T* row = lastRow; row > firstRow; row -= rowAdvance)
				if (!row[x])
					row[x] = row[x - rowAdvance] & ~alphaMask;
		}
	}
}

void Q3Pixmap_ApplyEdgePadding(TQ3Pixmap* pm)
{
	switch (pm->pixelType)
	{
		case kQ3PixelTypeARGB16:
			Assert(pm->rowBytes >= pm->width * 2, "EdgePadding ARGB16: incorrect rowBytes");
			_EdgePadding<uint16_t>(
					(uint16_t *) pm->image,
					pm->width,
					pm->height,
					pm->rowBytes,
					pm->byteOrder==kQ3EndianBig? 0x0080: 0x8000);
			break;

		case kQ3PixelTypeARGB32:
			Assert(pm->rowBytes >= pm->width * 4, "EdgePadding ARGB32: incorrect rowBytes");
			_EdgePadding<uint32_t>(
					(uint32_t *) pm->image,
					pm->width,
					pm->height,
					pm->rowBytes,
					pm->byteOrder==kQ3EndianBig? 0x000000FF: 0xFF000000);
			break;

		case kQ3PixelTypeRGB16:
		case kQ3PixelTypeRGB16_565:
		case kQ3PixelTypeRGB24:
		case kQ3PixelTypeRGB32:
			// Unnecessary to apply edge padding here because there's no alpha channel
			break;

		default:
			Assert(false, "EdgePadding: pixel type unsupported");
			break;
	}
}

void Q3Pixmap_Dispose(TQ3Pixmap* pixmap)
{
	__Q3Dispose(pixmap->image,					'IMAG');
	__Q3Dispose(pixmap,							'PXMP');
}

#pragma mark -

TQ3TriMeshData* Q3TriMeshData_New(int numTriangles,	int numPoints)
{
	TQ3TriMeshData* mesh	= __Q3Alloc<TQ3TriMeshData>(1, 'MESH');

	mesh->numTriangles		= numTriangles;
	mesh->numPoints			= numPoints;
	mesh->points			= __Q3Alloc<TQ3Point3D>(numPoints, 'TMpt');
	mesh->triangles			= __Q3Alloc<TQ3TriMeshTriangleData>(numTriangles, 'TMtr');
	mesh->vertexNormals		= __Q3Alloc<TQ3Vector3D>(numPoints, 'TMvn');
	mesh->vertexUVs			= __Q3Alloc<TQ3Param2D>(numPoints, 'TMuv');
	mesh->vertexColors		= nullptr;
	mesh->diffuseColor		= {1, 1, 1, 1};
	mesh->hasTexture		= false;
	mesh->textureHasTransparency = false;

	for (int i = 0; i < numPoints; i++)
	{
		mesh->vertexNormals[i] = {0, 1, 0};
		mesh->vertexUVs[i] = {.5f, .5f};
//		triMeshData->vertexColors[i] = {1, 1, 1, 1};
	}

	return mesh;
}

TQ3TriMeshData* Q3TriMeshData_Duplicate(const TQ3TriMeshData* source)
{
	TQ3TriMeshData* mesh	= __Q3Copy(source, 'MESH');
	mesh->points			= __Q3Copy(source->points,			'TMpt');
	mesh->triangles			= __Q3Copy(source->triangles,		'TMtr');
	mesh->vertexNormals		= __Q3Copy(source->vertexNormals,	'TMvn');
	mesh->vertexColors		= __Q3Copy(source->vertexColors,	'TMvc');
	mesh->vertexUVs			= __Q3Copy(source->vertexUVs,		'TMuv');
	return mesh;
}

void Q3TriMeshData_Dispose(TQ3TriMeshData* mesh)
{
	__Q3DisposeArray(&mesh->points,				'TMpt');
	__Q3DisposeArray(&mesh->triangles,			'TMtr');
	__Q3DisposeArray(&mesh->vertexNormals,		'TMvn');
	__Q3DisposeArray(&mesh->vertexColors,		'TMvc');
	__Q3DisposeArray(&mesh->vertexUVs,			'TMuv');
	__Q3Dispose(mesh, 'MESH');
}
