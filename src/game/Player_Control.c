/****************************/
/*   	PLAYER_Control.C    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>

#include "globals.h"
#include "3dmath.h"
#include "objects.h"
#include "misc.h"
#include "skeletonanim.h"
#include "collision.h"
#include "player_control.h"
#include "sound2.h"
#include "main.h"
#include "file.h"
#include "input.h"
#include "myguy.h"
#include "terrain.h"
#include "effects.h"
#include "infobar.h"
#include "mobjtypes.h"
#include "skeletonJoints.h"
#include "camera.h"
#include "pickups.h"

extern	TQ3ViewObject			gGameViewObject;
extern	ObjNode		*gCurrentNode,*gInventoryObject;
extern	float		gFramesPerSecondFrac,gFramesPerSecond,gFuel;
extern	Byte		gCameraMode;
extern	TQ3Point3D		gCoord;
extern	TQ3Vector3D		gDelta;
extern	Byte			gNumPlayers;
extern	TQ3Vector3D		gRecentTerrainNormal;
extern	TQ3Matrix4x4	gRecentJointFullMatrix;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	u_short			gMyLatestPathTileNum,gMyLatestTileAttribs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static KeyControlType KeysToControlBits(void);
static pascal Boolean KeySetupDialogCallback (DialogPtr dp,EventRecord *event, short *item);
static void MakeSpeedPuff(void);
static void StartJetPack(ObjNode *theNode);
static void MoveJetPackFlame(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SPEED_PUFF_RATE		.085f

#define	SLOPE_ACCEL_VALUE	780.0f					// force from hills

#define	JETPACK_INITIAL_YBOOST	20

#define	MAX_JET_THRUST		2.5f

#define	MAX_JET_HEIGHT		((255.0f * HEIGHT_EXTRUDE_FACTOR) - 80.0f)

/*********************/
/*    VARIABLES      */
/*********************/

short			gJetSoundChannel = -1;
KeyControlType	gMyControlBits;


short	gCurrentSelectedTextEditItem;

float	gMostRecentCharacterFloorY,gMyHeightOffGround;

float	gMySpeedPuffCounter;

#define	ExhaustTimer	SpecialF[0]


/****************** CALC PLAYER KEY CONTROLS *********************/
//
// Converts keys to key control bits or reads the bit fields from the network.
//
// The only reason to do this instead of just calling GetKeyState directly is
// so it'll port to InputSprockets easier.
//

void CalcPlayerKeyControls(void)
{
	gMyControlBits = KeysToControlBits();		// calc control bits for me
}


/******************* KEYS TO CONTROL BITS *********************/
//
// Converts keyboard keys into control bits.
//
// OUTPUT:	KeyControlType = control bits
//

static KeyControlType KeysToControlBits(void)
{			
KeyControlType	bits;
	
	bits = 0;							// init to 0

	if (GetKeyState(kKey_TurnLeft))		// rot left
		bits |= KEYCONTROL_ROTLEFT;
	else
	if (GetKeyState(kKey_TurnRight))	// rot right
		bits |= KEYCONTROL_ROTRIGHT;
				
	if (GetKeyState(kKey_Forward))		// forward
		bits |= KEYCONTROL_FORWARD;
	else
	if (GetKeyState(kKey_Backward))		// backward
		bits |= KEYCONTROL_BACKWARD;
	
	if (GetNewKeyState(kKey_Jump))		// jump
		bits |= KEYCONTROL_JUMP;
	else
	if (GetKeyState(kKey_Attack))		// attack
		bits |= KEYCONTROL_ATTACK;
	else
	if (GetKeyState(kKey_JetUp))		// Jet Up
		bits |= KEYCONTROL_JETUP;
	else
	if (GetKeyState(kKey_JetDown))		// Jet Down
		bits |= KEYCONTROL_JETDOWN;
	
	if (GetNewKeyState(kKey_AttackMode))	// attack mode change
		bits |= KEYCONTROL_ATTACKMODE;

	if (GetNewKeyState(kKey_PickUp))	// try pickup
		bits |= KEYCONTROL_PICKUP;
		

	return(bits);
}


/**************** DO PLAYER CONTROL ***************/
//
// Moves a player based on its control bit settings.
// These settings are already set either by keyboard interpretation or reading off the network.
//
// INPUT:	theNode = the node of the player
//			slugFactor = how much of acceleration to apply (varies if jumping et.al)
//


void DoPlayerControl(ObjNode *theNode, float slugFactor)
{
KeyControlType	bits; 
Byte		currentAnim;

	if (theNode->JetThrust)							// shouldn't be here if jetthrust is going
		return;

	currentAnim = theNode->Skeleton->AnimNum;

	bits = gMyControlBits;					// get player control bits


			/******************************/
			/* SEE IF ROTATE PLAYER LEFT  */
			/******************************/
				
	if (bits & KEYCONTROL_ROTLEFT)
	{
		theNode->Rot.y += 2.0f*slugFactor*gFramesPerSecondFrac;				
		
			/* IF SLOW ENOUGH, THEN DO LEFT TURN ANIM */
			
		if (fabs(theNode->Speed) < 210.0f)					
		{
			if (currentAnim != PLAYER_ANIM_TURNLEFT)
				if (theNode->StatusBits & STATUS_BIT_ONGROUND)					// must be on ground
					if (gCameraMode != CAMERA_MODE_FIRSTPERSON)					// dont do it in 1st person camera mode
						MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_TURNLEFT,6);
		}
		
			/* TOO FAST, SEE IF NEED TO STOP TURN ANIM */
			
		else
		if ((currentAnim == PLAYER_ANIM_TURNLEFT) || (currentAnim == PLAYER_ANIM_TURNRIGHT)) 
			MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_WALK,3);
	}
	
			/******************************/
			/* SEE IF ROTATE PLAYER RIGHT */
			/******************************/
	
	else
	if (bits & KEYCONTROL_ROTRIGHT)
	{
		theNode->Rot.y -= 2.0f*slugFactor*gFramesPerSecondFrac;

			/* IF SLOW ENOUGH, THEN DO RIGHT TURN ANIM */
			
		if (fabs(theNode->Speed) < 210.0f)					
		{
			if (currentAnim != PLAYER_ANIM_TURNRIGHT)
				if (theNode->StatusBits & STATUS_BIT_ONGROUND)					// must be on ground
					if (gCameraMode != CAMERA_MODE_FIRSTPERSON)					// dont do it in 1st person camera mode
						MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_TURNRIGHT,4);
		}
		
			/* TOO FAST, SEE IF NEED TO STOP TURN ANIM */
			
		else
		if ((currentAnim == PLAYER_ANIM_TURNLEFT) || (currentAnim == PLAYER_ANIM_TURNRIGHT)) 
			MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_WALK,3);
	}
	
			/* NOT TURNING, SO SEE IF SHOULD STOP TURN ANIM */
	else
	if ((currentAnim == PLAYER_ANIM_TURNLEFT) ||
		(currentAnim == PLAYER_ANIM_TURNRIGHT)) 
		MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND,3);




				/*******************/
				/* SEE IF MAKE RUN */
				/*******************/
				
	if (bits & KEYCONTROL_FORWARD)							// forward
	{
		theNode->Accel = (PLAYER_WALK_ACCEL*slugFactor);
		if (theNode->Speed < 40)							// see if jump-start if going slow
			theNode->Accel *= 2;
	}
	else
	if (bits & KEYCONTROL_BACKWARD)							// backward
		theNode->Accel = (-PLAYER_WALK_ACCEL*slugFactor);
	else
		theNode->Accel = 0;
	
	
	
			/*****************************/
			/* SEE IF CHANGE ATTACK MODE */
			/*****************************/
			
	if (bits & KEYCONTROL_ATTACKMODE)
		NextAttackMode();
	
	
	
			/*****************/
			/* SEE IF JUMP 	 */
			/*****************/

	if (bits & KEYCONTROL_JUMP)
	{
		if ((currentAnim == PLAYER_ANIM_JUMP1) || (currentAnim == PLAYER_ANIM_FALLING))	// see if this is the double-jump
		{
			if ((gDelta.y < 450.0f) && (gDelta.y > -300.0f))							// only double-jump at apex of other jump
			{
				gDelta.y = PLAYER_JUMP_SPEED;
				MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_JUMP2,4.0);
				PlayEffect_Parms(EFFECT_JUMP,FULL_CHANNEL_VOLUME/2,kMiddleC+4);
			}
		}
		else
		if (currentAnim != PLAYER_ANIM_JUMP2)								// otherwise its the initial jump
		{
			if (theNode->StatusBits & STATUS_BIT_ONGROUND)					// must be on ground
			{
				gDelta.y = PLAYER_JUMP_SPEED;
				SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_JUMP1);
				PlayEffect_Parms(EFFECT_JUMP,FULL_CHANNEL_VOLUME/2,kMiddleC);
			}
		}
	}

			/*********************/
			/* SEE IF TRY PICKUP */
			/*********************/
	
	if (bits & KEYCONTROL_PICKUP)
	{
		/* IF ALREADY HOLDING A PICKUP, THEN THROW IT */
		
		if (theNode->StatusBits & STATUS_BIT_ISCARRYING)
		{
			if (theNode->JetThrust)								// if jetting, then just drop it
				DropItem(theNode);		
			else
				SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_THROW);
		}
		
			/* OTHERWISE SEE IF WE SHOULD ATTEMPT A PICKUP */
			
		else
		{		
			if (theNode->StatusBits & STATUS_BIT_ONGROUND)					// must be on ground
				SetSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_PICKUP);
		}
	}
}


/**************** DO PLAYER JET CONTROL ***************/
//
// Handles Motion of player when jetpack is active.
// Note, it doesnt handle thrust control with is in the function listed
// later.
//


void DoPlayerJetControl(ObjNode *theNode)
{
KeyControlType	bits; 
Byte		currentAnim;

	currentAnim = theNode->Skeleton->AnimNum;

	bits = gMyControlBits;											// get player control bits

			/* SEE IF ROTATE PLAYER LEFT  */
				
	if (bits & KEYCONTROL_ROTLEFT)
		theNode->Rot.y += 2*gFramesPerSecondFrac;						
	
			/* SEE IF ROTATE PLAYER RIGHT */
	
	else
	if (bits & KEYCONTROL_ROTRIGHT)
		theNode->Rot.y -= 2*gFramesPerSecondFrac;


			/* SEE IF FORWARD/BACKWARD */
				
	if (bits & KEYCONTROL_FORWARD)							// forward
		theNode->Accel = (PLAYER_WALK_ACCEL * .3);
	else
	if (bits & KEYCONTROL_BACKWARD)							// backward
		theNode->Accel = (-PLAYER_WALK_ACCEL * .3);
	else
		theNode->Accel = 0;

	
			/* SEE IF CHANGE ATTACK MODE */
			
	if (bits & KEYCONTROL_ATTACKMODE)
		NextAttackMode();	
}




/********************** CHECK JET THRUST CONTROL **************************/
//
// Specialized control handler for checking jetpack thrust controls.
//

void CheckJetThrustControl(ObjNode *theNode)
{
KeyControlType	bits; 

	bits = gMyControlBits;						// get player control bits

	if (bits & KEYCONTROL_JETUP)				// see if increase thrust
	{
		if (theNode->JetThrust <= 0.0f)			// see if just started it
			StartJetPack(theNode);
		else
		if (theNode->JetThrust < MAX_JET_THRUST)
			theNode->JetThrust += gFramesPerSecondFrac;
	}
	else
	if (bits & KEYCONTROL_JETDOWN)				// see if decrease thrust
	{
		if (theNode->JetThrust > 0.0f)				// see if stop thrust
		{
			theNode->JetThrust -= gFramesPerSecondFrac;
			if (theNode->JetThrust <= 0.0f)
				StopJetPack(theNode);
		}
	}
}


/******************* DO PLAYER MOVEMENT *********************/
//
// Handles the gravity & velocity movment of the input character.
//
// OUTPUT: True if on ground
//

void DoPlayerMovement(ObjNode *theNode)
{
Boolean	tryPuff = false;
float	r,dx,dz,acc,fps;
float	sumDX,sumDY,sumDZ,maxSpeed;

	fps = gFramesPerSecondFrac;

			/* CALC AIM VECTOR */
			
	r = theNode->Rot.y;
	acc = theNode->Accel * fps;
	
	dx = -sin(r) * acc;										// calc walking vector
	dz = -cos(r) * acc;
	
	
			/* APPLY TERRAIN SLOPE VECTOR */
			
	if (gMyLatestPathTileNum & PATH_TILE_SLOPEFORCE)		// see if apply extremely heavy force
	{
		dx += theNode->TerrainAccel.x * fps * 10.0f;		// apply terrain slope vector
		dz += theNode->TerrainAccel.y * fps * 10.0f;
	}
	else
	{
		dx += theNode->TerrainAccel.x * fps;				// apply terrain slope vector
		dz += theNode->TerrainAccel.y * fps;
	}


			/* DO X/Z ACCELERATION */
			
	gDelta.x += dx;
	gDelta.z += dz;
	
				/* CALC SPEED */
				
	if ((gMyLatestTileAttribs & (TILE_ATTRIB_WATER|TILE_ATTRIB_LAVA)) && (theNode->StatusBits & STATUS_BIT_ONGROUND))
		maxSpeed = PLAYER_MAX_WALK_SPEED / 2;
	else
		maxSpeed = PLAYER_MAX_WALK_SPEED;
		
	theNode->Speed = sqrt(gDelta.x * gDelta.x + gDelta.z * gDelta.z);	// calc speed
	if (theNode->Speed > maxSpeed)										// check max/min speeds
	{
		tryPuff = true;
		r = maxSpeed/theNode->Speed;									// get 100-% over
		gDelta.x *= r;													// adjust back to max
		gDelta.z *= r;
		theNode->Speed = maxSpeed;
	}



				/* CALC SUM DELTAS */
				
	if (theNode->PlatformNode)										// include MPlatform
	{
		ObjNode	*pNode = theNode->PlatformNode;
		
		sumDX = gDelta.x + pNode->Delta.x;
		sumDZ = gDelta.z + pNode->Delta.z;
		sumDY = gDelta.y + pNode->Delta.y;
		
		theNode->Rot.y += pNode->RotDelta.y*fps;					// also apply it's rot
	}
	else
	{
		sumDX = gDelta.x;
		sumDY = gDelta.y;
		sumDZ = gDelta.z;
	}
	
	
			/* MOVE IT */
	
	gCoord.y += sumDY*fps;	
	gCoord.x += sumDX*fps;
	gCoord.z += sumDZ*fps;

	gMostRecentCharacterFloorY = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);		// get center Y


			/********************/
			/* SEE IF ON GROUND */
			/********************/

	gMyHeightOffGround = (gCoord.y-DIST_FROM_ORIGIN_TO_FEET) - gMostRecentCharacterFloorY;
			
	if (gMyHeightOffGround <= 0.001f)
	{
		gCoord.y = gMostRecentCharacterFloorY+DIST_FROM_ORIGIN_TO_FEET;
		gDelta.y = -GRAVITY_CONSTANT*fps;							// always keep a little dy force down
		gMyHeightOffGround = 0;
		theNode->StatusBits |= STATUS_BIT_ONGROUND;	
		
					/* PUSH DOWN HILL */
					//
					// this accel isnt used until next frame
					//
					// also note that as normal.y gets smaller, accel gets bigger.
					//
					
		if (gRecentTerrainNormal.y > .85f)			// if fairly flat, then no sliding effect
		{
			theNode->TerrainAccel.x = theNode->TerrainAccel.y = 0;
		}
		else
		{
			theNode->TerrainAccel.x = gRecentTerrainNormal.x * SLOPE_ACCEL_VALUE * (2.2f - gRecentTerrainNormal.y);
			theNode->TerrainAccel.y = gRecentTerrainNormal.z * SLOPE_ACCEL_VALUE * (2.2f - gRecentTerrainNormal.y);				
		}
	}
	else
	{
		theNode->StatusBits &= ~STATUS_BIT_ONGROUND;			// assume not on anything solid
		theNode->TerrainAccel.x = theNode->TerrainAccel.y = 0;
	}
	
	
			/* SEE IF SHOULD MAKE PUFF */
			
	if (tryPuff)
		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
			if (theNode->InvincibleTimer <= 0.0f)
				if (gMyLatestTileAttribs & TILE_ATTRIB_MAKEDUST)
					MakeSpeedPuff();
}



/******************* DO PLAYER JET MOVEMENT *********************/
//
// Handles motion updating for player while jetting.
//

void DoPlayerJetMovement(ObjNode *theNode)
{
float	r,dx,dz,dy,acc;
float	targetHeight,diff,fps;

	fps = gFramesPerSecondFrac;											// get register copy of this

	theNode->StatusBits &= ~STATUS_BIT_ONGROUND;						// assume not on anything solid

			/* CALC AIM VECTOR */
			
	r = theNode->Rot.y;
	acc = theNode->Accel * fps;
	
	dx = -sin(r) * acc;													// calc flying vector
	dz = -cos(r) * acc;


			/* DO X/Z ACCELERATION */
			
	dx = gDelta.x + dx;
	dz = gDelta.z + dz;
	
				/* DO AIR FRICTION */
				
	if (dx > 0)
	{
		dx -= fps * 100;
		if (dx < 0)
			dx = 0;
	}
	else
	if (dx < 0)
		dx += fps * 100;

	if (dz > 0)
	{
		dz -= fps * 100;
		if (dz < 0)
			dz = 0;
	}
	else
	if (dz < 0)
		dz += fps * 100;

	
				/* CALC SPEED */
				
	theNode->Speed = CalcQuickDistance(0,0,dx,dz);						// calc speed
	if (theNode->Speed > PLAYER_MAX_FLY_SPEED)							// check max/min speeds
	{
		r = PLAYER_MAX_FLY_SPEED/theNode->Speed;						// get 100-% over
		dx *= r;														// adjust back to max
		dz *= r;	
		theNode->Speed = PLAYER_MAX_FLY_SPEED;
	}


	
			/* MOVE IT */
	
	gCoord.x += dx*fps;
	gCoord.z += dz*fps;

	gMostRecentCharacterFloorY = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);// get center Y


			/* DEAL WITH Y */

	targetHeight = 	gMostRecentCharacterFloorY + (theNode->JetThrust * 300.0f) + 100.0f;	// calc desired hover height	
	if (targetHeight > MAX_JET_HEIGHT)
		targetHeight = MAX_JET_HEIGHT;	
	
	diff = targetHeight - gCoord.y;													// calc dist to hover height
	dy =  diff * 2.0f;																// accel toward target height
	
	gCoord.y += dy * fps;
			


			/********************/
			/* SEE IF ON GROUND */
			/********************/

	gMyHeightOffGround = (gCoord.y-DIST_FROM_ORIGIN_TO_FEET) - gMostRecentCharacterFloorY;
			



			/* UPDATE GLOBALS */
			
	gDelta.y = dy;
	gDelta.x = dx;
	gDelta.z = dz;
}



/************************** MAKE SPEED PUFF *****************************/

static void MakeSpeedPuff(void)
{
short	i;

	gMySpeedPuffCounter += gFramesPerSecondFrac;
	i = gMySpeedPuffCounter / SPEED_PUFF_RATE;
	if (i > 0)
	{
		gMySpeedPuffCounter -= (float)i * SPEED_PUFF_RATE;
		while(i--)
			MakeDustPuff(gCoord.x, gCoord.y-DIST_FROM_ORIGIN_TO_FEET+10, gCoord.z, .3);
	}
}



/************************ DO FRICTION & GRAVITY ****************************/
//
// Applies friction to the gDeltas
//

void DoFrictionAndGravity(ObjNode *theNode, float friction)
{
TQ3Vector2D	v;
float	x,z;

			/* DO FRICTION */
			
	friction *= gFramesPerSecondFrac;							// adjust friction
	
	v.x = gDelta.x;
	v.y = gDelta.z;
	Q3Vector2D_Normalize(&v,&v);								// get motion vector
	x = -v.x * friction;										// make counter-motion vector
	z = -v.y * friction;
		
	if (gDelta.x < 0.0f)										// decelerate against vector
	{
		gDelta.x += x;
		if (gDelta.x > 0.0f)									// see if sign changed
			gDelta.x = 0;											
	}
	else
	if (gDelta.x > 0.0f)									
	{
		gDelta.x += x;
		if (gDelta.x < 0)								
			gDelta.x = 0;											
	}
	
	if (gDelta.z < 0.0f)								
	{
		gDelta.z += z;
		if (gDelta.z > 0)								
			gDelta.z = 0;											
	}
	else
	if (gDelta.z > 0.0f)									
	{
		gDelta.z += z;
		if (gDelta.z < 0)								
			gDelta.z = 0;											
	}
		
			// see if pretty much stopped
			
	if (fabs(gDelta.x) < (200.0f * gFramesPerSecondFrac))
		gDelta.x = 0;					
	if (fabs(gDelta.z) < (200.0f * gFramesPerSecondFrac))
		gDelta.z = 0;

	if ((gDelta.x == 0.0f) && (gDelta.z == 0.0f))
	{
		theNode->Speed = 0;
	}

			/* DO GRAVITY */
			
	gDelta.y -= GRAVITY_CONSTANT*gFramesPerSecondFrac;			// add gravity

	if (gDelta.y < 0.0f)											// if falling, keep dy at least -1.0 to avoid collision jitter on platforms
		if (gDelta.y > (-1.0f * gFramesPerSecond))
			gDelta.y = (-1.0f * gFramesPerSecond);

}


/************************ START JET PACK *****************************/

static void StartJetPack(ObjNode *theNode)
{
ObjNode	*newObj,*newObj2;

	if (gFuel <= 0)											// dont let this happen if I'm out of fuel!
		return;

	theNode->JetThrust += gFramesPerSecondFrac;
	MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_JETNEUTRAL, 6);
	gDelta.y = JETPACK_INITIAL_YBOOST;
	
			/*********************/
			/* CREATE JET FLAMES */
			/*********************/
			
			/* MAKE RIGHT FLAME */
	
	gNewObjectDefinition.group = GLOBAL_MGroupNum_JetFlame;	
	gNewObjectDefinition.type = GLOBAL_MObjType_JetFlame;	
	gNewObjectDefinition.flags = STATUS_BIT_DONTCULL | STATUS_BIT_NOCOLLISION | STATUS_BIT_NOTRICACHE;
	gNewObjectDefinition.slot = SLOT_OF_DUMB+10;				// after me
	gNewObjectDefinition.moveCall = MoveJetPackFlame;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return;
	
	MakeObjectTransparent(newObj, .5);							// make transparent
	MakeObjectKeepBackfaces(newObj);							// also keep backfaces
	
	theNode->JetFlameObj = (u_long)newObj;						// point to this flame
	theNode->ExhaustTimer = 0;									// init exhaust timer

		/* MAKE LEFT FLAME */	
	
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall = nil;
	newObj2 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj2 == nil)
		return;
	
	MakeObjectTransparent(newObj2, .5);							// make transparent
	
	newObj->JetFlameObj = (u_long)newObj2;						// other flame points to this flame	
	newObj2->JetFlameObj = (u_long)theNode;						// this flame points back to player
	
			/* JET SOUND */
			
	gJetSoundChannel = PlayEffect_Parms(EFFECT_JETLOOP,230,kMiddleC);
}


/*********************** STOP JET PACK *****************************/

void StopJetPack(ObjNode *theNode)
{
ObjNode	*flameObj,*flameObj2;

	if (gJetSoundChannel != -1)										// stop jet sound
	{
		StopAChannel(&gJetSoundChannel);
	}

	if (theNode->JetThrust == 0)
		return;

	theNode->JetThrust = 0;
	MorphToSkeletonAnim(theNode->Skeleton,PLAYER_ANIM_STAND, 5);
	
		/* KILL FLAME */
		
	if (theNode->JetFlameObj)
	{
		flameObj = (ObjNode *)theNode->JetFlameObj;					// point to right flame
		flameObj2 = (ObjNode *)flameObj->JetFlameObj;				// point to left flame
		DeleteObject(flameObj);										// delete right flame
		theNode->JetFlameObj = nil;	
		if (flameObj2)
			DeleteObject(flameObj2);								// delete left flame		
	}
}


/********************* MOVE JET PACK FLAME *********************/

static void MoveJetPackFlame(ObjNode *theNode)
{
ObjNode	*playerObj,*leftObj;
TQ3Matrix4x4	mat,transMat,scaleMat,jointMat;
static const TQ3Point3D pt = {0,39,55};
ObjNode	*dustObj;
TQ3Point3D	pt2;

	leftObj = (ObjNode *)theNode->JetFlameObj;					// point to left flame
	playerObj = (ObjNode *)leftObj->JetFlameObj;				// get ptr to player


				/* HANDLE RIGHT FLAME */
				
	Q3Matrix4x4_SetScale(&scaleMat, .45, .45, RandomFloat()+.5);			// build scale matrix
	Q3Matrix4x4_SetTranslate(&transMat, 10,34,37);							// build trans matrix to jet nozzle
	FindJointFullMatrix(playerObj, MYGUY_LIMB_BODY, &jointMat);				// get transform matrix for joint
	Q3Matrix4x4_Multiply(&scaleMat,&transMat,&mat);							// concat the matrices
	Q3Matrix4x4_Multiply(&mat,&jointMat,&theNode->BaseTransformMatrix);		// concat the matrices
	SetObjectTransformMatrix(theNode);										// udpate as object's new matrix


				/* HANDLE LEFT FLAME */
				
	scaleMat.value[2][2] = RandomFloat()+.5;								// modify scale matrix
	transMat.value[3][0] = -10;												// modify trans matrix
	Q3Matrix4x4_Multiply(&scaleMat,&transMat,&mat);							// concat the matrices
	Q3Matrix4x4_Multiply(&mat,&jointMat,&leftObj->BaseTransformMatrix);		// concat the matrices
	SetObjectTransformMatrix(leftObj);										// udpate as object's new matrix
	
	
				/* MAKE EXHAUST */
				
	theNode->ExhaustTimer += gFramesPerSecondFrac;
	if (theNode->ExhaustTimer > .05)
	{
		theNode->ExhaustTimer = 0;
		
		Q3Point3D_Transform(&pt, &jointMat, &pt2);							// calc coord to put exhaust

		dustObj = MakeDustPuff(pt2.x, pt2.y, pt2.z, .15);					// make exhaust
		dustObj->Delta.x = playerObj->Delta.x;			
		dustObj->Delta.z = playerObj->Delta.z;
		dustObj->Delta.y = -200;
	}
	
	
			/* UPDATE JET FREQUENCY */

	if (gJetSoundChannel != -1)
		ChangeChannelFrequency(gJetSoundChannel, 0x10000 - 0x4000 + (playerObj->JetThrust * 7000.0f));
}

















