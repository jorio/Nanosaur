/*********************************/
/*    OBJECT MANAGER 		     */
/* By Brian Greenstone      	 */
/* (c)1993-1997 Pangea Software  */
/* (c)2023 Iliyas Jorio          */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisposeObjNodeMemory(ObjNode* node);
static void FlushObjectDeleteQueue(int qid);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	OBJ_DEL_Q_SIZE	1024	// number of ObjNodes that can be deleted during any given frame
#define	OBJ_BUDGET		1024


/**********************/
/*     VARIABLES      */
/**********************/

static ObjNode		gObjNodeMemory[OBJ_BUDGET];
Pool				*gObjNodePool = NULL;

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;
					
ObjNode		*gCurrentNode,*gMostRecentlyAddedNode, *gNextNode;


NewObjectDefinitionType	gNewObjectDefinition;

TQ3Point3D	gCoord;
TQ3Vector3D	gDelta;

// Source port change: We now have a double-buffered deletion queue, so objects
// that get queued for deletion during frame N will be deleted at the end of frame N+1.
// This gives live objects a full frame to clean up references to dead objects.
static int			gNumObjsInDeleteQueue[2];
static ObjNode*		gObjectDeleteQueue[2][OBJ_DEL_Q_SIZE];
static int			gObjectDeleteQueueFlipFlop = 0;


extern RenderStats	gRenderStats;

//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CREATION ------

/************************ INIT OBJECT MANAGER **********************/

void InitObjectManager(void)
{
		/* INIT LINKED LIST */

	gCurrentNode = nil;
	gFirstNodePtr = nil;									// no node yet

		/* INIT OBJECT POOL */

	memset(gObjNodeMemory, 0, sizeof(gObjNodeMemory));

	if (!gObjNodePool)
		gObjNodePool = Pool_New(OBJ_BUDGET);
	else
		Pool_Reset(gObjNodePool);
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef)
{
	ObjNode* newNodePtr;

		/* TRY TO GET AN OBJECT FROM THE POOL */

	GAME_ASSERT(gObjNodePool);

	int pooledIndex = Pool_AllocateIndex(gObjNodePool);
	if (pooledIndex >= 0)
	{
		newNodePtr = &gObjNodeMemory[pooledIndex];
	}
	else
	{
		// pool full, alloc new node on heap
		newNodePtr = (ObjNode*) AllocPtr(sizeof(ObjNode));
	}

		/* MAKE SURE WE GOT ONE */

	GAME_ASSERT(newNodePtr);

				/* INITIALIZE NEW NODE */

	memset(newNodePtr, 0, sizeof(ObjNode));

	newNodePtr->Slot		= newObjDef->slot;
	newNodePtr->Type		= newObjDef->type;
	newNodePtr->Group		= newObjDef->group;
	newNodePtr->MoveCall	= newObjDef->moveCall;						// save move routine
	newNodePtr->Genre		= newObjDef->genre;
	newNodePtr->Coord		= newNodePtr->OldCoord = newObjDef->coord;	// save coords
	newNodePtr->StatusBits	= newObjDef->flags;
	newNodePtr->Rot			= (TQ3Vector3D){ 0, newObjDef->rot, 0 };
	newNodePtr->Scale		= (TQ3Vector3D){ newObjDef->scale, newObjDef->scale, newObjDef->scale };
	newNodePtr->Radius		= 4;								// set default radius
	newNodePtr->TerrainAccel.x = newNodePtr->TerrainAccel.y = 0;
	newNodePtr->StreamingEffect = -1;					// no streaming sound effect
	newNodePtr->TerrainItemPtr = nil;					// assume not a terrain item
	newNodePtr->Skeleton = nil;

	newNodePtr->RenderModifiers.statusBits = 0;
	newNodePtr->RenderModifiers.diffuseColor = (TQ3ColorRGBA) { 1,1,1,1 };	// default diffuse color is opaque white

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
	if (newNodePtr->Slot < gFirstNodePtr->Slot)		// INSERT AS FIRST NODE
	{
		newNodePtr->PrevNode = nil;					// no prev
		newNodePtr->NextNode = gFirstNodePtr; 		// next pts to old 1st
		gFirstNodePtr->PrevNode = newNodePtr; 		// old pts to new 1st
		gFirstNodePtr = newNodePtr;
	}
	else
	{
		ObjNode* reNodePtr = gFirstNodePtr;
		ObjNode* scanNodePtr = gFirstNodePtr->NextNode;		// start scanning for insertion slot on 2nd node
			
		while (scanNodePtr != nil)
		{
			if (newNodePtr->Slot < scanNodePtr->Slot)		// INSERT IN MIDDLE HERE
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
		GAME_ASSERT(theNode->NumMeshes <= MAX_DECOMPOSED_TRIMESHES);

		theNode->MeshList[nodeMeshIndex] = meshList[i];
		theNode->OwnsMeshMemory[nodeMeshIndex] = false;
		theNode->OwnsMeshTexture[nodeMeshIndex] = false;
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

			/* FLUSH THE DELETE QUEUE */
	
	gObjectDeleteQueueFlipFlop = !gObjectDeleteQueueFlipFlop;
	FlushObjectDeleteQueue(gObjectDeleteQueueFlipFlop);
}




/**************************** DRAW OBJECTS ***************************/

void DrawObjects(QD3DSetupOutputType *setupInfo)
{
ObjNode		*theNode;

	(void) setupInfo;

	if (gFirstNodePtr == nil)							// see if there are any objects
		return;

				/* FIRST DO OUR CULLING */
				
	CheckAllObjectsInConeOfVision();
	
	theNode = gFirstNodePtr;


			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/			
	do
	{
		uint32_t statusBits = theNode->StatusBits;				// get obj's status bits

		if (statusBits & STATUS_BIT_ISCULLED)					// see if is culled
			goto next;

		if (statusBits & STATUS_BIT_HIDDEN)						// see if is hidden
			goto next;		

		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;		

		theNode->RenderModifiers.statusBits = statusBits;

		switch (theNode->Genre)
		{
			case	SKELETON_GENRE:
					GetModelCurrentPosition(theNode->Skeleton);
					UpdateSkinnedGeometry(theNode);
					Render_SubmitMeshList(
							theNode->NumMeshes,
							theNode->MeshList,
							nil,		// Don't mult matrix with BaseTransformMatrix -- skeleton code already does it
							&theNode->RenderModifiers,
							&theNode->Coord);
					break;

			case	DISPLAY_GROUP_GENRE:
					Render_SubmitMeshList(
							theNode->NumMeshes,
							theNode->MeshList,
							&theNode->BaseTransformMatrix,
							&theNode->RenderModifiers,
							&theNode->Coord);
					break;
		}

next:
		theNode = theNode->NextNode;
	}while (theNode != nil);
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
	
	FlushObjectDeleteQueue(0);
	FlushObjectDeleteQueue(1);
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

	if (theNode->CollisionTriangles)				// free collision triangle memory
		DisposeCollisionTriangleMemory(theNode);

		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */

	for (int i = 0; i < theNode->NumMeshes; i++)
	{
		// If the node has ownership of this mesh's OpenGL texture name, delete it
		if (theNode->MeshList[i]->glTextureName && theNode->OwnsMeshTexture[i])
		{
			glDeleteTextures(1, &theNode->MeshList[i]->glTextureName);
			theNode->MeshList[i]->glTextureName = 0;
		}

		// If the node has ownership of this mesh's memory, dispose of it
		if (theNode->OwnsMeshMemory[i])
		{
			Q3TriMeshData_Dispose(theNode->MeshList[i]);
		}

		theNode->MeshList[i] = nil;
		theNode->OwnsMeshMemory[i] = false;
	}
	theNode->NumMeshes = 0;

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


	const int qid = gObjectDeleteQueueFlipFlop;
	if (gNumObjsInDeleteQueue[qid] >= OBJ_DEL_Q_SIZE)
	{
		// We're out of room in the delete queue. Just delete the node immediately,
		// but that might cause the game to become instable (unless we're here from DeleteAllNodes).
		DisposeObjNodeMemory(theNode);
	}
	else
	{
		// Defer actual deletion of this node to the next frame.
		gObjectDeleteQueue[qid][gNumObjsInDeleteQueue[qid]++] = theNode;
	}
}


/***************** DISPOSE OBJECT MEMORY ****************/

static void DisposeObjNodeMemory(ObjNode* node)
{
	GAME_ASSERT(node != NULL);

	ptrdiff_t poolIndex = node - gObjNodeMemory;

	if (poolIndex >= 0 && poolIndex < OBJ_BUDGET)
	{
		// node is pooled, put back into pool
		GAME_ASSERT_MESSAGE(Pool_IsUsed(gObjNodePool, (int) poolIndex), "double-free on pooled node!");
		Pool_ReleaseIndex(gObjNodePool, (int) poolIndex);
	}
	else
	{
		// node was allocated on heap
		DisposePtr((Ptr) node);
	}
}


/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(int qid)
{
	for (int i = 0; i < gNumObjsInDeleteQueue[qid]; i++)
	{
		if (gObjectDeleteQueue[qid][i])
		{
			DisposeObjNodeMemory(gObjectDeleteQueue[qid][i]);
			gObjectDeleteQueue[qid][i] = NULL;
		}
	}

	gNumObjsInDeleteQueue[qid] = 0;
}




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
	if (theNode->NumCollisionBoxes != 0)
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
		Q3Matrix4x4_SetRotate_XYZ(&matrix, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);		// init rotation matrix
		Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
	}
	
					/********************/
					/* NOW TRANSLATE IT */
					/********************/

	Q3Matrix4x4_SetTranslate(&matrix, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);	// make translate matrix
	Q3Matrix4x4_Multiply(&theNode->BaseTransformMatrix,&matrix, &theNode->BaseTransformMatrix);
}



/********************* MAKE OBJECT TRANSPARENT ***********************/
//
// Puts a transparency attribute object in the base group.
//
// INPUT: transPecent = 0..1.0   0 = totally trans, 1.0 = totally opaque

void MakeObjectTransparent(ObjNode *theNode, float transPercent)
{
	theNode->RenderModifiers.diffuseColor.a = transPercent;
}
