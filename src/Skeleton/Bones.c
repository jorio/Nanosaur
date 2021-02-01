/****************************/
/*   	BONES.C	    	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/



#include <QD3D.h>
#include <QD3DMath.h>

#include "globals.h"
#include "misc.h"
#include "bones.h"
#include "objects.h"
#include "3dmf.h"
#include "3dmath.h"
#include "skeletonjoints.h"
#include "GamePatches.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DecomposeATriMesh(TQ3TriMeshData* triMeshData);
static void DecompRefMo_Recurse(TQ3Object inObj);
static void DecomposeReferenceModel(TQ3Object theModel);
static void UpdateSkinnedGeometry_Recurse(short joint);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DO_INVT_MAT		0

/*********************/
/*    VARIABLES      */
/*********************/


SkeletonDefType		*gCurrentSkeleton;
SkeletonObjDataType	*gCurrentSkelObjData;

static	TQ3Matrix4x4		gMatrix;

static	TQ3BoundingBox		gBBox = {{0,0,0}, {0,0,0}, kQ3False};

static	TQ3Vector3D			gTransformedNormals[MAX_DECOMPOSED_NORMALS];	// temporary buffer for holding transformed normals before they're applied to their trimeshes


/******************** LOAD BONES REFERENCE MODEL *********************/
//
// INPUT: inSpec = spec of 3dmf file to load or nil to StdDialog it.
//

void LoadBonesReferenceModel(FSSpec	*inSpec, SkeletonDefType *skeleton)
{
#if 1	// NOQUESA
	Pomme3DMF_FileHandle the3DMFFile = Pomme3DMF_LoadModelFile(inSpec);
	GAME_ASSERT(the3DMFFile);
#else
TQ3Object		newModel;

	newModel = Load3DMFModel(inSpec);
	if (newModel == nil)
		DoFatalAlert("LoadBonesReferenceModel: cant load 3dmf file!");
#endif

#if 0	// TODO noquesa (might not be needed anymore)
	PatchSkeleton3DMF(inSpec->cName, newModel);		// patch 3DMF (add alpha test)
#endif

	gCurrentSkeleton = skeleton;

#if 1	// NOQUESA
			/* DECOMPOSE REFERENCE MODEL */

	gCurrentSkeleton->numDecomposedTriMeshes	= 0;
	gCurrentSkeleton->numDecomposedPoints		= 0;
	gCurrentSkeleton->numDecomposedNormals		= 0;

	TQ3TriMeshFlatGroup meshList = Pomme3DMF_GetAllMeshes(the3DMFFile);
	for (int i = 0; i < meshList.numMeshes; i++)
	{
		DecomposeATriMesh(meshList.meshes[i]);
	}
#else
	DecomposeReferenceModel(newModel);

	Q3Object_Dispose(newModel);			// dont need original 3DMF anymore
#endif
}


/****************** DECOMPOSE REFERENCE MODEL ************************/

static void DecomposeReferenceModel(TQ3Object theModel)
{
	gCurrentSkeleton->numDecomposedTriMeshes = 0;
	gCurrentSkeleton->numDecomposedPoints = 0;
	gCurrentSkeleton->numDecomposedNormals = 0;


		/* DO SUBRECURSIVE SCAN */
		
	DecompRefMo_Recurse(theModel);

}


/***************** DECOM REF MO: RECURSE ***************************/

static void DecompRefMo_Recurse(TQ3Object inObj)
#if 1	// TODO noquesa
{ DoFatalAlert2("TODO noquesa", __func__); }
#else
{
TQ3GroupPosition	position;
TQ3Object   		newObj;
TQ3ObjectType		oType;

				/* SEE IF FOUND GEOMETRY */

	if (Q3Object_IsType(inObj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(inObj);									// get geometry type
		switch(oType)
		{
			case	kQ3GeometryTypeTriMesh:
					DecomposeATriMesh(inObj);
					break;
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(inObj,kQ3ShapeTypeGroup))
 	{
  		for (Q3Group_GetFirstPosition(inObj, &position); position != nil;	// scan all objects in group
  			 Q3Group_GetNextPosition(inObj, &position))	
 		{
   			Q3Group_GetPositionObject (inObj, position, &newObj);			// get object from group
			if (newObj != NULL)
   			{
    			DecompRefMo_Recurse(newObj);								// sub-recurse this object
    			Q3Object_Dispose(newObj);									// dispose local ref
   			}
  		}
	}
}
#endif


/******************* DECOMPOSE A TRIMESH ***********************/

static void DecomposeATriMesh(TQ3TriMeshData* triMeshData)
{
//TQ3Status			status;				// NOQUESA
unsigned long		numVertecies,vertNum;
TQ3Point3D			*vertexList;
long				i,n,refNum,pointNum;
TQ3Vector3D			*normalPtr;
//TQ3TriMeshData		*triMeshData;	// NOQUESA
DecomposedPointType	*decomposedPoint;

	n = gCurrentSkeleton->numDecomposedTriMeshes;												// get index into list of trimeshes
	GAME_ASSERT(n < MAX_DECOMPOSED_TRIMESHES);

			/* GET TRIMESH DATA */

#if 1	// NOQUESA
	gCurrentSkeleton->decomposedTriMeshPtrs[n] = triMeshData;
#else
	status = Q3TriMesh_GetData(theTriMesh, &gCurrentSkeleton->decomposedTriMeshes[n]);			// get trimesh data
	if (status != kQ3Success) 
		DoFatalAlert("PreTransformTriMesh: Q3TriMesh_GetData failed!");
		
	triMeshData = &gCurrentSkeleton->decomposedTriMeshes[n];
#endif
		
	numVertecies = triMeshData->numPoints;														// get # verts in trimesh
	vertexList = triMeshData->points;															// point to vert list
#if 1	// NOQUESA
	normalPtr  = triMeshData->vertexNormals;													// point to normals
#else
	normalPtr  = (TQ3Vector3D *)(triMeshData->vertexAttributeTypes[0].data); 					// point to normals
#endif

				/*******************************/
				/* EXTRACT VERTECIES & NORMALS */
				/*******************************/
				
	for (vertNum = 0; vertNum < numVertecies; vertNum++)
	{				
			/* SEE IF THIS POINT IS ALREADY IN DECOMPOSED LIST */
				
		for (pointNum=0; pointNum < gCurrentSkeleton->numDecomposedPoints; pointNum++)
		{
			decomposedPoint = &gCurrentSkeleton->decomposedPointList[pointNum];					// point to this decomposed point
			
			TQ3Point3D tqpp = decomposedPoint->realPoint;
			if (PointsAreCloseEnough(&vertexList[vertNum],&tqpp))			// see if close enough to match
			{
					/* ADD ANOTHER REFERENCE */
					
				refNum = decomposedPoint->numRefs;												// get # refs for this point
				GAME_ASSERT(refNum < MAX_POINT_REFS);

				decomposedPoint->whichTriMesh[refNum] = n;										// set triMesh #
				decomposedPoint->whichPoint[refNum] = vertNum;									// set point #
				decomposedPoint->numRefs++;														// inc counter
				goto added_vert;
			}
		}
				/* IT'S A NEW POINT SO ADD TO LIST */
				
		pointNum = gCurrentSkeleton->numDecomposedPoints;
		GAME_ASSERT(pointNum < MAX_DECOMPOSED_POINTS);

		refNum = 0;																			// it's the 1st entry (need refNum for below).
		
		decomposedPoint = &gCurrentSkeleton->decomposedPointList[pointNum];					// point to this decomposed point
		decomposedPoint->realPoint = vertexList[vertNum];									// add new point to list			
		decomposedPoint->whichTriMesh[refNum] = n;											// set triMesh #
		decomposedPoint->whichPoint[refNum] = vertNum;										// set point #
		decomposedPoint->numRefs = 1;														// set # refs to 1
		
		gCurrentSkeleton->numDecomposedPoints++;											// inc # decomposed points
		
added_vert:
		
		
				/***********************************************/
				/* ADD THIS POINT'S NORMAL TO THE NORMALS LIST */
				/***********************************************/
				
					/* SEE IF NORMAL ALREADY IN LIST */
					
		Q3Vector3D_Normalize(&normalPtr[vertNum],&normalPtr[vertNum]);						// normalize to be safe
					
		for (i=0; i < gCurrentSkeleton->numDecomposedNormals; i++)
			if (VectorsAreCloseEnough(&normalPtr[vertNum],&gCurrentSkeleton->decomposedNormalsList[i]))	// if already in list, then dont add it again
				goto added_norm;
	

				/* ADD NEW NORMAL TO LIST */
				
		i = gCurrentSkeleton->numDecomposedNormals;										// get # decomposed normals already in list
		GAME_ASSERT(i < MAX_DECOMPOSED_NORMALS);

		gCurrentSkeleton->decomposedNormalsList[i] = normalPtr[vertNum];				// add new normal to list			
		gCurrentSkeleton->numDecomposedNormals++;										// inc # decomposed normals
		
added_norm:
					/* KEEP REF TO NORMAL IN POINT LIST */

		decomposedPoint->whichNormal[refNum] = i;										// save index to normal	
	}

	gCurrentSkeleton->numDecomposedTriMeshes++;											// inc # of trimeshes in decomp list

}



/************************** UPDATE SKINNED GEOMETRY *******************************/
//
// Updates all of the points in the local trimesh data's to coordinate with the
// current joint transforms.
//

void UpdateSkinnedGeometry(ObjNode *theNode)
{	
long	numTriMeshes,i;
SkeletonObjDataType	*currentSkelObjData;

			/* MAKE SURE OBJNODE IS STILL VALID */
			//
			// It's possible that Deleting a Skeleton and then creating a new
			// ObjNode like an explosion can make it seem like the current
			// objNode pointer is still valid when in fact it isn't.
			// Detecting this is too complex to be worth-while, so it's
			// easier to just verify the objNode here instead.
			//
			
	if (theNode->CType == INVALID_NODE_FLAG)
		return;
	gCurrentSkelObjData = currentSkelObjData = theNode->Skeleton;
	if (gCurrentSkelObjData == nil)
		return;
	
	gCurrentSkeleton = currentSkelObjData->skeletonDefinition;
	if (gCurrentSkeleton == nil)
		DoFatalAlert("UpdateSkinnedGeometry: gCurrentSkeleton is invalid!");

	gMatrix = theNode->BaseTransformMatrix;	
	
	
	gBBox.min =	gBBox.max = theNode->Coord;												// init bounding box calc
	
	if (gCurrentSkeleton->Bones[0].parentBone != NO_PREVIOUS_JOINT)
		DoFatalAlert("UpdateSkinnedGeometry: joint 0 isnt base - fix code Brian!");
	UpdateSkinnedGeometry_Recurse(0);													// start @ base
	
			/* UPDATE ALL TRIMESH BBOXES */
			
	numTriMeshes = currentSkelObjData->skeletonDefinition->numDecomposedTriMeshes;
	for (i = 0; i < numTriMeshes; i++)
		currentSkelObjData->localTriMeshPtrs[i]->bBox = gBBox;							// apply to local copy of trimesh
}


/******************** UPDATE SKINNED GEOMETRY: RECURSE ************************/

static void UpdateSkinnedGeometry_Recurse(short joint)
{
long			numChildren,numPoints,p,i,numRefs,r,triMeshNum,p2,c,numNormals,n;
TQ3Matrix4x4	oldM;
TQ3Vector3D		*normalAttribs;
BoneDefinitionType	*bonePtr;
float			minX,maxX,maxY,minY,maxZ,minZ;
float			m00,m01,m02,m10,m11,m12,m20,m21,m22,m30,m31,m32;
float			newX,newY,newZ;
SkeletonObjDataType	*currentSkelObjData = gCurrentSkelObjData;
SkeletonDefType		*currentSkeleton = gCurrentSkeleton;
float				*jointMat = &currentSkelObjData->jointTransformMatrix[joint].value[0][0];
float				*matPtr = &gMatrix.value[0][0];
float				x,y,z;
DecomposedPointType	*decomposedPointList = currentSkeleton->decomposedPointList;
TQ3TriMeshData		**localTriMeshPtrs = currentSkelObjData->localTriMeshPtrs;

	minX = minY = minZ = 1000000000;
	maxX = maxY = maxZ = -minX;									// calc local bbox with registers for speed

				/*********************************/
				/* FACTOR IN THIS JOINT'S MATRIX */
				/*********************************/
				//
				// I do my own 3x4 matrix multiply here since I know that the right column is 0,0,0,1
				// This is faster than Q3Matrix4x4_Multiply which assumes a full 4x4 matrix.
				//
				
	m00 = jointMat[0];	m01 = jointMat[1];	m02 = jointMat[2];
	m10 = jointMat[4];	m11 = jointMat[5];	m12 = jointMat[6];
	m20 = jointMat[8];	m21 = jointMat[9];	m22 = jointMat[10];
	m30 = jointMat[12];	m31 = jointMat[13];	m32 = jointMat[14];

	x = (m00 * matPtr[0]) + (m01 * matPtr[4]) + (m02 * matPtr[8]);			
	y = (m10 * matPtr[0]) + (m11 * matPtr[4]) + (m12 * matPtr[8]);			
	z = (m20 * matPtr[0]) + (m21 * matPtr[4]) + (m22 * matPtr[8]);			
	matPtr[12] = (m30 * matPtr[0]) + (m31 * matPtr[4]) + (m32 * matPtr[8]) + matPtr[12];
	matPtr[0] = x;
	matPtr[4] = y;
	matPtr[8] = z;

	x = (m00 * matPtr[1]) + (m01 * matPtr[5]) + (m02 * matPtr[9]);
	y = (m10 * matPtr[1]) + (m11 * matPtr[5]) + (m12 * matPtr[9]);
	z = (m20 * matPtr[1]) + (m21 * matPtr[5]) + (m22 * matPtr[9]);
	matPtr[13] = (m30 * matPtr[1]) + (m31 * matPtr[5]) + (m32 * matPtr[9]) + matPtr[13];
	matPtr[1] = x;
	matPtr[5] = y;
	matPtr[9] = z;

	x = (m00 * matPtr[2]) + (m01 * matPtr[6]) + (m02 * matPtr[10]);
	y = (m10 * matPtr[2]) + (m11 * matPtr[6]) + (m12 * matPtr[10]);
	z = (m20 * matPtr[2]) + (m21 * matPtr[6]) + (m22 * matPtr[10]);
	matPtr[14] = (m30 * matPtr[2]) + (m31 * matPtr[6]) + (m32 * matPtr[10]) + matPtr[14];
	matPtr[2] = x;
	matPtr[6] = y;
	matPtr[10]= z;


			/* DO INV-TRANSP FOR VECTOR MULTIPLIES */
			//
			// I think this is only needed if non-uniform scales are applied to the matrix.
			// Since I don't do this anywhere in the game, I don't need to waste
			// time doing this, but leave code here just in case things go bad.
			//
			

#if DO_INVT_MAT
	Q3Matrix4x4_Invert(&matPtr, &invM);								// calc inverse-transpose for vector transforms
	Q3Matrix4x4_Transpose(&invM, &transpM);
#endif

			/*************************/
			/* TRANSFORM THE NORMALS */
			/*************************/

		/* APPLY MATRIX TO EACH NORMAL VECTOR */
			
#if DO_INVT_MAT		
	m00 = transpM.value[0][0];	m01 = transpM.value[0][1];	m02 = transpM.value[0][2];
	m10 = transpM.value[1][0];	m11 = transpM.value[1][1];	m12 = transpM.value[1][2];
	m20 = transpM.value[2][0];	m21 = transpM.value[2][1];	m22 = transpM.value[2][2];
#else
	m00 = matPtr[0];	m01 = matPtr[1];	m02 = x;
	m10 = matPtr[4];	m11 = matPtr[5];	m12 = y;
	m20 = matPtr[8];	m21 = matPtr[9];	m22 = z;
#endif

	bonePtr = &currentSkeleton->Bones[joint];									// point to bone def
	numNormals = bonePtr->numNormalsAttachedToBone;								// get # normals attached to this bone
				
				
	for (p=0; p < numNormals; p++)
	{
		i = bonePtr->normalList[p];												// get index to normal in gDecomposedNormalsList
		
		x = currentSkeleton->decomposedNormalsList[i].x;						// get xyz of normal
		y = currentSkeleton->decomposedNormalsList[i].y;
		z = currentSkeleton->decomposedNormalsList[i].z;

		gTransformedNormals[i].x = (m00*x) + (m10*y) + (m20*z);					// transform the normal
		gTransformedNormals[i].y = (m01*x) + (m11*y) + (m21*z);
		gTransformedNormals[i].z = (m02*x) + (m12*y) + (m22*z);
	}
	
	

			/* APPLY TRANSFORMED VECTORS TO ALL REFERENCES */
	
	numPoints = bonePtr->numPointsAttachedToBone;									// get # points attached to this bone

	for (p = 0; p < numPoints; p++)
	{
		i = bonePtr->pointList[p];													// get index to point in gDecomposedPointList
				
		numRefs = decomposedPointList[i].numRefs;									// get # times this point is referenced
		if (numRefs == 1)															// SPECIAL CASE IF ONLY 1 REF (OPTIMIZATION)
		{
			triMeshNum = decomposedPointList[i].whichTriMesh[0];					// get triMesh # that uses this point
			p2 = decomposedPointList[i].whichPoint[0];								// get point # in the triMesh 
			n = decomposedPointList[i].whichNormal[0];								// get index into gDecomposedNormalsList

			normalAttribs = localTriMeshPtrs[triMeshNum]->vertexNormals;			// point to normals attribute list in local trimesh
			normalAttribs[p2] = gTransformedNormals[n];								// copy transformed normal into triMesh
		}
		else																		// handle multi-case
		{		
			for (r = 0; r < numRefs; r++)
			{		
				triMeshNum = decomposedPointList[i].whichTriMesh[r];					
				p2 = decomposedPointList[i].whichPoint[r];								
				n = decomposedPointList[i].whichNormal[r];								

				normalAttribs = localTriMeshPtrs[triMeshNum]->vertexNormals;
				normalAttribs[p2] = gTransformedNormals[n];
			}
		}
	}

			/************************/
			/* TRANSFORM THE POINTS */
			/************************/

	m00 = gMatrix.value[0][0];	m01 = gMatrix.value[0][1];	m02 = gMatrix.value[0][2];
	m10 = gMatrix.value[1][0];	m11 = gMatrix.value[1][1];	m12 = gMatrix.value[1][2];
	m20 = gMatrix.value[2][0];	m21 = gMatrix.value[2][1];	m22 = gMatrix.value[2][2];
	m30 = gMatrix.value[3][0];	m31 = gMatrix.value[3][1];	m32 = gMatrix.value[3][2];

	for (p = 0; p < numPoints; p++)
	{
		float	x,y,z;
		
		i = bonePtr->pointList[p];														// get index to point in gDecomposedPointList
		x = decomposedPointList[i].boneRelPoint.x;										// get xyz of point
		y = decomposedPointList[i].boneRelPoint.y;
		z = decomposedPointList[i].boneRelPoint.z;

				/* TRANSFORM & UPDATE BBOX */
				
		newX = (m00*x) + (m10*y) + (m20*z) + m30;										// transform x value
		if (newX < minX)																// update bbox
			minX = newX;
		else
		if (newX > maxX)
			maxX = newX;

		newY = (m01*x) + (m11*y) + (m21*z) + m31;										// transform y
		if (newY < minY)																// update bbox
			minY = newY;
		else
		if (newY > maxY)
			maxY = newY;

		newZ = (m02*x) + (m12*y) + (m22*z) + m32;										// transform z
		if (newZ > maxZ)																// update bbox
			maxZ = newZ;
		else
		if (newZ < minZ)
			minZ = newZ;
		
	
				/* APPLY NEW POINT TO ALL REFERENCES */
				
		numRefs = decomposedPointList[i].numRefs;										// get # times this point is referenced
		if (numRefs == 1)																// SPECIAL CASE IF ONLY 1 REF (OPTIMIZATION)
		{
			triMeshNum = decomposedPointList[i].whichTriMesh[0];						// get triMesh # that uses this point
			p2 = decomposedPointList[i].whichPoint[0];									// get point # in the triMesh
	
			localTriMeshPtrs[triMeshNum]->points[p2].x = newX;								// set the point in local copy of trimesh
			localTriMeshPtrs[triMeshNum]->points[p2].y = newY;
			localTriMeshPtrs[triMeshNum]->points[p2].z = newZ;
		}
		else																			// multi-refs
		{		
			for (r = 0; r < numRefs; r++)
			{
				triMeshNum = decomposedPointList[i].whichTriMesh[r];					
				p2 = decomposedPointList[i].whichPoint[r];								
		
				localTriMeshPtrs[triMeshNum]->points[p2].x = newX;
				localTriMeshPtrs[triMeshNum]->points[p2].y = newY;
				localTriMeshPtrs[triMeshNum]->points[p2].z = newZ;
			}
		}
	}

				/* UPDATE GLOBAL BBOX */
				
	if (minX < gBBox.min.x)
		gBBox.min.x = minX;
	else
	if (maxX > gBBox.max.x)
		gBBox.max.x = maxX;

	if (minY < gBBox.min.y)
		gBBox.min.y = minY;
	else
	if (maxY > gBBox.max.y)
		gBBox.max.y = maxY;

	if (minZ < gBBox.min.z)
		gBBox.min.z = minZ;
	else
	if (maxZ > gBBox.max.z)
		gBBox.max.z = maxZ;


			/* RECURSE THRU ALL CHILDREN */
			
	numChildren = currentSkeleton->numChildren[joint];									// get # children
	for (c = 0; c < numChildren; c++)
	{
		oldM = gMatrix;																	// push matrix
		UpdateSkinnedGeometry_Recurse(currentSkeleton->childIndecies[joint][c]);
		gMatrix = oldM;																	// pop matrix
	}

}


/******************* PRIME BONE DATA *********************/
//
// After a skeleton file is loaded, this will calc some other needed things.
//

void PrimeBoneData(SkeletonDefType *skeleton)
{
long	i,b,j;

	if (skeleton->NumBones == 0)
		DoFatalAlert("PrimeBoneData: # = 0??");

			
			/* SET THE FORWARD LINKS */
			
	for (b = 0; b < skeleton->NumBones; b++)
	{
		skeleton->numChildren[b] = 0;								// init child counter
		
		for (i = 0; i < skeleton->NumBones; i++)					// look for a child
		{
			if (skeleton->Bones[i].parentBone == b)					// is this "i" a child of "b"?
			{
				j = skeleton->numChildren[b];						// get # children
				if (j >= MAX_CHILDREN)
					DoFatalAlert("CreateSkeletonFromBones: MAX_CHILDREN exceeded!");
				
				skeleton->childIndecies[b][j] = i;					// set index to child
	
				skeleton->numChildren[b]++;							// inc # children
			}
		}
	}
}















