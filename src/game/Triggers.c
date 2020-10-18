/****************************/
/*    	TRIGGERS            */
/* (c) 1997 Pangea Software */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "triggers.h"
#include "objects.h"
#include "mobjtypes.h"
#include "misc.h"
#include "main.h"
#include "terrain.h"
#include "qd3d_geometry.h"
#include "weapons.h"
#include "sound2.h"
#include "infobar.h"
#include "items.h"
#include "myguy.h"


extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode					*gFirstNodePtr,*gPlayerObj;
extern	TQ3Point3D				gCoord;
extern	float					gFramesPerSecondFrac,gMyHealth;
extern	unsigned long 			gInfobarUpdateBits;

/*******************/
/*   PROTOTYPES    */
/*******************/

static Boolean DoTrig_BonusBox(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveBonusBox(ObjNode *theNode);
static void MovePowerUp(ObjNode *theNode);
static Boolean DoTrig_PowerUp(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveCrystal(ObjNode *theNode);
static Boolean DoTrig_Crystal(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveStepStone(ObjNode *theNode);
static Boolean DoTrig_StepStone(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	TRIGGER_SLOT			4					// needs to be early in the collision list

enum
{
	POW_KIND_HEATSEEK,
	POW_KIND_LASER,
	POW_KIND_TRIBLAST,
	POW_KIND_HEALTH,
	POW_KIND_SHIELD,
	POW_KIND_NUKE,
	POW_KIND_SONIC
};


enum
{
	STEPSTONE_MODE_IDLE,
	STEPSTONE_MODE_FALL,
	STEPSTONE_MODE_RISE,
	STEPSTONE_MODE_HOLD
};

/**********************/
/*     VARIABLES      */
/**********************/

#define	PowerUpQuan		Special[0]

#define	StepStoneMode			Flag[0]
#define	StepStoneStartY			SpecialF[0]
#define	StepStoneHoldTimer		SpecialF[1]
#define	StepStoneReincarnate	Flag[1]

												// TRIGGER HANDLER TABLE
												//========================

Boolean	(*gTriggerTable[])(ObjNode *, ObjNode *, Byte) = 
{
	DoTrig_PowerUp,
	DoTrig_Crystal,
	DoTrig_StepStone
};
					

/******************** HANDLE TRIGGER ***************************/
//
// INPUT: triggerNode = ptr to trigger's node
//		  whoNode = who touched it?
//		  side = side bits from collision.  Which side (of hitting object) hit the trigger
//
// OUTPUT: true if we want to handle the trigger as a solid object
//
// NOTE:  Triggers cannot self-delete in their DoTrig_ calls!!!  Bad things will happen in the hander collision function!
//

Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side)
{

		/* SEE IF THIS TRIGGER CAN ONLY BE TRIGGERED BY PLAYER */
		
	if (triggerNode->CType & CTYPE_PLAYERTRIGGERONLY)
		if (whoNode != gPlayerObj)
			return(true);


			/* CHECK SIDES */
			
	if (side & SIDE_BITS_BACK)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_FRONT)		// if my back hit, then must be front-triggerable
			return(gTriggerTable[triggerNode->TriggerType](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_FRONT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BACK)			// if my front hit, then must be back-triggerable
			return(gTriggerTable[triggerNode->TriggerType](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_LEFT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_RIGHT)		// if my left hit, then must be right-triggerable
			return(gTriggerTable[triggerNode->TriggerType](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_RIGHT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_LEFT)			// if my right hit, then must be left-triggerable
			return(gTriggerTable[triggerNode->TriggerType](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_TOP)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BOTTOM)		// if my top hit, then must be bottom-triggerable
			return(gTriggerTable[triggerNode->TriggerType](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_BOTTOM)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_TOP)			// if my bottom hit, then must be top-triggerable
			return(gTriggerTable[triggerNode->TriggerType](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
		return(true);											// assume it can be solid since didnt trigger
}

//===========================================================================================
//===========================================================================================
//===========================================================================================

#if 0
/************************* ADD BONUS BOX *********************************/

Boolean AddBonusBox(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_BonusBox;	
	gNewObjectDefinition.type = LEVEL0_MObjType_BonusBox;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Quick(x,z) + .5;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = TRIGGER_SLOT;
	gNewObjectDefinition.moveCall = MoveBonusBox;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list
		

			/* SET TRIGGER STUFF */

	newObj->CType = CTYPE_TRIGGER|CTYPE_BLOCKSHADOW|CTYPE_PLAYERTRIGGERONLY;
	newObj->CBits = CBITS_ALLSOLID;
	newObj->TriggerSides = SIDE_BITS_BOTTOM;	// side(s) to activate it
	newObj->TriggerType = TRIGTYPE_BONUSBOX;


			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,133,84,-24,24,24,-24,0);


	newObj->RotDelta.y =  PI/3.0f;

	return(true);							// item was added
}


/************************** MOVE BONUS BOX ******************************/

static void MoveBonusBox(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))		// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	theNode->Rot.y += gFramesPerSecondFrac * theNode->RotDelta.y;

	UpdateObjectTransforms(theNode);
}


/************** DO TRIGGER - BONUS BOX ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_BonusBox(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
					
	return(true);
}
#endif


//=======================================================================================================


/************************* ADD POWERUP *********************************/
//
// parm0 = powerup kind
// parm1 = quantity / 0 = default
//

Boolean AddPowerUp(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	types[] = {GLOBAL_MObjType_HeatSeekPOW,GLOBAL_MObjType_LaserPOW,GLOBAL_MObjType_TriBlastPOW,
					GLOBAL_MObjType_HealthPOW,GLOBAL_MObjType_ShieldPOW,GLOBAL_MObjType_NukePOW,GLOBAL_MObjType_Sonic};
short	n;

	n = itemPtr->parm[0];												// parm0 = powerup type
	if ((n < 0) || (n > 6))
		DoFatalAlert("AddPowerUp: illegal powerup subtype");
	
	gNewObjectDefinition.group = MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type = types[n];									
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Quick(x,z) + .5f;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = TRIGGER_SLOT;
	gNewObjectDefinition.moveCall = MovePowerUp;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list
		

			/* SET TRIGGER STUFF */

	newObj->CType = CTYPE_TRIGGER|CTYPE_PLAYERTRIGGERONLY;
	newObj->CBits = CBITS_ALLSOLID;
	newObj->TriggerSides = ALL_SOLID_SIDES;		// side(s) to activate it
	newObj->TriggerType = TRIGTYPE_POWERUP;

	newObj->PowerUpQuan = itemPtr->parm[1];		// remember quantity

			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,70,0,-34,34,34,-34);

	newObj->Kind = n;							// remember which POW kind this is

	return(true);							// item was added
}


/************************** MOVE POWERUP ******************************/

static void MovePowerUp(ObjNode *theNode)
{
float	y;

	if (theNode->TerrainItemPtr == nil)			// see if was deleted during DoTrig
		goto del;
		
	if (TrackTerrainItem(theNode))				// just check to see if it's gone
	{
del:	
		DeleteObject(theNode);
		return;
	}

	theNode->Rot.y += gFramesPerSecondFrac;

	y = GetTerrainHeightAtCoord_Planar(theNode->Coord.x,theNode->Coord.z) + .5f;	// recalc y
	if (y != theNode->Coord.y)
	{
		theNode->Coord.y = y;
		CalcObjectBoxFromNode(theNode);
	}

	UpdateObjectTransforms(theNode);
}


/************** DO TRIGGER - POWERUP ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_PowerUp(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	theNode->TerrainItemPtr = nil;							// it aint never comin' back

			/* HANDLE THE POW */
			
	switch(theNode->Kind)
	{
		case	POW_KIND_HEATSEEK:
				GetWeaponPowerUp(ATTACK_MODE_HEATSEEK,theNode->PowerUpQuan);
				break;
		
		case	POW_KIND_LASER:
				GetWeaponPowerUp(ATTACK_MODE_BLASTER,theNode->PowerUpQuan);
				break;


		case	POW_KIND_TRIBLAST:
				GetWeaponPowerUp(ATTACK_MODE_TRIBLAST,theNode->PowerUpQuan);
				break;

		case	POW_KIND_NUKE:
				GetWeaponPowerUp(ATTACK_MODE_NUKE,theNode->PowerUpQuan);
				break;
				
		case	POW_KIND_SONIC:
				GetWeaponPowerUp(ATTACK_MODE_SONICSCREAM,theNode->PowerUpQuan);
				break;

		case	POW_KIND_HEALTH:
				GetHealth(.33);
				break;
				
		case	POW_KIND_SHIELD:
				StartMyShield(whoNode);
				break;
	}	
	
	PlayEffect(EFFECT_POWPICKUP);

	return(false);
}


//===================================================================================================================


/************************* ADD CRYSTAL *********************************/

Boolean AddCrystal(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	types[] = {LEVEL0_MObjType_Crystal1,LEVEL0_MObjType_Crystal2,LEVEL0_MObjType_Crystal3};
short	n;
float	scale;

	n = itemPtr->parm[0];												// parm0 = crystal type
	if ((n < 0) || (n > 2))												// verify crystal type
		return(true);
	
	gNewObjectDefinition.group = LEVEL0_MGroupNum_Crystal1;	
	gNewObjectDefinition.type = types[n];									
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Quick(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = TRIGGER_SLOT;
	gNewObjectDefinition.moveCall = MoveCrystal;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = scale = 1.5f + RandomFloat();
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list
		
	MakeObjectTransparent(newObj,.7);
	MakeObjectKeepBackfaces(newObj);

			/* SET TRIGGER STUFF */

	newObj->CType = CTYPE_TRIGGER|CTYPE_CRYSTAL;
	newObj->CBits = CBITS_ALLSOLID;
	newObj->TriggerSides = ALL_SOLID_SIDES;		// side(s) to activate it
	newObj->TriggerType = TRIGTYPE_CRYSTAL;


			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,70.0f*scale,0,-34.0f*scale,34.0f*scale,34.0f*scale,-34.0f*scale);

	return(true);							// item was added
}


/************************** MOVE CRYSTAL ******************************/

static void MoveCrystal(ObjNode *theNode)
{
	if (theNode->TerrainItemPtr == nil)			// see if was exploded during DoTrig
	{
		QD3D_ExplodeGeometry(theNode, 570.0f, 0, 1, .3);
		goto del;
	}
		
	if (TrackTerrainItem(theNode))				// just check to see if it's gone
	{
del:	
		DeleteObject(theNode);
		return;
	}
}


/************** DO TRIGGER - CRYSTAL ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Crystal(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	PlayEffect(EFFECT_CRYSTAL);
	theNode->TerrainItemPtr = nil;							// it aint never comin' back
	return(true);
}


/**************** EXPLODE CRYSTAL ******************/
//
// Called from weapons code when bullet hits one of these
//

void ExplodeCrystal(ObjNode *theNode)
{
	QD3D_ExplodeGeometry(theNode, 570.0f, 0, 1, .3);
	PlayEffect(EFFECT_CRYSTAL);
	theNode->TerrainItemPtr = nil;							// it aint never comin' back
	DeleteObject(theNode);
}


//===================================================================================================================


/************************* ADD STEP STONE *********************************/

Boolean AddStepStone(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	y;	
	
	
	gNewObjectDefinition.group = LEVEL0_MGroupNum_StepStone;	
	gNewObjectDefinition.type = LEVEL0_MObjType_StepStone;									
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y = GetTerrainHeightAtCoord_Quick(x,z) + (LAVA_Y_OFFSET/2);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = TRIGGER_SLOT;
	gNewObjectDefinition.moveCall = MoveStepStone;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list
		
			/* SET TRIGGER STUFF */

	newObj->CType = CTYPE_TRIGGER|CTYPE_BLOCKSHADOW|CTYPE_MISC|CTYPE_PLAYERTRIGGERONLY;
	newObj->CBits = CBITS_ALLSOLID;
	newObj->TriggerSides = SIDE_BITS_TOP;		// side(s) to activate it
	newObj->TriggerType = TRIGTYPE_STEPSTONE;

	newObj->StepStoneStartY = y;							// remember starting y
	newObj->StepStoneReincarnate = itemPtr->parm[3] & 1;	// see if reincarnate
	newObj->StepStoneMode = STEPSTONE_MODE_IDLE;

			/* SET COLLISION INFO */
			
	SetObjectCollisionBounds(newObj,98,-94,-94,94,94,-94);



	return(true);							// item was added
}


/************************** MOVE STEPSTONE ******************************/

static void MoveStepStone(ObjNode *theNode)
{
			/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))				// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* HANDLE CURRENT MODE */
				
	switch(theNode->StepStoneMode)
	{
		case	STEPSTONE_MODE_IDLE:
				break;
		
		case	STEPSTONE_MODE_FALL:
				theNode->Coord.y -= 20.0f * gFramesPerSecondFrac;				// lower it
				if (theNode->Coord.y < (theNode->StepStoneStartY - 150.0f))	// see if down far enough
				{
					if (theNode->StepStoneReincarnate)						// see if just nuke it or come back
					{
						theNode->StepStoneMode = STEPSTONE_MODE_HOLD;
						theNode->StepStoneHoldTimer = 0;
					}
					else
					{
						DeleteObject(theNode);
						return;
					}
				}
				break;
				
				
		case	STEPSTONE_MODE_RISE:
				theNode->Coord.y += 20.0f * gFramesPerSecondFrac;				// raise it
				if (theNode->Coord.y >= theNode->StepStoneStartY)			// see if back up
				{
					theNode->Coord.y = theNode->StepStoneStartY;
					theNode->StepStoneMode = STEPSTONE_MODE_IDLE;
				}
				break;
				
		case	STEPSTONE_MODE_HOLD:
				theNode->StepStoneHoldTimer += gFramesPerSecondFrac;
				if (theNode->StepStoneHoldTimer >= 6.0f)
					theNode->StepStoneMode = STEPSTONE_MODE_RISE;
				break;								
	}

	UpdateObjectTransforms(theNode);
	CalcObjectBoxFromNode(theNode);
}


/************** DO TRIGGER - STEPSTONE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_StepStone(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
	if (theNode->StepStoneMode == STEPSTONE_MODE_IDLE)
	{
		theNode->StepStoneMode = STEPSTONE_MODE_FALL;
		if (!theNode->StepStoneReincarnate)
			theNode->TerrainItemPtr = nil;							// never come back once deleted
	}
	return(true);
}


