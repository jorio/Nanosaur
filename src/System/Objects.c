/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)1993-1997 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "globals.h"

#include "QD3D.h"
#include "QD3DMath.h"

#if __APPLE__
	#include <OpenGL/glu.h>		// gluPerspective
#else
	#include <GL/glu.h>			// gluPerspective
#endif

#include <GamePatches.h>
#include <string.h>

#include "objects.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "qd3d_support.h"
#include "qd3d_geometry.h"
#include "3dmath.h"
#include "3dmf.h"
#include "sound2.h"
#include "camera.h"
#include "terrain.h"
#include "mobjtypes.h"
#include "environmentmap.h"
#include "bones.h"
#include "collision.h"

extern	TQ3TriMeshFlatGroup	gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	float		gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	short		gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	float		gFramesPerSecondFrac;
extern	ObjNode		*gPlayerObj;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	PrefsType	gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void FlushObjectDeleteQueue(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	OBJ_DEL_Q_SIZE	100


/**********************/
/*     VARIABLES      */
/**********************/

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;
					
ObjNode		*gCurrentNode,*gMostRecentlyAddedNode, *gNextNode;
										
NewObjectDefinitionType	gNewObjectDefinition;

TQ3Point3D	gCoord;
TQ3Vector3D	gDelta;

TQ3Object	gKeepBackfaceStyleObject = nil;

long		gNumObjsInDeleteQueue = 0;
ObjNode		*gObjectDeleteQueue[OBJ_DEL_Q_SIZE];

long		gNodesDrawn = 0;  // Source port addition - nodes drawn during a frame (for statistics)
long		gTrianglesDrawn = 0;  // Source port addition - triangles drawn during a frame (for statistics)
long		gMeshesDrawn = 0;

//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CREATION ------

/************************ INIT OBJECT MANAGER **********************/

void InitObjectManager(void)
{

				/* INIT LINKED LIST */

															
	gCurrentNode = nil;
	
					/* CLEAR ENTIRE OBJECT LIST */
		
	gFirstNodePtr = nil;									// no node yet

#if 1
	printf("TODO noquesa: Make Backface Style Object\n");
#else
			/* MAKE BACKFACE STYLE OBJECT */
			
	if (gKeepBackfaceStyleObject == nil)
	{
		gKeepBackfaceStyleObject = Q3BackfacingStyle_New(kQ3BackfacingStyleBoth);
		if (gKeepBackfaceStyleObject == nil)
			DoFatalAlert("InitObjectManager: Q3BackfacingStyle_New failed!");
	}
	else
		DoFatalAlert("InitObjectManager: gKeepBackfaceStyleObject != nil");

#endif
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNodePtr,*scanNodePtr,*reNodePtr;
long	slot;

	
				/* INITIALIZE NEW NODE */
	
	newNodePtr = (ObjNode *) NewPtrClear(sizeof(ObjNode));
	if (newNodePtr == nil)
		DoFatalAlert("MakeNewObject: Alloc Ptr failed!");

	slot = newObjDef->slot;

	newNodePtr->Slot = slot;
	newNodePtr->Type = newObjDef->type;
	newNodePtr->Group = newObjDef->group;
	newNodePtr->MoveCall = newObjDef->moveCall;						// save move routine
	newNodePtr->Genre = newObjDef->genre;
	newNodePtr->Coord = newNodePtr->OldCoord = newObjDef->coord;	// save coords
	newNodePtr->StatusBits = newObjDef->flags;
	memset(newNodePtr->Flag, 0, sizeof(newNodePtr->Flag));
	memset(newNodePtr->Special, 0, sizeof(newNodePtr->Special));
	memset(newNodePtr->SpecialF, 0, sizeof(newNodePtr->SpecialF));
	memset(newNodePtr->SpecialRef, 0, sizeof(newNodePtr->SpecialRef));
	newNodePtr->CType =							// must init ctype to something ( INVALID_NODE_FLAG might be set from last delete)
	newNodePtr->CBits =
	newNodePtr->Delta.x =
	newNodePtr->Delta.y = 
	newNodePtr->Delta.z = 
	newNodePtr->RotDelta.x = 
	newNodePtr->RotDelta.y = 
	newNodePtr->RotDelta.z = 
	newNodePtr->Rot.x =
	newNodePtr->Rot.z = 0;
	newNodePtr->Rot.y =  newObjDef->rot;
	newNodePtr->Scale.x =
	newNodePtr->Scale.y = 
	newNodePtr->Scale.z = newObjDef->scale;
	newNodePtr->TargetOff.x =
	newNodePtr->TargetOff.y = 0;
	newNodePtr->Radius = 4;								// set default radius
	
	newNodePtr->Damage = 0;
	newNodePtr->Health = 0;
	newNodePtr->Mode = 0;
		
	newNodePtr->Speed = 0;
	newNodePtr->Accel = 0;
	newNodePtr->TerrainAccel.x = newNodePtr->TerrainAccel.y = 0;

#if 0	// NOQUESA - we alloc'd it with NewPtrClear, so we're fine
	newNodePtr->CarriedObj = nil;						// nothing being carried
	
	newNodePtr->ChainNode = nil;
	newNodePtr->ChainHead = nil;
	newNodePtr->BaseGroup = nil;						// nothing attached as QD3D object yet
	newNodePtr->ShadowNode = nil;
	newNodePtr->PlatformNode = nil;						// assume not on any platform
	newNodePtr->BaseTransformObject = nil;
	newNodePtr->NumCollisionBoxes = 0;
	newNodePtr->CollisionBoxes = nil;					// no collision boxes yet
	newNodePtr->CollisionTriangles = nil;
#endif

	newNodePtr->StreamingEffect = -1;					// no streaming sound effect

	newNodePtr->TerrainItemPtr = nil;					// assume not a terrain item

	newNodePtr->Skeleton = nil;
	
			/* MAKE SURE SCALE != 0 */
			
	if (newNodePtr->Scale.x == 0.0f)
		newNodePtr->Scale.x = 0.0001f;
	if (newNodePtr->Scale.y == 0.0f)
		newNodePtr->Scale.y = 0.0001f;
	if (newNodePtr->Scale.z == 0.0f)
		newNodePtr->Scale.z = 0.0001f;


					/* FIND INSERTION PLACE FOR NODE */
					
	if (gFirstNodePtr == nil)							// special case only entry
	{
		gFirstNodePtr = newNodePtr;
		newNodePtr->PrevNode = nil;
		newNodePtr->NextNode = nil;
	}
	else
	if (slot < gFirstNodePtr->Slot)					// INSERT AS FIRST NODE
	{
		newNodePtr->PrevNode = nil;					// no prev
		newNodePtr->NextNode = gFirstNodePtr; 		// next pts to old 1st
		gFirstNodePtr->PrevNode = newNodePtr; 		// old pts to new 1st
		gFirstNodePtr = newNodePtr;
	}
	else
	{
		reNodePtr = gFirstNodePtr;
		scanNodePtr = gFirstNodePtr->NextNode;		// start scanning for insertion slot on 2nd node
			
		while (scanNodePtr != nil)
		{
			if (slot < scanNodePtr->Slot)					// INSERT IN MIDDLE HERE
			{
				newNodePtr->NextNode = scanNodePtr;
				newNodePtr->PrevNode = reNodePtr;
				reNodePtr->NextNode = newNodePtr;
				scanNodePtr->PrevNode = newNodePtr;			
				goto out;
			}
			reNodePtr = scanNodePtr;
			scanNodePtr = scanNodePtr->NextNode;			// try next node
		} 
	
		newNodePtr->NextNode = nil;							// TAG TO END
		newNodePtr->PrevNode = reNodePtr;
		reNodePtr->NextNode = newNodePtr;		
	}

out:
	gMostRecentlyAddedNode = newNodePtr;					// remember this
	return(newNodePtr);
}

/************* MAKE NEW DISPLAY GROUP OBJECT *************/
//
// This is an ObjNode who's BaseGroup is a group, therefore these objects
// can be transformed (scale,rot,trans).
//

ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newObj;
Byte	group,type;


	newObjDef->genre = DISPLAY_GROUP_GENRE;
	
	newObj = MakeNewObject(newObjDef);		
	if (newObj == nil)
		return(nil);

			/* MAKE BASE GROUP & ADD GEOMETRY TO IT */
	
	newObj->Rot.y = newObjDef->rot;		
	CreateBaseGroup(newObj);											// create group object
	group = newObjDef->group;											// get group #
	type = newObjDef->type;												// get type #
	
	GAME_ASSERT(type < gNumObjectsInGroupList[group]);					// see if illegal

	TQ3TriMeshFlatGroup* meshList = &gObjectGroupList[group][type];
	AttachGeometryToDisplayGroupObject(newObj, meshList->numMeshes, meshList->meshes);

			/* CALC RADIUS */
			
	newObj->Radius = gObjectGroupRadiusList[group][type] * newObj->Scale.x;	
	
	return(newObj);
}


/************************* ADD GEOMETRY TO DISPLAY GROUP OBJECT ***********************/
//
// Attaches a geometry object to the BaseGroup object. MakeNewDisplayGroupObject must have already been
// called which made the group & transforms.
//

void AttachGeometryToDisplayGroupObject(ObjNode* theNode, int numMeshes, TQ3TriMeshData** meshList)
{
	for (int i = 0; i < numMeshes; i++)
	{
		int nodeMeshIndex = theNode->NumMeshes;

		theNode->NumMeshes++;
		GAME_ASSERT(theNode->NumMeshes <= MAX_MESHHANDLES_IN_OBJNODE);

		theNode->MeshList[nodeMeshIndex] = meshList[i];
	}
}



/***************** CREATE BASE GROUP **********************/
//
// The base group contains the base transform matrix plus any other objects you want to add into it.
// This routine creates the new group and then adds a transform matrix.
//
// The base is composed of BaseGroup & BaseTransformObject.
//

void CreateBaseGroup(ObjNode *theNode)
{
TQ3Matrix4x4			transMatrix,scaleMatrix,rotMatrix;

				/* CREATE THE GROUP OBJECT */

	theNode->NumMeshes = 0;

					/* SETUP BASE MATRIX */
			
	if ((theNode->Scale.x == 0) || (theNode->Scale.y == 0) || (theNode->Scale.z == 0))
		DoFatalAlert("CreateBaseGroup: A scale component == 0");
		
			
	Q3Matrix4x4_SetScale(&scaleMatrix, theNode->Scale.x, theNode->Scale.y,		// make scale matrix
							theNode->Scale.z);
			
	Q3Matrix4x4_SetRotate_XYZ(&rotMatrix, theNode->Rot.x, theNode->Rot.y,		// make rotation matrix
								 theNode->Rot.z);

	Q3Matrix4x4_SetTranslate(&transMatrix, theNode->Coord.x, theNode->Coord.y,	// make translate matrix
							 theNode->Coord.z);

	Q3Matrix4x4_Multiply(&scaleMatrix,											// mult scale & rot matrices
						 &rotMatrix,
						 &theNode->BaseTransformMatrix);

	Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,							// mult by trans matrix
						 &transMatrix,
						 &theNode->BaseTransformMatrix);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT PROCESSING ------


/*******************************  MOVE OBJECTS **************************/

void MoveObjects(void)
{
ObjNode		*thisNodePtr;

	if (gFirstNodePtr == nil)								// see if there are any objects
		return;

	thisNodePtr = gFirstNodePtr;
	
	do
	{
		gNextNode = thisNodePtr->NextNode;
		gCurrentNode = thisNodePtr;						// set current object node
		
		KeepOldCollisionBoxes(thisNodePtr);					// keep old box
		
			
				/* UPDATE ANIMATION */
				
		if (thisNodePtr->StatusBits & STATUS_BIT_ANIM)
			UpdateSkeletonAnimation(thisNodePtr);


					/* NEXT TRY TO MOVE IT */
					
		if ((!(thisNodePtr->StatusBits & STATUS_BIT_NOMOVE)) &&
			(thisNodePtr->MoveCall != nil))
		{
			thisNodePtr->MoveCall(thisNodePtr);				// call object's move routine
		}
			
					
		thisNodePtr = gNextNode;		// next node
	}
	while (thisNodePtr != nil);

			/* CALL SOUND MAINTENANCE HERE FOR CONVENIENCE */
			
	DoSoundMaintenance();
	
	DoSDLMaintenance(); // Source port addition, for convenience as well

			/* FLUSH THE DELETE QUEUE */
			
	FlushObjectDeleteQueue();
}




/**************************** DRAW OBJECTS ***************************/

static void DrawTriMeshList(int numMeshes, TQ3TriMeshData** meshList)
{
	for (int i = 0; i < numMeshes; i++)
	{
		const TQ3TriMeshData* mesh = meshList[i];

		glVertexPointer(3, GL_FLOAT, 0, mesh->points);
		glNormalPointer(GL_FLOAT, 0, mesh->vertexNormals);
		glColorPointer(4, GL_FLOAT, 0, mesh->vertexColors);
		CHECK_GL_ERROR();

		if (mesh->hasTexture)
		{
			glEnable(GL_TEXTURE_2D);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindTexture(GL_TEXTURE_2D, mesh->glTextureName);
			glTexCoordPointer(2, GL_FLOAT, 0, mesh->vertexUVs);
			CHECK_GL_ERROR();
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			CHECK_GL_ERROR();
		}

		glDrawRangeElements(GL_TRIANGLES, 0, mesh->numPoints-1, mesh->numTriangles*3, GL_UNSIGNED_SHORT, mesh->triangles);
		CHECK_GL_ERROR();

		gTrianglesDrawn += mesh->numTriangles;
		gMeshesDrawn++;
	}
}

	void DrawObjects(QD3DSetupOutputType *setupInfo)
{
ObjNode		*theNode;
unsigned long	statusBits;
#if 0	// NOQUESA
TQ3Status	myStatus;
short		i,numTriMeshes;
TQ3ViewObject	view = setupInfo->viewObject;
#if 0 // Source port removal: completely removed triangle caching
Boolean			cacheMode;
#endif
#endif

	gNodesDrawn = 0;
	gTrianglesDrawn = 0;
	gMeshesDrawn = 0;

	if (gFirstNodePtr == nil)							// see if there are any objects
		return;

				/* FIRST DO OUR CULLING */
				
	CheckAllObjectsInConeOfVision();
	
	theNode = gFirstNodePtr;



#if 0
			/* TURN ON TRIANGLE CACHING */
			
	QD3D_SetTriangleCacheMode(true);
	cacheMode = true;
#endif


#if 0	// NOQUESA
				/* SET TEXTURE FILTER FOR ALL NODES */
	// Source port change: the original game used to default to nearest-neighbor texturing.
	// You had to enable texture filtering by setting STATUS_BIT_HIGHFILTER in the statusBits of
	// individual nodes. However, this was applied inconsistently -- some props (mushrooms,
	// trees etc.) didn't have this status bit and therefore always used NN. I assume this was
	// unintentional, so I'm setting texture filtering for ALL nodes here, rather than per-node.
	QD3D_SetTextureFilter(gGamePrefs.highQualityTextures
			? kQATextureFilter_Best
			: kQATextureFilter_Fast);
#endif

			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/			
	do
	{
#if 1	// NOQUESA -- TODO: nuke this
		if (theNode->StatusBits & STATUS_BIT_REFLECTIONMAP)
		{
			theNode->StatusBits &= ~STATUS_BIT_REFLECTIONMAP;
			printf("TODO noquesa: %s:%d: I'm killing the reflectionmap flag for now\n", __func__, __LINE__);
		}
#endif

		statusBits = theNode->StatusBits;						// get obj's status bits

		if (statusBits & STATUS_BIT_ISCULLED)					// see if is culled
			goto next;

		if (statusBits & STATUS_BIT_HIDDEN)						// see if is hidden
			goto next;		

		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;		

#if 0
				/* CHECK TRIANGLE CACHING */
				
		if (statusBits & STATUS_BIT_NOTRICACHE)					// see if disable caching
		{
			if (cacheMode)										// only disable if currently on
			{
				QD3D_SetTriangleCacheMode(false);
				cacheMode = false;
			}
		}
		else
		if (!cacheMode)											// if caching disabled, reenable it
			QD3D_SetTriangleCacheMode(true);
#endif


		if (!(statusBits & STATUS_BIT_REFLECTIONMAP))
		{
#if 0   // Source port removal
				/* CHECK TEXTURE FILTERING */
				
			if (statusBits & STATUS_BIT_HIGHFILTER)
			{
				if (statusBits & STATUS_BIT_HIGHFILTER2)
					QD3D_SetTextureFilter(kQATextureFilter_Best);			// set best textures
				else
					QD3D_SetTextureFilter(kQATextureFilter_Mid);			// set nice textures
			}
		
#endif
#if 0   // Source port removal
				/* CHECK BLENDING */
				
			if (statusBits & STATUS_BIT_BLEND_INTERPOLATE)
				ONCE(TODOMINOR2("QD3D_SetBlendingMode(kQABlend_Interpolate);"));
#endif
		
				/* CHECK NULL SHADER */

			if (statusBits & STATUS_BIT_NULLSHADER)
#if 1	// NOQUESA
				printf("TODO noquesa: %s:%d: submit null shader\n", __func__, __LINE__);
#else
				Q3Shader_Submit(setupInfo->nullShaderObject, view);
#endif
		
			switch(theNode->Genre)
			{
				case	SKELETON_GENRE:
						GetModelCurrentPosition(theNode->Skeleton);
						UpdateSkinnedGeometry(theNode);
						// Don't mult matrix with BaseTransformMatrix -- skeleton code already does it
						DrawTriMeshList(theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes, theNode->Skeleton->localTriMeshPtrs);
						gNodesDrawn++;
						break;

				case	DISPLAY_GROUP_GENRE:
						glPushMatrix();
						glMultMatrixf(&theNode->BaseTransformMatrix.value[0][0]);
						DrawTriMeshList(theNode->NumMeshes, theNode->MeshList);
						glPopMatrix();
						gNodesDrawn++;
						break;
			}

					/* UNDO STATUS MODES */
								
#if 0   // Source port removal
			if (statusBits & STATUS_BIT_HIGHFILTER)
				QD3D_SetTextureFilter(kQATextureFilter_Fast);				// undo nice textures			
#endif

			if (statusBits & STATUS_BIT_NULLSHADER)							// undo NULL shader
#if 1	// NOQUESA
				printf("TODO noquesa: %s:%d: undo null shader\n", __func__, __LINE__);
#else
				Q3Shader_Submit(setupInfo->shaderObject, view);
#endif
				
#if 0   // Source port removal
			if (statusBits & STATUS_BIT_BLEND_INTERPOLATE)
				ONCE(TODOMINOR2("QD3D_SetBlendingMode(kQABlend_PreMultiply);"));						// premul is normal
#endif
				
		}
		
next:
		theNode = (ObjNode *)theNode->NextNode;
	}while (theNode != nil);

	SubmitReflectionMapQueue(setupInfo);				// draw anything in the reflection map queue
}


/********************* MOVE STATIC OBJECT **********************/

void MoveStaticObject(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
		DeleteObject(theNode);

}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT DELETING ------


/******************** DELETE ALL OBJECTS ********************/

void DeleteAllObjects(void)
{
	while (gFirstNodePtr != nil)
		DeleteObject(gFirstNodePtr);
		
	FlushObjectDeleteQueue();
	InitReflectionMapQueue();						// also purge data from here
}


/************************ DELETE OBJECT ****************/

void DeleteObject(ObjNode	*theNode)
{
ObjNode *tempNode;

	if (theNode == nil)								// see if passed a bogus node
		return;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
	{
		Str255	errString;		//-----------
		NumToStringC(theNode->Type,errString);		//------------
		DoFatalAlert2("Attempted to Double Delete an Object.  Object was already deleted!", errString);
	}

			/* RECURSIVE DELETE OF CHAIN NODE & SHADOW NODE */
			//
			// should do these first so that base node will have appropriate nextnode ptr
			// since it's going to be used next pass thru the moveobjects loop.  This
			// assumes that all chained nodes are later in list.
			//
			
	if (theNode->ChainNode)
		DeleteObject(theNode->ChainNode);

	if (theNode->ShadowNode)
		DeleteObject(theNode->ShadowNode);


			/* SEE IF NEED TO FREE UP SPECIAL MEMORY */
			
	if (theNode->Genre == SKELETON_GENRE)
	{
		FreeSkeletonBaseData(theNode->Skeleton);	// purge all alloced memory for skeleton data
		theNode->Skeleton = nil;
	}
	
	if (theNode->CollisionBoxes != nil)				// free collision box memory
	{
		DisposePtr((Ptr)theNode->CollisionBoxes);
		theNode->CollisionBoxes = nil;
	}

	if (theNode->CollisionTriangles)				// free collision triangle memory
		DisposeCollisionTriangleMemory(theNode);

		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */
	
	DisposeObjectBaseGroup(theNode);		


					/* DO NODE SWITCHING */

	if (theNode == gNextNode)						// see if this was to be the next node in MoveObjects
		gNextNode = theNode->NextNode;

	if (theNode->PrevNode == nil)					// special case 1st node
	{
		gFirstNodePtr = theNode->NextNode;		
		tempNode = theNode->NextNode;
		if (tempNode != nil)
			tempNode->PrevNode = nil;
	}
	else if (theNode->NextNode == nil)				// special case last node
	{
		tempNode = theNode->PrevNode;
		tempNode->NextNode = nil;
	}
	else											// generic middle deletion
	{
		tempNode = theNode->PrevNode;
		tempNode->NextNode = theNode->NextNode;
		tempNode = theNode->NextNode;
		tempNode->PrevNode = theNode->PrevNode;
	}


			/* SEE IF MARK AS NOT-IN-USE IN ITEM LIST */
			
	if (theNode->TerrainItemPtr)
		theNode->TerrainItemPtr->flags &= ~ITEM_FLAGS_INUSE;		// clear the "in use" flag


			/* DELETE THE NODE BY ADDING TO DELETE QUEUE */

	theNode->CType = INVALID_NODE_FLAG;							// INVALID_NODE_FLAG indicates its deleted


	gObjectDeleteQueue[gNumObjsInDeleteQueue++] = theNode;
	if (gNumObjsInDeleteQueue >= OBJ_DEL_Q_SIZE)
		FlushObjectDeleteQueue();
	
}

/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(void)
{
long	i,num;

	num = gNumObjsInDeleteQueue;
	
	for (i = 0; i < num; i++)
		DisposePtr((Ptr)gObjectDeleteQueue[i]);					

	gNumObjsInDeleteQueue = 0;
}


/****************** DISPOSE OBJECT BASE GROUP **********************/

void DisposeObjectBaseGroup(ObjNode *theNode)
#if 1	// TODO noquesa
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
TQ3Status	status;

	if (theNode->BaseGroup != nil)
	{
		status = Q3Object_Dispose(theNode->BaseGroup);
		if (status != kQ3Success)
			DoFatalAlert("DisposeObjectBaseGroup: Q3Object_Dispose Failed!");

		theNode->BaseGroup = nil;
	}
	
	if (theNode->BaseTransformObject != nil)							// also nuke extra ref to transform object
	{
		Q3Object_Dispose(theNode->BaseTransformObject);
		theNode->BaseTransformObject = nil;
	}
}
#endif




//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT INFORMATION ------


/********************** GET OBJECT INFO *********************/

void GetObjectInfo(ObjNode *theNode)
{
	gCoord = theNode->Coord;
	gDelta = theNode->Delta;
}


/********************** UPDATE OBJECT *********************/

void UpdateObject(ObjNode *theNode)
{
	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;
		
	theNode->Coord = gCoord;
	theNode->Delta = gDelta;
	UpdateObjectTransforms(theNode);
	if (theNode->CollisionBoxes)
		CalcObjectBoxFromNode(theNode);


		/* UPDATE ANY SHADOWS */
				
	if (theNode->ShadowNode)
		UpdateShadow(theNode);		
}



/****************** UPDATE OBJECT TRANSFORMS *****************/
//
// This updates the skeleton object's base translate & rotate transforms
//

void UpdateObjectTransforms(ObjNode *theNode)
{
TQ3Matrix4x4	matrix;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

						/* INIT MATRIX */
						
	Q3Matrix4x4_SetIdentity(&theNode->BaseTransformMatrix);


					/********************/
					/* SET SCALE MATRIX */
					/********************/

	if ((theNode->Scale.x != 1) || (theNode->Scale.y != 1) || (theNode->Scale.z != 1))		// see if ignore scale
	{
		Q3Matrix4x4_SetScale(&matrix, theNode->Scale.x,	theNode->Scale.y, theNode->Scale.z);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix, &matrix, &theNode->BaseTransformMatrix);
	}
	
					/*****************/
					/* NOW ROTATE IT */
					/*****************/
			
					/* SEE IF DO Z->Y->X */
					
	if (theNode->StatusBits & STATUS_BIT_ROTZYX)
	{
		Q3Matrix4x4_SetRotate_Z(&matrix, theNode->Rot.z);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
		
		Q3Matrix4x4_SetRotate_Y(&matrix, theNode->Rot.y);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
		
		Q3Matrix4x4_SetRotate_X(&matrix, theNode->Rot.x);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
	}
	else
					/* SEE IF DO X->Z->Y */
	
	if (theNode->StatusBits & STATUS_BIT_ROTXZY)
	{
		Q3Matrix4x4_SetRotate_X(&matrix, theNode->Rot.x);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
		
		Q3Matrix4x4_SetRotate_Z(&matrix, theNode->Rot.z);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
		
		Q3Matrix4x4_SetRotate_Y(&matrix, theNode->Rot.y);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
	}
	
	
					/* DO X->Y->Z */
	else
	{
//		SetQuickRotationMatrix_XYZ(&matrix, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
//		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
	
		Q3Matrix4x4_SetRotate_XYZ(&matrix,													// init rotation matrix
								 theNode->Rot.x, theNode->Rot.y,
								 theNode->Rot.z);
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
	}
	
					/********************/
					/* NOW TRANSLATE IT */
					/********************/

	Q3Matrix4x4_SetTranslate(&matrix, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);	// make translate matrix
	Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);


#if 0	// NOQUESA
				/* UPDATE TRANSFORM OBJECT */
				
	SetObjectTransformMatrix(theNode);
#endif
}


/********************* MAKE OBJECT KEEP BACKFACES ***********************/
//
// Puts a backfacing style object in the base group.
//

void MakeObjectKeepBackfaces(ObjNode *theNode)
#if 1	// TODO noquesa
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
TQ3GroupPosition	position;
TQ3Status			status;
TQ3ObjectType		oType;

	if (theNode->BaseGroup == nil)
		DoFatalAlert("MakeObjectKeepBackfaces: BaseGroup == nil");

	oType = Q3Group_GetType(theNode->BaseGroup);
	if (oType == kQ3ObjectTypeInvalid)
		DoFatalAlert("MakeObjectKeepBackfaces: BaseGroup is not a Group Object!");

	status = Q3Group_GetFirstPosition(theNode->BaseGroup, &position);
	if ((status == kQ3Failure) || (position == nil))
		DoFatalAlert("MakeObjectKeepBackfaces: Q3Group_GetFirstPosition failed!");
	
	if (Q3Group_AddObjectBefore(theNode->BaseGroup, position, gKeepBackfaceStyleObject) == nil)
		DoFatalAlert("MakeObjectKeepBackfaces: Q3Group_AddObjectBefore failed!");
}
#endif




/********************* MAKE OBJECT TRANSPARENT ***********************/
//
// Puts a transparency attribute object in the base group.
//
// INPUT: transPecent = 0..1.0   0 = totally trans, 1.0 = totally opaque

void MakeObjectTransparent(ObjNode *theNode, float transPercent)
#if 1	// TODO noquesa
{ printf("TODO noquesa: %s\n", __func__); }
#else
{
TQ3GroupPosition	position;
TQ3Status			status;
TQ3ObjectType		oType;
TQ3Object			attrib;
TQ3ColorRGB			transColor;
TQ3AttributeType	attribType;

	if (theNode->BaseGroup == nil)
		DoFatalAlert("MakeObjectTransparent: BaseGroup == nil");

	oType = Q3Group_GetType(theNode->BaseGroup);
	if (oType == kQ3ObjectTypeInvalid)
		DoFatalAlert("MakeObjectTransparent: BaseGroup is not a Group Object!");


			/* GET CURRENT ATTRIBUTE OBJECT OR MAKE NEW ONE */

	status = Q3Group_GetFirstPositionOfType(theNode->BaseGroup, kQ3SharedTypeSet, &position);	// see if attribute object in the group
	if (position != nil)																		// YES
	{
		status = Q3Group_GetPositionObject(theNode->BaseGroup, position, &attrib);				// get the attribute object
		if (status == kQ3Failure)
			DoFatalAlert("MakeObjectTransparent: Q3Group_GetPositionObject failed");

		if (transPercent == 1.0)															// if totally opaque then remove this attrib
		{
			Q3AttributeSet_Clear(attrib, kQ3AttributeTypeTransparencyColor);				// remove this attrib type from attrib
			
			attribType = kQ3AttributeTypeNone;
			Q3AttributeSet_GetNextAttributeType(attrib, &attribType);						// if nothing in attrib set then remove attrib obj
			if (attribType == kQ3AttributeTypeNone)
			{
				TQ3Object	removedObj;
				
				removedObj = Q3Group_RemovePosition(theNode->BaseGroup, position);			// remove attrib from group		
				if (removedObj)
					Q3Object_Dispose(removedObj);											// now throw it out
			}
			goto bye;
		}		
	}
	
					/* NO ATTRIB OBJ IN GROUP, SO MAKE NEW ATTRIB SET */
	else
	{
		if (transPercent == 1.0)															// if totally opaque then dont do it
			return;

		attrib = Q3AttributeSet_New();														// make new attrib set
		if (attrib == nil)
			DoFatalAlert("MakeObjectTransparent: Q3AttributeSet_New failed");
			
				/* ADD NEW ATTRIB SET TO FRONT OF GROUP */
					
		status = Q3Group_GetFirstPosition(theNode->BaseGroup, &position);
		if ((status == kQ3Failure) || (position == nil))
			DoFatalAlert("MakeObjectTransparent: Q3Group_GetFirstPosition failed!");
		
		if (Q3Group_AddObjectBefore(theNode->BaseGroup, position, attrib) == nil)
			DoFatalAlert("MakeObjectTransparent: Q3Group_AddObjectBefore failed!");
	}
				
					/* ADD TRANSPARENCY */
					
	transColor.r = transColor.g = transColor.b = transPercent;
	status = Q3AttributeSet_Add(attrib, kQ3AttributeTypeTransparencyColor, &transColor);
	if (status == kQ3Failure)
		DoFatalAlert("Q3AttributeSet_Add: Q3Group_GetPositionObject failed");
	
bye:
	Q3Object_Dispose(attrib);								// dispose of extra ref
}
#endif

