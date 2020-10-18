/****************************/
/*   ENEMY: SPITTER.C			*/
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
#include "skeletonjoints.h"
#include "3dmath.h"
#include "myguy.h"
#include "sound2.h"
#include "mobjtypes.h"

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

static void MoveSpitter(ObjNode *theNode);
static void MoveSpitter_Walking(ObjNode *theNode);
static void MoveSpitter_Standing(ObjNode *theNode);
static void  MoveSpitter_Spitting(ObjNode *theNode);
static void ShootSpit(ObjNode *theNode);
static void MoveDinoSpit(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#ifdef PRO_MODE
#define	MAX_SPITTER				2
#else
#define	MAX_SPITTER				12
#endif

#define	SPITTER_MAX_ATTACK_RANGE	1200.0f
#define	SPITTER_MIN_ATTACK_RANGE	300.0f

#define SPITTER_TURN_SPEED		3.5f
#define SPITTER_WALK_SPEED		380.0f

#define	SPITTER_TARGET_SCALE	200.0f


#define	SPITTER_HEALTH		0.8f	
#define	SPITTER_DAMAGE		0.01f

#define	SPITTER_SCALE		.8f

enum
{
	SPITTER_ANIM_STAND,
	SPITTER_ANIM_SPITTING,
	SPITTER_ANIM_WALK
};

#define	FOOT_OFFSET			0.0f	//(-49*SPITTER_SCALE)

#define	SPIT_SPEED			500


/*********************/
/*    VARIABLES      */
/*********************/

#define	ShootSpitFlag			Flag[0]
#define	TargetChangeTimer	SpecialF[0]			// timer for target offset recalc
#define SpitTimer			SpecialF[1]
#define SpitRegulator		SpecialF[2]

/************************ ADD SPITTER ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Spitter(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)				// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))				// see if always add 
	{
		if (gNumEnemyOfKind[ENEMY_KIND_SPITTER] >= MAX_SPITTER)
			return(false);
	}

				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SPITTER,x,z);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, SPITTER_ANIM_WALK);
	

				/* SET BETTER INFO */
			
	newObj->Coord.y -= FOOT_OFFSET;			
	newObj->MoveCall = MoveSpitter;							// set move call
	newObj->Health = SPITTER_HEALTH;
	newObj->Damage = SPITTER_DAMAGE;
	newObj->Kind = ENEMY_KIND_SPITTER;
	newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = SPITTER_SCALE;	// set scale
	newObj->Radius *= SPITTER_SCALE;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 80,FOOT_OFFSET,-90,90,90,-90);


	CalcNewTargetOffsets(newObj,SPITTER_TARGET_SCALE);
	newObj->TargetChangeTimer = 0;
	newObj->SpitTimer = 100000;


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 1.6, 1.6*2.5);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_SPITTER]++;
	return(true);
}



/********************* MOVE SPITTER **************************/

static void MoveSpitter(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveSpitter_Standing,
					MoveSpitter_Spitting,
					MoveSpitter_Walking
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}

/********************** MOVE SPITTER: STANDING ******************************/

static void  MoveSpitter_Standing(ObjNode *theNode)
{
float	d;


				/* SEE IF IN WALK RANGE */
				
	d = CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z);
	if ((d < SPITTER_MAX_ATTACK_RANGE) && (d > SPITTER_MIN_ATTACK_RANGE))
		MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_WALK,7);

					/* SEE IF SHOULD SPIT */
					
	else
	{
		theNode->SpitTimer -= gFramesPerSecondFrac;							// dec timer
		if (theNode->SpitTimer < 0)
		{
			theNode->SpitTimer = 1000000;
			MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_SPITTING,7);
			theNode->ShootSpitFlag = false;
		}
	}

			/* IF CLOSE, KEEP AIMED AT ME */
			
	if (d < (SPITTER_MIN_ATTACK_RANGE * 2.0f))
	{
		TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, SPITTER_TURN_SPEED/3, false);			
	}

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
	
}


/********************** MOVE SPITTER: WALKING ******************************/

static void  MoveSpitter_Walking(ObjNode *theNode)
{
float	r,speed,dist;

			/* MOVE TOWARD PLAYER */
			
	TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, SPITTER_TURN_SPEED, true);			

	r = theNode->Rot.y;
	speed = theNode->Speed = SPITTER_WALK_SPEED;	
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;				// add gravity

	MoveEnemy(theNode, theNode->BottomOff);


			/* SEE IF OUT OF WALK-RANGE */
			
	dist = CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z);
	if (dist > (SPITTER_MAX_ATTACK_RANGE*1.3f))
	{
		theNode->SpitTimer = 10000000;
		MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_STAND,4);
	}
	else
	if (dist < (SPITTER_MIN_ATTACK_RANGE*.8f))
	{
		if (MyRandomLong()&1)											// see if stand or spit
		{
			theNode->SpitTimer = (RandomFloat()+.5f);				// wait random amount of time before spitting
			MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_STAND,8);
		}
		else
		{
			MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_SPITTING,8);
			theNode->ShootSpitFlag = false;
		}
	}


		/* SEE IF CHANGE TARGET */

	if ((theNode->TargetChangeTimer += gFramesPerSecondFrac) > 3.0f)	// every n seconds
	{
		CalcNewTargetOffsets(theNode,SPITTER_TARGET_SCALE);
		theNode->TargetChangeTimer	= 0;
	}
	
		/* UPDATE ANIM SPEED */

	theNode->Skeleton->AnimSpeed = speed * .008f;
	

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
	UpdateEnemy(theNode);		
}

/********************** MOVE SPITTER: SPITTING ******************************/

static void  MoveSpitter_Spitting(ObjNode *theNode)
{

			/* SEE IF CAN SHOOT SPIT NOW */
			
	if (theNode->ShootSpitFlag)
	{
		ShootSpit(theNode);
	
	}
	
			/* SEE IF DONE WITH ANIM */
			
	if (theNode->Skeleton->AnimHasStopped)
	{	
		float	d;

			/* FIGURE OUT WHICH ANIM TO GO TO */
			
		d = CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z);
		if ((d < SPITTER_MAX_ATTACK_RANGE) && (d > SPITTER_MIN_ATTACK_RANGE))
			MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_WALK,7);
		else
		{
			theNode->SpitTimer = (RandomFloat()+.5f);				// wait random amount of time before spitting again
			MorphToSkeletonAnim(theNode->Skeleton, SPITTER_ANIM_STAND,7);
		}
	}

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
	
}


/********************** SHOOT SPIT *****************************/

static void ShootSpit(ObjNode *theNode)
{
TQ3Matrix4x4		matrix;
static TQ3Point3D	originOff = {0,15,-40};
static TQ3Vector3D	inVector = {0,-.1,-.9};
TQ3Vector3D			vector;
ObjNode				*newObj;

			/* SEE IF TIME TO SPIT */
			
	theNode->SpitRegulator += gFramesPerSecondFrac;
	if (theNode->SpitRegulator < .08f)
		return;
		
	theNode->SpitRegulator = 0;	
	

			/* CALCULATE COORD OF MOUTH */
			
	FindJointFullMatrix(theNode, 2, &matrix);
	Q3Point3D_Transform(&originOff, &matrix, &gNewObjectDefinition.coord);


		/* CALC DIRECTIONAL VECTOR FOR SPEW */
			
	Q3Vector3D_Transform(&inVector, &matrix, &vector);


				/* MAKE SPIT WAD */

	gNewObjectDefinition.group = GLOBAL_MGroupNum_DinoSpit;	
	gNewObjectDefinition.type = GLOBAL_MObjType_DinoSpit;	
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 100;
	gNewObjectDefinition.moveCall = MoveDinoSpit;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj)
	{
		newObj->Delta.x = vector.x * SPIT_SPEED + ((RandomFloat() - .5f) * 60.0f);
		newObj->Delta.y = vector.y * SPIT_SPEED + ((RandomFloat() - .5f) * 60.0f);
		newObj->Delta.z = vector.z * SPIT_SPEED + ((RandomFloat() - .5f) * 60.0f);

		newObj->CType = CTYPE_HURTME;
		newObj->CBits = CBITS_TOUCHABLE;
		newObj->Damage = .1;
		
		
		SetObjectCollisionBounds(newObj,20,-20,-20,20,20,-20);
	}
}


/******************** MOVE DINO SPIT ***********************/

static void MoveDinoSpit(ObjNode *theNode)
{
float	y,fps;

	GetObjectInfo(theNode);

			/* APPLY GRAVITY */

	fps = gFramesPerSecondFrac;	
	gDelta.y -= fps * (GRAVITY_CONSTANT/6);

			/* MOVE IT */
			
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SEE IF HIT GROUND */

	y = GetTerrainHeightAtCoord_Quick(gCoord.x,gCoord.z);
	if (gCoord.y <= y)
	{
del:	
		DeleteObject(theNode);
		return;
	}

			/* DISINTEGRATE */

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z -= (fps * .9f);
	if (theNode->Scale.x <= 0.0f)
		goto del;

	UpdateObject(theNode);
}







