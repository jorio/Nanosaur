#include "structpack.h"

#include "globals.h"
#include "sprites.h"
#include "file.h"
#include "Terrain.h"

void RegisterUnpackableTypes()
{
	// to unpack simple arrays
	structpack::Register<SInt16>("h");
	structpack::Register<UInt16>("H");
	structpack::Register<SInt32>("l");
	structpack::Register<UInt32>("L");
	structpack::Register<float>("f");
	structpack::Register<double>("d");

	structpack::Register<SkeletonFile_Header_Type>		("> h h h h");
	structpack::Register<File_BoneDefinitionType>		("> l 32c 3f H H 8L");
	structpack::Register<Joit_Rez_Type>					("> 3f 3f l 32c l");
	structpack::Register<SkeletonFile_AnimHeader_Type>	("> B32cx h");

	structpack::Register<AnimEntryType>					("> h h");
	structpack::Register<FramesFile_Header_Type>		("> H H H 3H");
	structpack::Register<FrameHeaderType>				("> l l l l ?3x");
	structpack::Register<AnimHeaderType>                ("> H");

	structpack::Register<TileAttribType>				("> H h 2b h");
}
