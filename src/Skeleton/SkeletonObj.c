/****************************/
/*   	SKELETON.C    	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/



#include <QD3D.h>

#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "skeletonanim.h"
#include "skeletonobj.h"
#include "skeletonjoints.h"
#include "3dmath.h"
#include "file.h"
#include "qd3d_geometry.h"
#include "sound2.h"
#include "bones.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static SkeletonDefType	*gLoadedSkeletonsList[MAX_SKELETON_TYPES];


/**************** INIT SKELETON MANAGER *********************/

void InitSkeletonManager(void)
{
short	i;

	CalcAccelerationSplineCurve();									// calc accel curve

	for (i =0; i < MAX_SKELETON_TYPES; i++)
		gLoadedSkeletonsList[i] = nil;
}


/******************** LOAD A SKELETON ****************************/

void LoadASkeleton(Byte num)
{
	if (gLoadedSkeletonsList[num] == nil)					// check if already loaded
		gLoadedSkeletonsList[num] = LoadSkeletonFile(num);
}


/****************** FREE SKELETON FILE **************************/
//
// Disposes of all memory used by a skeleton file (from File.c)
//

void FreeSkeletonFile(Byte skeletonType)
{
	if (gLoadedSkeletonsList[skeletonType])										// make sure this really exists
	{
		DisposeSkeletonDefinitionMemory(gLoadedSkeletonsList[skeletonType]);	// free skeleton data
		gLoadedSkeletonsList[skeletonType] = nil;
	}
}


/*************** FREE ALL SKELETON FILES ***************************/
//
// Free's all except for the input type (-1 == none to skip)
//

void FreeAllSkeletonFiles(short skipMe)
{
short	i;

	for (i = 0; i < MAX_SKELETON_TYPES; i++)
	{
		if (i != skipMe)
	 		FreeSkeletonFile(i);
	}
}



/***************** MAKE NEW SKELETON OBJECT *******************/
//
// This routine simply initializes the blank object.
// The function CopySkeletonInfoToNewSkeleton actually attaches the specific skeleton
// file to this ObjNode.
//

ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNode;
TQ3BoundingBox *bbox;

			/* CREATE NEW OBJECT NODE */

	newObjDef->flags |= STATUS_BIT_HIGHFILTER;
	newObjDef->genre = SKELETON_GENRE;
	newNode = MakeNewObject(newObjDef);
	if (newNode == nil)
		return(nil);

	newNode->StatusBits |= STATUS_BIT_ANIM;						// turn on animation


			/* GET MASTER SKELETON DEFINITION */

	short sourceSkeletonNum = newObjDef->type;

	const SkeletonDefType* skeletonDef = gLoadedSkeletonsList[sourceSkeletonNum];				// get ptr to source skeleton definition info
	GAME_ASSERT_MESSAGE(skeletonDef, "Skeleton data isn't loaded!");


			/* ALLOC MEMORY FOR NEW SKELETON OBJECT DATA STRUCTURE */

	newNode->Skeleton = (SkeletonObjDataType *)AllocPtr(sizeof(SkeletonObjDataType));
	GAME_ASSERT(newNode->Skeleton);


			/* INIT NEW SKELETON */

	newNode->Skeleton->skeletonDefinition = skeletonDef;						// point to source animation data
	newNode->Skeleton->AnimSpeed = 1.0;


			/* MAKE COPY OF TRIMESHES FOR LOCAL USE */

	GAME_ASSERT(newNode->NumMeshes == 0);

	newNode->NumMeshes = skeletonDef->numDecomposedTriMeshes;

	for (int i = 0; i < skeletonDef->numDecomposedTriMeshes; i++)
	{
		GAME_ASSERT_MESSAGE(!newNode->MeshList[i], "Node already had a mesh at that index!");

		newNode->MeshList[i] = Q3TriMeshData_Duplicate(skeletonDef->decomposedTriMeshPtrs[i]);
		newNode->OwnsMeshMemory[i] = true;
	}


				/*  SET INITIAL DEFAULT POSITION */

	UpdateObjectTransforms(newNode);

	SetSkeletonAnim(newNode->Skeleton, newObjDef->animNum);
	UpdateSkeletonAnimation(newNode);
	GetModelCurrentPosition(newNode->Skeleton);					// set positions of all joints
	UpdateSkinnedGeometry(newNode);								// prime the trimesh


				/* CALC RADIUS OF OBJECT */
				//
				// find largest extent of bounding boxes and use as radius

	float max = 0.0f;

	for (int i = 0; i < newNode->NumMeshes; i++)
	{
		float	d;

		bbox = &newNode->MeshList[i]->bBox;

		d = fabsf(newNode->Coord.x - bbox->min.x);	if (d > max) max = d;		// left
		d = fabsf(newNode->Coord.x - bbox->max.x);	if (d > max) max = d;		// right
		d = fabsf(newNode->Coord.y - bbox->max.y);	if (d > max) max = d;		// top
		d = fabsf(newNode->Coord.y - bbox->min.y);	if (d > max) max = d;		// bottom
		d = fabsf(newNode->Coord.z - bbox->max.z);	if (d > max) max = d;		// front
		d = fabsf(newNode->Coord.z - bbox->min.z);	if (d > max) max = d;		// back
	}
	newNode->Radius = max;


	return(newNode);
}




/***************** ALLOC SKELETON DEFINITION MEMORY **********************/
//
// Allocates all of the sub-arrays for a skeleton file's definition data.
// ONLY called by ReadDataFromSkeletonFile in file.c.
//
// NOTE: skeleton has already been allocated by LoadSkeleton!!!
//

void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{
long	i;
long	numAnims,numJoints;

	numJoints = skeleton->NumBones;											// get # joints in skeleton
	numAnims = skeleton->NumAnims;											// get # anims in skeleton

				/***************************/
				/* ALLOC ANIM EVENTS LISTS */
				/***************************/

	skeleton->NumAnimEvents = (Byte *)AllocPtr(sizeof(Byte)*numAnims);		// array which holds # events for each anim
	GAME_ASSERT(skeleton->NumAnimEvents);

	skeleton->AnimEventsList = (AnimEventType **)AllocPtr(sizeof(AnimEventType *)*numAnims);	// alloc 1st dimension (for each anim)
	GAME_ASSERT(skeleton->AnimEventsList);

	for (i=0; i < numAnims; i++)
	{
		skeleton->AnimEventsList[i] = (AnimEventType *)AllocPtr(sizeof(AnimEventType)*MAX_ANIM_EVENTS);	// alloc 2nd dimension (for max events)
		GAME_ASSERT(skeleton->AnimEventsList[i]);
	}
	
			/* ALLOC BONE INFO */
			
	skeleton->Bones = (BoneDefinitionType *)AllocPtr(sizeof(BoneDefinitionType)*numJoints);	
	GAME_ASSERT(skeleton->Bones);


		/* ALLOC DECOMPOSED DATA */

	skeleton->decomposedTriMeshPtrs = (TQ3TriMeshData**) AllocPtr(sizeof(TQ3TriMeshData*) * MAX_DECOMPOSED_TRIMESHES);
	GAME_ASSERT(skeleton->decomposedTriMeshPtrs);

	skeleton->decomposedPointList = (DecomposedPointType *)AllocPtr(sizeof(DecomposedPointType)*MAX_DECOMPOSED_POINTS);		
	GAME_ASSERT(skeleton->decomposedPointList);

	skeleton->decomposedNormalsList = (TQ3Vector3D *)AllocPtr(sizeof(TQ3Vector3D)*MAX_DECOMPOSED_NORMALS);		
	GAME_ASSERT(skeleton->decomposedNormalsList);

}


/*************** DISPOSE SKELETON DEFINITION MEMORY ***************************/
//
// Disposes of all alloced memory (from above) used by a skeleton file definition.
//

static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{
short	i,j,numAnims,numJoints;

	if (skeleton == nil)
		return;

	numAnims = skeleton->NumAnims;										// get # anims in skeleton
	numJoints = skeleton->NumBones;	

			/* NUKE THE SKELETON BONE POINT & NORMAL INDEX ARRAYS */
			
	for (j=0; j < numJoints; j++)
	{
		if (skeleton->Bones[j].pointList)
			DisposePtr((Ptr)skeleton->Bones[j].pointList);
		if (skeleton->Bones[j].normalList)
			DisposePtr((Ptr)skeleton->Bones[j].normalList);			
	}
	DisposePtr((Ptr)skeleton->Bones);									// free bones array
	skeleton->Bones = nil;

				/* DISPOSE ANIM EVENTS LISTS */
				
	DisposePtr((Ptr)skeleton->NumAnimEvents);
	for (i=0; i < numAnims; i++)
		DisposePtr((Ptr)skeleton->AnimEventsList[i]);
	DisposePtr((Ptr)skeleton->AnimEventsList);

			/* DISPOSE JOINT INFO */
			
	for (j=0; j < numJoints; j++)
	{
		Free_2d_array(skeleton->JointKeyframes[j].keyFrames);		// dispose 2D array of keyframe data
		skeleton->JointKeyframes[j].keyFrames = nil;
	}

			/* DISPOSE DECOMPOSED DATA ARRAYS */

	// DON'T call Q3TriMeshData_Dispose on every decomposedTriMeshPtrs as they're just pointers
	// to trimeshes in the 3DMF. We dispose of the 3DMF at the end.
	if (skeleton->decomposedTriMeshPtrs)
	{
		DisposePtr((Ptr)skeleton->decomposedTriMeshPtrs);
		skeleton->decomposedTriMeshPtrs = nil;
	}

	if (skeleton->decomposedPointList)
	{
		DisposePtr((Ptr)skeleton->decomposedPointList);
		skeleton->decomposedPointList = nil;
	}

	if (skeleton->decomposedNormalsList)
	{
		DisposePtr((Ptr)skeleton->decomposedNormalsList);
		skeleton->decomposedNormalsList = nil;
	}


			/* DISPOSE OF 3DMF */

	if (skeleton->associated3DMF)
	{
		Render_Dispose3DMFTextures(skeleton->associated3DMF);
		Q3MetaFile_Dispose(skeleton->associated3DMF);
		skeleton->associated3DMF = nil;
	}

			/* DISPOSE OF MASTER DEFINITION BLOCK */
			
	DisposePtr((Ptr)skeleton);
}



/************************ FREE SKELETON BASE DATA ***************************/

void FreeSkeletonBaseData(SkeletonObjDataType *data)
{
			/* FREE THE SKELETON DATA */

	DisposePtr((Ptr)data);
}
