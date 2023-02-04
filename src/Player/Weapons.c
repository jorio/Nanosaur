/****************************/
/*   	MY ATTACK.C			*/
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

static void ShootSonicScream(ObjNode *theNode);
static void MoveSonicScream(ObjNode *theNode);
static void MoveBlaster(ObjNode *theNode);
static void ShootBlaster(ObjNode *theNode);
static void ShootHeatSeek(ObjNode *theNode);
static void MoveHeatSeek(ObjNode *theNode);
static void MoveHeatSeekEcho(ObjNode *theNode);
static Boolean DoBulletCollisionDetect(ObjNode *theBullet);
static void ExplodeBullet(ObjNode *theNode);
static void MakeExplosion(TQ3Point3D *coord);
static void MoveExplosion(ObjNode *theNode);
static void ShootTriBlast(ObjNode *theNode);
static void MoveTriBlast(ObjNode *theNode);
static void MoveNuke(ObjNode *theNode);
static void ShootNuke(ObjNode *theNode);
static void DetonateNuke(ObjNode *theNode);
static void MoveNukeShockwave(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

extern const float	SONIC_SCREAM_RATE;	// bigger = faster (# shots / second)
extern const float	BLASTER_RATE;
extern const float	HEATSEEK_RATE;
extern const float	TRIBLAST_RATE;
extern const float	NUKE_RATE;

#define	SONIC_SCREAM_SPEED	500
#define	SONIC_SCREAM_DAMAGE	1.0

#define	BLASTER_SPEED		1100
#define	BLASTER_DAMAGE		.4

#define	HEATSEEK_SPEED		700
#define	HEATSEEK_DAMAGE		.8

#define	TRIBLAST_SPEED		1100
#define	TRIBLAST_DAMAGE		.3

#define	NUKE_SPEED		900
#define	NUKE_DAMAGE		.5


#define HeatSeekTurnSpeed	SpecialF[0]
#define HeatSeekPrevTrailX	SpecialF[1]
#define HeatSeekPrevTrailY	SpecialF[2]
#define HeatSeekPrevTrailZ	SpecialF[3]

static  TQ3Point3D gGunTipOffset = {0,0,-65};


/*********************/
/*    VARIABLES      */
/*********************/

short	gWeaponInventory[NUM_ATTACK_MODES];						// # bullets
Boolean	gPossibleAttackModes[NUM_ATTACK_MODES];
Byte	gCurrentAttackMode;

float	gAutoFireCounter;
float	gSonicScreamWave;


/******************** INIT WEAPONS MANAGER ***********************/
//
// Also called by ResetPlayer after death to reinit weapons selection
//

void InitWeaponManager(void)
{
short	i;

	gCurrentAttackMode = ATTACK_MODE_BLASTER;
	gAutoFireCounter = 0;
	gSonicScreamWave = 0;
	
			/* INIT ATTACK MODE POWERUPS */
			
	for (i=0; i < NUM_ATTACK_MODES; i++)
		gPossibleAttackModes[i] = false;

	gPossibleAttackModes[ATTACK_MODE_BLASTER] 	= true;
	gWeaponInventory[ATTACK_MODE_BLASTER] 		= 50;

	gPossibleAttackModes[ATTACK_MODE_HEATSEEK] 	= false;
	gWeaponInventory[ATTACK_MODE_HEATSEEK] 		= 0;	

	gPossibleAttackModes[ATTACK_MODE_SONICSCREAM] = false;
	gWeaponInventory[ATTACK_MODE_SONICSCREAM] 	= 0;	

	gPossibleAttackModes[ATTACK_MODE_TRIBLAST] = false;
	gWeaponInventory[ATTACK_MODE_TRIBLAST] 		= 0;	

	gPossibleAttackModes[ATTACK_MODE_NUKE] = false;
	gWeaponInventory[ATTACK_MODE_NUKE] 		= 0;	
	
	gInfobarUpdateBits |= UPDATE_WEAPONICON;
}


/********************* GET CHEAT WEAPONS *************************/
//
// gives me full weapons
//

void GetCheatWeapons(void)
{
	gPossibleAttackModes[ATTACK_MODE_BLASTER] 	= true;
	gWeaponInventory[ATTACK_MODE_BLASTER] 		= 999;

	gPossibleAttackModes[ATTACK_MODE_HEATSEEK] 	= true;
	gWeaponInventory[ATTACK_MODE_HEATSEEK] 		= 999;	

	gPossibleAttackModes[ATTACK_MODE_SONICSCREAM] = true;
	gWeaponInventory[ATTACK_MODE_SONICSCREAM] 	= 999;	

	gPossibleAttackModes[ATTACK_MODE_TRIBLAST] = true;
	gWeaponInventory[ATTACK_MODE_TRIBLAST] 		= 999;	

	gPossibleAttackModes[ATTACK_MODE_NUKE] = true;
	gWeaponInventory[ATTACK_MODE_NUKE] 		= 999;	
	
	gInfobarUpdateBits |= UPDATE_WEAPONICON;
}


/*********************** GET WEAPON POWERUP *************************/

void GetWeaponPowerUp(short kind, short quantity)
{
static const short quans[] =
{
	25,					// blaster
	10,					// heat seek
	50,					// sonic scream
	30,					// triblast
	1,					// Nuke
	25,
	25,
	25
};
		/* SEE IF USE DEFAULT QUANTITY */
		
	if (quantity == 0)
		quantity = quans[kind];
	
	
			/* ADD TO INVENTORY */
			
	gPossibleAttackModes[kind] = true;					// we now have this weapon
	gWeaponInventory[kind] += quantity;					// inc inventory count
	if (gWeaponInventory[kind] > 999)
		gWeaponInventory[kind] = 999;

	gInfobarUpdateBits |= UPDATE_WEAPONICON;			// tell system to update this at end of frame

}


/*********************** CHECK IF ME ATTACK ****************************/

void CheckIfMeAttack(ObjNode *theNode)
{
			/*****************/
			/* SEE IF ATTACK */
			/*****************/
		
	if ((gMyControlBits & KEYCONTROL_ATTACK) && (gWeaponInventory[gCurrentAttackMode] > 0))
	{	
			/* SEE WHICH ATTACK */
			
		switch(gCurrentAttackMode)
		{
			case	ATTACK_MODE_BLASTER:
					ShootBlaster(theNode);
					break;						

			case	ATTACK_MODE_HEATSEEK:
					ShootHeatSeek(theNode);
					break;
					
			case	ATTACK_MODE_SONICSCREAM:
					ShootSonicScream(theNode);
					break;

			case	ATTACK_MODE_NUKE:
					ShootNuke(theNode);
					break;
										
			case	ATTACK_MODE_TRIBLAST:
					ShootTriBlast(theNode);
					break;
		}
	}
	else
		gAutoFireCounter = 1.0;									// not shooting now, so ready to shoot immediately
}	
	
	
/************************** DO BULLET COLLISION DETECT ************************/
//
// OUTPUT: true = hit something
//

static Boolean DoBulletCollisionDetect(ObjNode *theBullet)
{
short	numCollisions,i;
ObjNode	*hitObj;

			/*************************/
			/* CREATE COLLISION LIST */
			/*************************/
			
		/* FIRST DO SIMPLE POINT COLLISION */
				
	numCollisions = DoSimplePointCollision(&gCoord, CTYPE_ENEMY|CTYPE_PLAYER|CTYPE_MISC|CTYPE_CRYSTAL);
	if (numCollisions == 0)
	{
		DoTriangleCollision(theBullet, CTYPE_MISC);				
		if (gNumCollisions == 0)
			return(false);
			
	}


			/* SEE WHAT WE HIT */
			
	for (i = 0; i < numCollisions; i++)
	{
		hitObj = gCollisionList[i].objectPtr;							// get objnode of collision
		if (hitObj)
		{
				/* BULLET HIT AN ENEMY */
				
			if (hitObj->CType & CTYPE_ENEMY)
			{
				EnemyGotHurt(hitObj, theBullet, theBullet->Damage);		// hurt the enemy
			}
			
				/* BULLET HIT A CRYSTAL */
				
			else
			if (hitObj->CType & CTYPE_CRYSTAL)
			{
				ExplodeCrystal(hitObj);
			}
			break;
		}
	}
	
	
			/* EXPLODE THE BULLET */
			
	ExplodeBullet(theBullet);
	return(true);
}
	
/****************** EXPLODE BULLET *********************/

static void ExplodeBullet(ObjNode *theNode)
{
float	d;
long	volume;

			/* SPECIAL IF NUKE */
			
	if (theNode->Kind == ATTACK_MODE_NUKE)
	{
		DetonateNuke(theNode);
		return;
	}
	
			/* SPECIAL IF SONIC SCREAM */
			
	else
	if (theNode->Kind == ATTACK_MODE_SONICSCREAM)
	{
		DeleteObject(theNode);
		return;
	}


			/* GENERIC */
			
	DeleteObject(theNode);
	MakeExplosion(&theNode->Coord);

	d = CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z);	// calc volume of explosion based on distance
	volume = FULL_CHANNEL_VOLUME - (long)(d * .15);
	if (volume > 0)
		PlayEffect_Parms(EFFECT_EXPLODE,volume,kMiddleC);							// play sound

}	


/********************** MAKE EXPLOSION *****************************/

static void MakeExplosion(TQ3Point3D *coord)
{
ObjNode *newObj;

	gNewObjectDefinition.coord = *coord;
	gNewObjectDefinition.group = GLOBAL_MGroupNum_Explosion;	
	gNewObjectDefinition.type = GLOBAL_MObjType_Explosion;	
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;	
	gNewObjectDefinition.moveCall = MoveExplosion;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj)
	{
		newObj->Health = .9;
		MakeObjectTransparent(newObj, newObj->Health);					// make transparent
	}	
}


/********************* MOVE EXPLOSION ***********************/

static void MoveExplosion(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Health -= 3.0 * fps;								// decay it
	if (theNode->Health <= 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	MakeObjectTransparent(theNode, theNode->Health);
	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 20;
	UpdateObjectTransforms(theNode);
}


	
	
/****************** SHOOT SONIC SCREAM ***********************/

static void ShootSonicScream(ObjNode *theNode)
{
ObjNode	*newObj;
float	r,fps;

	fps = gFramesPerSecondFrac;

	/* SEE IF SPEW RATE IT GOOD */

	gAutoFireCounter += fps * SONIC_SCREAM_RATE;
	if (gAutoFireCounter >= 1.0)
	{
		gAutoFireCounter -= 1.0;
	
						/* CALC START COORD */
						
		FindCoordOnJoint(theNode, MYGUY_LIMB_GUN, &gGunTipOffset, &gNewObjectDefinition.coord);
	
					/* MAKE MODEL */
					
		gNewObjectDefinition.group = GLOBAL_MGroupNum_SonicScream;	
		gNewObjectDefinition.type = GLOBAL_MObjType_SonicScream;	
		gNewObjectDefinition.flags = STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NULLSHADER;
		gNewObjectDefinition.slot = PLAYER_SLOT-1;						// dont update till next loop
		gNewObjectDefinition.moveCall = MoveSonicScream;
		gNewObjectDefinition.rot = r = theNode->Rot.y;
		gNewObjectDefinition.scale = .05 + sin(gSonicScreamWave) * .02;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		gSonicScreamWave += fps*25;

		newObj->Kind = ATTACK_MODE_SONICSCREAM; 

		newObj->Health = .7;											// transparency value
		
		newObj->Delta.x = (-sin(r) * SONIC_SCREAM_SPEED) + gDelta.x;	// calc deltas
		newObj->Delta.z = (-cos(r) * SONIC_SCREAM_SPEED) + gDelta.z;

		MakeObjectTransparent(newObj, newObj->Health);
		
		
				/* MAKE COLLISION BOX */
		
		newObj->CType = CTYPE_MYBULLET;									// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;								// .. which also means I don't need a collision box since bullets use point collision checks
		
		newObj->Damage = SONIC_SCREAM_DAMAGE;

				/* UPDATE INFOBAR */
				
		if (--gWeaponInventory[gCurrentAttackMode] == 0)				// dec inventory & if run out, select next weapon
			NextAttackMode();
		gInfobarUpdateBits |= UPDATE_WEAPONICON;						// tell system to update this at end of frame


		PlayEffect_Parms(EFFECT_SONIC,200,kMiddleC);					// play sound
	}
}



/******************** MOVE SONIC SCREAM ************************/

static void MoveSonicScream(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	x,y,z;

	theNode->Health -= .9 * fps;			// decay it
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	GetObjectInfo(theNode);
	
	MakeObjectTransparent(theNode, theNode->Health);


			/* MOVE IT */

	theNode->Scale.x += fps * 1.0;
	theNode->Scale.y += fps * 1.0;
	theNode->Scale.z += fps * 1.0;
	theNode->Radius = theNode->Scale.x * gObjectGroupRadiusList[GLOBAL_MGroupNum_SonicScream][GLOBAL_MObjType_SonicScream];

	x = gCoord.x += gDelta.x * fps;
	y = gCoord.y += gDelta.y * fps;
	z = gCoord.z += gDelta.z * fps;


		/* SEE IF HIT TERRAIN */
		
	if (y <= GetTerrainHeightAtCoord_Planar(x,z))
	{
		ExplodeBullet(theNode);
		return;
	}

			/* DO BULLET COLLISION */
			
	if (DoBulletCollisionDetect(theNode))
		return;


	UpdateObject(theNode);
}

//====================================================================================================


/****************** SHOOT BLASTER ***********************/

static void ShootBlaster(ObjNode *theNode)
{
ObjNode	*newObj;
float	r;

	gAutoFireCounter += gFramesPerSecondFrac * BLASTER_RATE;			// check auto fire timer	
	if (gAutoFireCounter >= 1.0)
	{
		gAutoFireCounter -= 1.0;
	
						/* CALC START COORD */
						
		FindCoordOnJoint(theNode, MYGUY_LIMB_GUN, &gGunTipOffset, &gNewObjectDefinition.coord);
	
	
					/* MAKE MODEL */
					
		gNewObjectDefinition.group = GLOBAL_MGroupNum_Blaster;	
		gNewObjectDefinition.type = GLOBAL_MObjType_Blaster;	
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.slot = PLAYER_SLOT-1;						// dont update till next loop
		gNewObjectDefinition.moveCall = MoveBlaster;
		gNewObjectDefinition.rot = r = theNode->Rot.y;
		gNewObjectDefinition.scale = 1.0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		newObj->Health = 1.0;											
		
		newObj->Kind = ATTACK_MODE_BLASTER; 

		newObj->Delta.x = (-sin(r) * BLASTER_SPEED) + gDelta.x;			// calc deltas
		newObj->Delta.z = (-cos(r) * BLASTER_SPEED) + gDelta.z;
				
		newObj->CType = CTYPE_MYBULLET;									// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;								// .. which also means I don't need a collision box since bullets use point collision checks
		
		newObj->Damage = BLASTER_DAMAGE;
				
		PlayEffect(EFFECT_BLASTER);										// play sound
		
				/* UPDATE INFOBAR */
				
		if (--gWeaponInventory[gCurrentAttackMode] == 0)				// dec inventory & if run out, select next weapon
		{
			gPossibleAttackModes[gCurrentAttackMode] = false;
			NextAttackMode();
		}
		gInfobarUpdateBits |= UPDATE_WEAPONICON;						// tell system to update this at end of frame
	}
}



/******************** MOVE BLASTER ************************/

static void MoveBlaster(ObjNode *theNode)
{
float	x,y,z,fps;

	GetObjectInfo(theNode);

	fps = gFramesPerSecondFrac;

			/* DECAY IT */
			
	theNode->Health -= .9 * fps;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
			/* MOVE IT */
			
	x = gCoord.x += gDelta.x * fps;
	y = gCoord.y += gDelta.y * fps;
	z = gCoord.z += gDelta.z * fps;


		/* SEE IF HIT TERRAIN */
		
	if (y <= GetTerrainHeightAtCoord_Planar(x,z))
	{
		ExplodeBullet(theNode);
		return;
	}

			/* DO BULLET COLLISION */
			
	if (DoBulletCollisionDetect(theNode))
		return;

	UpdateObject(theNode);
}


//====================================================================================================


/****************** SHOOT HEATSEEK ***********************/

static void ShootHeatSeek(ObjNode *theNode)
{
ObjNode	*newObj;
float	r;


	/* SEE IF SPEW RATE IT GOOD */

	gAutoFireCounter += gFramesPerSecondFrac * HEATSEEK_RATE;
	if (gAutoFireCounter >= 1.0)
	{
		gAutoFireCounter -= 1.0;
	
						/* CALC START COORD */
						
		FindCoordOnJoint(theNode, MYGUY_LIMB_GUN, &gGunTipOffset, &gNewObjectDefinition.coord);
	
	
					/* MAKE MODEL */
					
		gNewObjectDefinition.group = GLOBAL_MGroupNum_HeatSeek;	
		gNewObjectDefinition.type = GLOBAL_MObjType_HeatSeek;	
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.slot = PLAYER_SLOT-1;					// dont update till next loop
		gNewObjectDefinition.moveCall = MoveHeatSeek;
		gNewObjectDefinition.rot = r = theNode->Rot.y;
		gNewObjectDefinition.scale = 1.0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		newObj->Health = 1.0;											
		
		newObj->Kind = ATTACK_MODE_HEATSEEK; 

		newObj->Delta.x = (-sin(r) * HEATSEEK_SPEED) + gDelta.x;	// calc deltas
		newObj->Delta.z = (-cos(r) * HEATSEEK_SPEED) + gDelta.z;


				/* MAKE COLLISION BOX */
		
		newObj->CType = CTYPE_MYBULLET;								// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;							// .. which also means I don't need a collision box since bullets use point collision checks
		
		newObj->Damage = HEATSEEK_DAMAGE;		

		newObj->HeatSeekTurnSpeed = 0;								// init auto-target delay
		newObj->HeatSeekPrevTrailX = newObj->Coord.x;
		newObj->HeatSeekPrevTrailY = newObj->Coord.y;
		newObj->HeatSeekPrevTrailZ = newObj->Coord.z;

		PlayEffect(EFFECT_HEATSEEK);								// play sound

				/* UPDATE INFOBAR */
				
		if (--gWeaponInventory[gCurrentAttackMode] == 0)			// dec inventory & if run out, select next weapon
		{
			gPossibleAttackModes[gCurrentAttackMode] = false;
			NextAttackMode();
		}
		gInfobarUpdateBits |= UPDATE_WEAPONICON;					// tell system to update this at end of frame

	}

}



/******************** MOVE HEATSEEK ************************/

static void MoveHeatSeek(ObjNode *theNode)
{
	float fps = gFramesPerSecondFrac;

			/* DECAY LIFE */

	theNode->Health -= .5 * fps;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	GetObjectInfo(theNode);
	
	
			/* FIND CLOSEST ENEMY & ACCEL TO IT */

	theNode->HeatSeekTurnSpeed += fps * 4;
	ObjNode* enemyObj = FindClosestEnemy(&gCoord, NULL);						// get closest
	if (enemyObj)
	{
		float y = enemyObj->Coord.y + ((enemyObj->BottomOff + enemyObj->TopOff)/2);	// calc center y of enemy as target y
		gDelta.y += (y - gCoord.y) * fps;
	
		float turnSp = theNode->HeatSeekTurnSpeed;								// calc turn speed
		if (turnSp > 7)
			turnSp = 7;
		TurnObjectTowardTarget(theNode, enemyObj->Coord.x,						// rotate toward target
							 enemyObj->Coord.z, turnSp, false);			
	
		float r = theNode->Rot.y;												// calc deltas based on aiming
		gDelta.x = -sin(r) * HEATSEEK_SPEED;
		gDelta.z = -cos(r) * HEATSEEK_SPEED;	
	}
		
	
			/* MOVE IT */
			
	float x = gCoord.x += gDelta.x * fps;
	float y = gCoord.y += gDelta.y * fps;
	float z = gCoord.z += gDelta.z * fps;

			/* CALC YAW/PITCH ROTATION */
			
	theNode->Rot.x = gDelta.y * .001;
	if (theNode->Rot.x > PI/2)
		theNode->Rot.x = PI/2;
	else
	if (theNode->Rot.x < -PI/2)
		theNode->Rot.x = -PI/2;
	

		/* SEE IF HIT TERRAIN */
		
	if (y <= GetTerrainHeightAtCoord_Planar(x,z))
	{
		ExplodeBullet(theNode);
		return;
	}
	

			/* DO BULLET COLLISION */
			
	if (DoBulletCollisionDetect(theNode))
		return;
	

			/* LEAVE TRAIL */

	float oldX = theNode->SpecialF[1];
	float oldY = theNode->SpecialF[2];
	float oldZ = theNode->SpecialF[3];

	float numEchoes = (1.0f/15.0f) * CalcQuickDistance(x, z, oldX, oldZ);
	if (numEchoes >= 1.0f)
	{
		theNode->SpecialF[1] = gCoord.x;
		theNode->SpecialF[2] = gCoord.y;
		theNode->SpecialF[3] = gCoord.z;

		numEchoes++;
		float xi = (gCoord.x - oldX) * (-1.0f / numEchoes);
		float yi = (gCoord.y - oldY) * (-1.0f / numEchoes);
		float zi = (gCoord.z - oldZ) * (-1.0f / numEchoes);

		for (int i = 0; i <= numEchoes; i++)
		{
			NewObjectDefinitionType newObjDef =
			{
				.coord		= {x,y,z},
				.group		= GLOBAL_MGroupNum_HeatSeekEcho,
				.type		= GLOBAL_MObjType_HeatSeekEcho,
				.flags		= 0,
				.slot		= SLOT_OF_DUMB,
				.moveCall	= MoveHeatSeekEcho,
				.rot		= 0,
				.scale		= .7 + RandomFloat()*.5,
			};

			ObjNode* newObj = MakeNewDisplayGroupObject(&newObjDef);
			if (newObj)
			{
				newObj->Health = .7 + RandomFloat()*.3;							// transparency value
				MakeObjectTransparent(newObj, newObj->Health);					// make transparent
			}

			x += xi;
			y += yi;
			z += zi;
		}
	}


	UpdateObject(theNode);
}


/********************* MOVE HEAT SEEK ECHO ***********************/

static void MoveHeatSeekEcho(ObjNode *theNode)
{
	theNode->Health -= 3.0 * gFramesPerSecondFrac;			// decay it
	if (theNode->Health <= 0)
	{
		DeleteObject(theNode);
		return;
	}
	
	MakeObjectTransparent(theNode, theNode->Health);
}



//====================================================================================================


/****************** SHOOT TRIBLAST ***********************/

static void ShootTriBlast(ObjNode *theNode)
{
ObjNode	*newObj;
float	r;

	gAutoFireCounter += gFramesPerSecondFrac * TRIBLAST_RATE;			// check auto fire timer	
	if (gAutoFireCounter >= 1.0)
	{
		gAutoFireCounter -= 1.0;
	
						/* CALC START COORD */
						
		FindCoordOnJoint(theNode, MYGUY_LIMB_GUN, &gGunTipOffset, &gNewObjectDefinition.coord);
	
					/***************/
					/* MAKE MODELS */
					/***************/
					
					/* MAKE TOP TRI */
					
		gNewObjectDefinition.group = GLOBAL_MGroupNum_TriBlast;	
		gNewObjectDefinition.type = GLOBAL_MObjType_TriBlast;	
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.slot = PLAYER_SLOT-1;						// dont update till next loop
		gNewObjectDefinition.moveCall = MoveTriBlast;
		gNewObjectDefinition.rot = r = theNode->Rot.y;
		gNewObjectDefinition.scale = .5;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		newObj->Health = 1.0;											
		
		newObj->Delta.x = (-sin(r) * TRIBLAST_SPEED) + gDelta.x;			// calc deltas
		newObj->Delta.z = (-cos(r) * TRIBLAST_SPEED) + gDelta.z;
		newObj->Delta.y = newObj->Delta.y + 200;
				
		newObj->CType = CTYPE_MYBULLET;									// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;								// .. which also means I don't need a collision box since bullets use point collision checks
		
		newObj->Damage = TRIBLAST_DAMAGE;
		newObj->Kind = ATTACK_MODE_TRIBLAST;
				
		UpdateObjectTransforms(newObj);
		
				
					/* MAKE LEFT TRI */
					
		gNewObjectDefinition.rot = r = theNode->Rot.y-.2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		newObj->Health = 1.0;													
		newObj->Delta.x = (-sin(r) * TRIBLAST_SPEED) + gDelta.x;			// calc deltas
		newObj->Delta.z = (-cos(r) * TRIBLAST_SPEED) + gDelta.z;
		newObj->Delta.y = newObj->Delta.y - 20;
		
		newObj->CType = CTYPE_MYBULLET;									// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;								// .. which also means I don't need a collision box since bullets use point collision checks		
		newObj->Damage = TRIBLAST_DAMAGE;
		newObj->Kind = ATTACK_MODE_TRIBLAST;
				
		UpdateObjectTransforms(newObj);
				
					/* MAKE RIGHT TRI */
					
		gNewObjectDefinition.rot = r = theNode->Rot.y + .2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		newObj->Health = 1.0;													
		newObj->Delta.x = (-sin(r) * TRIBLAST_SPEED) + gDelta.x;			// calc deltas
		newObj->Delta.z = (-cos(r) * TRIBLAST_SPEED) + gDelta.z;
		newObj->Delta.y = newObj->Delta.y - 20;

		newObj->CType = CTYPE_MYBULLET;									// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;								// .. which also means I don't need a collision box since bullets use point collision checks		
		newObj->Damage = TRIBLAST_DAMAGE;
		newObj->Kind = ATTACK_MODE_TRIBLAST;
				
		UpdateObjectTransforms(newObj);
				
				
		PlayEffect(EFFECT_BLASTER);										// play sound
		
				/* UPDATE INFOBAR */
				
		if (--gWeaponInventory[gCurrentAttackMode] == 0)				// dec inventory & if run out, select next weapon
		{
			gPossibleAttackModes[gCurrentAttackMode] = false;
			NextAttackMode();
		}
		gInfobarUpdateBits |= UPDATE_WEAPONICON;						// tell system to update this at end of frame
	}
}



/******************** MOVE TRIBLAST ************************/

static void MoveTriBlast(ObjNode *theNode)
{
float	x,y,z,fps;

	GetObjectInfo(theNode);

	fps = gFramesPerSecondFrac;

			/* DECAY IT */
			
	theNode->Health -= 1.1f * fps;
	if (theNode->Health < 0)
	{
		DeleteObject(theNode);
		return;
	}
	
			/* MOVE IT */
			
	x = gCoord.x += gDelta.x * fps;
	y = gCoord.y += gDelta.y * fps;
	z = gCoord.z += gDelta.z * fps;


		/* SEE IF HIT TERRAIN */
		
	if (y <= GetTerrainHeightAtCoord_Planar(x,z))
	{
		ExplodeBullet(theNode);
		return;
	}

			/* DO BULLET COLLISION */
			
	if (DoBulletCollisionDetect(theNode))
		return;


			/* RANDOM SPIN */
			
	theNode->Rot.x = RandomFloat()*PI2;
	theNode->Rot.y = RandomFloat()*PI2;

	UpdateObject(theNode);
}


//====================================================================================================


/****************** SHOOT NUKE ***********************/

static void ShootNuke(ObjNode *theNode)
{
ObjNode	*newObj;
float	r;

	gAutoFireCounter += gFramesPerSecondFrac * NUKE_RATE;			// check auto fire timer	
	if (gAutoFireCounter >= 1.0)
	{
		gAutoFireCounter -= 1.0;
	
						/* CALC START COORD */
						
		FindCoordOnJoint(theNode, MYGUY_LIMB_GUN, &gGunTipOffset, &gNewObjectDefinition.coord);
	
	
					/* MAKE MODEL */
					
		gNewObjectDefinition.group = GLOBAL_MGroupNum_Nuke;	
		gNewObjectDefinition.type = GLOBAL_MObjType_Nuke;	
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.slot = PLAYER_SLOT-1;						// dont update till next loop
		gNewObjectDefinition.moveCall = MoveNuke;
		gNewObjectDefinition.rot = 0;
		gNewObjectDefinition.scale = .6;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj == nil)
			return;

		newObj->Health = 1.0;											
		
		r = theNode->Rot.y;
		newObj->Delta.x = (-sin(r) * NUKE_SPEED) + gDelta.x;			// calc deltas
		newObj->Delta.z = (-cos(r) * NUKE_SPEED) + gDelta.z;
				
		newObj->CType = CTYPE_MYBULLET;									// note: don't set as CTYPE_HURTENEMY since bullets do their own collision check
		newObj->CBits = CBITS_TOUCHABLE;								// .. which also means I don't need a collision box since bullets use point collision checks
		
		newObj->Kind = ATTACK_MODE_NUKE;
		newObj->Damage = NUKE_DAMAGE;
				
//		PlayEffect(EFFECT_BLASTER);										// play sound
		
				/* UPDATE INFOBAR */
				
		if (--gWeaponInventory[gCurrentAttackMode] == 0)				// dec inventory & if run out, select next weapon
		{
			gPossibleAttackModes[gCurrentAttackMode] = false;
			NextAttackMode();
		}
		gInfobarUpdateBits |= UPDATE_WEAPONICON;						// tell system to update this at end of frame
	}
}



/******************** MOVE NUKE ************************/

static void MoveNuke(ObjNode *theNode)
{
float	x,y,z,fps;

	GetObjectInfo(theNode);

	fps = gFramesPerSecondFrac;

			/* SEE IF READY TO EXPLODE */
			
	theNode->Health -= .8 * fps;
	if (theNode->Health < 0)
	{
		ExplodeBullet(theNode);
		return;
	}
	
			/* MOVE IT */
			
	x = gCoord.x += gDelta.x * fps;
	y = gCoord.y += gDelta.y * fps;
	z = gCoord.z += gDelta.z * fps;

	theNode->Rot.y += fps * 6;
	theNode->Rot.z += fps * 8;

		/* SEE IF HIT TERRAIN */
		
	if (y <= GetTerrainHeightAtCoord_Planar(x,z))
	{
		ExplodeBullet(theNode);
		return;
	}

			/* DO BULLET COLLISION */
			
	if (DoBulletCollisionDetect(theNode))
		return;

	UpdateObject(theNode);
}


/***************** DETONATE NUKE ***********************/

static void DetonateNuke(ObjNode *theNode)
{
ObjNode *newObj;

			/* DELETE NUKE */
			
	DeleteObject(theNode);

	PlayEffect_Parms(EFFECT_EXPLODE,FULL_CHANNEL_VOLUME,kMiddleC-6);	// play sound


			/* CREATE SHOCKWAVE */

	gNewObjectDefinition.coord = theNode->Coord;
	gNewObjectDefinition.group = GLOBAL_MGroupNum_Explosion;	
	gNewObjectDefinition.type = GLOBAL_MObjType_Explosion;	
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 200;	
	gNewObjectDefinition.moveCall = MoveNukeShockwave;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj)
	{
		newObj->Health = .9;
		newObj->Damage = newObj->Health * 3;
		MakeObjectTransparent(newObj, newObj->Health);					// make transparent
		newObj->CType = CTYPE_HURTENEMY|CTYPE_HURTME;	
		newObj->CBits = CBITS_TOUCHABLE;	
		SetObjectCollisionBounds(newObj,10,-10,-10,10,10,-10);
	}	
}
 

/*********************** MOVE NUKE SHOCKWAVE **************************/

static void MoveNukeShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	s;

			/* DECAY IT */
			
	theNode->Health -= fps * .6;						// decay it
	if (theNode->Health <= 0)
	{
		DeleteObject(theNode);
		return;
	}
	MakeObjectTransparent(theNode, theNode->Health);
	theNode->Damage = theNode->Health * .5;				// update damage
	
	
			/* ENLARGE */
			
	s = theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 80;	
	SetObjectCollisionBounds(theNode,s*10,s*-10,s*-10,s*10,s*10,s*-10);

	theNode->Radius = s*10;


			/* UPATE */
			
	UpdateObjectTransforms(theNode);
}
