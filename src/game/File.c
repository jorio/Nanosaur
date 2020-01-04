/****************************/
/*      FILE ROUTINES       */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	<QD3D.h>
#include 	<QD3DErrors.h>
#include	"globals.h"
#include 	"objects.h"
#include	"misc.h"
#include	"skeletonanim.h"
#include	"skeletonobj.h"
#include	"skeletonjoints.h"
#include	"3dmf.h"
#include 	"mobjtypes.h"
#include	"file.h"
#include 	"windows_nano.h"
#include 	"main.h"
#include 	"terrain.h"
#include 	"sprites.h"
#include 	"bones.h"
#include 	"sound2.h"

extern	short			gMainAppRezFile;
extern  TQ3Object		gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern  short			gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	short		gNumItems;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID,gNumTerrainTextureTiles,gNumTileAnims;
extern	Ptr		gTerrainPtr,gTerrainHeightMapPtrs[];
extern	long	gTerrainTileWidth,gTerrainTileDepth,gTerrainUnitWidth,gTerrainUnitDepth;		
extern	long	gNumSuperTilesDeep,gNumSuperTilesWide;
extern	UInt16	**gTerrainTextureLayer,**gTerrainHeightMapLayer,**gTerrainPathLayer,*gTileDataPtr;
extern	TileAttribType	*gTileAttributes;
extern	short	*gTileAnimEntryList[];
extern	long	gCurrentSuperTileRow,gCurrentSuperTileCol;
extern	long	gMyStartX,gMyStartZ;
extern	FSSpec	gDataSpec;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	PICT_HEADER_SIZE	512

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1


typedef struct
{
	long		score;
	short		numLevelsRemaining;
	Byte		levelCompleteFlags[8];
	Byte		characterType;
	float		health;
	TQ3Point3D	shellCoords[8];
	Byte		spotIndex[8];
	short		customHeadNum;
}SaveGameType;


/**********************/
/*     VARIABLES      */
/**********************/

static	Str255		gBasePathName = "NewGame";
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
FSSpec		fsSpec;
SkeletonDefType	*skeleton;					

				/* SET CORRECT FILENAME */
					
	switch(skeletonType)
	{
		case	SKELETON_TYPE_PTERA:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Petra.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_REX:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Rex.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_STEGO:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Stego.skeleton", &fsSpec);
				break;
				
		case	SKELETON_TYPE_DEINON:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Deinon.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_TRICER:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Tricer.skeleton", &fsSpec);
				break;

		case	SKELETON_TYPE_SPITTER:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Diloph.skeleton", &fsSpec);
				break;
		
		default:
				DoFatalAlert("LoadSkeleton: Unknown skeletonType!");
	}
	
	
			/* OPEN THE FILE'S REZ FORK */
				
	fRefNum = FSpOpenResFile(&fsSpec,fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		DoAlert("Error opening Skel Rez file");
		ShowSystemErr(iErr);
	}
	
	UseResFile(fRefNum);
	if (iErr = ResError())
		DoFatalAlert("Error using Rez file!");

			
			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */
			
	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("Cannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */
			
	ReadDataFromSkeletonFile(skeleton,&fsSpec);
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

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec)
{
Handle				hand;
long				i,k,j;
long				numJoints,numAnims,numKeyframes;
AnimEventType		*animEventPtr;
JointKeyframeType	*keyFramePtr;
SkeletonFile_Header_Type	*headerPtr;
short				version;
AliasHandle				alias;
OSErr					iErr;
FSSpec					target;
Boolean					wasChanged;
TQ3Point3D				*pointPtr;
SkeletonFile_AnimHeader_Type	*animHeaderPtr;


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
	{
		DoAlert("Error reading header resource!");
		return;
	}
	
	headerPtr = (SkeletonFile_Header_Type *)*hand;
	version = headerPtr->version;
	if (version != SKELETON_FILE_VERS_NUM)
		DoFatalAlert("Skeleton file has wrong version #");
	
	numAnims = skeleton->NumAnims = headerPtr->numAnims;			// get # anims in skeleton
	numJoints = skeleton->NumBones = headerPtr->numJoints;			// get # joints in skeleton
	ReleaseResource(hand);

	if (numJoints > MAX_JOINTS)										// check for overload
		DoFatalAlert("ReadDataFromSkeletonFile: numJoints > MAX_JOINTS");


				/*************************************/
				/* ALLOCATE MEMORY FOR SKELETON DATA */
				/*************************************/

	AllocSkeletonDefinitionMemory(skeleton);



		/********************************/
		/* 	LOAD THE REFERENCE GEOMETRY */
		/********************************/
		
	alias = (AliasHandle)GetResource(rAliasType,1000);				// alias to geometry 3DMF file
	if (alias != nil)
	{
		iErr = ResolveAlias(fsSpec, alias, &target, &wasChanged);	// try to resolve alias
		if (!iErr)
			LoadBonesReferenceModel(&target,skeleton);
		else
			DoFatalAlert("ReadDataFromSkeletonFile: Cannot resolve alias to 3DMF file!");
		ReleaseResource((Handle)alias);
	}


		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		UInt16					*indexPtr;

			/* READ BONE DATA */
			
		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading Bone resource!");
		HLock(hand);
		bonePtr = (File_BoneDefinitionType *)*hand;


			/* COPY BONE DATA INTO ARRAY */
		
		skeleton->Bones[i].parentBone = bonePtr->parentBone;								// index to previous bone
		skeleton->Bones[i].coord = bonePtr->coord;											// absolute coord (not relative to parent!)
		skeleton->Bones[i].numPointsAttachedToBone = bonePtr->numPointsAttachedToBone;		// # vertices/points that this bone has
		skeleton->Bones[i].numNormalsAttachedToBone = bonePtr->numNormalsAttachedToBone;	// # vertex normals this bone has		
		ReleaseResource(hand);

			/* ALLOC THE POINT & NORMALS SUB-ARRAYS */
				
		skeleton->Bones[i].pointList = (UInt16 *)AllocPtr(sizeof(UInt16) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		if (skeleton->Bones[i].pointList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/pointList failed!");

		skeleton->Bones[i].normalList = (UInt16 *)AllocPtr(sizeof(UInt16) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		if (skeleton->Bones[i].normalList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/normalList failed!");

			/* READ POINT INDEX ARRAY */
			
		hand = GetResource('BonP',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonP resource!");
		HLock(hand);
		indexPtr = (UInt16 *)(*hand);
			

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = FromBE(indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */
			
		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonN resource!");
		HLock(hand);
		indexPtr = (UInt16 *)(*hand);
			
			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = FromBE(indexPtr[j]);
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
	if (hand == nil)
		DoFatalAlert("Error reading RelP resource!");
	HLock(hand);
	pointPtr = (TQ3Point3D *)*hand;
	
	if ((GetHandleSize(hand) / sizeof(TQ3Point3D)) != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
		for (i = 0; i < skeleton->numDecomposedPoints; i++)
			skeleton->decomposedPointList[i].boneRelPoint = FromBE(pointPtr[i]);

	ReleaseResource(hand);
	
	
			/*********************/
			/* READ ANIM INFO   */
			/*********************/
			
	for (i=0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		if (hand == nil)
			DoFatalAlert("Error getting anim header resource");
		HLock(hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = animHeaderPtr->numAnimEvents;			// copy # anim events in anim	
		ReleaseResource(hand);


			/* READ ANIM-EVENT DATA */
			
		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading anim-event data resource!");
		animEventPtr = (AnimEventType *)*hand;
		for (j=0;  j < skeleton->NumAnimEvents[i]; j++)
			skeleton->AnimEventsList[i][j] = *animEventPtr++;
		ReleaseResource(hand);		


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */
					
		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		if (hand == nil)
			DoFatalAlert("Error reading # keyframes/joint resource!");
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
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("Error: numKeyframes > MAX_KEYFRAMES");
		
					/* READ A JOINT KEYFRAME */
					
			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("Error reading joint keyframes resource!");
			keyFramePtr = (JointKeyframeType *)*hand;
			for (k = 0; k < numKeyframes; k++)												// copy this joint's keyframes for this anim
				skeleton->JointKeyframes[j].keyFrames[i][k] = *keyFramePtr++;
			ReleaseResource(hand);		
		}
	}
	
}



/**************** OPEN GAME FILE **********************/

void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString)
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
		NumToString(iErr,s);
		DoFatalAlert2(errString,s);
	}
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
				
				/*************/
				/* READ FILE */
				/*************/
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Nanosaur:Prefs", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr)
		return(iErr);

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count,  (Ptr)prefBlock);		// read data from file
	if (iErr)
	{
		FSClose(refNum);			
		return(iErr);
	}
	
	FSClose(refNum);			
	
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
				
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Nanosaur:Prefs", &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'NanO', 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */
					
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
kill:	
		FSpDelete(&file);
		return;
	}
		
				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, (Ptr)prefs);	
	FSClose(refNum);
}


/***************************** SAVE GAME ********************************/

void SaveGame(void)
{
#if 0
SaveGameType	**dataHandle;
Str255		prompt = "Save Game As:";
Str255		name = "WW Saved Game";
short		fRefNum;
int			oldResF;
FSSpec		*specPtr;
StandardFileReply	reply;
long		i;

	oldResF = CurResFile();

					/* DO STANDARD FILE DIALOG */
					
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);	
	StandardPutFile(prompt,name,&reply);
	HideCursor();
	CleanScreenBorder();		
	if (!reply.sfGood)											// see if cancelled
		return;
	specPtr = &reply.sfFile;

			
			/*************************/	
			/* CREATE SAVE GAME DATA */
			/*************************/	
					
	dataHandle = (SaveGameType **)AllocHandle(sizeof(SaveGameType));
	if (dataHandle == nil)
		DoFatalAlert("SaveGame: AllocHandle failed!");

	(**dataHandle).score = gTotalWinnings;
	(**dataHandle).numLevelsRemaining = gNumLevelsRemaining;
	for (i = 0; i < 8; i++)
	{
		(**dataHandle).levelCompleteFlags[i] = gShellWasUsed[i];
		(**dataHandle).shellCoords[i] = gShellCoords[i];
		(**dataHandle).spotIndex[i] = gSpotShellNum[i];
	}
	(**dataHandle).characterType = gMyCharacterType;
	(**dataHandle).health = 0;

	if (gCustomHeadGeometry[SERVER_PLAYER_NUM] == nil)			// see if remember custom head #
		(**dataHandle).customHeadNum = -1;
	else
		(**dataHandle).customHeadNum = gMyCustomHeadNum;

			/* DELETE ANY EXISTING FILE */

	if (reply.sfReplacing)
		FSpDelete(&reply.sfFile);


				/* CREATE & OPEN THE REZ-FORK */
			
	FSpCreateResFile(specPtr,'Pill','Save',nil);
	if (ResError())
	{
		DoAlert("Error creating Save file");
		DisposeHandle((Handle)dataHandle);
		return;
	}
	fRefNum = FSpOpenResFile(specPtr,fsCurPerm);
	if (fRefNum == -1)
	{
		DoAlert("Error opening Save Rez file");
		DisposeHandle((Handle)dataHandle);
		return;
	}
				
	UseResFile(fRefNum);
	
				/* WRITE TO FILE */

	AddResource((Handle)dataHandle, 'Save', 1000, "Save Data");
	WriteResource((Handle)dataHandle);									// force the update
	ReleaseResource((Handle)dataHandle);								// free the data
	
			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	
	UseResFile(oldResF);
#endif	
}


/***************************** LOAD SAVED GAME ********************************/

OSErr LoadSavedGame(void)
{
#if 0
SaveGameType	**dataHandle;
Str255		prompt = "Select A Saved Game:";
short		fRefNum;
int			oldResF;
FSSpec		*specPtr;
StandardFileReply	reply;
long		i;
SFTypeList	typeList;

	oldResF = CurResFile();

					/* DO STANDARD FILE DIALOG */

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);	


	typeList[0] = 'Save';
	StandardGetFile(nil,1,typeList,&reply);
	HideCursor();
	CleanScreenBorder();		
	
	if (!reply.sfGood)													// see if cancelled
		return(!noErr);
	specPtr = &reply.sfFile;											// get ptr to FSSPEC
			
				/* OPEN THE REZ-FORK */
			
	fRefNum = FSpOpenResFile(specPtr,fsRdPerm);
	if (fRefNum == -1)
		return(!noErr);
	UseResFile(fRefNum);

				/* READ FROM FILE */

	dataHandle = (SaveGameType **)GetResource('Save',1000);
	if (dataHandle == nil)
	{
		DoAlert("Error reading save game resource!");
		return(!noErr);
	}
	
			/**********************/	
			/* USE SAVE GAME DATA */
			/**********************/	
					
	gTotalWinnings = (**dataHandle).score;
	gNumLevelsRemaining = (**dataHandle).numLevelsRemaining;
	for (i = 0; i < 8; i++)
	{
		gShellWasUsed[i] = (**dataHandle).levelCompleteFlags[i];
		gShellCoords[i] = (**dataHandle).shellCoords[i];
		gSpotShellNum[i] = (**dataHandle).spotIndex[i];
		
	}
	gMyCharacterType = (**dataHandle).characterType;
		
	CreateMyCustomHead((**dataHandle).customHeadNum); 				// restore custom head if any
	
	ReleaseResource((Handle)dataHandle);
	
	
			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	UseResFile(oldResF);
	
	gRestartSavedGame = true;
#endif	
	return(noErr);
}


/********************** LOAD A FILE **************************/

Ptr	LoadAFile(FSSpec *fsSpec)
{
OSErr	iErr;
short 	fRefNum;
long	size;
Ptr		data;


			/* OPEN FILE */
			
	iErr = FSpOpenDF(fsSpec, fsRdPerm, &fRefNum);
	if (iErr != noErr)
	{
		DoAlert("LoadAFile: FSpOpenDF failed!");
		DoFatalAlert(fsSpec->name);
	}

			/* GET SIZE OF FILE */
			
	iErr = GetEOF(fRefNum, &size);
	if (iErr != noErr)
		DoFatalAlert("LoadAFile: GetEOF failed!");


			/* ALLOC MEMORY FOR FILE */
			
	data = AllocPtr(size);	
	if (data == nil)
		DoFatalAlert("LoadAFile: AllocPtr failed!");
	
	
			/* READ DATA */

	iErr = FSRead(fRefNum, &size, data);
	if (iErr != noErr)
		DoFatalAlert("LoadAFile: FSRead failed!");


		/*  CLOSE THE FILE */
				
	iErr = FSClose(fRefNum);
	if (iErr != noErr)
		DoFatalAlert("LoadAFile: FSClose failed!");
	
	return(data);
}



/****************** LOAD TERRAIN TILESET ******************/

void LoadTerrainTileset(FSSpec *fsSpec)
{
long		*longPtr;

			/* LOAD THE FILE */
			
	gTileFilePtr = LoadAFile(fsSpec);
	if (gTileFilePtr == nil)
		DoFatalAlert("LoadTerrainTileset: LoadAFile failed!");


			/*********************/
			/* EXTRACT SOME DATA */
			/*********************/

	longPtr = (long *)gTileFilePtr;
	
				/* GET # TEXTURES */
				
	gNumTerrainTextureTiles = *longPtr++;   													// get # texture tiles
	if (gNumTerrainTextureTiles > MAX_TERRAIN_TILES)
		DoFatalAlert("LoadTerrainTileset: gNumTerrainTextureTiles > MAX_TERRAIN_TILES");

	gTileDataPtr = (UInt16 *)longPtr;															// point to tile data


//	ExtractTileData();
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
			
	gTerrainPtr = LoadAFile(fsSpec);
	if (gTerrainPtr == nil)
		DoAlert("Error loading Terrain file!");


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

	offset = *((long *)(gTerrainPtr+0));										// get offset to TEXTURE_LAYER
	shortPtr = (UInt16 *)(gTerrainPtr+offset);									// calc ptr to TEXTURE_LAYER

	for (row = 0; row < gTerrainTileDepth; row++)
	{
		gTerrainTextureLayer[row] = shortPtr;									// set [row] to point to layer's row(n)
		shortPtr += gTerrainTileWidth;
	}


			/* INIT HEIGHTMAP_LAYER */

	gTerrainHeightMapLayer = (UInt16 **)AllocPtr(sizeof(short *)*				// alloc mem for 1st dimension of array (map is 1/2 dimensions of wid/dep values!)
							gTerrainTileDepth);
							
	offset = *((long *)(gTerrainPtr+4));										// get offset to HEIGHTMAP_LAYER
	if (offset > 0)
	{
		shortPtr = (UInt16 *)(gTerrainPtr+offset);								// calc ptr to HEIGHTMAP_LAYER

		for (row = 0; row < gTerrainTileDepth; row++)
		{
			gTerrainHeightMapLayer[row] = shortPtr;								// set [row] to point to layer's row(n)
			shortPtr += gTerrainTileWidth;
		}
	}


			/* INIT PATH_LAYER */

	gTerrainPathLayer = (UInt16 **)AllocPtr(sizeof(short *)*					// alloc mem for 1st dimension of array (map is 1/2 dimensions of wid/dep values!)
							gTerrainTileDepth);
							
	offset = *((long *)(gTerrainPtr+8));										// get offset to PATH_LAYER
	if (offset > 0)
	{
		shortPtr = (UInt16 *)(gTerrainPtr+offset);								// calc ptr to PATH_LAYER

		for (row = 0; row < gTerrainTileDepth; row++)
		{
			gTerrainPathLayer[row] = shortPtr;									// set [row] to point to layer's row(n)
			shortPtr += gTerrainTileWidth;
		}
	}


			/* GET TEXTURE_ATTRIBUTES */

	offset = *((long *)(gTerrainPtr+32));									// get offset to TEXTURE_ATTRIBUTES
	gTileAttributes = (TileAttribType *)(gTerrainPtr+offset);				// calc ptr to TEXTURE_ATTRIBUTES


			/* GET HEIGHTMAP_TILES */

	offset = *((long *)(gTerrainPtr+20));									// get offset to HEIGHTMAP_TILES
	if (offset > 0)
	{
		miscPtr = gTerrainPtr+offset;										// calc ptr to HEIGHTMAP_TILES
		for (i=0; i < MAX_HEIGHTMAP_TILES; i++)
		{
			gTerrainHeightMapPtrs[i] = miscPtr; 	   						// point to texture(n)
			miscPtr += (TERRAIN_HMTILE_SIZE * TERRAIN_HMTILE_SIZE);			// skip tile definition
		}
	}

#if 0
			/**********************/
			/* GET TILE_ANIM_DATA */
			/**********************/

	offset = *((long *)(gTerrainPtr+36));						// get offset to TILE_ANIM_DATA
	longPtr = (long *)(gTerrainPtr+offset);	  					// calc ptr to TILE_ANIM_DATA

	gQuickTileAnimListPtr = gTerrainPtr + (*longPtr++);			// calc ptr to TILE_ANIM_QUICKLIST
	gNumTileAnims = *longPtr++;									// get # tile anims

	if (gNumTileAnims > MAX_TILE_ANIMS)
		DoAlert("Too many tile anims!");

	for (i=0; i < gNumTileAnims; i++)
	{
		offset = *longPtr++;									// get offset to TILE_ANIM_ENTRY(n)
		gTileAnimEntryList[i] = (short *)(gTerrainPtr + offset);// keep ptr to TILE_ANIM_ENTRY(n)
	}

	InitTileAnims();
#endif

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





/**************** LOAD A PICT ***********************/

PicHandle LoadAPict(FSSpec *specPtr)
{
PicHandle			picture;
long				pictSize,headerSize;
OSErr				iErr;
short				fRefNum;
char				pictHeader[PICT_HEADER_SIZE];

	iErr = FSpOpenDF(specPtr,fsCurPerm,&fRefNum);
	if (iErr)
		DoFatalAlert("LoadAPict: FSpOpenDF failed!");

	if	(GetEOF(fRefNum,&pictSize) != noErr)								// get size of file		
		DoFatalAlert("LoadAPict:  GetEOF failed");
			
	headerSize = PICT_HEADER_SIZE;										// check the header					
	if (FSRead(fRefNum,&headerSize,pictHeader) != noErr)
		DoFatalAlert("LoadAPict:  FSRead failed!!");

	if ((pictSize -= PICT_HEADER_SIZE) <= 0)
		DoFatalAlert("Error reading PICT file!");
		
	if ((picture = (PicHandle)AllocHandle(pictSize)) == nil)
		DoFatalAlert("LoadAPict: enough memory to read PICT file!");
	HLock((Handle)picture);
		
	if (FSRead(fRefNum,&pictSize,(Ptr)*picture) != noErr)
		DoFatalAlert("LoadAPict: reading PICT file!");
		
	FSClose(fRefNum);	
	
	
	return(picture);	
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
				
#ifdef PRO_MODE
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:level1pro.ter", &spec);
#else	
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:level1.ter", &spec);
#endif				
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











