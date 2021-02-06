#include <cstring>
#include "Pomme.h"
#include "PommeFiles.h"
#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"

#if __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

static void Assert(bool condition, const char* message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

static void ThrowGLError(GLenum error, const char* func, int line)
{
	char message[512];

	snprintf(message, sizeof(message), "OpenGL error 0x%x in %s:%d (\"%s\")",
				error, func, line, (const char*) gluErrorString(error));

	throw std::runtime_error(message);
}

#define CHECK_GL_ERROR()												\
	do {					 											\
		GLenum error = glGetError();									\
		if (error != GL_NO_ERROR)										\
			ThrowGLError(error, __func__, __LINE__);					\
	} while(0)

TQ3MetaFile* Q3MetaFile_Load3DMF(const FSSpec* spec)
{
	short refNum;
	OSErr err;

	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
		return nullptr;

	printf("========== LOADING 3DMF: %s ===========\n", spec->cName);

	TQ3MetaFile* metaFile = __Q3Alloc<TQ3MetaFile>(1, '3DMF');

	auto& fileStream = Pomme::Files::GetStream(refNum);
	Q3MetaFileParser(fileStream, *metaFile).Parse3DMF();
	FSClose(refNum);

	//-------------------------------------------------------------------------
	// Load textures

	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3Pixmap* textureDef = metaFile->textures[i];
		Assert(textureDef->glTextureName == 0, "texture already allocated");

		GLuint textureName;

		glGenTextures(1, &textureName);
		CHECK_GL_ERROR();

		printf("Loading GL texture #%d\n", textureName);

		textureDef->glTextureName = textureName;

		glBindTexture(GL_TEXTURE_2D, textureName);				// this is now the currently active texture
		CHECK_GL_ERROR();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLenum internalFormat;
		GLenum format;
		GLenum type;
		switch (textureDef->pixelType)
		{
			case kQ3PixelTypeRGB16:
				internalFormat = GL_RGB;
				format = GL_BGRA_EXT;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			case kQ3PixelTypeARGB16:
				internalFormat = GL_RGBA;
				format = GL_BGRA_EXT;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			default:
				throw std::runtime_error("3DMF texture: Unsupported kQ3PixelType");
		}

		glTexImage2D(GL_TEXTURE_2D,
					 0,										// mipmap level
					 internalFormat,						// format in OpenGL
					 textureDef->width,						// width in pixels
					 textureDef->height,					// height in pixels
					 0,										// border
					 format,								// what my format is
					 type,									// size of each r,g,b
					 textureDef->image);					// pointer to the actual texture pixels
		CHECK_GL_ERROR();

		// Set glTextureName on meshes
		for (int j = 0; j < metaFile->numMeshes; j++)
		{
			if (metaFile->meshes[j]->hasTexture && metaFile->meshes[j]->internalTextureID == i)
			{
				metaFile->meshes[j]->glTextureName = textureName;
			}
		}
	}

	//-------------------------------------------------------------------------
	// Done

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

#pragma -

void Q3Pixmap_Dispose(TQ3Pixmap* pixmap)
{
	__Q3Dispose(pixmap->image,					'IMAG');
	__Q3Dispose(pixmap,							'PXMP');
}

#pragma -

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
