#pragma once

#include <QD3D.h>

typedef struct RenderStats
{
	int			nodesDrawn;
	int			trianglesDrawn;
	int			meshesDrawn;
	int 		batchedStateChanges;
} RenderStats;


void Render_InitState(void);

void Render_DrawTriMeshList(int numMeshes, TQ3TriMeshData** meshList, bool envMap, const TQ3Matrix4x4* transform);

