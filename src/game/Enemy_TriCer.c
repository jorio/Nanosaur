/****************************/
/*   ENEMY: TRICER.C			*/
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
#include "enemy.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "3dmath.h"
#include "myguy.h"
#include "items.h"
#include "sound2.h"
#include "mobjtypes.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gMostRecentlyAddedNode;
 
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumItems,gNumEnemies;
extern	Byte					gNumPlayers;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveTricer(ObjNode *theNode);
static void MoveTricer_Walking(ObjNode *theNode);
static void MoveTricer_Standing(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#ifdef PRO_MODE
#define MAX_TRICER			10
#else
#define MAX_TRICER			3
#endif

#define	TRICER_ATTACK_RANGE	600
#define	TRICER_ATTACK_RANGE_IN_BUSH	550
#define TRICER_TURN_SPEED	1.5
#define	MAX_WALK_SPEED		240

#define	TRICER_TARGET_SCALE	40



#define	TRICER_HEALTH		4.0		
#define	TRICER_DAMAGE		.1

#define	TRICER_SCALE		2.2

enum
{
	TRICER_ANIM_WALK,
	TRICER_ANIM_STAND
};

#define	FOOT_OFFSET			0


/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gLastTricer;

#define	TargetChangeTimer	SpecialF[0]			// timer for target offset recalc

#define InBush				Flag[3]
#define TheBush				Special[3]

/******************** MAKE TRICER ENEMY ************************/
//
// Puts inside bush
//

ObjNode *MakeTriceratops(ObjNode *theBush, long x, long z)
{
	if (AddEnemy_Tricer(nil, x, z))
	{
		gLastTricer->InBush = true;
		gLastTricer->TheBush = (long)theBush;
		return(gLastTricer);
	}
	return(nil);
}


/************************ ADD TRICER ENEMY *************************/
//
// Called from above with itemPtr = nil, so be careful with itemPtr
//

Boolean AddEnemy_Tricer(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)					// keep this from getting absurd
		return(false);

	if (itemPtr)									// (itemptr == nil if in bush)
	{
		if (!(itemPtr->parm[3] & 1))				// see if always add 
		{
			if (gNumEnemyOfKind[ENEMY_KIND_TRICER] >= MAX_TRICER)
				return(false);		
		}
	}


				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_TRICER,x,z);
	if (newObj == nil)
		return(false);

	gLastTricer = newObj;

	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, TRICER_ANIM_STAND);
	
	newObj->Coord.y -= FOOT_OFFSET;							// adjust y
	
				/* SET BETTER INFO */
						
	newObj->MoveCall = MoveTricer;							// set move call

	newObj->Health = TRICER_HEALTH;
	newObj->Damage = TRICER_DAMAGE;
	newObj->Kind = ENEMY_KIND_TRICER;

	newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = TRICER_SCALE;	// set scale
	newObj->Radius *= TRICER_SCALE;
	
	newObj->Rot.y = RandomFloat()*PI2;						// random rotation
		
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 90,FOOT_OFFSET,-120,120,120,-120);
	CalcNewTargetOffsets(newObj,TRICER_TARGET_SCALE);
	newObj->TargetChangeTimer = 0;


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 2.7, 2.7*1.5);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TRICER]++;
	return(true);
}



/********************* MOVE TRICER **************************/

static void MoveTricer(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveTricer_Walking,
					MoveTricer_Standing
				};


	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}

/********************** MOVE TRICER: STANDING ******************************/

static void  MoveTricer_Standing(ObjNode *theNode)
{
float	dist;
ObjNode	*bushObj;

	if (theNode->InBush)
		dist = TRICER_ATTACK_RANGE_IN_BUSH;
	else
		dist = TRICER_ATTACK_RANGE;


	if (CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z) < dist)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TRICER_ANIM_WALK,5);
		
		if (theNode->InBush)
		{
			theNode->InBush = false;
			bushObj = (ObjNode *)theNode->TheBush;
			if (bushObj->Type == LEVEL0_MObjType_Bush)							// make sure it's still a bush
				ExplodeBush(bushObj);
			PlayEffect_Parms(EFFECT_ROAR,FULL_CHANNEL_VOLUME,kMiddleC+11);
		}
	}

			/* IF INSIDE BUSH, KEEP AIMED */
			
	if (theNode->InBush)
	{
		TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, TRICER_TURN_SPEED*10, false);			
	}

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateEnemy(theNode);		
	
}


/********************** MOVE TRICER: WALKING ******************************/

static void  MoveTricer_Walking(ObjNode *theNode)
{
float	r;

	theNode->Skeleton->AnimSpeed = MAX_WALK_SPEED / 63.3;			// tweak speed to sync

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, TRICER_TURN_SPEED, true);			

	r = theNode->Rot.y;
	theNode->Speed = MAX_WALK_SPEED;	
	gDelta.x = -sin(r) * MAX_WALK_SPEED;
	gDelta.z = -cos(r) * MAX_WALK_SPEED;
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode, theNode->BottomOff);

	if (CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z) > (TRICER_ATTACK_RANGE*4))
		MorphToSkeletonAnim(theNode->Skeleton, TRICER_ANIM_STAND,3);

	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
	UpdateEnemy(theNode);		
}









