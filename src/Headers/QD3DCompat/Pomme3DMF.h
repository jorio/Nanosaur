#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "PommeTypes.h"

#include "QD3D.h"

typedef void*				Pomme3DMF_FileHandle;

#pragma mark -

Pomme3DMF_FileHandle Pomme3DMF_LoadModelFile(const FSSpec* spec);

void Pomme3DMF_DisposeModelFile(Pomme3DMF_FileHandle the3DMFFile);

TQ3TriMeshFlatGroup Pomme3DMF_GetAllMeshes(Pomme3DMF_FileHandle the3DMFFile);

int Pomme3DMF_CountTopLevelMeshGroups(Pomme3DMF_FileHandle the3DMFFile);

TQ3TriMeshFlatGroup Pomme3DMF_GetTopLevelMeshGroup(Pomme3DMF_FileHandle the3DMFFile, int groupNumber);

#pragma mark -

TQ3TriMeshData* Q3TriMeshData_New(int numTriangles, int numPoints);

TQ3TriMeshData* Q3TriMeshData_Duplicate(const TQ3TriMeshData* source);

void Q3TriMeshData_Dispose(TQ3TriMeshData*);

#ifdef __cplusplus
}
#endif
