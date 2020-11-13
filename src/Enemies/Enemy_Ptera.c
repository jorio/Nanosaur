/****************************/
/*   ENEMY: PTERA.C			*/
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
#include "mobjtypes.h"
#include "skeletonjoints.h"
#include "qd3d_geometry.h"


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gPlayerNode[],*gCurrentNode,*gPlayerNode[];
 
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumItems,gNumEnemies;
extern	Byte					gNumPlayers;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];

extern const int MAX_PTERA;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MovePtera(ObjNode *theNode);
static void MovePtera_Flying(ObjNode *theNode);
static void  MovePtera_Diving(ObjNode *theNode);
static void  MovePtera_Carrying(ObjNode *theNode);
static void AttachARock(ObjNode *theEnemy);
static void MovePteraRock(ObjNode *theRock);


/****************************/
/*    CONSTANTS             */
/****************************/

#define PTERA_TURN_SPEED	1.8f
#define PTERA_WALK_SPEED	170

#define	PTERA_TARGET_SCALE	200

#define	PTERA_SCALE			1.0f

#define	PTERA_HEALTH		.5f		
#define	PTERA_DAMAGE		0.03f

#define	ATTACK_DIST			300

#define	MAX_ROCK_DROP_DIST	200
#define	MIN_ROCK_DROP_DIST	50

enum
{
	PTERA_ANIM_FLY,
	PTERA_ANIM_DIVE,
	PTERA_ANIM_CARRY
};

#define	FLIGHT_HEIGHT			100


/*********************/
/*    VARIABLES      */
/*********************/

#define	TargetChangeTimer	SpecialF[0]			// timer for target offset recalc
#define	Occillate			SpecialF[1]
#define	RockDropper			Flag[1]
#define	HasRock				Flag[2]



/************************ ADD PTERA ENEMY *************************/
//
// parm[3]: b0 = always add
//			b1 = rock dropper
//

Boolean AddEnemy_Ptera(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)				// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))				// see if always add 
	{
		if (gNumEnemyOfKind[ENEMY_KIND_PTERA] >= MAX_PTERA)
			return(false);
	}

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_PTERA,x,z);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;


			/* SEE IF ROCK-DROPPER */
			
	newObj->RockDropper = itemPtr->parm[3] & (1<<1);
	if (newObj->RockDropper)
	{
		AttachARock(newObj);
		SetSkeletonAnim(newObj->Skeleton, PTERA_ANIM_CARRY);	
	}
	else
		SetSkeletonAnim(newObj->Skeleton, PTERA_ANIM_FLY);
		
	newObj->Skeleton->AnimSpeed = RandomFloat()*.5 + 1;	
	newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = PTERA_SCALE;
	newObj->Radius *= PTERA_SCALE;
	

				/* SET BETTER INFO */
			
	newObj->Coord.y += FLIGHT_HEIGHT;
	newObj->MoveCall = MovePtera;							// set move call
	newObj->Health = PTERA_HEALTH;
	newObj->Damage = PTERA_DAMAGE;
	newObj->Kind = ENEMY_KIND_PTERA;
	newObj->Occillate = RandomFloat();
	
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 40,-40,-70,70,70,-70);
	CalcNewTargetOffsets(newObj,PTERA_TARGET_SCALE);
	newObj->TargetChangeTimer = RandomFloat()*5;


					/* MAKE SHADOW */
					
	AttachShadowToObject(newObj, 4, 4.5);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_PTERA]++;
	return(true);
}


/******************** ATTACH A ROCK ***************************/

static void AttachARock(ObjNode *theEnemy)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Boulder2;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Boulder2;	
	gNewObjectDefinition.scale = .4;
	gNewObjectDefinition.coord = gCoord;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = theEnemy->Slot + 1;
	gNewObjectDefinition.moveCall = MovePteraRock;
	gNewObjectDefinition.rot = 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj)
	{
		theEnemy->ChainNode = newObj;					// setup chain links
		newObj->ChainHead = theEnemy;
	
		theEnemy->HasRock = true;	
	}
}	


/***************** MOVE PTERA ROCK ************************/

static void MovePteraRock(ObjNode *theRock)
{
ObjNode	*theEnemy;
TQ3Point3D inPoint = {10,-10,80};


			/* SEE IF STILL ATTACHED TO ENEMY */
			
	theEnemy = theRock->ChainHead;
	if (theEnemy)
	{
		FindCoordOnJoint(theEnemy, 3, &inPoint, &theRock->Coord);
		UpdateObjectTransforms(theRock);
	}
	
			/* NOT ATTACHED ANYMORE */
	else
	{
		GetObjectInfo(theRock);
		
		gDelta.y -= GRAVITY_CONSTANT * gFramesPerSecondFrac;		

		gCoord.x += gDelta.x * gFramesPerSecondFrac;
		gCoord.y += gDelta.y * gFramesPerSecondFrac;
		gCoord.z += gDelta.z * gFramesPerSecondFrac;
		
		if (gCoord.y <= GetTerrainHeightAtCoord_Planar(gCoord.x,gCoord.y))
		{		
			QD3D_ExplodeGeometry(theRock, 400, 0, 2, .4);
			DeleteObject(theRock);
			return;
		}
		
		UpdateObject(theRock);
	}

}


/**************** DROP THE ROCK *******************/

static void DropTheRock(ObjNode *theEnemy)
{
ObjNode *theRock;

	theRock = theEnemy->ChainNode;
	if (theRock)
	{
		theEnemy->ChainNode = nil;
		theRock->ChainHead = nil;
		theRock->Delta = gDelta;
		
		theRock->CType = CTYPE_HURTME;
		theRock->CBits = CBITS_TOUCHABLE;		
		theRock->Damage = .1;
		SetObjectCollisionBounds(theRock,30,-30,-30,30,30,-30);

		MorphToSkeletonAnim(theEnemy->Skeleton, PTERA_ANIM_FLY, 2);		// do regular attack now
	}
}


/********************* MOVE PTERA **************************/

static void MovePtera(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MovePtera_Flying,
					MovePtera_Diving,
					MovePtera_Carrying
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}

/********************** MOVE PTERA: FLYING ******************************/

static void  MovePtera_Flying(ObjNode *theNode)
{
float	r,aim,dist,fps = gFramesPerSecondFrac;
float	occ,y;

			/* AIM AT PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, PTERA_TURN_SPEED, true);			
		
		
				/* MOVE IT */
				
	occ = theNode->Occillate += fps * 2.0f;		// inc occilation index
				
	r = theNode->Rot.y;
	theNode->Speed = PTERA_WALK_SPEED;	
	gDelta.x = -sin(r) * PTERA_WALK_SPEED;
	gDelta.z = -cos(r) * PTERA_WALK_SPEED;
	y = gMyCoord.y + 200 + cos(occ)*150;		// calc desired y offset
	gDelta.y = (y - gCoord.y);
	
	MoveEnemy(theNode,-FLIGHT_HEIGHT);


		/* SEE IF CHANGE TARGET */

	if ((theNode->TargetChangeTimer += fps) > 5.0f)	// every n seconds
	{
		CalcNewTargetOffsets(theNode,PTERA_TARGET_SCALE);
		theNode->TargetChangeTimer	= 0;
	}


			/* SEE IF SHOULD ATTACK */
			
	if (aim < 0.5f)
	{
		if ((gCoord.y - gMyCoord.y) > 100.0f)				// must be above me a ways
		{
			if (!theNode->Skeleton->IsMorphing)
			{
				dist = CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y,
									gMyCoord.x, gMyCoord.z);

				if (dist < ATTACK_DIST)
				{
					MorphToSkeletonAnim(theNode->Skeleton, PTERA_ANIM_DIVE,3);
				}
			}
		}
	}

	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES&(~CTYPE_BGROUND2)))		// normal collision but no BG2 for flying things
		return;


	UpdateEnemy(theNode);		
}


/********************** MOVE PTERA: DIVING ******************************/

static void  MovePtera_Diving(ObjNode *theNode)
{
float	r,dy;
float	fps = gFramesPerSecondFrac;
Boolean	onGround;

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, PTERA_TURN_SPEED*1.5, false);			

		
		/* MOVE IT */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * PTERA_WALK_SPEED;
	gDelta.z = -cos(r) * PTERA_WALK_SPEED;
	
	dy = (gMyCoord.y + 80 - gCoord.y) * 2.3f;
	gDelta.y += dy * fps;

	onGround = MoveEnemy(theNode,-FLIGHT_HEIGHT);


		/* SEE IF STOP DIVE */

	if (onGround || (gCoord.y < gMyCoord.y) || (gDelta.y > 0.0f))
	{
		MorphToSkeletonAnim(theNode->Skeleton, PTERA_ANIM_FLY,2);
	}

	
		/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;


	UpdateEnemy(theNode);		
}



/********************** MOVE PTERA: CARRYING ******************************/

static void  MovePtera_Carrying(ObjNode *theNode)
{
float	r,aim,dist,fps = gFramesPerSecondFrac;
float	occ,y;

			/* AIM AT PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, PTERA_TURN_SPEED, true);			
		
		
				/* MOVE IT */
				
	occ = theNode->Occillate += fps * 2.0f;		// inc occilation index
				
	r = theNode->Rot.y;
	theNode->Speed = PTERA_WALK_SPEED;	
	gDelta.x = -sin(r) * PTERA_WALK_SPEED;
	gDelta.z = -cos(r) * PTERA_WALK_SPEED;
	y = gMyCoord.y + 300 + cos(occ)*150;		// calc desired y offset
	gDelta.y = (y - gCoord.y);
	
	MoveEnemy(theNode,-FLIGHT_HEIGHT);


		/* SEE IF CHANGE TARGET */

	if ((theNode->TargetChangeTimer += fps) > 5.0f)		// every n seconds
	{
		CalcNewTargetOffsets(theNode,PTERA_TARGET_SCALE);
		theNode->TargetChangeTimer	= 0;
	}

			/***************************/
			/* SEE IF SHOULD DROP ROCK */
			/***************************/
			
	if (aim < 0.5f)
	{
		if ((gCoord.y - gMyCoord.y) > 100.0f)				// must be above me a ways
		{
			if (!theNode->Skeleton->IsMorphing)
			{
				dist = CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y,
									gMyCoord.x, gMyCoord.z);

				if ((dist < MAX_ROCK_DROP_DIST) && (dist > MIN_ROCK_DROP_DIST))
				{
					DropTheRock(theNode);
				}
			}
		}
	}

	
				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES&(~CTYPE_BGROUND2)))		// normal collision but no BG2 for flying things
		return;


	UpdateEnemy(theNode);		
}







