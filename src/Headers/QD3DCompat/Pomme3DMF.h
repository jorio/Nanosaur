#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "PommeTypes.h"

typedef void*			Pomme3DMF_FileHandle;
typedef uint32_t*		Pomme3DMF_MeshGroupHandle;


Pomme3DMF_FileHandle Pomme3DMF_LoadModelFile(const FSSpec* spec);

void Pomme3DMF_DisposeModelFile(Pomme3DMF_FileHandle the3DMFFile);

int Pomme3DMF_CountTopLevelMeshGroups(Pomme3DMF_FileHandle the3DMFFile);

Pomme3DMF_MeshGroupHandle Pomme3DMF_GetTopLevelMeshGroup(Pomme3DMF_FileHandle the3DMFFile, int groupNumber);



#ifdef __cplusplus
}
#endif
