/****************************/
/*   ENEMY: INSECT.C		*/
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
extern	TQ3Point3D				gCoord;
extern	short					gNumItems,gNumEnemies;
extern	Byte					gTotalSides;
extern	float					gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord,gMyCoord;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveInsect(ObjNode *theNode);
static void MoveInsect_Walking(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_INSECTS			4

#define	INSECT_SCALE		.2

#define	INSECT_CHASE_RANGE	1400
#define	INSECT_ATTACK_RANGE	100

#define INSECT_TURN_SPEED	1.0
#define INSECT_WALK_SPEED	35

#define	INSECT_HEALTH		.7		
#define	INSECT_DAMAGE		0.1


enum
{
	INSECT_ANIM_WALK
};


#define	INSECT_BOUNDS_SIZE	60 * INSECT_SCALE


/*********************/
/*    VARIABLES      */
/*********************/



/************************ ADD INSECT ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Insect(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	return(false);	//-----------

	if (gNumEnemies >= MAX_ENEMIES)
		return(false);
	if (gNumEnemyOfKind[ENEMY_KIND_INSECT] >= MAX_INSECTS)
		return(false);
		

				/* MAKE DEFAULT SKELETON ENEMY */
				
//	newObj = MakeEnemySkeleton(SKELETON_TYPE_INSECT,x,z);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;

	newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = INSECT_SCALE;	// set scale

	newObj->Skeleton->AnimSpeed = RandomFloat() + 1.0;

				/* SET BETTER INFO */
			
	newObj->MoveCall = MoveInsect;							// set move call

	newObj->Health = INSECT_HEALTH;
	newObj->Damage = INSECT_DAMAGE;
	newObj->Kind = ENEMY_KIND_INSECT;
	
				/* SET COLLISION INFO */
	
	SetObjectCollisionBounds(newObj, 60,-0,-INSECT_BOUNDS_SIZE,INSECT_BOUNDS_SIZE,INSECT_BOUNDS_SIZE,-INSECT_BOUNDS_SIZE,0);
	CalcNewTargetOffsets(newObj,6);
	


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_INSECT]++;
	return(true);
}



/********************* MOVE INSECT **************************/

static void MoveInsect(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveInsect_Walking
				};
	
	if (TrackTerrainItem(theNode))		// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}
	
	GetObjectInfo(theNode);
	
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}





/******************** MOVE INSECT: WALKING ************************/

static void MoveInsect_Walking(ObjNode *theNode)
{
float	r;


			/* DO MOTION */

	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, INSECT_TURN_SPEED, true);			

	r = theNode->Rot.y;
	
	if (theNode->Flag[0])
		theNode->Speed = INSECT_WALK_SPEED * theNode->Skeleton->AnimSpeed;	
	else
		theNode->Speed = 0;	

	gDelta.x = -sin(r) * theNode->Speed;
	gDelta.z = -cos(r) * theNode->Speed;
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode, theNode->BottomOff);
	
	
			/* DO COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
//	RotateOnTerrain(theNode, 30, 30);
}













