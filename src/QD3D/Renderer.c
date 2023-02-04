// RENDERER.C
// (C)2021 Iliyas Jorio
// This file is part of Nanosaur. https://github.com/jorio/nanosaur


/****************************/
/*    EXTERNALS             */
/****************************/

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <stdlib.h>		// qsort
#include "game.h"

extern TQ3Param2D				gEnvMapUVs[];
extern RenderStats				gRenderStats;
extern PrefsType				gGamePrefs;
extern UInt32*					gBackdropPixels;
extern int						gWindowWidth;
extern int						gWindowHeight;
extern SDL_Window*				gSDLWindow;
extern TQ3Matrix4x4				gCameraWorldToFrustumMatrix;

#pragma mark -

/****************************/
/*    PROTOTYPES            */
/****************************/

enum
{
	kRenderPass_Opaque,
	kRenderPass_Transparent
};

typedef struct RendererState
{
	GLuint		boundTexture;
	bool		hasClientState_GL_TEXTURE_COORD_ARRAY;
	bool		hasClientState_GL_VERTEX_ARRAY;
	bool		hasClientState_GL_COLOR_ARRAY;
	bool		hasClientState_GL_NORMAL_ARRAY;
	bool		hasState_GL_NORMALIZE;
	bool		hasState_GL_CULL_FACE;
	bool		hasState_GL_ALPHA_TEST;
	bool		hasState_GL_DEPTH_TEST;
	bool		hasState_GL_SCISSOR_TEST;
	bool		hasState_GL_COLOR_MATERIAL;
	bool		hasState_GL_TEXTURE_2D;
	bool		hasState_GL_BLEND;
	bool		hasState_GL_LIGHTING;
	bool		hasState_GL_FOG;
	bool		hasFlag_glDepthMask;
	TQ3ColorRGBA	viewportClearColor;
	TQ3ColorRGBA	backdropClearColor;
} RendererState;

typedef struct MeshQueueEntry
{
	int						numMeshes;
	TQ3TriMeshData**		meshPtrList;
	TQ3TriMeshData*			mesh0;
	const TQ3Matrix4x4*		transform;
	const RenderModifiers*	mods;
	float					depthSortZ;
} MeshQueueEntry;

#define MESHQUEUE_MAX_SIZE 4096

static MeshQueueEntry		gMeshQueueBuffer[MESHQUEUE_MAX_SIZE];
static MeshQueueEntry*		gMeshQueuePtrs[MESHQUEUE_MAX_SIZE];
static int					gMeshQueueSize = 0;

static int DepthSortCompare(void const* a_void, void const* b_void);
static void DrawMeshList(int renderPass, const MeshQueueEntry* entry);
static void DrawFadeOverlay(float opacity);

#pragma mark -

/****************************/
/*    CONSTANTS             */
/****************************/

static const RenderModifiers kDefaultRenderMods =
{
	.statusBits = 0,
	.diffuseColor = {1,1,1,1},
};

static const float kFreezeFrameFadeOutDuration = .33f;

//		2----3
//		| \  |
//		|  \ |
//		0----1
static const TQ3Point2D kFullscreenQuadPointsNDC[4] =
{
	{-1.0f, -1.0f},
	{ 1.0f, -1.0f},
	{-1.0f,  1.0f},
	{ 1.0f,  1.0f},
};

static const uint8_t kFullscreenQuadTriangles[2][3] =
{
	{0, 1, 2},
	{1, 3, 2},
};

static const TQ3Param2D kFullscreenQuadUVs[4] =
{
	{0, 1},
	{1, 1},
	{0, 0},
	{1, 0},
};


#pragma mark -

/****************************/
/*    VARIABLES             */
/****************************/

static RendererState gState;

static	GLuint			gBackdropTextureName = 0;
static	GLuint			gBackdropWidth = 0;
static	GLuint			gBackdropHeight = 0;

float					gFadeOverlayOpacity = 0;

#pragma mark -

/****************************/
/*    MACROS/HELPERS        */
/****************************/

static inline void ClearColorRGBA(TQ3ColorRGBA c)
{
	glClearColor(c.r, c.g, c.b, c.a);
}

static void SetInitialState_Impl(GLenum stateEnum, bool* stateFlagPtr, bool initialValue)
{
	*stateFlagPtr = initialValue;
	if (initialValue)
		glEnable(stateEnum);
	else
		glDisable(stateEnum);
	CHECK_GL_ERROR();
}

static void SetInitialClientState_Impl(GLenum stateEnum, bool* stateFlagPtr, bool initialValue)
{
	*stateFlagPtr = initialValue;
	if (initialValue)
		glEnableClientState(stateEnum);
	else
		glDisableClientState(stateEnum);
	CHECK_GL_ERROR();
}

static inline void SetState_Impl(GLenum stateEnum, bool* stateFlagPtr, bool enable)
{
	if (enable != *stateFlagPtr)
	{
		if (enable)
			glEnable(stateEnum);
		else
			glDisable(stateEnum);
		*stateFlagPtr = enable;
	}
	else
		gRenderStats.batchedStateChanges++;
}

static inline void SetClientState_Impl(GLenum stateEnum, bool* stateFlagPtr, bool enable)
{
	if (enable != *stateFlagPtr)
	{
		if (enable)
			glEnableClientState(stateEnum);
		else
			glDisableClientState(stateEnum);
		*stateFlagPtr = enable;
	}
	else
		gRenderStats.batchedStateChanges++;
}

#define SetInitialState(stateEnum, initialValue) SetInitialState_Impl(stateEnum, &gState.hasState_##stateEnum, initialValue)
#define SetInitialClientState(stateEnum, initialValue) SetInitialClientState_Impl(stateEnum, &gState.hasClientState_##stateEnum, initialValue)

#define EnableState(stateEnum) SetState_Impl(stateEnum, &gState.hasState_##stateEnum, true)
#define EnableClientState(stateEnum) SetClientState_Impl(stateEnum, &gState.hasClientState_##stateEnum, true)

#define DisableState(stateEnum) SetState_Impl(stateEnum, &gState.hasState_##stateEnum, false)
#define DisableClientState(stateEnum) SetClientState_Impl(stateEnum, &gState.hasClientState_##stateEnum, false)

#define RestoreStateFromBackup(stateEnum, backup) SetState_Impl(stateEnum, &gState.hasState_##stateEnum, (backup)->hasState_##stateEnum)
#define RestoreClientStateFromBackup(stateEnum, backup) SetClientState_Impl(stateEnum, &gState.hasClientState_##stateEnum, (backup)->hasClientState_##stateEnum)

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

void DoFatalGLError(GLenum error, const char* file, int line)
{
	static char alertbuf[1024];
	snprintf(alertbuf, sizeof(alertbuf), "OpenGL error 0x%x\nin %s:%d", error, file, line);
	DoFatalAlert(alertbuf);
}

void Render_SetDefaultModifiers(RenderModifiers* dest)
{
	memcpy(dest, &kDefaultRenderMods, sizeof(RenderModifiers));
}

void Render_InitState(void)
{
	SetInitialClientState(GL_VERTEX_ARRAY,				true);
	SetInitialClientState(GL_NORMAL_ARRAY,				true);
	SetInitialClientState(GL_COLOR_ARRAY,				false);
	SetInitialClientState(GL_TEXTURE_COORD_ARRAY,		false);
	SetInitialState(GL_NORMALIZE,		true);		// Normalize normal vectors. Required so lighting looks correct on scaled meshes.
	SetInitialState(GL_CULL_FACE,		true);
	SetInitialState(GL_ALPHA_TEST,		true);
	SetInitialState(GL_DEPTH_TEST,		true);
	SetInitialState(GL_SCISSOR_TEST,	false);
	SetInitialState(GL_COLOR_MATERIAL,	true);
	SetInitialState(GL_TEXTURE_2D,		false);
	SetInitialState(GL_BLEND,			false);
	SetInitialState(GL_LIGHTING,		true);
//	SetInitialState(GL_FOG,				true);		// Let game code manage fog

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	gState.blendFuncIsAdditive = false;		// must match glBlendFunc call above!  --No additive blending in Nanosaur

	glDepthMask(true);
	gState.hasFlag_glDepthMask = true;		// must match glDepthMask call above!

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//	gState.wantColorMask = true;			// must match glColorMask call above!

	gState.backdropClearColor = (TQ3ColorRGBA) {0, 0, 0, 1};
	gState.viewportClearColor = (TQ3ColorRGBA) {0, 0, 0.5f, 1};
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	ClearColorRGBA(gState.backdropClearColor);

	gState.boundTexture = 0;
//	gState.sceneHasFog = false;
//	gState.currentTransform = NULL;

	// Set misc GL defaults that apply throughout the entire game
	glAlphaFunc(GL_GREATER, 0.4999f);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// Set up mesh queue
	gMeshQueueSize = 0;
	for (int i = 0; i < MESHQUEUE_MAX_SIZE; i++)
		gMeshQueuePtrs[i] = &gMeshQueueBuffer[i];

	// Clear the buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CHECK_GL_ERROR();
}

void Render_SetBackdropClearColor(TQ3ColorRGBA clearColor)
{
	gState.backdropClearColor = clearColor;
}

void Render_SetViewportClearColor(TQ3ColorRGBA clearColor)
{
	gState.viewportClearColor = clearColor;
}

#pragma mark -

void Render_BindTexture(GLuint textureName)
{
	if (gState.boundTexture != textureName)
	{
		glBindTexture(GL_TEXTURE_2D, textureName);
		gState.boundTexture = textureName;
	}
	else
	{
		gRenderStats.batchedStateChanges++;
	}
}

GLuint Render_LoadTexture(
		GLenum internalFormat,
		int width,
		int height,
		GLenum bufferFormat,
		GLenum bufferType,
		const GLvoid* pixels,
		RendererTextureFlags flags)
{
	GLuint textureName;

	glGenTextures(1, &textureName);
	CHECK_GL_ERROR();

	Render_BindTexture(textureName);				// this is now the currently active texture
	CHECK_GL_ERROR();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gGamePrefs.highQualityTextures? GL_LINEAR: GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gGamePrefs.highQualityTextures? GL_LINEAR: GL_NEAREST);

	if (flags & kRendererTextureFlags_ClampU)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	if (flags & kRendererTextureFlags_ClampV)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(
			GL_TEXTURE_2D,
			0,						// mipmap level
			internalFormat,			// format in OpenGL
			width,					// width in pixels
			height,					// height in pixels
			0,						// border
			bufferFormat,			// what my format is
			bufferType,				// size of each r,g,b
			pixels);				// pointer to the actual texture pixels
	CHECK_GL_ERROR();

	return textureName;
}

void Render_Load3DMFTextures(TQ3MetaFile* metaFile, GLuint* outTextureNames)
{
	for (int i = 0; i < metaFile->numTextures; i++)
	{
		TQ3TextureShader* textureShader = &metaFile->textures[i];

		GAME_ASSERT(textureShader->pixmap);

		TQ3TexturingMode meshTexturingMode = kQ3TexturingModeOff;
		GLenum internalFormat;
		GLenum format;
		GLenum type;
		switch (textureShader->pixmap->pixelType)
		{
			case kQ3PixelTypeRGB16:
				meshTexturingMode = kQ3TexturingModeOpaque;
				internalFormat = GL_RGB;
				format = GL_BGRA;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			case kQ3PixelTypeARGB16:
				meshTexturingMode = kQ3TexturingModeAlphaTest;
				internalFormat = GL_RGBA;
				format = GL_BGRA;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			default:
				DoAlert("3DMF texture: Unsupported kQ3PixelType");
				continue;
		}

		outTextureNames[i] = Render_LoadTexture(
				internalFormat,						// format in OpenGL
				textureShader->pixmap->width,		// width in pixels
				textureShader->pixmap->height,		// height in pixels
				format,								// what my format is
				type,								// size of each r,g,b
				textureShader->pixmap->image,		// pointer to the actual texture pixels
				0);

		// Set glTextureName on meshes
		for (int j = 0; j < metaFile->numMeshes; j++)
		{
			if (metaFile->meshes[j]->internalTextureID == i)
			{
				metaFile->meshes[j]->glTextureName = outTextureNames[i];
				metaFile->meshes[j]->texturingMode = meshTexturingMode;
			}
		}
	}
}

#pragma mark -

void Render_StartFrame(void)
{
	// Clear rendering statistics
	memset(&gRenderStats, 0, sizeof(gRenderStats));

	// Clear transparent queue
	gMeshQueueSize = 0;
}

void Render_SetViewport(TQ3Area pane)
{
	int x = pane.min.x;
	int y = pane.min.y;
	int w = pane.max.x-pane.min.x;
	int h = pane.max.y-pane.min.y;
	bool needScissor = x != 0 || y != 0 || ((int)pane.max.x != gWindowWidth) || ((int)pane.max.y != gWindowHeight);

	if (needScissor)
	{
		EnableState(GL_SCISSOR_TEST);
		glScissor(x,y,w,h);
	}
	else
	{
		DisableState(GL_SCISSOR_TEST);
	}

	glViewport(x,y,w,h);

	// Clear color & depth buffers
	ClearColorRGBA(gState.viewportClearColor);
	EnableFlag(glDepthMask);	// The depth mask must be re-enabled so we can clear the depth buffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Render_EndFrame(void)
{
	// Keep track of transparent queue size for debug stats
	gRenderStats.meshQueueSize = gMeshQueueSize;

	// Flush mesh draw queue
	if (gMeshQueueSize != 0)
	{
		// Sort mesh draw queue, front to back
		qsort(
				gMeshQueuePtrs,
				gMeshQueueSize,
				sizeof(gMeshQueuePtrs[0]),
				DepthSortCompare
		);

		// PASS 1: draw opaque meshes, front to back
		for (int i = 0; i < gMeshQueueSize; i++)
		{
			DrawMeshList(kRenderPass_Opaque, gMeshQueuePtrs[i]);
		}

		// PASS 2: draw transparent meshes, back to front
		for (int i = gMeshQueueSize-1; i >= 0; i--)
		{
			DrawMeshList(kRenderPass_Transparent, gMeshQueuePtrs[i]);
		}

		// Clear mesh draw queue
		gMeshQueueSize = 0;
	}

	if (gState.hasState_GL_SCISSOR_TEST)
	{
		DisableState(GL_SCISSOR_TEST);
	}

#if ALLOW_FADE
	// Draw fade overlay
	if (gFadeOverlayOpacity > 0.01f)
	{
		DrawFadeOverlay(gFadeOverlayOpacity);
	}
#endif
}

#pragma mark -

void Render_SubmitMeshList(
		int						numMeshes,
		TQ3TriMeshData**		meshList,
		const TQ3Matrix4x4*		transform,
		const RenderModifiers*	mods,
		const TQ3Point3D*		centerCoord)
{
	TQ3Point3D coordInFrustum;
	Q3Point3D_Transform(centerCoord, &gCameraWorldToFrustumMatrix, &coordInFrustum);

	GAME_ASSERT(gMeshQueueSize < MESHQUEUE_MAX_SIZE);

	MeshQueueEntry* entry = gMeshQueuePtrs[gMeshQueueSize++];
	entry->numMeshes		= numMeshes;
	entry->meshPtrList		= meshList;
	entry->mesh0			= nil;
	entry->transform		= transform;
	entry->mods				= mods ? mods : &kDefaultRenderMods;
	entry->depthSortZ		= coordInFrustum.z;
}

void Render_SubmitMesh(
		TQ3TriMeshData*			mesh,
		const TQ3Matrix4x4*		transform,
		const RenderModifiers*	mods,
		const TQ3Point3D*		centerCoord)
{
	TQ3Point3D coordInFrustum;
	Q3Point3D_Transform(centerCoord, &gCameraWorldToFrustumMatrix, &coordInFrustum);

	GAME_ASSERT(gMeshQueueSize < MESHQUEUE_MAX_SIZE);

	MeshQueueEntry* entry = gMeshQueuePtrs[gMeshQueueSize++];
	entry->numMeshes		= 1;
	entry->meshPtrList		= &entry->mesh0;
	entry->mesh0			= mesh;
	entry->transform		= transform;
	entry->mods				= mods ? mods : &kDefaultRenderMods;
	entry->depthSortZ		= coordInFrustum.z;
}

#pragma mark -

static int DepthSortCompare(void const* a_void, void const* b_void)
{
	static const int AFirst		= -1;
	static const int BFirst		= +1;
	static const int DontCare	= 0;

	const MeshQueueEntry* a = *(MeshQueueEntry**) a_void;
	const MeshQueueEntry* b = *(MeshQueueEntry**) b_void;

	// First check manual priority
	if (a->mods->sortPriority < b->mods->sortPriority)
		return AFirst;

	if (a->mods->sortPriority > b->mods->sortPriority)
		return BFirst;

	// Next, if both A and B have the same manual priority, compare their depth coord
	if (a->depthSortZ < b->depthSortZ)		// A is closer to the camera
		return AFirst;

	if (a->depthSortZ > b->depthSortZ)		// B is closer to the camera
		return BFirst;

	return DontCare;
}

static void DrawMeshList(int renderPass, const MeshQueueEntry* entry)
{
	bool applyEnvironmentMap = entry->mods->statusBits & STATUS_BIT_REFLECTIONMAP;

	bool matrixPushedYet = false;

	for (int i = 0; i < entry->numMeshes; i++)
	{
		const TQ3TriMeshData* mesh = entry->meshPtrList[i];

		bool meshIsTransparent = mesh->texturingMode == kQ3TexturingModeAlphaBlend
				|| mesh->diffuseColor.a < .999f
				|| entry->mods->diffuseColor.a < .999f;

		// Decide whether or not to draw this mesh in this pass, depending on which pass we're in
		// (opaque or transparent), and whether the mesh has transparency.
		if ((renderPass == kRenderPass_Opaque		&& meshIsTransparent) ||
			(renderPass == kRenderPass_Transparent	&& !meshIsTransparent))
		{
			// Skip this mesh in this pass
			continue;
		}

		// Enable alpha blending if the mesh has transparency
		if (meshIsTransparent)
		{
			EnableState(GL_BLEND);
			DisableState(GL_ALPHA_TEST);
		}
		else
		{
			DisableState(GL_BLEND);
			EnableState(GL_ALPHA_TEST);
		}

		// Environment map effect
		if (applyEnvironmentMap)
		{
			EnvironmentMapTriMesh(mesh, entry->transform);
		}

		// Cull backfaces or not
		if (entry->mods->statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			if (meshIsTransparent)
			{
				// If we want to keep backfaces on a transparent mesh, keep GL_CULL_FACE enabled,
				// draw the backfaces first, then the frontfaces. This enhances the appearance of
				// e.g. Nanosaur shield spheres, without the need to depth-sort individual faces.
				
				EnableState(GL_CULL_FACE);
				glCullFace(GL_FRONT);		// Pass 1: draw backfaces (cull frontfaces)
				
				// Pass 2 (at the end of the function) will draw the mesh again with backfaces culled.
				// It will restore glCullFace to GL_BACK.
			}
			else
			{
				DisableState(GL_CULL_FACE);		// opaque mesh -- don't cull faces
			}
		}
		else
		{
			EnableState(GL_CULL_FACE);
		}

		// Apply gouraud or null illumination
		if (entry->mods->statusBits & STATUS_BIT_NULLSHADER)
		{
			DisableState(GL_LIGHTING);
		}
		else
		{
			EnableState(GL_LIGHTING);
		}

		// Write geometry to depth buffer or not
		if (meshIsTransparent || entry->mods->statusBits & STATUS_BIT_NOZWRITE)
		{
			DisableFlag(glDepthMask);
		}
		else
		{
			EnableFlag(glDepthMask);
		}

		// Texture mapping
		if (mesh->texturingMode != kQ3TexturingModeOff)
		{
			EnableState(GL_TEXTURE_2D);
			EnableClientState(GL_TEXTURE_COORD_ARRAY);
			Render_BindTexture(mesh->glTextureName);

			glTexCoordPointer(2, GL_FLOAT, 0, applyEnvironmentMap ? gEnvMapUVs : mesh->vertexUVs);
			CHECK_GL_ERROR();
		}
		else
		{
			DisableState(GL_TEXTURE_2D);
			DisableClientState(GL_TEXTURE_COORD_ARRAY);
			CHECK_GL_ERROR();
		}

		// Per-vertex colors (unused in Nanosaur, will be in Bugdom)
		if (mesh->hasVertexColors)
		{
			EnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, mesh->vertexColors);
		}
		else
		{
			DisableClientState(GL_COLOR_ARRAY);
		}

		// Apply diffuse color for the entire mesh
		glColor4f(
				mesh->diffuseColor.r * entry->mods->diffuseColor.r,
				mesh->diffuseColor.g * entry->mods->diffuseColor.g,
				mesh->diffuseColor.b * entry->mods->diffuseColor.b,
				mesh->diffuseColor.a * entry->mods->diffuseColor.a
				);

		// Submit transformation matrix if any
		if (!matrixPushedYet && entry->transform)
		{
			glPushMatrix();
			glMultMatrixf((float*)entry->transform->value);
			matrixPushedYet = true;
		}

		// Submit vertex and normal data
		glVertexPointer(3, GL_FLOAT, 0, mesh->points);
		glNormalPointer(GL_FLOAT, 0, mesh->vertexNormals);
		CHECK_GL_ERROR();

		// Draw the mesh
		glDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->triangles);
		CHECK_GL_ERROR();

		// Pass 2 to draw transparent meshes without face culling (see above for an explanation)
		if (meshIsTransparent && entry->mods->statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			glCullFace(GL_BACK);	// pass 2: draw frontfaces (cull backfaces)
			// We've restored glCullFace to GL_BACK, which is the default for all other meshes.
			
			// Draw the mesh again
			glDrawElements(GL_TRIANGLES, mesh->numTriangles * 3, GL_UNSIGNED_SHORT, mesh->triangles);
			CHECK_GL_ERROR();
		}

		// Update stats
		gRenderStats.trianglesDrawn += mesh->numTriangles;
	}

	if (matrixPushedYet)
	{
		glPopMatrix();
	}
}

#pragma mark -

//=======================================================================================================

/****************************/
/*    2D    */
/****************************/

static void Render_EnterExit2D(bool enter)
{
	static RendererState backup3DState;

	if (enter)
	{
		backup3DState = gState;
		DisableState(GL_LIGHTING);
		DisableState(GL_FOG);
		DisableState(GL_DEPTH_TEST);
		DisableState(GL_ALPHA_TEST);
//		DisableState(GL_TEXTURE_2D);
//		DisableClientState(GL_TEXTURE_COORD_ARRAY);
		DisableClientState(GL_COLOR_ARRAY);
		DisableClientState(GL_NORMAL_ARRAY);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-1, 1,  -1, 1, 0, 1000);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		RestoreStateFromBackup(GL_LIGHTING,		&backup3DState);
		RestoreStateFromBackup(GL_FOG,			&backup3DState);
		RestoreStateFromBackup(GL_DEPTH_TEST,	&backup3DState);
		RestoreStateFromBackup(GL_ALPHA_TEST,	&backup3DState);
//		RestoreStateFromBackup(GL_TEXTURE_2D,	&backup3DState);
//		RestoreClientStateFromBackup(GL_TEXTURE_COORD_ARRAY, &backup3DState);
		RestoreClientStateFromBackup(GL_COLOR_ARRAY,	&backup3DState);
		RestoreClientStateFromBackup(GL_NORMAL_ARRAY,	&backup3DState);
	}
}

void Render_Enter2D(void)
{
	Render_EnterExit2D(true);
}

void Render_Exit2D(void)
{
	Render_EnterExit2D(false);
}

static void Render_Draw2DFullscreenQuad(int fit)
{
	//		2----3
	//		| \  |
	//		|  \ |
	//		0----1
	TQ3Point2D pts[4] = {
			{-1,	-1},
			{ 1,	-1},
			{-1,	 1},
			{ 1,	 1},
	};

	float screenLeft   = 0.0f;
	float screenRight  = (float)gWindowWidth;
	float screenTop    = 0.0f;
	float screenBottom = (float)gWindowHeight;

	// Adjust screen coordinates if we want to pillarbox/letterbox the image.
	if (fit & (kBackdropFit_Letterbox | kBackdropFit_Pillarbox))
	{
		const float targetAspectRatio = (float) gWindowWidth / gWindowHeight;
		const float sourceAspectRatio = (float) gBackdropWidth / gBackdropHeight;

		if (fabsf(sourceAspectRatio - targetAspectRatio) < 0.1)
		{
			// source and window have nearly the same aspect ratio -- fit (no-op)
		}
		else if ((fit & kBackdropFit_Letterbox) && sourceAspectRatio > targetAspectRatio)
		{
			// source is wider than window -- letterbox
			float letterboxedHeight = gWindowWidth / sourceAspectRatio;
			screenTop = (gWindowHeight - letterboxedHeight) / 2;
			screenBottom = screenTop + letterboxedHeight;
		}
		else if ((fit & kBackdropFit_Pillarbox) && sourceAspectRatio < targetAspectRatio)
		{
			// source is narrower than window -- pillarbox
			float pillarboxedWidth = sourceAspectRatio * gWindowWidth / targetAspectRatio;
			screenLeft = (gWindowWidth / 2.0f) - (pillarboxedWidth / 2.0f);
			screenRight = screenLeft + pillarboxedWidth;
		}
	}

	// Compute normalized device coordinates for the quad vertices.
	float ndcLeft   = 2.0f * screenLeft  / gWindowWidth - 1.0f;
	float ndcRight  = 2.0f * screenRight / gWindowWidth - 1.0f;
	float ndcTop    = 1.0f - 2.0f * screenTop    / gWindowHeight;
	float ndcBottom = 1.0f - 2.0f * screenBottom / gWindowHeight;

	pts[0] = (TQ3Point2D) { ndcLeft, ndcBottom };
	pts[1] = (TQ3Point2D) { ndcRight, ndcBottom };
	pts[2] = (TQ3Point2D) { ndcLeft, ndcTop };
	pts[3] = (TQ3Point2D) { ndcRight, ndcTop };


	glColor4f(1, 1, 1, 1);
	EnableState(GL_TEXTURE_2D);
	EnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);
	glTexCoordPointer(2, GL_FLOAT, 0, kFullscreenQuadUVs);
	glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_BYTE, kFullscreenQuadTriangles);
}

#pragma mark -

//=======================================================================================================

TQ3Vector2D FitRectKeepAR(
		int logicalWidth,
		int logicalHeight,
		float displayWidth,
		float displayHeight)
{
	float displayAR = (float)displayWidth / (float)displayHeight;
	float logicalAR = (float)logicalWidth / (float)logicalHeight;

	if (displayAR >= logicalAR)
	{
		return (TQ3Vector2D) { displayHeight * logicalAR, displayHeight };
	}
	else
	{
		return (TQ3Vector2D) { displayWidth, displayWidth / logicalAR };
	}
}

/*******************************************/
/*    BACKDROP/OVERLAY (COVER WINDOW)      */
/*******************************************/

void Render_AllocBackdrop(int width, int height)
{
	GAME_ASSERT_MESSAGE(gBackdropTextureName == 0, "cover texture already allocated");

	gBackdropWidth = width;
	gBackdropHeight = height;

	gBackdropTextureName = Render_LoadTexture(
			GL_RGBA,
			width,
			height,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8,
			gBackdropPixels,
			kRendererTextureFlags_ClampBoth
	);

	ClearPortDamage();
}

void Render_DisposeBackdrop(void)
{
	if (gBackdropTextureName == 0)
		return;

	glDeleteTextures(1, &gBackdropTextureName);
	gBackdropTextureName = 0;
}

void Render_ClearBackdrop(UInt32 argb)
{
	UInt32 bgra = UnpackU32BE(&argb);

	UInt32* backdropPixPtr = gBackdropPixels;

	for (GLuint i = 0; i < gBackdropWidth * gBackdropHeight; i++)
	{
		*(backdropPixPtr++) = bgra;
	}

	GrafPtr port;
	GetPort(&port);
	DamagePortRegion(&port->portRect);
}

void Render_DrawBackdrop(int fit)
{
	if (fit != kBackdropFit_FillScreen)
	{
		ClearColorRGBA(gState.backdropClearColor);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	if (gBackdropTextureName == 0)
		return;

	Render_BindTexture(gBackdropTextureName);

	// If the screen port has dirty pixels ("damaged"), update the texture
	if (IsPortDamaged())
	{
		Rect damageRect;
		GetPortDamageRegion(&damageRect);

		// Set unpack row length to 640
		GLint pUnpackRowLength;
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pUnpackRowLength);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, gBackdropWidth);

		glTexSubImage2D(
				GL_TEXTURE_2D,
				0,
				damageRect.left,
				damageRect.top,
				damageRect.right - damageRect.left,
				damageRect.bottom - damageRect.top,
				GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8,
				gBackdropPixels + (damageRect.top * gBackdropWidth + damageRect.left));
		CHECK_GL_ERROR();

		// Restore unpack row length
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pUnpackRowLength);

		ClearPortDamage();
	}

	glViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D();
	Render_Draw2DFullscreenQuad(fit);
	Render_Exit2D();
}

static void DrawFadeOverlay(float opacity)
{
	glViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D();
	EnableState(GL_BLEND);
	DisableState(GL_TEXTURE_2D);
	DisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(0, 0, 0, opacity);
	glVertexPointer(2, GL_FLOAT, 0, kFullscreenQuadPointsNDC);
	glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_BYTE, kFullscreenQuadTriangles);
	Render_Exit2D();
}

#pragma mark -

void Render_SetWindowGamma(float percent)
{
	gFadeOverlayOpacity = (100.0f - percent) / 100.0f;
}

void Render_FreezeFrameFadeOut(void)
{
#if !ALLOW_FADE
	return;
#endif

	Uint32 startTicks = SDL_GetTicks();
//	gFadeOverlayOpacity = 0;

	while (gFadeOverlayOpacity < 1)
	{
		Uint32 ticks = SDL_GetTicks();
		gFadeOverlayOpacity = ((ticks - startTicks) / 1000.0f / kFreezeFrameFadeOutDuration);
		if (gFadeOverlayOpacity > 1.0f)
			gFadeOverlayOpacity = 1.0f;

		UpdateInput();

		if (!gGameViewInfoPtr)
		{
			Render_StartFrame();
			Render_DrawBackdrop(kBackdropFit_KeepRatio);
			Render_EndFrame();
			SDL_GL_SwapWindow(gSDLWindow);
		}
		else if (gTerrainPtr)
		{
			QD3D_DrawScene(gGameViewInfoPtr, DrawTerrain);
		}
		else
		{
			QD3D_DrawScene(gGameViewInfoPtr, DrawObjects);
		}

//		if (fadeSound)
//		{
//			FadeGlobalVolume(gGammaFadeFactor);
//		}

		QD3D_CalcFramesPerSecond();
//		DoSDLMaintenance();
	}

	gFadeOverlayOpacity = 1;		// hold pitch black until fading back in

//	if (fadeSound)
//	{
//		StopAllEffectChannels();
//		KillSong();
//		FadeGlobalVolume(1);
//	}
//
//	FlushMouseButtonPress();
}
