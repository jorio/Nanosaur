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

// Sets up the initial renderer state.
// Call this function after creating the OpenGL context.
void Render_InitState(void);

// Uploads all textures from a 3DMF file to the GPU.
// Requires an OpenGL context to be active.
void Render_Load3DMFTextures(TQ3MetaFile* metaFile);

// Deletes OpenGL texture names previously loaded from a 3DMF file.
void Render_Dispose3DMFTextures(TQ3MetaFile* metaFile);

// Instructs the renderer to get ready to draw a new frame.
// Call this function before submitting any draw calls.
void Render_StartFrame(void);

// Submits a list of trimeshes for drawing.
// Requires an OpenGL context to be active.
// Arguments transform and mods may be nil.
void Render_DrawTriMeshList(
		int numMeshes,
		TQ3TriMeshData** meshList,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods);

