/****************************/
/*   ENEMY: ALIEN.C			*/
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

static void MoveAlien(ObjNode *theNode);
static void MoveAlien_Standing(ObjNode *theNode);
static void MoveAlien_Walking(ObjNode *theNode);
static void MoveAlien_Landing(ObjNode *theNode);
static void MoveAlien_Jumping(ObjNode *theNode);
static void MoveAlien_Attack(ObjNode *theNode);
static void MakeAlienAttack(ObjNode *theNode);
static void StopAlienAttack(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ALIENS			2


#define	ALIEN_CHASE_RANGE	1400
#define	ALIEN_ATTACK_RANGE	100

#define ALIEN_TURN_SPEED	2.0
#define ALIEN_WALK_SPEED	55

#define	ALIEN_HEALTH		.7		
#define	ALIEN_DAMAGE		0.1

#define	JUMP_VELOCITY_Y		500
#define	JUMP_SPEED			ALIEN_WALK_SPEED*2

enum
{
	ALIEN_ANIM_STAND,
	ALIEN_ANIM_WALK,
	ALIEN_ANIM_JUMP,
	ALIEN_ANIM_LAND,
	ALIEN_ANIM_ATTACK
};


enum
{
	ALIEN_MODE_WALKER,
	ALIEN_MODE_JUMPER
};

#define	ALIEN_BOUNDS_SIZE	30

/*********************/
/*    VARIABLES      */
/*********************/

#define	JumpFlag	Flag[0]



/************************ ADD ALIEN ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Alien(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)
		return(false);
	if (gNumEnemyOfKind[ENEMY_KIND_ALIEN] >= MAX_ALIENS)
		return(false);
		

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_ALIEN,x,z);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;


				/* SET BETTER INFO */
			
	newObj->MoveCall = MoveAlien;							// set move call

	newObj->Health = ALIEN_HEALTH;
	newObj->Damage = ALIEN_DAMAGE;
	newObj->Kind = ENEMY_KIND_ALIEN;
	
				/* SET COLLISION INFO */
	
	SetObjectCollisionBounds(newObj, 60,0,-ALIEN_BOUNDS_SIZE,ALIEN_BOUNDS_SIZE,ALIEN_BOUNDS_SIZE,-ALIEN_BOUNDS_SIZE,0);
	CalcNewTargetOffsets(newObj,6);
	


			/* WHAT KIND OF ALIEN? */
			
	if (MyRandomLong()&1)
		newObj->Mode = ALIEN_MODE_WALKER;
	else
		newObj->Mode = ALIEN_MODE_JUMPER;



				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 1.5, 1.5);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ALIEN]++;
	return(true);
}



/********************* MOVE ALIEN **************************/

static void MoveAlien(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveAlien_Standing,
					MoveAlien_Walking,
					MoveAlien_Jumping,
					MoveAlien_Landing,
					MoveAlien_Attack
				};
	
	if (TrackTerrainItem(theNode))		// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}
	
	GetObjectInfo(theNode);
	
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/******************** MOVE ALIEN: STANDING ************************/

static void MoveAlien_Standing(ObjNode *theNode)
{
float	d;

	gDelta.x = gDelta.z = 0;							// not moving

				/* SEE IF START CHASING */

	d = CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z);
	if (d < ALIEN_CHASE_RANGE)
	{
		switch(theNode->Mode)
		{
			case	ALIEN_MODE_WALKER:
					MorphToSkeletonAnim(theNode->Skeleton, ALIEN_ANIM_WALK,5);
					break;
					
			case	ALIEN_MODE_JUMPER:
					MorphToSkeletonAnim(theNode->Skeleton, ALIEN_ANIM_JUMP,4);
					theNode->JumpFlag = false;
					break;
		}
	}

				/* SEE IF ATTACK */
				
	if (d < ALIEN_ATTACK_RANGE)
		if (RandomFloat() < gFramesPerSecondFrac)
			MakeAlienAttack(theNode);


	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, ALIEN_TURN_SPEED, true);


				/* DO GRAVITY */
				
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity
	MoveEnemy(theNode, theNode->BottomOff);


			/* DO COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateEnemy(theNode);		
}


/******************** MOVE ALIEN: WALKING ************************/

static void MoveAlien_Walking(ObjNode *theNode)
{
float	r,d;

	theNode->Skeleton->AnimSpeed = 1.3;									// avoid moonwalking
	
				/* SEE IF STOP CHASING */
	
	d = CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z);		
	if (d > ALIEN_CHASE_RANGE)
		MorphToSkeletonAnim(theNode->Skeleton, ALIEN_ANIM_STAND,5);


				/* SEE IF ATTACK */
				
	if (d < ALIEN_ATTACK_RANGE)
		if (RandomFloat() < gFramesPerSecondFrac)
			MakeAlienAttack(theNode);
				

			/* DO MOTION */

	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, ALIEN_TURN_SPEED, true);			

	r = theNode->Rot.y;
	theNode->Speed = ALIEN_WALK_SPEED;	
	gDelta.x = -sin(r) * theNode->Speed;
	gDelta.z = -cos(r) * theNode->Speed;
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode, theNode->BottomOff);
	
	
			/* DO COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;



	UpdateEnemy(theNode);		
}


/******************** MOVE ALIEN: JUMPING ************************/

static void MoveAlien_Jumping(ObjNode *theNode)
{
Boolean	onGround = false;


			/* DO GRAVITY */
			
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity
	if (MoveEnemy(theNode, theNode->BottomOff))
		onGround++;


			/* DO COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	if (gTotalSides & SIDE_BITS_BOTTOM)							// see if landed on something
		onGround++;
	

			/* SEE IF START THE JUMP */
			
	if (theNode->JumpFlag == 1)
	{
		theNode->JumpFlag++;
		gDelta.y = JUMP_VELOCITY_Y;
		gCoord.y += gDelta.y*gFramesPerSecondFrac;					// move NOW
		
		gDelta.x = -sin(theNode->Rot.y) * JUMP_SPEED;
		gDelta.z = -cos(theNode->Rot.y) * JUMP_SPEED;
		
		onGround = false;
	}
	else
	if (theNode->JumpFlag == 2)
	{
		if (onGround)
		{
			MorphToSkeletonAnim(theNode->Skeleton,ALIEN_ANIM_LAND,9);
			theNode->JumpFlag = false;
		}
	}

	gCoord.x += gDelta.x*gFramesPerSecondFrac;
	gCoord.z += gDelta.z*gFramesPerSecondFrac;
	

	UpdateEnemy(theNode);		
}




/******************** MOVE ALIEN: LANDING ************************/

static void MoveAlien_Landing(ObjNode *theNode)
{
	gDelta.x = gDelta.z = 0;							// not moving

	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, ALIEN_TURN_SPEED, true);			

			/* DO GRAVITY */
			
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity
	
	MoveEnemy(theNode, theNode->BottomOff);


			/* SEE IF DONE */
			
	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton,ALIEN_ANIM_JUMP,3);


			/* DO COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;



	UpdateEnemy(theNode);		
}




/******************** MOVE ALIEN: ATTACK ************************/

static void MoveAlien_Attack(ObjNode *theNode)
{
	gDelta.x = gDelta.z = 0;									// not moving

	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, ALIEN_TURN_SPEED*1.5, true);			

			/* DO GRAVITY */
			
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity
	
	MoveEnemy(theNode, theNode->BottomOff);


			/* SEE IF DONE */
			
	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton,ALIEN_ANIM_STAND,3);


			/* DO COLLISION */
			
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

			/* SEE IF STOPPED ATTACK */
			
	if (theNode->Skeleton->AnimNum != ALIEN_ANIM_ATTACK)
		StopAlienAttack(theNode);

	UpdateEnemy(theNode);		
}


/******************* MAKE ALIEN ATTACK ************************/

static void MakeAlienAttack(ObjNode *theNode)
{
float	dx,dz;

			/* CHANGE COLLISION INFO */
			
	theNode->CBits = CBITS_TOUCHABLE;
	theNode->CType |= CTYPE_HURTIFTOUCH;

	
	dx = -sin(theNode->Rot.y) * 20;
	dz = -cos(theNode->Rot.y) * 20;
	
	theNode->LeftOff += dx;								// move collision box
	theNode->RightOff += dx;
	theNode->FrontOff += dz;
	theNode->BackOff += dz;

			/* MAKE ATTACK */
				
	SetSkeletonAnim(theNode->Skeleton,ALIEN_ANIM_ATTACK);
}


/********************* STOP ALIEN ATTACK **************************/

static void StopAlienAttack(ObjNode *theNode)
{

	theNode->LeftOff = -ALIEN_BOUNDS_SIZE;							
	theNode->RightOff = ALIEN_BOUNDS_SIZE;		
	theNode->FrontOff = ALIEN_BOUNDS_SIZE;		
	theNode->BackOff = -ALIEN_BOUNDS_SIZE;		


	theNode->CType &= ~CTYPE_HURTIFTOUCH;
	theNode->CBits = CBITS_ALLSOLID;

}













