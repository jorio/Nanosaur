/****************************/
/*      LEMMINGS.C			*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DMath.h>


#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "lemmings.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "3dmath.h"
#include "myguy.h"
#include "terrain.h"
#include "collision.h"

extern	NewObjectDefinitionType	gNewObjectDefinition; 
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumItems;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveLemming(ObjNode *theNode);
static void MoveLemming_Walking(ObjNode *theNode);
static void MoveLemming_Standing(ObjNode *theNode);
static float CalcLemmingWalkSpeed(ObjNode *theNode, float dist);
static Boolean DoLemmingMove(ObjNode *theNode);
static void DeleteLemming(ObjNode *theNode);
static long ReserveGridSlot(ObjNode *theNode);
static float TurnLemmingTowardGrid(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ACTIVE_LEMMINGS		6
#define MAX_CAUGHT_LEMMINGS		6

#define LEMMING_TURN_SPEED		1.9
#define LEMMING_WALK_SPEED		60
#define	LEMMING_TARGET_SCALE	250
#define	MAX_WALK_SPEED			200
#define	LEMMING_HEALTH		1.0		
#define	LEMMING_SCALE		.25	

enum
{
	LEMMING_ANIM_STAND,
	LEMMING_ANIM_WALK,
	LEMMING_ANIM_STUNNED,
	LEMMING_ANIM_DEATH
};

#define	FOOT_OFFSET			(40*LEMMING_SCALE)

#define	LEMMING_COLLISION_BITS	(CTYPE_MISC|CTYPE_ENEMY|CTYPE_BGROUND)


/*********************/
/*    VARIABLES      */
/*********************/

#define	TargetChangeTimer	SpecialF[0]			// timer for target offset recalc
#define	GridPosition		Special[0]			// position in grid which lemming is attracted to (-1 == none)


short	gNumLemmingsActive;
float	gLemmingSwapTimer;						// timer for randomly swapping slots of lemmings on grid

static const TQ3Point2D	gLemmingGrid[MAX_CAUGHT_LEMMINGS] = 
{
			0,60,
	-40,120,	40,120,
-80,180,	0,180,	80,180
};

static TQ3Point2D	gTransformedLemmingGrid[MAX_CAUGHT_LEMMINGS];

static ObjNode	*gGridOccupants[MAX_CAUGHT_LEMMINGS];


/******************* INIT LEMMINGS ***************************/

void InitLemmings(void)
{
short	i;

	gNumLemmingsActive = 0;
	gLemmingSwapTimer = 0;
	
	for (i=0;i < MAX_CAUGHT_LEMMINGS; i++)				// make grid empty
		gGridOccupants[i] = nil;
}


/******************** UPDATE LEMMING GRID ***********************/
//
// Calculates the world coordinates of each grid position.
// Call from player's move function
//

void UpdateLemmingGrid(ObjNode *playerNode)
{
float			r;
TQ3Matrix3x3	m;
short			i,i2;
static TQ3Point2D origin = {0,0};

			/* UPDATE COORDS OF SLOTS */
			
	r = -playerNode->Rot.y;										// get rotation

	Q3Matrix3x3_SetRotateAboutPoint(&m, &origin, r);			// set rotation matrix
	
	for (i = 0; i < MAX_CAUGHT_LEMMINGS; i++)
	{
		Q3Point2D_Transform(&gLemmingGrid[i], &m, &gTransformedLemmingGrid[i]);	// transform each grid position
		gTransformedLemmingGrid[i].x += playerNode->Coord.x;
		gTransformedLemmingGrid[i].y += playerNode->Coord.z;
	}
	
	
			/* SEE IF RANDOMLY SWAP 2 LEMMINGS */
			
	gLemmingSwapTimer += gFramesPerSecondFrac;
	if (gLemmingSwapTimer > 1.5)
	{
		gLemmingSwapTimer = 0;											// reset timer
		
		i = MyRandomLong()&0x7;											// get random index to 1st lemming
		if (i >= MAX_CAUGHT_LEMMINGS)
			i = 0;
			
		i2 = MyRandomLong()&0x7;										// get random index to 2nd lemming
		if (i2 >= MAX_CAUGHT_LEMMINGS)
			i2 = 0;
			
		if (i != i2)
		{
			if (gGridOccupants[i] && gGridOccupants[i2])
			{
				ObjNode	*temp = gGridOccupants[i];						// swap 'em
				gGridOccupants[i] = gGridOccupants[i2];
				gGridOccupants[i2] = temp;
				
				gGridOccupants[i]->GridPosition = i;
				gGridOccupants[i2]->GridPosition = i2;
			}
		}
	}
	
}

/************************ RESERVE GRID SLOT ****************************/
//
// OUTPUT: -1 = no free slots\
//

static long ReserveGridSlot(ObjNode *theNode)
{
short	i;

	for (i=0;i < MAX_CAUGHT_LEMMINGS; i++)				// find empty slot
		if (gGridOccupants[i] == nil)
		{
			gGridOccupants[i] = theNode;
			return(i);
		}

	return(-1);											// none found
}



/************************ ADD LEMMING *************************/
//
// A skeleton character
//

Boolean AddLemming(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj,*shadowObj;

	if (gNumLemmingsActive >= MAX_ACTIVE_LEMMINGS)
		return(false);
		

			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 	= SKELETON_TYPE_NANO;
	gNewObjectDefinition.animNum = LEMMING_ANIM_STAND;
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Quick(x,z) + FOOT_OFFSET;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = PLAYER_SLOT-10;
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = LEMMING_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);
	
	newObj->TerrainItemPtr = itemPtr;
							
	newObj->MoveCall = MoveLemming;							// set move call

	newObj->Health = LEMMING_HEALTH;

	
				/* SET COLLISION INFO */

	newObj->CType = 0;
	newObj->CBits = 0;
	SetObjectCollisionBounds(newObj, 40,-FOOT_OFFSET,-10,10,10,-10,0);


	CalcNewTargetOffsets(newObj,LEMMING_TARGET_SCALE);
	newObj->TargetChangeTimer = 0;

	newObj->GridPosition = ReserveGridSlot(newObj);		// which grid position do I want to go towards


				/* MAKE SHADOW */
				
	shadowObj = AttachShadowToObject(newObj, .2, .2*2.5);

	gNumLemmingsActive++;
	
	
	return(true);
}



/********************* MOVE LEMMING **************************/

static void MoveLemming(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveLemming_Standing,
					MoveLemming_Walking
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteLemming(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE LEMMING: STANDING ******************************/

static void  MoveLemming_Standing(ObjNode *theNode)
{
float	dist;

	dist= TurnLemmingTowardGrid(theNode);

	if (CalcLemmingWalkSpeed(theNode,dist) > 0)
		SetSkeletonAnim(theNode->Skeleton, LEMMING_ANIM_WALK);

			/* HANDLE COLLISIONS */
			
	HandleCollisions(theNode, LEMMING_COLLISION_BITS);

	UpdateObject(theNode);		
}


/********************** MOVE LEMMING: WALKING ******************************/

static void  MoveLemming_Walking(ObjNode *theNode)
{
float	r,speed,dist;


			/* MOVE TOWARD PLAYER */
			
	dist = TurnLemmingTowardGrid(theNode);			

		
		/* MOVE FASTER WHEN FARTHER AWAY FROM ME */

	speed = CalcLemmingWalkSpeed(theNode,dist);

	r = theNode->Rot.y;
	theNode->Speed = speed;	
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;				// add gravity

	DoLemmingMove(theNode);

		/* UPDATE ANIM SPEED */

	theNode->Skeleton->AnimSpeed = speed * .015;

	if (speed == 0)													// if stopped, then stand
		MorphToSkeletonAnim(theNode->Skeleton, LEMMING_ANIM_STAND, 4);


		/* SEE IF CHANGE TARGET */

	if ((theNode->TargetChangeTimer += gFramesPerSecondFrac) > 7)	// every n seconds
	{
		CalcNewTargetOffsets(theNode,LEMMING_TARGET_SCALE);
		theNode->TargetChangeTimer	= 0;
	}

			/* HANDLE COLLISIONS */
			
	HandleCollisions(theNode, LEMMING_COLLISION_BITS);
	


	UpdateObject(theNode);		
}


/******************* CALC LEMMING WALK SPEED ***********************/
//
// INPUT: dist = dist to target
//

static float CalcLemmingWalkSpeed(ObjNode *theNode, float dist)
{
float	speed;

	speed = dist * .85;
	if (speed > MAX_WALK_SPEED)
		speed = MAX_WALK_SPEED;
	else
	if (speed < 6)
		speed = 0;
	else
		speed += 4;
		
	return(speed);
}


/********************* DO LEMMING MOVE **********************/
//
// OUTPUT: true = one ground
//

static Boolean DoLemmingMove(ObjNode *theNode)
{
float	y;

	gCoord.y += gDelta.y*gFramesPerSecondFrac;					// move
	gCoord.x += gDelta.x*gFramesPerSecondFrac;
	gCoord.z += gDelta.z*gFramesPerSecondFrac;


	y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);		// get center Y
	if ((gCoord.y + theNode->BottomOff) < y)
	{
		gCoord.y = y - theNode->BottomOff;
		gDelta.y = 0;
		return(true);
	}
	return(false);
}


/******************** DELETE LEMMING *************************/

static void DeleteLemming(ObjNode *theNode)
{
	gNumLemmingsActive--;

	if (theNode->GridPosition != -1)									// free grid location
		gGridOccupants[theNode->GridPosition] = nil;	
	
	DeleteObject(theNode);
}


/********************* TURN LEMMING TOWARD GRID ********************/
//
// OUTPUT: distance to final position
//

static float TurnLemmingTowardGrid(ObjNode *theNode)
{
float	x,z;
long	i;

			/* SEE IF NO GRID SLOT OPEN */
			
	if (theNode->GridPosition == -1)
	{
		x = gMyCoord.x;
		z = gMyCoord.z;
		TurnObjectTowardTarget(theNode, x, z, LEMMING_TURN_SPEED,true);			
		x += theNode->TargetOff.x;
		z += theNode->TargetOff.y;
	}
	else
	{
		i = theNode->GridPosition;
		x = gTransformedLemmingGrid[i].x;
		z = gTransformedLemmingGrid[i].y;
		
		TurnObjectTowardTarget(theNode, x, z, LEMMING_TURN_SPEED,false);			
	}

	return(CalcQuickDistance(gCoord.x, gCoord.z, x, z));
}







