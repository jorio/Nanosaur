/****************************/
/*   	MYGUY.C    			*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveMe(ObjNode *theNode);
static void UpdatePlayer(ObjNode *theNode);
static void MovePlayer_Standing(ObjNode *theNode);
static void MovePlayer_Walking(ObjNode *theNode);
static void MovePlayer_Jumping2(ObjNode *theNode);
static Boolean DoPlayerCollisionDetect(ObjNode *theNode);
static void MovePlayer_Landing(ObjNode *theNode);
static void MovePlayer_Biting(ObjNode *theNode);
static void MovePlayer_Turning(ObjNode *theNode);
static void MovePlayer_PickUp(ObjNode *theNode);
static void MovePlayer_Throw(ObjNode *theNode);
static void MovePlayer_GetHurt(ObjNode *theNode);
static void MovePlayer_JetNeutral(ObjNode *theNode);
static void MovePlayer_Jumping1(ObjNode *theNode);
static void MovePlayer_Death(ObjNode *theNode);
static void KillPlayer(ObjNode *theNode);
static void MovePlayer_Exit(ObjNode *theNode);

static Boolean DoMyKickCollision(ObjNode *theNode);
static void MoveShield(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	MY_SCALE			.5f 

#define	MY_FOOT_JOINT_NUM	7

#define	MY_KICK_DAMAGE	.3f

#define	SOME_SMALL_DELTA	.1f

#define	THROW_FORCE			500.0f

#define	HURT_DURATION		.6f

#define	INVINCIBILITY_DURATION	(HURT_DURATION + 1.0f)			// must be longer than hurt duration
#define	INVINCIBILITY_DURATION_SHORT .1f						// short invincibility

#define	FUEL_LOSS_RATE		1.0f

#define	SHIELD_TIME			20									// n seconds of shield per powerup


/*********************/
/*    VARIABLES      */
/*********************/


long		gMyStartX,gMyStartZ;
TQ3Point3D	gMyCoord;
ObjNode		*gPlayerObj,*gMyTimePortal,*gMyShield;
float		gMyHealth = 1.0;
float		gLavaSmokeCounter;
static float gMyTimePortalTimer;
UInt16		gMyLatestPathTileNum = 0, gMyLatestTileAttribs = 0;
float		gShieldTimer = 0;
short		gShieldChannel = -1;
Byte		gMyStartAim;


/*************************** INIT MY GUY ****************************/

void InitMyGuy(void)
{
ObjNode	*newObj;
float	y;
		
	gMyLatestPathTileNum = 	gMyLatestTileAttribs = 0;		
	gLavaSmokeCounter = gMySpeedPuffCounter = 0;	
	gShieldTimer = 0;				
	gMyShield = nil;
	gMyHealth = 1.0;
	gShieldChannel = -1;
	
	y = GetTerrainHeightAtCoord_Quick(gMyStartX,gMyStartZ)-DIST_FROM_ORIGIN_TO_FEET;
	
	gMyCoord.x = gMyStartX;
	gMyCoord.z = gMyStartZ;
	gMyCoord.y = y+50;
					
			/* CREATE MY CHARACTER */	
	
	gNewObjectDefinition.type 	= SKELETON_TYPE_DEINON;
	gNewObjectDefinition.animNum = PLAYER_ANIM_FALLING;
	gNewObjectDefinition.coord.x = gMyStartX;
	gNewObjectDefinition.coord.y = y + 400;
	gNewObjectDefinition.coord.z = gMyStartZ;
	gNewObjectDefinition.slot = PLAYER_SLOT;
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL|STATUS_BIT_HIDDEN;
	gNewObjectDefinition.moveCall = MoveMe;
	gNewObjectDefinition.rot = (float)gMyStartAim * (PI2/8);
	gNewObjectDefinition.scale = MY_SCALE;
	newObj = gPlayerObj = MakeNewSkeletonObject(&gNewObjectDefinition);	
	
	newObj->CType = CTYPE_SKELETON|CTYPE_PLAYER;
	newObj->CBits = CBITS_TOUCHABLE;
	
	
			/* INIT COLLISION BOX */
			
	SetObjectCollisionBounds(newObj,60*MY_SCALE,-DIST_FROM_ORIGIN_TO_FEET,-50*MY_SCALE,
							50*MY_SCALE,50*MY_SCALE,-50*MY_SCALE);

	newObj->Damage = .5;
	
	newObj->InvincibleTimer = 0;
	
	
				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, .9, .9*2.5);
				

			/* MAKE APPEARANCE TIME PORTAL */

	gMyTimePortal = MakeTimePortal(PORTAL_TYPE_ENTER, gMyStartX, gMyStartZ);
	gMyTimePortalTimer = 2.5;
	PlayEffect_Parms(EFFECT_PORTAL,FULL_CHANNEL_VOLUME,kMiddleC-8);	
}


/********************** RESET PLAYER **********************/
//
// Called after player dies and time to reincarnate.
//

void ResetPlayer(void)
{
ObjNode	*theNode = gPlayerObj;

	gNumLives--;
	if (gNumLives < 0)
	{
		gGameOverFlag = true;
		return;
	}
	

	gPlayerGotKilledFlag = false;
	gMyHealth = 1.0;
	gFuel = 0;	
	gInfobarUpdateBits |= UPDATE_HEALTH|UPDATE_FUEL|UPDATE_SCORE|UPDATE_LIVES;

	theNode->HurtTimer = 0;
	theNode->InvincibleTimer = INVINCIBILITY_DURATION*3;		// make me invincible for a while


			/* EXPLODE "OLD" ONE */
			
	QD3D_ExplodeGeometry(theNode, 500.0f, 0, 3, .4);		// note: this doesnt delete anything


			/* MAKE NEW APPEARANCE TIME PORTAL */

	gMyTimePortal = MakeTimePortal(PORTAL_TYPE_ENTER, theNode->Coord.x, theNode->Coord.z);
	gMyTimePortalTimer = 3.0;
	PlayEffect_Parms(EFFECT_PORTAL,FULL_CHANNEL_VOLUME,kMiddleC-8);


			/* RESET PLAYER INTO POSITION */
			
	SetSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_FALLING);		// start falling
	theNode->Coord.y += 400;
	theNode->StatusBits |= STATUS_BIT_HIDDEN;

	UpdateObjectTransforms(theNode);
	
	
			/* ALSO RESET CAMERA */
			
	ResetCameraSettings();
	
	
	
			/* AND RESET WEAPONS INVENTORY */
			
	InitWeaponManager();
}


/******************** MOVE ME: SKELETON ***********************/

static void MoveMe(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MovePlayer_Standing,
					MovePlayer_Walking,	
					MovePlayer_Jumping2,
					MovePlayer_Landing,
					MovePlayer_Biting,
					MovePlayer_Turning,
					MovePlayer_Turning,
					MovePlayer_PickUp,
					MovePlayer_Throw,
					MovePlayer_GetHurt,
					MovePlayer_JetNeutral,
					MovePlayer_Jumping1,				// jump 1
					MovePlayer_Jumping1,				// FALLING
					MovePlayer_Death,
					MovePlayer_Exit
				};
	
	GetObjectInfo(theNode);

	
		/* DON'T DO ANYTHING UNTIL ENTRY TIME PORTAL TIMES OUT */
				
	if (gMyTimePortalTimer > 0.0f)
	{
		gMyTimePortalTimer -= gFramesPerSecondFrac;
		if (gMyTimePortalTimer <= 0.0f)
		{
			theNode->StatusBits &= ~STATUS_BIT_HIDDEN;				// un-hide me
			PlayEffect_Parms(EFFECT_PORTAL,FULL_CHANNEL_VOLUME,kMiddleC+2);			
		}
		UpdatePlayer(theNode);			
		return;
	}
	
			/* UPDATE INVINCIBILITY */
			
	if (theNode->InvincibleTimer > 0)
	{
		theNode->InvincibleTimer -= gFramesPerSecondFrac;
		if (theNode->InvincibleTimer < 0)
			theNode->InvincibleTimer = 0;
	}
	
		/* FORCE ME DEAD IF THAT'S THE CASE */
		
	if (gPlayerGotKilledFlag)
		if (theNode->Skeleton->AnimNum != PLAYER_ANIM_DEATH)
			SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_DEATH);
	
	myMoveTable[theNode->Skeleton->AnimNum](theNode);

}



/************************ MOVE PLAYER: STANDING **************************/

static void MovePlayer_Standing(ObjNode *theNode)
{
			/* DO CONTROL */

	DoPlayerControl(theNode,1.0);
	CheckJetThrustControl(theNode);

			/* MOVE PLAYER */
			
	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);
	DoPlayerMovement(theNode);
		

			/* CHECK ANIM */
			
	if ((gDelta.z != 0.0f) || (gDelta.x != 0.0f))				// see if go/run
		if (!theNode->JetThrust)						// make sure jet didnt just now get activated
			MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_WALK,15.0);

	
			/* DO COLLISION DETECT */
			
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	

			/* SEE IF ATTACK */
			
	CheckIfMeAttack(theNode);


			/* UPDATE IT */
			
update:			
	UpdatePlayer(theNode);
}


/************************ MOVE PLAYER: WALKING **************************/

static void MovePlayer_Walking(ObjNode *theNode)
{
			/* DO CONTROL */

	DoPlayerControl(theNode,1.0);
	CheckJetThrustControl(theNode);


			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);
	DoPlayerMovement(theNode);
	
		
		/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = theNode->Speed * .005f;


			/* CHECK ANIM */
			
	if ((gDelta.z == 0.0f) && (gDelta.x == 0.0f))				// see if stop/stand
	{
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND,3.0);
	}

	
			/* DO COLLISION DETECT */
			
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	
			/* SEE IF ATTACK */
			
	CheckIfMeAttack(theNode);
	

			/* UPDATE IT */
			
update:			
	UpdatePlayer(theNode);
}





/************************ MOVE PLAYER: JUMPING 1 **************************/
//
// This is the initial basic jump.
//
// Also handles falling animation!
//

static void MovePlayer_Jumping1(ObjNode *theNode)
{
			/* DO CONTROL */

	DoPlayerControl(theNode,0.3);


			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL/10);			// do friction
			
	DoPlayerMovement(theNode);										// returns true if on terrain


			/* SEE IF SWITCH TO FALLING */
			
	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_FALLING)
	{
		if (gDelta.y <= 0)
			MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_FALLING,2.0);	// start falling
	}		
	
			/* DO COLLISION DETECT */
			
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)							// see if landed on something solid
	{
		DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL*5);				// do momentary friction upon landing
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_LANDING,12);
		MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET, gCoord.z, .1);
		MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET, gCoord.z, .2);
		MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET, gCoord.z, .25);
	}
	
			/* SEE IF ATTACK */
			
	CheckIfMeAttack(theNode);
	
	
			/* UPDATE IT */
update:			
	UpdatePlayer(theNode);
}



/************************ MOVE PLAYER: LANDING **************************/

static void MovePlayer_Landing(ObjNode *theNode)
{
	theNode->Skeleton->AnimSpeed = 1.5;

			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL/10);							// do friction
	DoPlayerMovement(theNode);
			
	if (theNode->Skeleton->AnimHasStopped)									// returns true if on terrain
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND,4);
		
	
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	
			/* UPDATE IT */
update:			
	UpdatePlayer(theNode);
}


/************************ MOVE PLAYER: JUMPING 2**************************/
//
// This is the 2nd stage of the jump where the player spins (and cannot fire)
//

static void MovePlayer_Jumping2(ObjNode *theNode)
{
//	theNode->Skeleton->AnimSpeed = 1.8;								// set good speed

			/* DO CONTROL */

	DoPlayerControl(theNode,0.3);


			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL/10.0f);			// do friction
			
	DoPlayerMovement(theNode);										// returns true if on terrain

	theNode->Rot.x -= 6.0f * gFramesPerSecondFrac;					// spin when jump
	if (theNode->Rot.x < -PI2)
	{
		theNode->Rot.x = 0;
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_FALLING, 3.0);
	}
	
	
			/* DO COLLISION DETECT */
			
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)							// see if landed on something solid
	{
		DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL*5);				// do momentary friction upon landing
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_LANDING,12);
		theNode->Rot.x = 0;
		MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET, gCoord.z, .1);
		MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET, gCoord.z, .2);
		MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET, gCoord.z, .25);
	}
	
			/* UPDATE IT */
update:			

	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_JUMP2)	// fix x-rot if anim has changed
		theNode->Rot.x = 0;

	UpdatePlayer(theNode);
}




/************************ MOVE PLAYER: BITING **************************/

static void MovePlayer_Biting(ObjNode *theNode)
{
			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);							// do friction
	DoPlayerMovement(theNode);
			
	if (theNode->Skeleton->AnimHasStopped)									// returns true if on terrain
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND,4);

	if (DoPlayerCollisionDetect(theNode))
		goto update;


				/* SEE IF FOOT HITS ANYTHING */

	if (theNode->KickIsLethal)
	{
		if (DoMyKickCollision(theNode))										// check foot collision
		{
			MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND,5);		// if hit, then end the anim
		}
	}


			/* UPDATE IT */
update:
	UpdatePlayer(theNode);
}



/************************ MOVE PLAYER: TURNING **************************/

static void MovePlayer_Turning(ObjNode *theNode)
{

			/* DO CONTROL */

	DoPlayerControl(theNode,1.0);
	CheckJetThrustControl(theNode);


			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);
	DoPlayerMovement(theNode);
		
		
			/* DO COLLISION DETECT */
			
	if (DoPlayerCollisionDetect(theNode))
		goto update;


		/* UPDATE ANIM SPEED */
			
	theNode->Skeleton->AnimSpeed = .7f + theNode->Speed * .006f;
	
			/* SEE IF ATTACK */
			
	CheckIfMeAttack(theNode);


			/* UPDATE IT */
update:			
	UpdatePlayer(theNode);
}



/************************ MOVE PLAYER: PICKUP **************************/
//
// Does both pickup and put down.
//

static void MovePlayer_PickUp(ObjNode *theNode)
{
			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);					// do friction
	DoPlayerMovement(theNode);
			
	if (theNode->Skeleton->AnimHasStopped)									// returns true if on terrain
		SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND);

	if (DoPlayerCollisionDetect(theNode))
		goto update;


				/* SEE IF TRY TO PICKUP/PUTDOWN OBJECT */

	if (theNode->PickUpNow)
	{
		theNode->PickUpNow = false;
		
		if (theNode->StatusBits & STATUS_BIT_ISCARRYING)
			DropItem(theNode);	
		else
			TryToDoPickUp(theNode,MYGUY_LIMB_HEAD);
	}


			/* UPDATE IT */
update:
	UpdatePlayer(theNode);
}


/************************ MOVE PLAYER: THROW **************************/
//
// Does both pickup and put down.
//

static void MovePlayer_Throw(ObjNode *theNode)
{
ObjNode	*itemObj;
float	r;

			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);		// do friction
	DoPlayerMovement(theNode);
			
	if (theNode->Skeleton->AnimHasStopped)
		SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND);

	if (DoPlayerCollisionDetect(theNode))
		goto update;


				/* SEE IF TRY TO PICKUP/PUTDOWN OBJECT */

	if (theNode->ThrowNow)
	{
		theNode->ThrowNow = false;								// clear flag
		
		if (theNode->StatusBits & STATUS_BIT_ISCARRYING)		// verify this is still valid
		{
			itemObj = theNode->CarriedObj;						// remember what obj is being thrown
			if (itemObj->CType == INVALID_NODE_FLAG)			// make sure object is valid
				return;
			
			DropItem(theNode);									// cause it to be dropped, but CarriedObj still pts to it
			
					/* CALC THROW VECTOR */
					
			r = theNode->Rot.y;									// get player y rot
			itemObj->Delta.x = gDelta.x + sin(r) * -THROW_FORCE;				
			itemObj->Delta.z = gDelta.z + cos(r) * -THROW_FORCE;				
			itemObj->Delta.y = 80;
			
			itemObj->RotDelta.x = (RandomFloat()-.5f)*30.0f;		// put random spin on it
			itemObj->RotDelta.y = (RandomFloat()-.5f)*30.0f;
			itemObj->RotDelta.z = (RandomFloat()-.5f)*30.0f;
		}			
	}


			/* UPDATE IT */
update:
	UpdatePlayer(theNode);
}


/************************ MOVE PLAYER: GET HURT **************************/

static void MovePlayer_GetHurt(ObjNode *theNode)
{
			/* MOVE PLAYER */

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL/10);					// do friction
	DoPlayerMovement(theNode);
			
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	
	if ((theNode->HurtTimer -= gFramesPerSecondFrac) < 0.0f)				// see if done being hurt
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND,4);
		
	
			/* UPDATE IT */
update:			
	UpdatePlayer(theNode);
}



/************************ MOVE PLAYER: JET NEUTRAL **************************/

static void MovePlayer_JetNeutral(ObjNode *theNode)
{
int	oldFuel;

			/* DO CONTROL */

	DoPlayerJetControl(theNode);
	CheckJetThrustControl(theNode);


			/* MOVE IT */
			
	DoPlayerJetMovement(theNode);

			
			/* SEE IF ATTACK */
			
	CheckIfMeAttack(theNode);


			/* LOSE FUEL */
			
	oldFuel = gFuel;
	gFuel -= gFramesPerSecondFrac * FUEL_LOSS_RATE;	
	if (gFuel <= 0.0f)
	{
		gFuel = 0;
		gInfobarUpdateBits |= UPDATE_FUEL;
		StopJetPack(theNode);
	}
	if ((int)gFuel != oldFuel)								// only update infobar if changed by integer value 
		gInfobarUpdateBits |= UPDATE_FUEL;

				/* COLLISION */
				
	if (DoPlayerCollisionDetect(theNode))
		goto update;

	
			/* UPDATE IT */
update:			
	UpdatePlayer(theNode);
}



/************************ MOVE PLAYER: DEATH **************************/

static void MovePlayer_Death(ObjNode *theNode)
{
			/* MOVE PLAYER */

	theNode->TerrainAccel.x = theNode->TerrainAccel.y = 0;
	gDelta.x = gDelta.z = 0;

	DoFrictionAndGravity(theNode,PLAYER_FRICTION_ACCEL);						// do friction
	DoPlayerMovement(theNode);
	
	if (DoPlayerCollisionDetect(theNode))
		goto update;
	
			/* UPDATE IT */
update:			
	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_DEATH)							// make sure stays dead!
		SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_DEATH);

	UpdatePlayer(theNode);
}


/************************ MOVE PLAYER: EXIT **************************/

static void MovePlayer_Exit(ObjNode *theNode)
{
TQ3Point3D	oldMyCoord;

			/* MOVE UP PORTAL */

	if (theNode->Flag[0]) 
	{
		gDelta.y += gFramesPerSecondFrac * 100;
		gCoord.y += gDelta.y * gFramesPerSecondFrac;
		theNode->ExitTimer += gFramesPerSecondFrac;				// change transparency value
		if (theNode->ExitTimer > 5)
		{
			gGameOverFlag = true;
			gWonGameFlag = true;
		}
	}
	else
	{
		gDelta.x = gDelta.y = gDelta.z;	
	}	
	
	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_EXIT)							// make sure stays exited
		SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_EXIT);

	oldMyCoord = gMyCoord;
	UpdatePlayer(theNode);
	gMyCoord = oldMyCoord;								// keep camera fixed on old location
}




/************************ UPDATE PLAYER ***************************/

static void UpdatePlayer(ObjNode *theNode)
{

		/* SEE IF ENTRY PORTAL SHOULD STOP */
		
	if (gMyTimePortal)
	{
		if (!(theNode->StatusBits & STATUS_BIT_HIDDEN))
		{
			if (theNode->StatusBits & STATUS_BIT_ONGROUND)
			{
				DeleteObject(gMyTimePortal);
				gMyTimePortal = nil;
			}
		}
	}
	else
		gMyCoord = gCoord;							// only if no portal so camera wont track me
		
	UpdateObject(theNode);
	
			/* CHECK FOR EASTER EGG */
			
	if (gMyCoord.y > 1500)						// while riding birdie does this
	{
		GetCheatWeapons();
		GetHealth(1);
		StartMyShield(gPlayerObj);
		gFuel = MAX_FUEL_CAPACITY;
		gInfobarUpdateBits |= UPDATE_FUEL;
	}
}



/******************** DO PLAYER COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

static Boolean DoPlayerCollisionDetect(ObjNode *theNode)
{
short	i;
ObjNode	*hitObj;
unsigned long	ctype;
float		y;
UInt8		sides;

			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			
	if (gPlayerGotKilledFlag)
		sides = HandleCollisions(theNode, CTYPE_MISC|CTYPE_BGROUND);	
	else
		sides = HandleCollisions(theNode, PLAYER_COLLISION_CTYPE);


			/* SEE IF NEED TO SET GROUND FLAG */
			
	if (sides & SIDE_BITS_BOTTOM)
		theNode->StatusBits |= STATUS_BIT_ONGROUND;	


			/* MAKE SURE I DIDN'T GET PUSHED UNDERGROUND */
			
	y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z) + DIST_FROM_ORIGIN_TO_FEET;
	if (gCoord.y < y)
		gCoord.y = y;


			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/
			
	for (i=0; i < gNumCollisions; i++)						
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision
			ctype = hitObj->CType;								// get collision ctype from hit obj
					
				/*******************/
				/* CHECK FOR ENEMY */
				/*******************/
				
			if (ctype & CTYPE_ENEMY)
			{
				hitObj->Delta.x = hitObj->Delta.z = 0;
				if (hitObj->CType & CTYPE_HURTIFTOUCH)			// see if enemy is deadly to touch
					PlayerGotHurt(theNode,hitObj->Damage,true,false);
			}
			
				/**********************/
				/* SEE IF HURTME ITEM */
				/**********************/
				
			else
			if (ctype & CTYPE_HURTME)
			{
				PlayerGotHurt(theNode,hitObj->Damage,true,false);
				
					/* SEE IF WAS DINO SPIT */
					
				if ((hitObj->Group == GLOBAL_MGroupNum_DinoSpit) && (hitObj->Type == GLOBAL_MObjType_DinoSpit))
				{
					QD3D_ExplodeGeometry(hitObj, 100, 0, 10, .5);
					DeleteObject(hitObj);
				}
			}			
		}
	}
	
		/********************************/
		/* CHECK FOR SPECIAL PATH TILES */
		/********************************/

	gMyLatestPathTileNum = GetPathTileNum(gCoord.x, gCoord.z);		// get path tile under there	
	gMyLatestTileAttribs = GetTileAttribs(gCoord.x, gCoord.z);		// get tile attributes here (before move)
		
	if (gMyHeightOffGround < 1.0f)									// must be on terrain for these checks
	{
			/* SEE IF ON LAVA */
			
		if (gMyLatestTileAttribs & TILE_ATTRIB_LAVA)
		{
			float	x,z;

			PlayerGotHurt(theNode,.05,false,true);
			gLavaSmokeCounter += gFramesPerSecondFrac;
			if (gLavaSmokeCounter > .08f)
			{
				gLavaSmokeCounter = 0;
				x = (RandomFloat() - .5f) * 40.0f + gCoord.x;
				z = (RandomFloat() - .5f) * 40.0f + gCoord.z;
				MakeSmokePuff(x, gCoord.y, z, .1);					
			}
		}
	}

	return(false);
}


/************************** DO MY KICK COLLISION *****************************/
//
// Called during kick animation to see if foot has hit anything important.
//

static Boolean DoMyKickCollision(ObjNode *theNode)
{
TQ3Point3D	footCoord;
short		n,i;
ObjNode		*hitObj;
UInt32		ctype;

				/* CALC COORD OF FOOT */

	FindCoordOfJoint(theNode, MY_FOOT_JOINT_NUM, &footCoord);

	n = DoSimplePointCollision(&footCoord, CTYPE_MISC+CTYPE_ENEMY);
	for (i=0; i < n; i++)
	{
		hitObj = gCollisionList[i].objectPtr;					// get ptr to hit obj
		ctype = hitObj->CType;									// get collision ctype
		
				/* HIT AN ENEMY */
				
		if (ctype & CTYPE_ENEMY)								
		{
			EnemyGotHurt(gCollisionList[i].objectPtr, theNode, MY_KICK_DAMAGE);
		}
	}
	
	return(n);
}


/******************** PLAYER GOT HURT ***************************/

void PlayerGotHurt(ObjNode *theNode, float damage, Boolean doHurtAnim, Boolean overrideShield)
{
	if (damage == 0.0f)
		return;
		
	if (!overrideShield)
		if (gMyShield)															// see if shielded
			return;
		
	StopJetPack(theNode);													// make sure this is stopped when get hit

	if (gPlayerGotKilledFlag)												// cant get hurt if already dead
		return;
		
	if (theNode->InvincibleTimer > 0)										// cant be harmed if invincible
		return;

	if (doHurtAnim)
	{
		if (theNode->HurtTimer <= 0)
		{
			if (theNode->StatusBits & STATUS_BIT_ONGROUND)
			{
				SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_GETHURT);
				theNode->HurtTimer = HURT_DURATION;
			}
		}
		theNode->InvincibleTimer = INVINCIBILITY_DURATION;			// make me invincible for a while
	}
	else
	if (theNode->InvincibleTimer < INVINCIBILITY_DURATION_SHORT)
		theNode->InvincibleTimer = INVINCIBILITY_DURATION_SHORT;	// make me invincible for a shorter while
	
	
	gMyHealth -= damage;										// take damage
	gInfobarUpdateBits |= UPDATE_HEALTH;						// tell system to update this at end of frame
	if (gMyHealth <= 0)											// see if was killed
		KillPlayer(theNode);
}


/******************** KILL PLAYER *****************************/

static void KillPlayer(ObjNode *theNode)
{
	if (!gPlayerGotKilledFlag)
	{
		StopJetPack(theNode);
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_DEATH,1.5);
		gPlayerGotKilledFlag = true;
	}
}


/*********************** START MY SHIELD **************************/

void StartMyShield(ObjNode *theNode)
{
	gShieldTimer = SHIELD_TIME;

		/* MAKE SHIELD GEOMETRY */
		
	if (gMyShield == nil)
	{
		gNewObjectDefinition.group = GLOBAL_MGroupNum_Shield;	
		gNewObjectDefinition.type = GLOBAL_MObjType_Shield;	
		gNewObjectDefinition.coord = gMyCoord;
		gNewObjectDefinition.flags = STATUS_BIT_KEEPBACKFACES;
		gNewObjectDefinition.slot = SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall = MoveShield;
		gNewObjectDefinition.rot = 0;
		gNewObjectDefinition.scale = 3.0;
		gMyShield = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (gMyShield)
		{
			MakeObjectTransparent(gMyShield,.3);
		}
	}
	
	if (gShieldChannel == -1)
		gShieldChannel = PlayEffect_Parms(EFFECT_SHIELD,150,kMiddleC);

}


/******************** MOVE SHIELD ***********************/

static void MoveShield(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF TIMED OUT */
			
	gShieldTimer -= fps;
	if (gShieldTimer <= 0.0f)
	{
		DeleteObject(gMyShield);
		gMyShield = nil;
		if (gShieldChannel != -1)
		{
			StopAChannel(&gShieldChannel);
		}
		return;
	}


		/* UPDATE POSITION */
		
	theNode->Coord = gMyCoord;
	theNode->Rot.y += fps * 6.0f;
	theNode->Rot.z += fps * 9.0f;
	
	UpdateObjectTransforms(theNode);
}





