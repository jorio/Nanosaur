/****************************/
/*   ENEMY: STEGO.C			*/
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

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gPlayerNode[],*gCurrentNode,*gPlayerNode[];
 
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumItems,gNumEnemies;
extern	Byte					gNumPlayers;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveStego(ObjNode *theNode);
static void MoveStego_Walking(ObjNode *theNode);
static void MoveStego_Standing(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#ifdef PRO_MODE
#define MAX_STEGO			10
#else
#define MAX_STEGO			2
#endif

#define	STEGO_ATTACK_RANGE	700
#define STEGO_TURN_SPEED	.45
#define	MAX_WALK_SPEED		80

#define	STEGO_TARGET_SCALE	350



#define	STEGO_HEALTH		5.0		
#define	STEGO_DAMAGE		.2

#define	STEGO_SCALE			1.4

enum
{
	STEGO_ANIM_NIL,
	STEGO_ANIM_STAND,
	STEGO_ANIM_WALK
};

#define	FOOT_OFFSET			(-72.0*STEGO_SCALE)


/*********************/
/*    VARIABLES      */
/*********************/

#define	TargetChangeTimer	SpecialF[0]			// timer for target offset recalc


/************************ ADD STEGO ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Stego(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)				// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))				// see if always add 
	{
		if (gNumEnemyOfKind[ENEMY_KIND_STEGO] >= MAX_STEGO)
			return(false);
	}

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_STEGO,x,z);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, STEGO_ANIM_STAND);
	
	newObj->Coord.y -= FOOT_OFFSET;							// adjust y
	
				/* SET BETTER INFO */
						
	newObj->MoveCall = MoveStego;							// set move call

	newObj->Health = STEGO_HEALTH;
	newObj->Damage = STEGO_DAMAGE;
	newObj->Kind = ENEMY_KIND_STEGO;

	newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = STEGO_SCALE;	// set scale
	newObj->Radius *= STEGO_SCALE;
	
	newObj->Rot.y = RandomFloat()*PI2;						// random rotation
	
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 90,FOOT_OFFSET,-130,130,130,-130);
	CalcNewTargetOffsets(newObj,STEGO_TARGET_SCALE);
	newObj->TargetChangeTimer = 0;


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 5, 5*2);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_STEGO]++;
	return(true);
}



/********************* MOVE STEGO **************************/

static void MoveStego(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveStego_Walking,	//nil
					MoveStego_Standing,
					MoveStego_Walking,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}

/********************** MOVE STEGO: STANDING ******************************/

static void  MoveStego_Standing(ObjNode *theNode)
{
	if (CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z) < STEGO_ATTACK_RANGE)
		MorphToSkeletonAnim(theNode->Skeleton, STEGO_ANIM_WALK,5);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateEnemy(theNode);		
	
}


/********************** MOVE STEGO: WALKING ******************************/

static void  MoveStego_Walking(ObjNode *theNode)
{
float	r;

	theNode->Skeleton->AnimSpeed = .5;								// tweak speed to avoid moonwalking

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, STEGO_TURN_SPEED, true);			

	r = theNode->Rot.y;
	theNode->Speed = MAX_WALK_SPEED;	
	gDelta.x = -sin(r) * MAX_WALK_SPEED;
	gDelta.z = -cos(r) * MAX_WALK_SPEED;
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode, theNode->BottomOff);

	if (CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z) > (STEGO_ATTACK_RANGE*2))
		MorphToSkeletonAnim(theNode->Skeleton, STEGO_ANIM_STAND,3);


		/* SEE IF CHANGE TARGET */

	if ((theNode->TargetChangeTimer += gFramesPerSecondFrac) > 10)	// every n seconds
	{
		CalcNewTargetOffsets(theNode,STEGO_TARGET_SCALE);
		theNode->TargetChangeTimer	= 0;
	}
	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
	UpdateEnemy(theNode);		
}









