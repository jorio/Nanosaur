/****************************/
/*   	PICKUPS.C		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "terrain.h"
#include "pickups.h"
#include "mobjtypes.h"
#include "qd3d_geometry.h"
#include "collision.h"
#include "skeletonjoints.h"
#include "infobar.h"
#include "sound2.h"
#include "timeportal.h"
#include "skeletonanim.h"
#include "myguy.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D		gCoord,gMyCoord;
extern	TQ3Vector3D		gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	CollisionRec	gCollisionList[];
extern	TQ3Vector3D		gRecentTerrainNormal;
extern	ObjNode			*gMyTimePortal,*gPlayerObj;
extern	short			gRecoveredEggs[];
extern	unsigned long 	gInfobarUpdateBits;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DefaultMovePickupObject(ObjNode *theNode);
static void MoveEgg(ObjNode *theNode);
static void MoveEggInPortal(ObjNode *theNode);
static void MakeNest(long  x, long z);
static Boolean SeeIfAllEggSpeciesRecovered(void);
static void MoveNest(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/

#define WhoHasPickUp	SpecialRef[0]	// objnode of object who has this pickup (nil == none)
#define HoldingLimb		Special[1]		// which limb obj above object is doing holding
#define	OldCType		Special[2]		// keeps old collision info when an obj is being held
#define	OldCBits		Special[3]	

#define	EggIsInPortal	Flag[0]			// when egg is inside time portal and being transported


/************************* ADD EGG *********************************/
//
// parm0 = species of egg
// parm3: bit0 = has nest
//

Boolean AddEgg(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	if (itemPtr->parm[0] >= NUM_EGG_SPECIES)				// make sure egg type is legal
		return(true);

			/****************/
			/* MAKE THE EGG */
			/****************/

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Egg;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Egg1 + itemPtr->parm[0];	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z)-5.0f;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = PLAYER_SLOT+10;
	gNewObjectDefinition.moveCall = MoveEgg;
	gNewObjectDefinition.rot = RandomFloat()*PI2;
	gNewObjectDefinition.scale = .6;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list


			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC|CTYPE_PICKUP;
	newObj->CBits = CBITS_TOUCHABLE;
	
	SetObjectCollisionBounds(newObj,10,-7,-15,15,15,-15);
	
	newObj->PickUpCollisionRadius = newObj->Radius * 4.0f;	// set pickup radius
	newObj->WhoHasPickUp = nil;					// noone is holding this yet
	newObj->EggIsInPortal = false;				// egg isn't in portal
	newObj->Kind = itemPtr->parm[0];			// remember species of egg
	


			/* MAKE NEST */
			
	if (itemPtr->parm[3] & 1)
		MakeNest(x,z);
			

	return(true);								// item was added
}


/************************ MOVE EGG **************************/

static void MoveEgg(ObjNode *theNode)
{
short	species;

	DefaultMovePickupObject(theNode);			// handle it as a pickup


		/* SEE IF IT'S INSIDE THE ACTIVE TIME PORTAL */
		
	
	if (DoSimplePointCollision(&gCoord,CTYPE_PORTAL))
	{		
		ObjNode	*whoHasMe;
		
			/* EGG HAS BEEN RECOVERED */

		whoHasMe = (ObjNode *)theNode->WhoHasPickUp;			// who has this?
		DropItem(whoHasMe);							// if I'm carrying it, get rid of it
		species = theNode->Kind;					// get species type
		gRecoveredEggs[species]++;					// inc counter
		if (gRecoveredEggs[species] == 1)			// see if 1st of this species
		{
			gInfobarUpdateBits |= UPDATE_EGGS;		// update infobar
			AddToScore(EGG_POINTS);					// get lots of points
		}
		else
			AddToScore(EGG_POINTS2);				// get smaller points
		

		theNode->TerrainItemPtr = nil;				// aint never comin back
		
		theNode->EggIsInPortal = true;				// it's being transported
		theNode->MoveCall = MoveEggInPortal;		// change move routine
		theNode->Health = 1.0;
		theNode->Delta.y = 0;
		theNode->CType = 0;
		
		if (SeeIfAllEggSpeciesRecovered())			// get extra points for getting all eggs
			AddToScore(EGG_POINTS3);				// get winning points
		
		PlayEffect(EFFECT_PORTAL);
		
		UpdateObjectTransforms(theNode);		
	}
}


/******************* MOVE EGG IN PORTAL *********************/
//
// moves egg while traveling in portal
//

static void MoveEggInPortal(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Delta.y += 220.0f * fps;						// climb up
	theNode->Coord.y += theNode->Delta.y * fps;
	
	theNode->Health -= fps * 0.4f;						// decay it
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}		

			/* KEEP SPINNING */
			
	theNode->Rot.x += theNode->RotDelta.x*fps;				
	theNode->Rot.y += theNode->RotDelta.y*fps;				
	theNode->Rot.z += theNode->RotDelta.z*fps;				


			/* UPDATE IT */
			
	MakeObjectTransparent(theNode,theNode->Health);	
	UpdateObjectTransforms(theNode);

}


/************************* MAKE NEST *********************************/

static void MakeNest(long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Nest;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Nest;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.moveCall = MoveNest;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;


			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	CreateCollisionTrianglesForObject(newObj);		// build triangle list
}


/***************** MOVE NEST **********************/

static void MoveNest(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
		
	theNode->Coord.y = GetTerrainHeightAtCoord_Planar(theNode->Coord.x, theNode->Coord.z);
	UpdateObjectTransforms(theNode);
}


//================================================================================================


/******************* DEFAULT MOVE PICKUP OBJECT ***********************/

static void DefaultMovePickupObject(ObjNode *theNode)
{
ObjNode	*holderObj;
Byte	limbNum;
TQ3Matrix4x4	matrix,matrix2;
float	iScale;

			/***********************/
			/* SEE IF IS PICKED UP */
			/***********************/
			
	if (theNode->WhoHasPickUp)
	{
		holderObj = (ObjNode *)theNode->WhoHasPickUp;						// get owner of pickup
		limbNum = theNode->HoldingLimb;										// which limb of owner is holding this?
		
		
		if (holderObj->CType == INVALID_NODE_FLAG)							// make sure holder is legal
			return;
		
		
			/* BUILD TRANSFORM MATRIX FOR PICKED UP OBJECT */
			//
			// Since the limb's matrix may contain scaling info, we need
			// to cancel that out since we don't want our picked up
			// object to be scaled.
			//
				
		iScale = (1.0f/holderObj->Scale.x);									// calc amount to reverse scaling by
		
					/* SET SCALE-TRANS MATRIX MANUALLY (FASTER) */
					
		matrix.value[0][0] = iScale*theNode->Scale.x;	// scale x
		matrix.value[1][1] = iScale*theNode->Scale.y;	// scale y
		matrix.value[2][2] = iScale*theNode->Scale.z;	// scale z
		matrix.value[3][3] = 1;
		matrix.value[0][1] = 0;
		matrix.value[0][2] = 0;
		matrix.value[0][3] = 0;
		matrix.value[1][0] = 0;
		matrix.value[1][2] = 0;
		matrix.value[1][3] = 0;
		matrix.value[2][0] = 0;
		matrix.value[2][1] = 0;
		matrix.value[2][3] = 0;
		matrix.value[3][0] = 0;							// trans x
		matrix.value[3][1] = 0;							// trans y
		matrix.value[3][2] = -55;						// trans z
				
		FindJointFullMatrix(holderObj,limbNum,&matrix2);						// get full matrix for mouth
		Q3Matrix4x4_Multiply(&matrix,&matrix2,&theNode->BaseTransformMatrix);	// concat final matrix

		SetObjectTransformMatrix(theNode);										// update object with transform	
	}
	
			/*****************/
			/* NOT PICKED UP */
			/*****************/
			
	else
	{
		float	y,f;
		
		if (TrackTerrainItem(theNode))								// check to see if it's gone
		{
			DeleteObject(theNode);
			return;
		}
						
		GetObjectInfo(theNode);
		
					/* DO GRAVITY & FRICTION */
					
					
		gDelta.y += -(GRAVITY_CONSTANT/2)*gFramesPerSecondFrac;		// add gravity

		f = 200.0f * gFramesPerSecondFrac;								// calc friction value
		
		ApplyFrictionToDeltas(f,&gDelta);
		

					/* MOVE IT */
					
		gCoord.y += gDelta.y*gFramesPerSecondFrac;					// move it
		gCoord.x += gDelta.x*gFramesPerSecondFrac;
		gCoord.z += gDelta.z*gFramesPerSecondFrac;
		
		y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);		// get y here
		if ((gCoord.y+theNode->BottomOff) < y)						// see if bottom below/on ground
		{
			gCoord.y = y-theNode->BottomOff;
			gDelta.y = -gDelta.y*.5f;								// bounce it
			if (fabs(gDelta.y) < 1.0f)								// when gets small enough, just make zero
				gDelta.y = 0;
			gDelta.x *= .9f;										// strong friction on landing
			gDelta.z *= .9f;
			
			theNode->RotDelta.x *= .8f;
			theNode->RotDelta.y *= .8f;
			theNode->RotDelta.z *= .8f;
		}
		
			/* DEAL WITH SLOPES */
			
		if (gRecentTerrainNormal.y < .95f)							// if fairly flat, then no sliding effect
		{	
			gDelta.x += gRecentTerrainNormal.x * gFramesPerSecondFrac * 900.0f;
			gDelta.z += gRecentTerrainNormal.z * gFramesPerSecondFrac * 900.0f;
		}		
		
		
				/* SPIN IT */

		theNode->Rot.x += theNode->RotDelta.x*gFramesPerSecondFrac;				
		theNode->Rot.y += theNode->RotDelta.y*gFramesPerSecondFrac;				
		theNode->Rot.z += theNode->RotDelta.z*gFramesPerSecondFrac;				
				
		UpdateObject(theNode);
	}
}



/*************************** TRY TO DO PICKUP ********************************/
//
// Attempts to find objects which can be affected by the pickup animation.
//
// INPUT:	theNode		:	who is trying to do pickup
//			limbNum		:	limb # of theNode which is capable of pickup
//

void TryToDoPickUp(ObjNode *theNode, Byte limbNum)
{
static TQ3Point3D	inPoint = {0,0,-30};
TQ3Point3D			testPt;
ObjNode				*pickedObj;


			/* FIND COORDINATE OF MOUTH */
										
	FindCoordOnJoint(theNode, limbNum, &inPoint, &testPt);
	
	
			/* DO COLLISION DETECT TO FIND OBJECT */
					
	pickedObj = IsPointInPickupCollisionSphere(&testPt);	// see if this coord is inside a pickup's collision sphere
					
	if (pickedObj)
	{
		theNode->StatusBits |= STATUS_BIT_ISCARRYING;
		theNode->CarriedObj = pickedObj;
		pickedObj->WhoHasPickUp = theNode;
		pickedObj->HoldingLimb = limbNum;
		pickedObj->OldCBits = pickedObj->CBits;				// keep collision settings
		pickedObj->OldCType = pickedObj->CType;	
		pickedObj->CBits = 0;								// clear collision stuff while being carried
		pickedObj->CType = 0;
		pickedObj->StatusBits |= STATUS_BIT_DONTCULL;		// dont do custom culling while being carried
		
	}
}


/*********************** DROP ITEM ******************************/
//
// INPUT:	theNode = who is carrying item
//

void DropItem(ObjNode *theNode)
{
ObjNode	*itemObj;
const static TQ3Point3D	inPoint = {0,0,0};

	if (!theNode)
		return;

	if (!(theNode->StatusBits & STATUS_BIT_ISCARRYING))				// make sure I've got something
		return;

	itemObj = theNode->CarriedObj;									// get item's obj
	itemObj->WhoHasPickUp = nil;									// let item know it's not being carried
	theNode->StatusBits &= ~STATUS_BIT_ISCARRYING;					// clear carrying flag
	theNode->CarriedObj = nil;										// clear carried obj ptr
	itemObj->StatusBits &= ~STATUS_BIT_DONTCULL;					// let cull again
	itemObj->CBits = itemObj->OldCBits;								// restore collision info
	itemObj->CType = itemObj->OldCType;
	itemObj->Delta.y = 0;											// start falling momentum @ 0

	itemObj->Rot.y = theNode->Rot.y;								// match y rotations
	
			/* GET CURRENT COORD OF ITEM */
			
	Q3Point3D_Transform(&inPoint, &itemObj->BaseTransformMatrix, &itemObj->Coord);
}


/********************* SEE IF ALL EGG SPECIES RECOVERED ***********************/

static Boolean SeeIfAllEggSpeciesRecovered(void)
{
int	i,n;
	
			/* COUNT # SPECIES GOTTEN */
			
	n = 0;
	for (i = 0; i < NUM_EGG_SPECIES; i++)
		if (gRecoveredEggs[i] > 0)
			n++;


			/* SEE IF ALL ACCOUNTED FOR */
				
	if (n >= NUM_EGG_SPECIES)
	{
		MakeTimePortal(PORTAL_TYPE_EXIT, gMyCoord.x, gMyCoord.z);
		MorphToSkeletonAnim(gPlayerObj->Skeleton, PLAYER_ANIM_EXIT, 3);
		gPlayerObj->Flag[0] = false;
		gPlayerObj->ExitTimer = 0;
		
		PlayEffect_Parms(EFFECT_PORTAL,FULL_CHANNEL_VOLUME,kMiddleC-8);		
		return(true);
	}
	return(false);
}


/***************** GET ALL EGGS CHEAT *********************/

void GetAllEggsCheat(void)
{
int	i;

	for (i = 0; i < NUM_EGG_SPECIES; i++)
		gRecoveredEggs[i] = 5;
	gInfobarUpdateBits |= UPDATE_EGGS;		// update infobar


	SeeIfAllEggSpeciesRecovered();
}







