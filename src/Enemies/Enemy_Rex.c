/****************************/
/*   ENEMY: REX.C			*/
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
#include "sound2.h"
#include "effects.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gPlayerNode[],*gCurrentNode,*gPlayerNode[];
 
extern	TQ3Point3D				gCoord,gMyCoord;
extern	short					gNumItems,gNumEnemies;
extern	Byte					gNumPlayers;
extern	float					gFramesPerSecondFrac;
extern	TQ3Vector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];

extern const int MAX_REX;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveRex(ObjNode *theNode);
static void MoveRex_Walking(ObjNode *theNode);
static void MoveRex_Standing(ObjNode *theNode);
static void  MoveRex_Pounce(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	REX_MAX_ATTACK_RANGE	1100.0f
#define	REX_MIN_ATTACK_RANGE	350.0f


#define REX_TURN_SPEED		2.0f
#define REX_WALK_SPEED		100.0f

#define	POUNCE_SPEED		1.0f
#define	POUNCE_YACC			700	//600.0f

#define	REX_TARGET_SCALE	200.0f

#define	MAX_WALK_SPEED		200.0f


#define	REX_HEALTH		1.0f		
#define	REX_DAMAGE		0.04f

#define	REX_SCALE		1.2f

enum
{
	REX_ANIM_STAND,
	REX_ANIM_WALK,
	REX_ANIM_POUNCE,
	REX_ANIM_LANDING
};

#define	FOOT_OFFSET			0	//(-49*REX_SCALE)


/*********************/
/*    VARIABLES      */
/*********************/

#define	PounceFlag			Flag[0]				// set by anim when time to leave ground
#define IsPouncingFlag		Flag[1]				// set when is actually in air doing the pounce
#define	TargetChangeTimer	SpecialF[0]			// timer for target offset recalc


/************************ ADD REX ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Rex(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= MAX_ENEMIES)					// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))				// see if always add 
	{
		if (gNumEnemyOfKind[ENEMY_KIND_REX] >= MAX_REX)
			return(false);
	}
				/* MAKE DEFAULT SKELETON ENEMY */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_REX,x,z);
	if (newObj == nil)
		return(false);
	newObj->TerrainItemPtr = itemPtr;

	SetSkeletonAnim(newObj->Skeleton, REX_ANIM_WALK);
	

				/* SET BETTER INFO */
			
	newObj->Coord.y -= FOOT_OFFSET;			
	newObj->MoveCall = MoveRex;							// set move call
	newObj->Health = REX_HEALTH;
	newObj->Damage = REX_DAMAGE;
	newObj->Kind = ENEMY_KIND_REX;
	newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = REX_SCALE;	// set scale
	newObj->Radius *= REX_SCALE;
	
				/* SET COLLISION INFO */
				
	SetObjectCollisionBounds(newObj, 120,FOOT_OFFSET,-120,120,120,-120);


	CalcNewTargetOffsets(newObj,REX_TARGET_SCALE);
	newObj->TargetChangeTimer = 0;


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, 2.6, 2.6*2.5);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_REX]++;
	return(true);
}



/********************* MOVE REX **************************/

static void MoveRex(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveRex_Standing,
					MoveRex_Walking,
					MoveRex_Pounce,
					MoveRex_Pounce				// landing
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}

/********************** MOVE REX: STANDING ******************************/

static void  MoveRex_Standing(ObjNode *theNode)
{
		MorphToSkeletonAnim(theNode->Skeleton, REX_ANIM_WALK,5);

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;

	UpdateEnemy(theNode);		
	
}


/********************** MOVE REX: WALKING ******************************/

static void  MoveRex_Walking(ObjNode *theNode)
{
float	r,speed,dist,fps,aim;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */
			
	aim = TurnObjectTowardTarget(theNode, gMyCoord.x, gMyCoord.z, REX_TURN_SPEED, true);			

	r = theNode->Rot.y;
	speed = theNode->Speed = MAX_WALK_SPEED;	
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= GRAVITY_CONSTANT*fps;				// add gravity

	MoveEnemy(theNode, theNode->BottomOff);

				/* SEE IF RETURN TO STANDING */
				
	dist = CalcQuickDistance(gCoord.x+theNode->TargetOff.x, gCoord.z+theNode->TargetOff.y, gMyCoord.x, gMyCoord.z);
	if (dist > (REX_MAX_ATTACK_RANGE*1.3f))
		MorphToSkeletonAnim(theNode->Skeleton, REX_ANIM_STAND,3);

			/* SEE IF CLOSE ENOUGH TO POUNCE */
	else	
	if (dist < REX_MIN_ATTACK_RANGE)
	{
		if (aim < 0.5f)									// must be aimed mostly at me
		{
			MorphToSkeletonAnim(theNode->Skeleton, REX_ANIM_POUNCE,5);
			theNode->PounceFlag = false;
			theNode->IsPouncingFlag = false;
			theNode->Speed = 0;
			PlayEffect_Parms(EFFECT_ROAR,FULL_CHANNEL_VOLUME,kMiddleC+7);
		}
	}


		/* SEE IF CHANGE TARGET */

	if ((theNode->TargetChangeTimer += fps) > 3.0f)	// every n seconds
	{
		CalcNewTargetOffsets(theNode,REX_TARGET_SCALE);
		theNode->TargetChangeTimer	= 0;
		
		
				/* MAKE ROAR */
				
		if ((MyRandomLong()&3) < 2)								// see if roar
		{
			long volume = FULL_CHANNEL_VOLUME - (long)(dist * .15f);
			if (volume > 0)
				PlayEffect_Parms(EFFECT_ROAR,volume,kMiddleC+(MyRandomLong()&7));
		}

	}
	
		/* UPDATE ANIM SPEED */

	theNode->Skeleton->AnimSpeed = speed * .005f;
	

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;
	
	UpdateEnemy(theNode);		
}




/********************** MOVE REX: POUNCE ******************************/

static void  MoveRex_Pounce(ObjNode *theNode)
{
float	r,speed,fps,dist;
Boolean	onGround;


	fps = gFramesPerSecondFrac;

			/* MOVE HIM */
			
	r = theNode->Rot.y;
	speed = theNode->Speed;	
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= GRAVITY_CONSTANT*fps;										// add gravity

	onGround = MoveEnemy(theNode, theNode->BottomOff);
	

				/* SEE IF RETURN TO STANDING */
				
	if (theNode->IsPouncingFlag && onGround)								// see if landed on something solid
	{
		theNode->IsPouncingFlag = false;
		theNode->Damage = REX_DAMAGE;										// reset normal damage
		theNode->Speed = 0;
		MorphToSkeletonAnim(theNode->Skeleton, REX_ANIM_LANDING,4);			// make him land
		MakeDustPuff(gCoord.x + (RandomFloat()-.05) * 90, gCoord.y-FOOT_OFFSET, gCoord.z+(RandomFloat()-.05) * 90, 1.0);
		MakeDustPuff(gCoord.x + (RandomFloat()-.05) * 90, gCoord.y-FOOT_OFFSET, gCoord.z+(RandomFloat()-.05) * 90, .7);
		MakeDustPuff(gCoord.x + (RandomFloat()-.05) * 90, gCoord.y-FOOT_OFFSET, gCoord.z+(RandomFloat()-.05) * 90, 1.2);
		PlayEffect_Parms(EFFECT_FOOTSTEP,FULL_CHANNEL_VOLUME,kMiddleC-5);	// play sound
	}
				/* SEE IF DONE WITH LANDING */
				
	else
	if (theNode->Skeleton->AnimNum)
	{
		if (theNode->Skeleton->AnimHasStopped)
			SetSkeletonAnim(theNode->Skeleton, REX_ANIM_STAND);
	}

			/* SEE IF START POUNCE NOW */
			
	if (theNode->PounceFlag)
	{
		dist = CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z);
		theNode->PounceFlag = false;	
		theNode->IsPouncingFlag = true;
		theNode->Speed = (POUNCE_SPEED * dist) + 50;
		gDelta.y = POUNCE_YACC;
		theNode->Damage = REX_DAMAGE*3;					// extra damage while in this mode
		theNode->Skeleton->AnimSpeed = 1.1;				// tweak anim speed
	}

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES))
		return;



	
	UpdateEnemy(theNode);		
}




