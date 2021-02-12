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
extern PrefsType				gGamePrefs;
extern UInt32*const				gCoverWindowPixPtr;
extern int						gWindowWidth;
extern int						gWindowHeight;

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
	bool		hasState_GL_FOG;
	bool		hasFlag_glDepthMask;
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

static	GLuint			gCoverWindowTextureName = 0;
static	GLuint			gCoverWindowTextureWidth = 0;
static	GLuint			gCoverWindowTextureHeight = 0;

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

static inline void __SetState(GLenum stateEnum, bool* stateFlagPtr, bool enable)
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

static inline void __SetClientState(GLenum stateEnum, bool* stateFlagPtr, bool enable)
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

#define SetInitialState(stateEnum, initialValue) __SetInitialState(stateEnum, &gState.hasState_##stateEnum, initialValue)
#define SetInitialClientState(stateEnum, initialValue) __SetInitialClientState(stateEnum, &gState.hasClientState_##stateEnum, initialValue)

#define EnableState(stateEnum) __SetState(stateEnum, &gState.hasState_##stateEnum, true)
#define EnableClientState(stateEnum) __SetClientState(stateEnum, &gState.hasClientState_##stateEnum, true)

#define DisableState(stateEnum) __SetState(stateEnum, &gState.hasState_##stateEnum, false)
#define DisableClientState(stateEnum) __SetClientState(stateEnum, &gState.hasClientState_##stateEnum, false)

#define RestoreStateFromBackup(stateEnum, backup) __SetState(stateEnum, &gState.hasState_##stateEnum, (backup)->hasState_##stateEnum)

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
				format = GL_BGRA;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			case kQ3PixelTypeARGB16:
				internalFormat = GL_RGBA;
				format = GL_BGRA;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			default:
				DoAlert("3DMF texture: Unsupported kQ3PixelType");
				continue;
		}

		textureDef->glTextureName = Render_LoadTexture(
					 internalFormat,						// format in OpenGL
					 textureDef->width,						// width in pixels
					 textureDef->height,					// height in pixels
					 format,								// what my format is
					 type,									// size of each r,g,b
					 textureDef->image,						// pointer to the actual texture pixels
					 0);

		// Set glTextureName on meshes
		for (int j = 0; j < metaFile->numMeshes; j++)
		{
			if (metaFile->meshes[j]->hasTexture && metaFile->meshes[j]->internalTextureID == i)
			{
				metaFile->meshes[j]->glTextureName = textureDef->glTextureName;
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

void Render_SetDefaultModifiers(RenderModifiers* dest)
{
	memcpy(dest, &kDefaultRenderMods, sizeof(RenderModifiers));
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
			Render_BindTexture(mesh->glTextureName);

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
//		DisableState(GL_TEXTURE_2D);
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
//		RestoreStateFromBackup(GL_TEXTURE_2D,	&backup3DState);
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

	static const TQ3Param2D uvs[4] = {
			{0,		1},
			{1,		1},
			{0,		0},
			{1,		0},
	};

	static const uint8_t tris[2][3] = {
			{0, 1, 2},
			{1, 3, 2},
	};


	float screenLeft   = 0.0f;
	float screenRight  = (float)gWindowWidth;
	float screenTop    = 0.0f;
	float screenBottom = (float)gWindowHeight;
	bool needClear = false;

	// Adjust screen coordinates if we want to pillarbox/letterbox the image.
	if (fit & (kCoverQuadLetterbox | kCoverQuadPillarbox))
	{
		const float targetAspectRatio = (float) gWindowWidth / gWindowHeight;
		const float sourceAspectRatio = (float) gCoverWindowTextureWidth / gCoverWindowTextureHeight;

		if (fabsf(sourceAspectRatio - targetAspectRatio) < 0.1)
		{
			// source and window have nearly the same aspect ratio -- fit (no-op)
		}
		else if ((fit & kCoverQuadLetterbox) && sourceAspectRatio > targetAspectRatio)
		{
			// source is wider than window -- letterbox
			needClear = true;
			float letterboxedHeight = gWindowWidth / sourceAspectRatio;
			screenTop = (gWindowHeight - letterboxedHeight) / 2;
			screenBottom = screenTop + letterboxedHeight;
		}
		else if ((fit & kCoverQuadPillarbox) && sourceAspectRatio < targetAspectRatio)
		{
			// source is narrower than window -- pillarbox
			needClear = true;
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
	DisableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);
	glTexCoordPointer(2, GL_FLOAT, 0, uvs);
	__glDrawRangeElements(GL_TRIANGLES, 0, 3*2, 3*2, GL_UNSIGNED_BYTE, tris);
}

#pragma mark -

//=======================================================================================================

/*******************************************/
/*    BACKDROP/OVERLAY (COVER WINDOW)      */
/*******************************************/

void Render_Alloc2DCover(int width, int height)
{
	GAME_ASSERT_MESSAGE(gCoverWindowTextureName == 0, "cover texture already allocated");

	gCoverWindowTextureWidth = width;
	gCoverWindowTextureHeight = height;

	gCoverWindowTextureName = Render_LoadTexture(
			GL_RGBA,
			width,
			height,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8,
			gCoverWindowPixPtr,
			kRendererTextureFlags_ClampBoth
	);

	ClearPortDamage();
}

void Render_Dispose2DCover(void)
{
	if (gCoverWindowTextureName == 0)
		return;

	glDeleteTextures(1, &gCoverWindowTextureName);
	gCoverWindowTextureName = 0;
}

void Render_Clear2DCover(UInt32 argb)
{
	UInt32 bgra = Byteswap32(&argb);

	UInt32* backdropPixPtr = gCoverWindowPixPtr;

	for (GLuint i = 0; i < gCoverWindowTextureWidth * gCoverWindowTextureHeight; i++)
	{
		*(backdropPixPtr++) = bgra;
	}

	GrafPtr port;
	GetPort(&port);
	DamagePortRegion(&port->portRect);
}

void Render_Draw2DCover(int fit)
{
	if (gCoverWindowTextureName == 0)
		return;

	Render_BindTexture(gCoverWindowTextureName);

	// If the screen port has dirty pixels ("damaged"), update the texture
	if (IsPortDamaged())
	{
		Rect damageRect;
		GetPortDamageRegion(&damageRect);

		// Set unpack row length to 640
		GLint pUnpackRowLength;
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pUnpackRowLength);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, gCoverWindowTextureWidth);

		glTexSubImage2D(
				GL_TEXTURE_2D,
				0,
				damageRect.left,
				damageRect.top,
				damageRect.right - damageRect.left,
				damageRect.bottom - damageRect.top,
				GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8,
				gCoverWindowPixPtr + (damageRect.top * gCoverWindowTextureWidth + damageRect.left));
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

#pragma -

void SetWindowGamma(int percent)
{
	printf("TODO noquesa: %s\n", __func__);
//	SDL_SetWindowBrightness(gSDLWindow, percent / 100.0f);
}
