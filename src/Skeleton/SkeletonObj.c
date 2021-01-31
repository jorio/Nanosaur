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
extern	ObjNode				*gPlayerNode[];
extern	QD3DSetupOutputType		*gGameViewInfoPtr;


/****************************/
/*    PROTOTYPES            */
/****************************/

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum);
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
short	n,i;
TQ3BoundingBox *bbox;
float	max,originX,originY,originZ;

			/* CREATE NEW OBJECT NODE */
			
	newObjDef->flags |= STATUS_BIT_HIGHFILTER;
	newObjDef->genre = SKELETON_GENRE;
	newNode = MakeNewObject(newObjDef);		
	if (newNode == nil)
		return(nil);
		
	newNode->StatusBits |= STATUS_BIT_ANIM;						// turn on animation		
		
			/* LOAD SKELETON FILE INTO OBJECT */
			
	newNode->Skeleton = MakeNewSkeletonBaseData(newObjDef->type);	// alloc & set skeleton data
	if (newNode->Skeleton == nil)
		DoFatalAlert("MakeNewSkeletonObject: MakeNewSkeletonBaseData == nil");

	UpdateObjectTransforms(newNode);

				/*  SET INITIAL DEFAULT POSITION */
				
	SetSkeletonAnim(newNode->Skeleton, newObjDef->animNum);		
	UpdateSkeletonAnimation(newNode);
	GetModelCurrentPosition(newNode->Skeleton);					// set positions of all joints
	UpdateSkinnedGeometry(newNode);								// prime the trimesh

				/* CALC RADIUS OF OBJECT */
				//
				// find largest extent of bounding boxes and use as radius
				
	max = 0.0f;
	n = newNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
	originX = newNode->Coord.x;
	originY = newNode->Coord.y;
	originZ = newNode->Coord.z;
	
	for (i = 0; i < n; i++)
	{
		float	d;
		
		bbox = &newNode->Skeleton->localTriMeshes[i].bBox;
		d = fabs(originX - bbox->min.x);			// left
		if (d > max)
			max = d;		
		d = fabs(originX - bbox->max.x);			// right
		if (d > max)
			max = d;

		d = fabs(originZ - bbox->max.z);			// front
		if (d > max)
			max = d;
		d = fabs(originZ - bbox->min.z);			// back
		if (d > max)
			max = d;

		d = fabs(originY - bbox->max.y);			// top
		if (d > max)
			max = d;
		d = fabs(originY - bbox->min.y);			// bottom
		if (d > max)
			max = d;
			
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
	if (skeleton->NumAnimEvents == nil)
		DoFatalAlert("Not enough memory to alloc NumAnimEvents");

	skeleton->AnimEventsList = (AnimEventType **)AllocPtr(sizeof(AnimEventType *)*numAnims);	// alloc 1st dimension (for each anim)
	if (skeleton->AnimEventsList == nil)
		DoFatalAlert("Not enough memory to alloc AnimEventsList");
	for (i=0; i < numAnims; i++)
	{
		skeleton->AnimEventsList[i] = (AnimEventType *)AllocPtr(sizeof(AnimEventType)*MAX_ANIM_EVENTS);	// alloc 2nd dimension (for max events)
		if (skeleton->AnimEventsList[i] == nil)
			DoFatalAlert("Not enough memory to alloc AnimEventsList");
	}	
	
			/* ALLOC BONE INFO */
			
	skeleton->Bones = (BoneDefinitionType *)AllocPtr(sizeof(BoneDefinitionType)*numJoints);	
	if (skeleton->Bones == nil)
		DoFatalAlert("Not enough memory to alloc Bones");


		/* ALLOC DECOMPOSED DATA */
			
	skeleton->decomposedTriMeshes = (TQ3TriMeshData *)AllocPtr(sizeof(TQ3TriMeshData)*MAX_DECOMPOSED_TRIMESHES);		
	if (skeleton->decomposedTriMeshes == nil)
		DoFatalAlert("Not enough memory to alloc decomposedTriMeshes");

	skeleton->decomposedPointList = (DecomposedPointType *)AllocPtr(sizeof(DecomposedPointType)*MAX_DECOMPOSED_POINTS);		
	if (skeleton->decomposedPointList == nil)
		DoFatalAlert("Not enough memory to alloc decomposedPointList");

	skeleton->decomposedNormalsList = (TQ3Vector3D *)AllocPtr(sizeof(TQ3Vector3D)*MAX_DECOMPOSED_NORMALS);		
	if (skeleton->decomposedNormalsList == nil)
		DoFatalAlert("Not enough memory to alloc decomposedNormalsList");
			
	
}


/*************** DISPOSE SKELETON DEFINITION MEMORY ***************************/
//
// Disposes of all alloced memory (from above) used by a skeleton file definition.
//

static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton)
#if 1	// TODO noquesa
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
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
			
	for (j = 0; j < skeleton->numDecomposedTriMeshes; j++)			// first dispose of the trimesh data in there
		Q3TriMesh_EmptyData(&skeleton->decomposedTriMeshes[j]);
			
	if (skeleton->decomposedTriMeshes)
	{
		DisposePtr((Ptr)skeleton->decomposedTriMeshes);
		skeleton->decomposedTriMeshes = nil;
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

			/* DISPOSE OF MASTER DEFINITION BLOCK */
			
	DisposePtr((Ptr)skeleton);
}
#endif



/****************** MAKE NEW SKELETON OBJECT DATA *********************/
//
// Allocates & inits the Skeleton data for an ObjNode.
//

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum)
{
SkeletonDefType		*skeletonDefPtr;
SkeletonObjDataType	*skeletonData;
short				i;

	skeletonDefPtr = gLoadedSkeletonsList[sourceSkeletonNum];				// get ptr to source skeleton definition info
	if (skeletonDefPtr == nil)
		DoFatalAlert("CopySkeletonInfoToNewSkeleton: Skeleton data isnt loaded!");
		

			/* ALLOC MEMORY FOR NEW SKELETON OBJECT DATA STRUCTURE */
			
	skeletonData = (SkeletonObjDataType *)AllocPtr(sizeof(SkeletonObjDataType));
	if (skeletonData == nil)
		DoFatalAlert("MakeNewSkeletonBaseData: Cannot alloc new SkeletonObjDataType");


			/* INIT NEW SKELETON */

	skeletonData->skeletonDefinition = skeletonDefPtr;						// point to source animation data
	skeletonData->AnimSpeed = 1.0;


			/****************************************/
			/* MAKE COPY OF TRIMESHES FOR LOCAL USE */
			/****************************************/
			
	for (i=0; i < skeletonDefPtr->numDecomposedTriMeshes; i++)
		QD3D_DuplicateTriMeshData(&skeletonDefPtr->decomposedTriMeshes[i],&skeletonData->localTriMeshes[i]);

	return(skeletonData);
}


/************************ FREE SKELETON BASE DATA ***************************/

void FreeSkeletonBaseData(SkeletonObjDataType *data)
{
short	i;

			/* FREE ALL LOCAL TRIMESH DATA */
		
	for (i=0; i < data->skeletonDefinition->numDecomposedTriMeshes; i++)		
		QD3D_FreeDuplicateTriMeshData(&data->localTriMeshes[i]);
	
	
			/* FREE THE SKELETON DATA */
			
	DisposePtr((Ptr)data);			
}


















