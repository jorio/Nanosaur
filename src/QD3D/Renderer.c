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

#if __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

extern TQ3Vector3D				gEnvMapNormals[];
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
//	bool		hasState_GL_FOG;
} RendererState;


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
	SetInitialClientState(GL_COLOR_ARRAY,				true);
	SetInitialClientState(GL_TEXTURE_COORD_ARRAY,		true);
	SetInitialState(GL_CULL_FACE,		true);
	SetInitialState(GL_ALPHA_TEST,		true);
	SetInitialState(GL_DEPTH_TEST,		true);
	SetInitialState(GL_COLOR_MATERIAL,	true);
	SetInitialState(GL_TEXTURE_2D,		false);
//	SetInitialState(GL_FOG,				true);

	gState.boundTexture = 0;
}

void Render_DrawTriMeshList(int numMeshes, TQ3TriMeshData** meshList, bool envMap, const TQ3Matrix4x4* transform)
{
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

		glVertexPointer(3, GL_FLOAT, 0, mesh->points);
		glColorPointer(4, GL_FLOAT, 0, mesh->vertexColors);
		glNormalPointer(GL_FLOAT, 0, envMap? gEnvMapNormals: mesh->vertexNormals);
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

			glTexCoordPointer(2, GL_FLOAT, 0, envMap? gEnvMapUVs: mesh->vertexUVs);
			CHECK_GL_ERROR();
		}
		else
		{
			DisableState(GL_TEXTURE_2D);
			DisableClientState(GL_TEXTURE_COORD_ARRAY);
			CHECK_GL_ERROR();
		}

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
