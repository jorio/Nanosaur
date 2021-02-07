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

void Render_InitState(void);

void Render_Load3DMFTextures(TQ3MetaFile* metaFile);

void Render_Dispose3DMFTextures(TQ3MetaFile* metaFile);

void Render_DrawTriMeshList(
		int numMeshes,
		TQ3TriMeshData** meshList,
		const TQ3Matrix4x4* transform,
		const RenderModifiers* mods);

