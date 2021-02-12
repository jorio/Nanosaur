#pragma once

#include <QD3D.h>

typedef struct RenderStats
{
	int			nodesDrawn;
	int			trianglesDrawn;
	int			meshesDrawn;
	int 		batchedStateChanges;
} RenderStats;

typedef struct RenderModifiers
{
	uint32_t 				statusBits;
	TQ3ColorRGBA			diffuseColor;
} RenderModifiers;

typedef enum
{
	kRendererTextureFlags_None			= 0,
	kRendererTextureFlags_ClampU		= (1 << 0),
	kRendererTextureFlags_ClampV		= (1 << 1),
	kRendererTextureFlags_ClampBoth		= kRendererTextureFlags_ClampU | kRendererTextureFlags_ClampV,
} RendererTextureFlags;

// Sets up the initial renderer state.
// Call this function after creating the OpenGL context.
void Render_InitState(void);

// Wrapper for glTexImage that takes care of all the boilerplate associated with texture creation.
// Returns an OpenGL texture name.
// Aborts the game on failure.
GLuint Render_LoadTexture(
		GLenum internalFormat,
		int width,
		int height,
		GLenum bufferFormat,
		GLenum bufferType,
		const GLvoid* pixels,
		RendererTextureFlags flags
);

// Uploads all textures from a 3DMF file to the GPU.
// Requires an OpenGL context to be active.
void Render_Load3DMFTextures(TQ3MetaFile* metaFile);

// Deletes OpenGL texture names previously loaded from a 3DMF file.
void Render_Dispose3DMFTextures(TQ3MetaFile* metaFile);

void Render_Enter2D(void);

void Render_Exit2D(void);

void Render_Draw2DFullscreenQuad(void);

// Instructs the renderer to get ready to draw a new frame.
// Call this function before submitting any draw calls.
void Render_StartFrame(void);

// Fills the argument with the default mesh rendering modifiers.
void Render_SetDefaultModifiers(RenderModifiers* dest);

// Submits a list of trimeshes for drawing.
// Requires an OpenGL context to be active.
// Arguments transform and mods may be nil.
void Render_DrawTriMeshList(
		int numMeshes,
		TQ3TriMeshData** meshList,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods);

