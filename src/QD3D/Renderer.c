// RENDERER.C
// (C)2021 Iliyas Jorio
// This file is part of Nanosaur. https://github.com/jorio/nanosaur


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <QD3D.h>
#include "misc.h"	// assertions
#include "environmentmap.h"
#include "renderer.h"
#include "globals.h"	// status bits

#if __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

extern TQ3Param2D				gEnvMapUVs[];
extern RenderStats				gRenderStats;


/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct RendererState
{
	GLuint		boundTexture;
	bool		hasClientState_GL_TEXTURE_COORD_ARRAY;
	bool		hasClientState_GL_VERTEX_ARRAY;
	bool		hasClientState_GL_COLOR_ARRAY;
	bool		hasClientState_GL_NORMAL_ARRAY;
	bool		hasState_GL_CULL_FACE;
	bool		hasState_GL_ALPHA_TEST;
	bool		hasState_GL_DEPTH_TEST;
	bool		hasState_GL_COLOR_MATERIAL;
	bool		hasState_GL_TEXTURE_2D;
	bool		hasState_GL_BLEND;
	bool		hasState_GL_LIGHTING;
	bool		hasFlag_glDepthMask;
//	bool		hasState_GL_FOG;
} RendererState;

/****************************/
/*    CONSTANTS             */
/****************************/

static const RenderModifiers kDefaultRenderMods =
{
	.statusBits = 0,
	.diffuseColor = {1,1,1,1},
};

/****************************/
/*    VARIABLES             */
/****************************/

static RendererState gState;

static PFNGLDRAWRANGEELEMENTSPROC __glDrawRangeElements;

/****************************/
/*    MACROS/HELPERS        */
/****************************/

static void __SetInitialState(GLenum stateEnum, bool* stateFlagPtr, bool initialValue)
{
	*stateFlagPtr = initialValue;
	if (initialValue)
		glEnable(stateEnum);
	else
		glDisable(stateEnum);
	CHECK_GL_ERROR();
}

static void __SetInitialClientState(GLenum stateEnum, bool* stateFlagPtr, bool initialValue)
{
	*stateFlagPtr = initialValue;
	if (initialValue)
		glEnableClientState(stateEnum);
	else
		glDisableClientState(stateEnum);
	CHECK_GL_ERROR();
}

static inline void __EnableState(GLenum stateEnum, bool* stateFlagPtr)
{
	if (!*stateFlagPtr)
	{
		glEnable(stateEnum);
		*stateFlagPtr = true;
	}
	else
		gRenderStats.batchedStateChanges++;
}

static inline void __EnableClientState(GLenum stateEnum, bool* stateFlagPtr)
{
	if (!*stateFlagPtr)
	{
		glEnableClientState(stateEnum);
		*stateFlagPtr = true;
	}
	else
		gRenderStats.batchedStateChanges++;
}

static inline void __DisableState(GLenum stateEnum, bool* stateFlagPtr)
{
	if (*stateFlagPtr)
	{
		glDisable(stateEnum);
		*stateFlagPtr = false;
	}
	else
		gRenderStats.batchedStateChanges++;
}

static inline void __DisableClientState(GLenum stateEnum, bool* stateFlagPtr)
{
	if (*stateFlagPtr)
	{
		glDisableClientState(stateEnum);
		*stateFlagPtr = false;
	}
	else
		gRenderStats.batchedStateChanges++;
}

#define SetInitialState(stateEnum, initialValue) __SetInitialState(stateEnum, &gState.hasState_##stateEnum, initialValue)
#define SetInitialClientState(stateEnum, initialValue) __SetInitialClientState(stateEnum, &gState.hasClientState_##stateEnum, initialValue)

#define EnableState(stateEnum) __EnableState(stateEnum, &gState.hasState_##stateEnum)
#define EnableClientState(stateEnum) __EnableClientState(stateEnum, &gState.hasClientState_##stateEnum)

#define DisableState(stateEnum) __DisableState(stateEnum, &gState.hasState_##stateEnum)
#define DisableClientState(stateEnum) __DisableClientState(stateEnum, &gState.hasClientState_##stateEnum)

#define EnableFlag(glFunction) do {					\
	if (!gState.hasFlag_##glFunction) {				\
		glFunction(GL_TRUE);						\
		gState.hasFlag_##glFunction = true;			\
	} } while(0)

#define DisableFlag(glFunction) do {				\
	if (gState.hasFlag_##glFunction) {				\
		glFunction(GL_FALSE);						\
		gState.hasFlag_##glFunction = false;		\
	} } while(0)

#pragma mark -

//=======================================================================================================

/****************************/
/*    API IMPLEMENTATION    */
/****************************/

void Render_GetGLProcAddresses(void)
{
	__glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)SDL_GL_GetProcAddress("glDrawRangeElements");  // missing link with something...?
	GAME_ASSERT(__glDrawRangeElements);
}

void Render_InitState(void)
{
	// On Windows, proc addresses are only valid for the current context, so we must get fetch everytime we recreate the context.
	Render_GetGLProcAddresses();

	SetInitialClientState(GL_VERTEX_ARRAY,				true);
	SetInitialClientState(GL_NORMAL_ARRAY,				true);
	SetInitialClientState(GL_COLOR_ARRAY,				false);
	SetInitialClientState(GL_TEXTURE_COORD_ARRAY,		true);
	SetInitialState(GL_CULL_FACE,		true);
	SetInitialState(GL_ALPHA_TEST,		true);
	SetInitialState(GL_DEPTH_TEST,		true);
	SetInitialState(GL_COLOR_MATERIAL,	true);
	SetInitialState(GL_TEXTURE_2D,		false);
	SetInitialState(GL_BLEND,			false);
	SetInitialState(GL_LIGHTING,		true);
//	SetInitialState(GL_FOG,				true);

	gState.hasFlag_glDepthMask = true;		// initially active on a fresh context

	gState.boundTexture = 0;
}

void Render_Load3DMFTextures(TQ3MetaFile* metaFile)
{
	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3Pixmap* textureDef = metaFile->textures[i];
		GAME_ASSERT_MESSAGE(textureDef->glTextureName == 0, "GL texture already allocated");

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
				DoAlert("3DMF texture: Unsupported kQ3PixelType");
				continue;
		}

		GLuint textureName;

		glGenTextures(1, &textureName);
		CHECK_GL_ERROR();

		textureDef->glTextureName = textureName;

		glBindTexture(GL_TEXTURE_2D, textureName);				// this is now the currently active texture
		CHECK_GL_ERROR();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
}

void Render_Dispose3DMFTextures(TQ3MetaFile* metaFile)
{
	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3Pixmap* textureDef = metaFile->textures[i];

		if (textureDef->glTextureName != 0)
		{
			glDeleteTextures(1, &textureDef->glTextureName);
			textureDef->glTextureName = 0;
		}
	}
}

void Render_StartFrame(void)
{
	memset(&gRenderStats, 0, sizeof(gRenderStats));

	// The depth mask must be re-enabled so we can clear the depth buffer.
	EnableFlag(glDepthMask);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Render_DrawTriMeshList(
		int numMeshes,
		TQ3TriMeshData** meshList,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods)
{
	if (mods == nil)
	{
		mods = &kDefaultRenderMods;
	}

	bool envMap = mods->statusBits & STATUS_BIT_REFLECTIONMAP;

	if (transform)
	{
		glPushMatrix();
		glMultMatrixf((float*) transform->value);
	}

	for (int i = 0; i < numMeshes; i++)
	{
		const TQ3TriMeshData* mesh = meshList[i];

		if (envMap)
		{
			EnvironmentMapTriMesh(mesh, transform);
		}

		if (mesh->textureHasTransparency || mesh->diffuseColor.a < .999f || mods->diffuseColor.a < .999f)
		{
			EnableState(GL_BLEND);
			DisableState(GL_ALPHA_TEST);
		}
		else
		{
			DisableState(GL_BLEND);
			EnableState(GL_ALPHA_TEST);
		}

		if (mods->statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			DisableState(GL_CULL_FACE);
		}
		else
		{
			EnableState(GL_CULL_FACE);
		}

		if (mods->statusBits & STATUS_BIT_NULLSHADER)
		{
			DisableState(GL_LIGHTING);
		}
		else
		{
			EnableState(GL_LIGHTING);
		}

		if (mods->statusBits & STATUS_BIT_NOZWRITE)
		{
			DisableFlag(glDepthMask);
		}
		else
		{
			EnableFlag(glDepthMask);
		}

		glVertexPointer(3, GL_FLOAT, 0, mesh->points);
		glNormalPointer(GL_FLOAT, 0, mesh->vertexNormals);
		CHECK_GL_ERROR();

		if (mesh->hasTexture)
		{
			EnableState(GL_TEXTURE_2D);
			EnableClientState(GL_TEXTURE_COORD_ARRAY);
			if (gState.boundTexture != mesh->glTextureName)
			{
				glBindTexture(GL_TEXTURE_2D, mesh->glTextureName);
				gState.boundTexture = mesh->glTextureName;
			}
			else
				gRenderStats.batchedStateChanges++;

			glTexCoordPointer(2, GL_FLOAT, 0, envMap ? gEnvMapUVs : mesh->vertexUVs);
			CHECK_GL_ERROR();
		}
		else
		{
			DisableState(GL_TEXTURE_2D);
			DisableClientState(GL_TEXTURE_COORD_ARRAY);
			CHECK_GL_ERROR();
		}

		if (mesh->hasVertexColors)
		{
			EnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, mesh->vertexColors);
		}
		else
		{
			DisableClientState(GL_COLOR_ARRAY);
		}

		glColor4f(
				mesh->diffuseColor.r * mods->diffuseColor.r,
				mesh->diffuseColor.g * mods->diffuseColor.g,
				mesh->diffuseColor.b * mods->diffuseColor.b,
				mesh->diffuseColor.a * mods->diffuseColor.a
				);

		__glDrawRangeElements(GL_TRIANGLES, 0, mesh->numPoints-1, mesh->numTriangles*3, GL_UNSIGNED_SHORT, mesh->triangles);
		CHECK_GL_ERROR();

		gRenderStats.trianglesDrawn += mesh->numTriangles;
		gRenderStats.meshesDrawn++;
	}

	if (transform)
	{
		glPopMatrix();
	}
}
