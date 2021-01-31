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
#include "frustumculling.h"

extern	TQ3Object	gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	float		gObjectGroupRadiusList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	short		gNumObjectsInGroupList[MAX_3DMF_GROUPS];
extern	float		gFramesPerSecondFrac;
extern	TQ3ShaderObject	gQD3D_gShadowTexture;
extern	ObjNode		*gPlayerObj,*gFirstNodePtr;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	TQ3Point3D	gCoord;
extern	TQ3Object	gKeepBackfaceStyleObject;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	PrefsType	gGamePrefs;

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

/**************** ALLOCATE COLLISION BOX MEMORY *******************/

void AllocateCollisionBoxMemory(ObjNode *theNode, short numBoxes)
{
Ptr	mem;

			/* FREE OLD STUFF */
			
	if (theNode->CollisionBoxes)
		DisposePtr((Ptr)theNode->CollisionBoxes);		

				/* SET # */
				
	theNode->NumCollisionBoxes = numBoxes;
	if (numBoxes == 0)
		DoFatalAlert("AllocateCollisionBoxMemory with 0 boxes?");


				/* CURRENT LIST */
				
	mem = AllocPtr(sizeof(CollisionBoxType)*numBoxes);
	if (mem == nil)
		DoFatalAlert("Couldnt alloc collision box memory");
	theNode->CollisionBoxes = (CollisionBoxType *)mem;
}


/*******************************  KEEP OLD COLLISION BOXES **************************/
//
// Also keeps old coordinate
//

void KeepOldCollisionBoxes(ObjNode *theNode)
{
signed char	i;
			
	i = theNode->NumCollisionBoxes;					// get # boxes for this obj
	if (i > 0)
	{
		i -= 1;										// convert counter into index
	
		do
		{
			theNode->CollisionBoxes[i].oldTop = theNode->CollisionBoxes[i].top;
			theNode->CollisionBoxes[i].oldBottom = theNode->CollisionBoxes[i].bottom;
			theNode->CollisionBoxes[i].oldLeft = theNode->CollisionBoxes[i].left;
			theNode->CollisionBoxes[i].oldRight = theNode->CollisionBoxes[i].right;
			theNode->CollisionBoxes[i].oldFront = theNode->CollisionBoxes[i].front;
			theNode->CollisionBoxes[i].oldBack = theNode->CollisionBoxes[i].back;
		}while(--i >= 0);
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

	if (theNode->CollisionBoxes == nil)
		DoFatalAlert("CalcObjectBox on objnode with no CollisionBoxType");
		
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

	if (theNode->CollisionBoxes == nil)
		DoFatalAlert("CalcObjectBoxFromGlobal: CalcObjectBox on objnode with no CollisionBoxType");
		
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

	AllocateCollisionBoxMemory(theNode, 1);					// alloc 1 collision box
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
#if 1	// TODO noquesa
{ DoFatalAlert2("TODO noquesa", __func__); return nil; }
#else
{
ObjNode	*shadowObj;
							
	if (!gGamePrefs.shadows)								// see if can do shadows
		return(nil);
		
	gNewObjectDefinition.group = GLOBAL_MGroupNum_Shadow;	
	gNewObjectDefinition.type = GLOBAL_MObjType_Shadow;	
	gNewObjectDefinition.coord = theNode->Coord;
	gNewObjectDefinition.coord.y += .5;
	gNewObjectDefinition.flags = STATUS_BIT_BLEND_INTERPOLATE;
	gNewObjectDefinition.slot = SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = scaleX;
	shadowObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (shadowObj == nil)
		return(nil);

	QD3D_ReplaceGeometryTexture(shadowObj->BaseGroup,gQD3D_gShadowTexture);	// swap map on icon also

	theNode->ShadowNode = shadowObj;

	shadowObj->SpecialF[0] = scaleX;							// need to remeber scales for update
	shadowObj->SpecialF[1] = scaleZ;

	return(shadowObj);
}
#endif




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
		if (thisNodePtr->CType & CTYPE_BLOCKSHADOW)				// look for things which can block the shadow
		{
			if (thisNodePtr->CollisionBoxes)
			{
				if (y < (*thisNodePtr->CollisionBoxes).bottom)
					goto next;
				if (x < (*thisNodePtr->CollisionBoxes).left)
					goto next;
				if (x > (*thisNodePtr->CollisionBoxes).right)
					goto next;
				if (z > (*thisNodePtr->CollisionBoxes).front)
					goto next;
				if (z < (*thisNodePtr->CollisionBoxes).back)
					goto next;

					/************************/
					/* SHADOW IS ON OBJECT  */
					/************************/

				shadowNode->Rot.x = shadowNode->Rot.z = 0;
				shadowNode->Coord.y = (float)((*thisNodePtr->CollisionBoxes).top) + .5;
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
			
		if (theNode->BaseGroup == nil)												// quick check if any geometry at all
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
