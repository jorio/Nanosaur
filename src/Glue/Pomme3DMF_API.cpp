#include "Pomme.h"
#include "PommeFiles.h"
#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"

#define CHECK_COOKIE(obj)												\
	do																	\
	{																	\
		if ( (obj).cookie != (obj).COOKIE )								\
			throw std::runtime_error("Pomme3DMF: illegal cookie");		\
	} while (0)

Pomme3DMF_FileHandle Pomme3DMF_LoadModelFile(const FSSpec* spec)
{
	short refNum;
	OSErr err;

	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (err != noErr)
		return nullptr;

	Q3MetaFile* metaFile = new Q3MetaFile();

	auto& fileStream = Pomme::Files::GetStream(refNum);
	Q3MetaFileParser(fileStream, *metaFile).Parse3DMF();
	FSClose(refNum);

	return (Pomme3DMF_FileHandle) metaFile;
}

void Pomme3DMF_DisposeModelFile(Pomme3DMF_FileHandle the3DMFFile)
{
	if (!the3DMFFile)
		return;

	auto metaFile = (Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(*metaFile);
	delete metaFile;
}

int Pomme3DMF_CountTopLevelMeshGroups(Pomme3DMF_FileHandle the3DMFFile)
{
	auto& metaFile = *(Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(metaFile);

	return metaFile.topLevelMeshGroups.size();
}

Pomme3DMF_MeshGroupHandle Pomme3DMF_GetTopLevelMeshGroup(Pomme3DMF_FileHandle the3DMFFile, int groupNumber)
{
	auto& metaFile = *(Q3MetaFile*) the3DMFFile;
	CHECK_COOKIE(metaFile);

	return metaFile.topLevelMeshGroups.at(groupNumber).data();
}
