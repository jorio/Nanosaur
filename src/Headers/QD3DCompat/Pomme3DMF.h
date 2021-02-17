#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "PommeTypes.h"

#include "QD3D.h"

#pragma mark -

TQ3MetaFile* Q3MetaFile_Load3DMF(const FSSpec* spec);

void Q3MetaFile_Dispose(TQ3MetaFile* the3DMFFile);

#pragma mark -

void Q3Pixmap_ApplyEdgePadding(TQ3Pixmap*);

void Q3Pixmap_Dispose(TQ3Pixmap*);

#pragma mark -

TQ3TriMeshData* Q3TriMeshData_New(int numTriangles, int numPoints);

TQ3TriMeshData* Q3TriMeshData_Duplicate(const TQ3TriMeshData* source);

void Q3TriMeshData_Dispose(TQ3TriMeshData*);

void Q3TriMeshData_SubdivideTriangles(TQ3TriMeshData* src);

#ifdef __cplusplus
}
#endif
