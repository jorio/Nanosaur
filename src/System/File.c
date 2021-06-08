/****************************/
/*      FILE ROUTINES       */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	<QD3D.h>
#include	<string.h>
#include	"globals.h"
#include 	"objects.h"
#include	"misc.h"
#include	"skeletonanim.h"
#include	"skeletonobj.h"
#include	"skeletonjoints.h"
#include	"3dmf.h"
#include 	"mobjtypes.h"
#include	"file.h"
#include    "window.h"
#include 	"main.h"
#include 	"terrain.h"
#include 	"sprites.h"
#include 	"bones.h"
#include 	"sound2.h"
#include	"structformats.h"

extern	short			gMainAppRezFile;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID,gNumTerrainTextureTiles;
extern	Ptr		gTerrainPtr,gTerrainHeightMapPtrs[];
extern	long	gTerrainTileWidth,gTerrainTileDepth,gTerrainUnitWidth,gTerrainUnitDepth;		
extern	long	gNumSuperTilesDeep,gNumSuperTilesWide;
extern	UInt16	**gTerrainTextureLayer,**gTerrainHeightMapLayer,**gTerrainPathLayer,*gTileDataPtr;
extern	TileAttribType	*gTileAttributes;
extern	long	gCurrentSuperTileRow,gCurrentSuperTileCol;
extern	long	gMyStartX,gMyStartZ;
extern	FSSpec	gDataSpec;
extern	const int	PRO_MODE;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *target);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1




/**********************/
/*     VARIABLES      */
/**********************/

Ptr		gTileFilePtr = nil;


/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
// 
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType)
{
OSErr		iErr;
short		fRefNum;
FSSpec		fsSpecSkel;
FSSpec		fsSpec3DMF;
SkeletonDefType	*skeleton;
const char* modelName = "Unknown";

				/* SET CORRECT FILENAME */

	switch(skeletonType)
	{
		case	SKELETON_TYPE_PTERA:	modelName = "Ptera";	break;
		case	SKELETON_TYPE_REX:		modelName = "Rex";		break;
		case	SKELETON_TYPE_STEGO:	modelName = "Stego";	break;
		case	SKELETON_TYPE_DEINON:	modelName = "Deinon";	break;
		case	SKELETON_TYPE_TRICER:	modelName = "Tricer";	break;
		case	SKELETON_TYPE_SPITTER:	modelName = "Diloph";	break;
		default:
				DoFatalAlert("LoadSkeleton: Unknown skeletonType!");
	}

	char filename[256];
	
	snprintf(filename, sizeof(filename), ":Skeletons:%s.skeleton", modelName);
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, filename, &fsSpecSkel);

	snprintf(filename, sizeof(filename), ":Skeletons:%s.3dmf", modelName);
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, filename, &fsSpec3DMF);


			/* OPEN THE FILE'S REZ FORK */

	fRefNum = FSpOpenResFile(&fsSpecSkel, fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		Str255 numStr;
		NumToStringC(iErr, numStr);
		DoFatalAlert2("Error opening Skel Rez file", numStr);
	}

	UseResFile(fRefNum);
	GAME_ASSERT(noErr == ResError());


			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */

	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	GAME_ASSERT(skeleton);


			/* READ SKELETON RESOURCES */

	ReadDataFromSkeletonFile(skeleton, &fsSpec3DMF);
	PrimeBoneData(skeleton);
	
			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);
		
		
	return(skeleton);
}


/************* READ DATA FROM SKELETON FILE *******************/
//
// Current rez file is set to the file. 
//

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *target)
{
Handle				hand;
long				i,k,j;
long				numJoints,numAnims,numKeyframes;
AnimEventType		*animEventPtr;
JointKeyframeType	*keyFramePtr;
SkeletonFile_Header_Type	*headerPtr;
short				version;
TQ3Point3D				*pointPtr;
SkeletonFile_AnimHeader_Type	*animHeaderPtr;


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	GAME_ASSERT(hand);

	headerPtr = (SkeletonFile_Header_Type *) *hand;
	ByteswapStructs(STRUCTFORMAT_SkeletonFile_Header_Type, sizeof(SkeletonFile_Header_Type), 1, headerPtr);
	version = headerPtr->version;
	GAME_ASSERT_MESSAGE(version == SKELETON_FILE_VERS_NUM, "Skeleton file has wrong version #");

	numAnims = skeleton->NumAnims = headerPtr->numAnims;			// get # anims in skeleton
	numJoints = skeleton->NumBones = headerPtr->numJoints;			// get # joints in skeleton
	ReleaseResource(hand);

	GAME_ASSERT(numJoints <= MAX_JOINTS);							// check for overload


				/*************************************/
				/* ALLOCATE MEMORY FOR SKELETON DATA */
				/*************************************/

	AllocSkeletonDefinitionMemory(skeleton);



		/********************************/
		/* 	LOAD THE REFERENCE GEOMETRY */
		/********************************/

	LoadBonesReferenceModel(target, skeleton);


		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		UInt16					*indexPtr;

			/* READ BONE DATA */
			
		hand = GetResource('Bone',1000+i);
		GAME_ASSERT(hand);
		HLock(hand);
		bonePtr = (File_BoneDefinitionType *) *hand;
		ByteswapStructs(STRUCTFORMAT_File_BoneDefinitionType, sizeof(File_BoneDefinitionType), 1, bonePtr);


			/* COPY BONE DATA INTO ARRAY */
		
		skeleton->Bones[i].parentBone = bonePtr->parentBone;								// index to previous bone
		skeleton->Bones[i].coord = bonePtr->coord;											// absolute coord (not relative to parent!)
		skeleton->Bones[i].numPointsAttachedToBone = bonePtr->numPointsAttachedToBone;		// # vertices/points that this bone has
		skeleton->Bones[i].numNormalsAttachedToBone = bonePtr->numNormalsAttachedToBone;	// # vertex normals this bone has		
		ReleaseResource(hand);

			/* ALLOC THE POINT & NORMALS SUB-ARRAYS */
				
		skeleton->Bones[i].pointList = (UInt16 *)AllocPtr(sizeof(UInt16) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		GAME_ASSERT(skeleton->Bones[i].pointList);

		skeleton->Bones[i].normalList = (UInt16 *)AllocPtr(sizeof(UInt16) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		GAME_ASSERT(skeleton->Bones[i].normalList);

			/* READ POINT INDEX ARRAY */
			
		hand = GetResource('BonP',1000+i);
		GAME_ASSERT(hand);
		HLock(hand);
		indexPtr = (UInt16 *) *hand;
		ByteswapInts(sizeof(UInt16), skeleton->Bones[i].numPointsAttachedToBone, indexPtr);
			

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = indexPtr[j];
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */
			
		hand = GetResource('BonN',1000+i);
		GAME_ASSERT(hand);
		HLock(hand);
		indexPtr = (UInt16 *) *hand;
		ByteswapInts(sizeof(UInt16), skeleton->Bones[i].numNormalsAttachedToBone, indexPtr);
			
			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = indexPtr[j];
		ReleaseResource(hand);
						
	}
	
	
		/*******************************/
		/* READ POINT RELATIVE OFFSETS */
		/*******************************/
		//
		// The "relative point offsets" are the only things
		// which do not get rebuilt in the ModelDecompose function.
		// We need to restore these manually.
	
	hand = GetResource('RelP', 1000);
	GAME_ASSERT(hand);
	HLock(hand);
	
	if ((GetHandleSize(hand) / (Size)sizeof(TQ3Point3D)) != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
	{
		pointPtr = (TQ3Point3D *) *hand;
		ByteswapStructs(STRUCTFORMAT_TQ3Point3D, sizeof(TQ3Point3D), skeleton->numDecomposedPoints, pointPtr);

		for (i = 0; i < skeleton->numDecomposedPoints; i++)
			skeleton->decomposedPointList[i].boneRelPoint = pointPtr[i];
	}

	ReleaseResource(hand);
	
	
			/*********************/
			/* READ ANIM INFO   */
			/*********************/
			
	for (i=0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		GAME_ASSERT(hand);
		HLock(hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *) *hand;
		ByteswapStructs(STRUCTFORMAT_SkeletonFile_AnimHeader_Type, sizeof(SkeletonFile_AnimHeader_Type), 1, animHeaderPtr);

		skeleton->NumAnimEvents[i] = animHeaderPtr->numAnimEvents;			// copy # anim events in anim	
		ReleaseResource(hand);


			/* READ ANIM-EVENT DATA */
			
		hand = GetResource('Evnt',1000+i);
		GAME_ASSERT(hand);
		animEventPtr = (AnimEventType *) *hand;
		ByteswapStructs(STRUCTFORMAT_AnimEventType, sizeof(AnimEventType), skeleton->NumAnimEvents[i], animEventPtr);
		for (j=0;  j < skeleton->NumAnimEvents[i]; j++)
			skeleton->AnimEventsList[i][j] = *animEventPtr++;
		ReleaseResource(hand);		


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */
					
		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		GAME_ASSERT(hand);
		for (j=0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (j=0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */
				
		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);
		
		if ((skeleton->JointKeyframes[j].keyFrames == nil) ||
			(skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("ReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */
					
		for (i=0; i < numAnims; i++)								
		{
			numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];					// get actual # of keyframes for this joint
			GAME_ASSERT(numKeyframes <= MAX_KEYFRAMES);
		
					/* READ A JOINT KEYFRAME */
					
			hand = GetResource('KeyF',1000+(i*100)+j);
			GAME_ASSERT(hand);
			keyFramePtr = (JointKeyframeType *) *hand;
			ByteswapStructs(STRUCTFORMAT_JointKeyframeType, sizeof(JointKeyframeType), numKeyframes, keyFramePtr);
			for (k = 0; k < numKeyframes; k++)												// copy this joint's keyframes for this anim
				skeleton->JointKeyframes[j].keyFrames[i][k] = *keyFramePtr++;
			ReleaseResource(hand);		
		}
	}
	
}



/**************** OPEN GAME FILE **********************/

void	OpenGameFile(const char* filename, short *fRefNumPtr, const char* errString)
{
OSErr		iErr;
FSSpec		spec;
Str255		s;

				/* FIRST SEE IF WE CAN GET IT OFF OF DEFAULT VOLUME */

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, filename, &spec);
	if (iErr == noErr)
	{
		iErr = FSpOpenDF(&spec, fsRdPerm, fRefNumPtr);
		if (iErr == noErr)
			return;
	}

	if (iErr == fnfErr)
		DoFatalAlert2(errString,"FILE NOT FOUND.");
	else
	{
		NumToStringC(iErr,s);
		DoFatalAlert2(errString,s);
	}
}




/******************** MAKE PREFS FSSPEC **********************/

OSErr MakePrefsFSSpec(const char* prefFileName, FSSpec* spec)
{
	static Boolean checkedOnce = false;
	static const char* PREFS_FOLDER = "Nanosaur";

	if (!checkedOnce)
	{
		checkedOnce = true;

		OSErr iErr = FindFolder(
				kOnSystemDisk,
				kPreferencesFolderType,
				kDontCreateFolder,
				&gPrefsFolderVRefNum,
				&gPrefsFolderDirID);

		if (iErr != noErr)
			DoAlert("Warning: Cannot locate Preferences folder.");

		long createdDirID;
		DirCreate(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FOLDER, &createdDirID);
	}

	char name[256];
	snprintf(name, 256, ":%s:%s", PREFS_FOLDER, prefFileName);
	return FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, name, spec);
}



/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(PrefsType *prefBlock)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;
PrefsType	prefs;
				
				/*************/
				/* READ FILE */
				/*************/
					
	MakePrefsFSSpec("Prefs", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr)
		return(iErr);

	iErr = GetEOF(refNum, &count);
	if (iErr)
	{
		FSClose(refNum);
		return iErr;
	}

	if (count != sizeof(PrefsType))
	{
		// size of file doesn't match size of struct
		FSClose(refNum);
		return badFileFormat;
	}

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count,  (Ptr)&prefs);		// read data from file
	FSClose(refNum);
	if (iErr
		|| count < (long)sizeof(PrefsType)
		|| 0 != strncmp(PREFS_MAGIC, prefs.magic, sizeof(prefs.magic)))
	{
		return(iErr);
	}
	
	*prefBlock = prefs;
	
	return(noErr);
}


/******************** SAVE PREFS **********************/

void SavePrefs(PrefsType *prefs)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;
						
				/* CREATE BLANK FILE */
				
	MakePrefsFSSpec("Prefs", &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'NanO', 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */
					
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return;
	}
		
				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, (Ptr)prefs);	
	FSClose(refNum);
}




/********************** LOAD A FILE **************************/

Ptr	LoadAFile(FSSpec* fsSpec, long* outSize)
{
OSErr	iErr;
short 	fRefNum;
long	size;
Ptr		data;


			/* OPEN FILE */

	iErr = FSpOpenDF(fsSpec, fsRdPerm, &fRefNum);
	GAME_ASSERT_MESSAGE(iErr == noErr, fsSpec->cName);


			/* GET SIZE OF FILE */

	iErr = GetEOF(fRefNum, &size);
	GAME_ASSERT(iErr == noErr);


			/* ALLOC MEMORY FOR FILE */

	data = AllocPtr(size);	
	GAME_ASSERT(data);


			/* READ DATA */

	iErr = FSRead(fRefNum, &size, data);
	GAME_ASSERT(iErr == noErr);


		/*  CLOSE THE FILE */

	iErr = FSClose(fRefNum);
	GAME_ASSERT(iErr == noErr);


			/*  STORE SIZE IN OUTPUT PARAM  */

	if (outSize)
		*outSize = size;


	return(data);
}



/****************** LOAD TERRAIN TILESET ******************/

void LoadTerrainTileset(FSSpec *fsSpec)
{
SInt32		*longPtr;
long		fileSize;

			/* LOAD THE FILE */
			
	gTileFilePtr = LoadAFile(fsSpec, &fileSize);
	GAME_ASSERT(gTileFilePtr);


			/*********************/
			/* EXTRACT SOME DATA */
			/*********************/

	longPtr = (SInt32 *)gTileFilePtr;

				/* GET # TEXTURES */

	SInt32 numTerrainTextureTiles =  *longPtr++;												// get # texture tiles
	ByteswapInts(sizeof(SInt32), 1, &numTerrainTextureTiles);
	gNumTerrainTextureTiles = numTerrainTextureTiles;
	GAME_ASSERT(gNumTerrainTextureTiles <= MAX_TERRAIN_TILES);

	gTileDataPtr = (UInt16 *)longPtr;															// point to tile data

				/* CONVERT TEXTURES TO LITTLE-ENDIAN */

	int numTexels = gNumTerrainTextureTiles * OREOMAP_TILE_SIZE * OREOMAP_TILE_SIZE;
	GAME_ASSERT(numTexels*2 == fileSize-4);

	ByteswapInts(sizeof(UInt16), numTexels, gTileDataPtr);
}



/****************** LOAD TERRAIN ******************/
//
//  Assumes old terrain has been purged!
//
//  INPUT: 	fileName
//

void LoadTerrain(FSSpec *fsSpec)
{
UInt16		*shortPtr;
long		offset;
Ptr			miscPtr;
long		row,i,x,y;
long		dummy1,dummy2;


			/* LOAD THE TERRAIN FILE */
			
	gTerrainPtr = LoadAFile(fsSpec, nil);
	if (gTerrainPtr == nil)
		DoAlert("Error loading Terrain file!");


	// ---- unpack header ----
	// 0    long    offset to texture layer
	// 4    long    offset to heightmap layer
	// 8    long    offset to path layer
	// 12	long	offset to object list (Terrain2.c)
	// 16	long	???
	// 20   long    offset to heightmap tiles
	// 24	long	???
	// 28   short   width
	// 30   short   depth
	// 32   long    offset to texture attributes
	// 36   long    [unused] offset to tile anim data
	//             0  4  8 12 16 20 24 28 32 36
	ByteswapStructs("l  l  l  l  l  l  l hh  l  l", 40, 1, gTerrainPtr);


			/*********************/
			/* INIT LAYER ARRAYS */
			/*********************/

	gTerrainTileWidth = *((short *)(gTerrainPtr+28));							// get width of terrain (in tiles)
	gTerrainTileDepth = *((short *)(gTerrainPtr+30));							// get height of terrain (in tiles)

	gTerrainUnitWidth = gTerrainTileWidth*TERRAIN_POLYGON_SIZE;					// calc world unit dimensions of terrain
	gTerrainUnitDepth = gTerrainTileDepth*TERRAIN_POLYGON_SIZE;

	gNumSuperTilesDeep = gTerrainTileDepth/SUPERTILE_SIZE;						// calc size in supertiles
	gNumSuperTilesWide = gTerrainTileWidth/SUPERTILE_SIZE;


			/* INIT TEXTURE_LAYER */

	gTerrainTextureLayer = (UInt16 **)AllocPtr(sizeof(short *)*				// alloc mem for 1st dimension of array (map is 1/2 dimensions of wid/dep values!)
							gTerrainTileDepth);

	offset = *((SInt32 *)(gTerrainPtr+0));										// get offset to TEXTURE_LAYER
	shortPtr = (UInt16 *)(gTerrainPtr + offset);								// calc ptr to TEXTURE_LAYER
	ByteswapInts(sizeof(UInt16), gTerrainTileDepth * gTerrainTileWidth, shortPtr);

	for (row = 0; row < gTerrainTileDepth; row++)
	{
		gTerrainTextureLayer[row] = shortPtr;									// set [row] to point to layer's row(n)
		shortPtr += gTerrainTileWidth;
	}


			/* INIT HEIGHTMAP_LAYER */

	gTerrainHeightMapLayer = (UInt16 **)AllocPtr(sizeof(short *)*				// alloc mem for 1st dimension of array (map is 1/2 dimensions of wid/dep values!)
							gTerrainTileDepth);
							
	offset = *((SInt32 *)(gTerrainPtr+4));										// get offset to HEIGHTMAP_LAYER
	if (offset > 0)
	{
		shortPtr = (UInt16 *)(gTerrainPtr + offset);							// calc ptr to HEIGHTMAP_LAYER
		ByteswapInts(sizeof(UInt16), gTerrainTileDepth * gTerrainTileWidth, shortPtr);

		for (row = 0; row < gTerrainTileDepth; row++)
		{
			gTerrainHeightMapLayer[row] = shortPtr;								// set [row] to point to layer's row(n)
			shortPtr += gTerrainTileWidth;
		}
	}


			/* INIT PATH_LAYER */

	gTerrainPathLayer = (UInt16 **)AllocPtr(sizeof(short *)*					// alloc mem for 1st dimension of array (map is 1/2 dimensions of wid/dep values!)
							gTerrainTileDepth);
							
	offset = *((SInt32 *)(gTerrainPtr+8));										// get offset to PATH_LAYER
	if (offset > 0)
	{
		shortPtr = (UInt16 *)(gTerrainPtr + offset);							// calc ptr to PATH_LAYER
		ByteswapInts(sizeof(UInt16), gTerrainTileDepth * gTerrainTileWidth, shortPtr);

		for (row = 0; row < gTerrainTileDepth; row++)
		{
			gTerrainPathLayer[row] = shortPtr;									// set [row] to point to layer's row(n)
			shortPtr += gTerrainTileWidth;
		}
	}


			/* GET TEXTURE_ATTRIBUTES */

	offset = *((SInt32 *)(gTerrainPtr+32));									// get offset to TEXTURE_ATTRIBUTES
	// SOURCE PORT CHEAT... don't know how to get the number of tile attributes otherwise..
	SInt32 offsetOfNextChunk = *((SInt32*)(gTerrainPtr + 36));
	int nTileAttributes = (offsetOfNextChunk - offset) / sizeof(TileAttribType);
	gTileAttributes = (TileAttribType *)(gTerrainPtr + offset);				// calc ptr to TEXTURE_ATTRIBUTES
	ByteswapStructs(STRUCTFORMAT_TileAttribType, sizeof(TileAttribType), nTileAttributes, gTileAttributes);


			/* GET HEIGHTMAP_TILES */

	offset = *((SInt32 *)(gTerrainPtr+20));									// get offset to HEIGHTMAP_TILES
	if (offset > 0)
	{
		miscPtr = gTerrainPtr+offset;										// calc ptr to HEIGHTMAP_TILES
		for (i=0; i < MAX_HEIGHTMAP_TILES; i++)
		{
			gTerrainHeightMapPtrs[i] = miscPtr; 	   						// point to texture(n)
			miscPtr += (TERRAIN_HMTILE_SIZE * TERRAIN_HMTILE_SIZE);			// skip tile definition
		}
	}


				/* BUILD ITEM LIST */

	BuildTerrainItemList();
	FindMyStartCoordItem();										// look thru items for my start coords


				/* INITIALIZE CURRENT SCROLL SETTINGS */

	x = gMyStartX-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	y = gMyStartZ-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	GetSuperTileInfo(x, y, &gCurrentSuperTileCol, &gCurrentSuperTileRow, &dummy1, &dummy2);



			/* INIT THE SCROLL BUFFER */

	ClearScrollBuffer();		
}



/************************** LOAD LEVEL ART ***************************/

void LoadLevelArt(short levelNum)
{
FSSpec	spec;

			/* LOAD GLOBAL STUFF */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:Global_Models.3dmf", &spec);
	LoadGrouped3DMF(&spec,MODEL_GROUP_GLOBAL);	
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_DEFAULT);
	
			/* LOAD LEVEL SPECIFIC STUFF */
			
	switch(levelNum)
	{
		case	LEVEL_NUM_0:
		
				
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:level1.trt", &spec);
				LoadTerrainTileset(&spec);
				
				FSMakeFSSpec(
					gDataSpec.vRefNum,
					gDataSpec.parID,
					PRO_MODE ? ":terrain:level1pro.ter" : ":terrain:level1.ter",
					&spec);
				LoadTerrain(&spec);

				/* LOAD MODELS */
						
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:Level1_Models.3dmf", &spec);
				LoadGrouped3DMF(&spec,MODEL_GROUP_LEVEL0);	
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:Infobar_Models.3dmf", &spec);
				LoadGrouped3DMF(&spec,MODEL_GROUP_INFOBAR);	
				
				
				/* LOAD SKELETON FILES */
				
				LoadASkeleton(SKELETON_TYPE_REX);			
				LoadASkeleton(SKELETON_TYPE_PTERA);		
				LoadASkeleton(SKELETON_TYPE_STEGO);		
				LoadASkeleton(SKELETON_TYPE_DEINON);		
				LoadASkeleton(SKELETON_TYPE_TRICER);		
				LoadASkeleton(SKELETON_TYPE_SPITTER);		
					
				/* LOAD SPRITES FILES */

				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:infobar.frames", &spec);
				LoadFramesFile(&spec, 0);
				break;

	

		default:
				DoFatalAlert("LoadLevelArt: unsupported level #");
	}
}

