/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)1993-1997 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT COLLISION ------

/*******************************  KEEP OLD COLLISION BOXES **************************/
//
// Also keeps old coordinate
//

void KeepOldCollisionBoxes(ObjNode *theNode)
{
	for (int i = 0; i < theNode->NumCollisionBoxes; i++)
	{
		theNode->CollisionBoxes[i].oldTop = theNode->CollisionBoxes[i].top;
		theNode->CollisionBoxes[i].oldBottom = theNode->CollisionBoxes[i].bottom;
		theNode->CollisionBoxes[i].oldLeft = theNode->CollisionBoxes[i].left;
		theNode->CollisionBoxes[i].oldRight = theNode->CollisionBoxes[i].right;
		theNode->CollisionBoxes[i].oldFront = theNode->CollisionBoxes[i].front;
		theNode->CollisionBoxes[i].oldBack = theNode->CollisionBoxes[i].back;
	}

	theNode->OldCoord = theNode->Coord;			// remember coord also
}


/**************** CALC OBJECT BOX FROM NODE ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on theNode's coords.
//

void CalcObjectBoxFromNode(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	GAME_ASSERT(theNode->NumCollisionBoxes == 1);
		
	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= theNode->Coord.x  + theNode->LeftOff;
	boxPtr->right 	= theNode->Coord.x + theNode->RightOff;
	boxPtr->back 	= theNode->Coord.z + theNode->BackOff;
	boxPtr->front 	= theNode->Coord.z + theNode->FrontOff;
	boxPtr->top 	= theNode->Coord.y + theNode->TopOff;
	boxPtr->bottom 	= theNode->Coord.y  + theNode->BottomOff;

}


/**************** CALC OBJECT BOX FROM GLOBAL ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on gCoord
//

void CalcObjectBoxFromGlobal(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	if (theNode == nil)
		return;

	GAME_ASSERT(theNode->NumCollisionBoxes == 1);

	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= gCoord.x  + theNode->LeftOff;
	boxPtr->right 	= gCoord.x  + theNode->RightOff;
	boxPtr->back 	= gCoord.z  + theNode->BackOff;
	boxPtr->front 	= gCoord.z  + theNode->FrontOff;
	boxPtr->top 	= gCoord.y  + theNode->TopOff;
	boxPtr->bottom 	= gCoord.y  + theNode->BottomOff;
}


/******************* SET OBJECT COLLISION BOUNDS **********************/
//
// Sets an object's collision offset/bounds.  Adjust accordingly for input rotation 0..3 (clockwise)
//

void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back)
{
	theNode->NumCollisionBoxes = 1;							// alloc 1 collision box

	theNode->TopOff 		= top;
	theNode->BottomOff 	= bottom;	
	theNode->LeftOff 	= left;
	theNode->RightOff 	= right;
	theNode->FrontOff 	= front;
	theNode->BackOff 	= back;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT SHADOWS ------

/******************* ATTACH SHADOW TO OBJECT ************************/

ObjNode	*AttachShadowToObject(ObjNode *theNode, float scaleX, float scaleZ)
{
ObjNode	*shadowObj;
							
	if (!gGamePrefs.shadows)								// see if can do shadows
		return(nil);
		
	gNewObjectDefinition.group = GLOBAL_MGroupNum_Shadow;	
	gNewObjectDefinition.type = GLOBAL_MObjType_Shadow;	
	gNewObjectDefinition.coord = theNode->Coord;
	gNewObjectDefinition.coord.y += .5;
	gNewObjectDefinition.flags = STATUS_BIT_BLEND_INTERPOLATE | STATUS_BIT_NOZWRITE;
	gNewObjectDefinition.slot = SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = scaleX;
	shadowObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (shadowObj == nil)
		return(nil);

	GAME_ASSERT(gShadowGLTextureName != 0);
	shadowObj->MeshList[0]->diffuseColor = (TQ3ColorRGBA) {0,0,0,1};		// taint shadow black
	shadowObj->MeshList[0]->glTextureName = gShadowGLTextureName;
	shadowObj->MeshList[0]->texturingMode = kQ3TexturingModeAlphaBlend;

	theNode->ShadowNode = shadowObj;

	shadowObj->SpecialF[0] = scaleX;							// need to remeber scales for update
	shadowObj->SpecialF[1] = scaleZ;

	shadowObj->RenderModifiers.sortPriority = +9999;			// shadows must be drawn underneath all other transparent meshes
	
	return(shadowObj);
}




/************************ UPDATE SHADOW *************************/

void UpdateShadow(ObjNode *theNode)
{
ObjNode *thisNodePtr,*shadowNode;
long	x,y,z;
float	dist;
Boolean	updateTrans = false;

	if (theNode == nil)
		return;

	shadowNode = theNode->ShadowNode;
	if (shadowNode == nil)
		return;
		
		
	x = gCoord.x;												// get integer copy for collision checks
	y = gCoord.y;
	z = gCoord.z;

	shadowNode->Coord = gCoord;
	shadowNode->Rot.y = theNode->Rot.y;

		/* SEE IF SHADOW IS ON BLOCKER OBJECT OR ON TERRAIN */
		
	thisNodePtr = gFirstNodePtr;
	do
	{
		if (thisNodePtr->CType & CTYPE_BLOCKSHADOW				// look for things which can block the shadow
			&& thisNodePtr->NumCollisionBoxes != 0)
		{
			if (y < thisNodePtr->CollisionBoxes[0].bottom
				|| x < thisNodePtr->CollisionBoxes[0].left
				|| x > thisNodePtr->CollisionBoxes[0].right
				|| z > thisNodePtr->CollisionBoxes[0].front
				|| z < thisNodePtr->CollisionBoxes[0].back)
			{
				goto next;
			}
			else
			{
					/************************/
					/* SHADOW IS ON OBJECT  */
					/************************/

				shadowNode->Rot.x = shadowNode->Rot.z = 0;
				shadowNode->Coord.y = (float)(thisNodePtr->CollisionBoxes[0].top) + .5;
				updateTrans = true;
				goto update;
			}
		}
next:					
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

		
		
			/************************/
			/* SHADOW IS ON TERRAIN */
			/************************/

	shadowNode->Coord.y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z) + 3;	
	RotateOnTerrain(shadowNode, 25*shadowNode->Scale.x, 25*shadowNode->Scale.x);

update:
			/* CALC SCALE OF SHADOW */
			
	dist = (gCoord.y - shadowNode->Coord.y)/400;					// as we go higher, shadow gets smaller
	if (dist > .5)
		dist = .5;
		
	dist = 1 - dist;
	
	shadowNode->Scale.x = dist * shadowNode->SpecialF[0];
	shadowNode->Scale.z = dist * shadowNode->SpecialF[1];
	
	if (updateTrans)
		UpdateObjectTransforms(shadowNode);								// update transforms
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CULLING ------


/**************** CHECK ALL OBJECTS IN CONE OF VISION *******************/
//
// Checks every ObjNode to see if the object is in the code of vision
//

void CheckAllObjectsInConeOfVision(void)
{
ObjNode* theNode;

	theNode = gFirstNodePtr;														// get & verify 1st node
	if (theNode == nil)
		return;

					/* PROCESS EACH OBJECT */
					
	do
	{	
		if (theNode->StatusBits & STATUS_BIT_ALWAYSCULL)
			goto try_cull;

		if (0 == theNode->NumMeshes)												// quick check if any geometry at all
			if (theNode->Genre != SKELETON_GENRE)
				goto next;

		if (theNode->StatusBits & STATUS_BIT_DONTCULL)								// see if dont want to use our culling
			goto draw_on;

try_cull:
		if (!IsSphereInFrustum_XZ(&theNode->Coord, theNode->Radius))
			goto draw_off;
				
draw_on:
		theNode->StatusBits &= ~STATUS_BIT_ISCULLED;							// clear cull bit
		goto next;

draw_off:
		theNode->StatusBits |= STATUS_BIT_ISCULLED;								// set cull bit
	
	
				/* NEXT NODE */
next:			
		theNode = theNode->NextNode;		// next node
	}
	while (theNode != nil);	
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- MISC OBJECT FUNCTIONS ------


/************************ START OBJECT STREAM EFFECT *****************************/

void StartObjectStreamEffect(ObjNode *theNode, short effectNum)
{
	theNode->StreamingEffect = PlayEffect(effectNum);
}


/************************ STOP OBJECT STREAM EFFECT *****************************/

void StopObjectStreamEffect(ObjNode *theNode)
{
	if (theNode->StreamingEffect != -1)
	{
		StopAChannel(&theNode->StreamingEffect);
	}
}


/******************** CALC NEW TARGET OFFSETS **********************/

void CalcNewTargetOffsets(ObjNode *theNode, float scale)
{
	theNode->TargetOff.x = (RandomFloat()-0.5f) * scale;
	theNode->TargetOff.y = (RandomFloat()-0.5f) * scale;
}
