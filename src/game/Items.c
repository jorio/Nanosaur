/****************************/
/*   		ITEMS.C		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "terrain.h"
#include "items.h"
#include "mobjtypes.h"
#include "qd3d_geometry.h"
#include "collision.h"
#include "3dmath.h"
#include "3dmf.h"
#include "infobar.h"
#include "sound2.h"
#include "enemy.h"
#include "effects.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gFuel;
extern	TQ3Point3D			gCoord,gMyCoord;
extern	TQ3Vector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	TQ3Object			gObjectGroupList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TQ3BoundingBox 		gObjectGroupBBoxList[MAX_3DMF_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	unsigned long 		gInfobarUpdateBits;
extern	short				gAmbientEffect;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveLavaPatch(ObjNode *theNode);
static void MoveGasVent(ObjNode *theNode);
static void MoveWaterPatch(ObjNode *theNode);
static void MoveFireball(ObjNode *theNode);
static void MoveTree(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DIST_TO_FRONT	36
#define	DIST_TO_SIDE	21



/*********************/
/*    VARIABLES      */
/*********************/

#define	TreeYOff			SpecialF[0]

#define	UndulationIndex		SpecialF[0]
#define	ShootFireballs		Flag[0]
#define	LavaAutoY			Flag[1]
#define	FireballTimer		SpecialF[1]

#define FireballPuffTimer	SpecialF[0]

static float gLavaU, gLavaV;				// uv offsets values for lava
static short gNumLavaPatches;
short 	gLavaSoundChannel = -1;
float	gMinLavaDist = 10000000;

static float gWaterU, gWaterV;				// uv offsets values for Water
static short gNumWaterPatches;


static short gNumSteamVents = 0;
short 	gSteamSoundChannel = -1;
float	gMinSteamDist = 10000000;

#define	VentHasLimit		Flag[0]
#define VentTimer			SpecialF[0]

/********************* INIT ITEMS MANAGER *************************/

void InitItemsManager(void)
{
	gLavaU = gLavaV = 0.0f;
	gNumLavaPatches = 0;
	
	gWaterU = gWaterV = 0.0f;
	gNumWaterPatches = 0;
	
	
	gNumSteamVents = 0;
	
			/* NO STREAMING SOUNDS YET */
			
	gLavaSoundChannel = -1;
	gSteamSoundChannel = -1;
	gAmbientEffect = -1;
}


/************************* ADD LAVA PATCH *********************************/
//
// A lava patch is 8x8 tiles
//

Boolean AddLavaPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	y;

	x -= x % TERRAIN_POLYGON_SIZE;									// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= z % TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;

			/* GET Y COORD */
			
	if (itemPtr->parm[3] & 1)
		 y = GetTerrainHeightAtCoord_Planar(x,z)+LAVA_Y_OFFSET;
	else
		y = FIXED_LAVA_Y;



	gNewObjectDefinition.group = LEVEL0_MGroupNum_LavaPatch;	
	gNewObjectDefinition.type = LEVEL0_MObjType_LavaPatch;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveLavaPatch;
	gNewObjectDefinition.rot = 0;
	
	if (itemPtr->parm[3] & (1<<2))									// see if to 1/2 size
		gNewObjectDefinition.scale = 1.0;	
	else
		gNewObjectDefinition.scale = 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	MakeObjectKeepBackfaces(newObj);								// no bother to remove these

	newObj->StatusBits |= STATUS_BIT_HIGHFILTER|STATUS_BIT_HIGHFILTER2;	// make it look nice

	newObj->UndulationIndex = 1;
	newObj->ShootFireballs = itemPtr->parm[3] & (1<<1);				// see if shoot fireballs
	newObj->LavaAutoY = itemPtr->parm[3] & 1;						// remember if auto-y

	gNumLavaPatches++;												// keep count of these
	
				/* START LAVA BUBBLES */
				
	if (gLavaSoundChannel == -1)
		gLavaSoundChannel = PlayEffect_Parms(EFFECT_BUBBLES,1,kMiddleC);
	
	
	return(true);													// item was added
}


/********************* MOVE LAVA PATCH **********************/

static void MoveLavaPatch(ObjNode *theNode)
{
float	d;

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		Nano_DeleteObject(theNode);
		gNumLavaPatches--;
		if (gNumLavaPatches == 0)
		{
			if (gLavaSoundChannel != -1)					// stop bubbles sound
			{
				StopAChannel(&gLavaSoundChannel);
			}		
		}
		return;
	}

	if (theNode->LavaAutoY)
	{
		theNode->Coord.y = GetTerrainHeightAtCoord_Planar(theNode->Coord.x,theNode->Coord.z)+LAVA_Y_OFFSET;	
	}
	
		/* IF CULLED, DONT DO ANYTHING ELSE */
	
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
		return;
	
			/* MAKE UNDULATE */
			
	theNode->UndulationIndex += gFramesPerSecondFrac*2.0f;
	theNode->Scale.y = sin(theNode->UndulationIndex)*.5f + .501f;
	UpdateObjectTransforms(theNode);
	
	
		/* CALC DIST FOR SOUND MANAGER */
		
	d = CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z);
	if (d < gMinLavaDist)
		gMinLavaDist = d;
		
			
		/* SEE IF SHOOT FIREBALL */
		
	if (theNode->ShootFireballs)
	{
		theNode->FireballTimer += gFramesPerSecondFrac;
		if (theNode->FireballTimer > .4f)
		{
			ObjNode	*newObj;
			float	x,z,y;

			theNode->FireballTimer = 0;

			x = gNewObjectDefinition.coord.x = theNode->Coord.x + (RandomFloat()-.5f) * (TERRAIN_SUPERTILE_UNIT_SIZE);
			z = gNewObjectDefinition.coord.z = theNode->Coord.z + (RandomFloat()-.5f) * (TERRAIN_SUPERTILE_UNIT_SIZE);
			y = gNewObjectDefinition.coord.y = theNode->Coord.y - 20.0f;
			
			if (GetTerrainHeightAtCoord_Planar(x,z) < y)			// make sure this area is above ground
			{
				gNewObjectDefinition.group = LEVEL0_MGroupNum_Fireball;	
				gNewObjectDefinition.type = LEVEL0_MObjType_Fireball;	
				gNewObjectDefinition.flags = 0;
				gNewObjectDefinition.slot = 200;
				gNewObjectDefinition.moveCall = MoveFireball;
				gNewObjectDefinition.rot = 0;
				gNewObjectDefinition.scale = .3;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				if (newObj)
				{
					newObj->Delta.y = 300 + (RandomFloat()*400.0f);
					newObj->Delta.x = (RandomFloat()-.5f) * 300.0f;
					newObj->Delta.z = (RandomFloat()-.5f) * 300.0f;
					
					newObj->CType = CTYPE_HURTME;
					newObj->CBits = CBITS_TOUCHABLE;
					SetObjectCollisionBounds(newObj,10,-10,-10,10,10,-10);			
					newObj->Damage = .08;
					
					newObj->FireballPuffTimer = 0;
				}
			}		
		}		
	}
}


/************************ MOVE FIREBALL *********************************/

static void MoveFireball(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;
ObjNode	*puff;

	GetObjectInfo(theNode);
	
			/* MOVE IT */
			
	gDelta.y -= (GRAVITY_CONSTANT/2.5) * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += fps * PI*3;
	theNode->Rot.z -= fps * PI*2;


		/* SEE IF MAKE PUFF TRAIL */
				
	theNode->FireballPuffTimer += fps;
	if (theNode->FireballPuffTimer > .06f)
	{
		theNode->FireballPuffTimer = 0;
		puff = MakeSmokePuff(gCoord.x, gCoord.y, gCoord.z, .4);
		puff->Delta.x = puff->Delta.y = puff->Delta.z = 0;
		puff->Health = .5;										// transparency value
		puff->SpecialF[0] += .7;								// decay rate
	}


		/* SEE IF HIT GROUND */
			
	y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);		// get y here
	if (gCoord.y <= y)
	{
		Nano_DeleteObject(theNode);
		return;
	}
	UpdateObject(theNode);

}


/******************** UPDATE LAVA TEXTURE ANIMATION ************************/

void UpdateLavaTextureAnimation(void)
{
	if (gNumLavaPatches)
	{
					/* MOVE UVS */
					
		QD3D_ScrollUVs(gObjectGroupList[LEVEL0_MGroupNum_LavaPatch][LEVEL0_MObjType_LavaPatch],
						gLavaU,gLavaV);
		
		gLavaU += .07*gFramesPerSecondFrac;
		gLavaV += .03*gFramesPerSecondFrac;
	}
}


//=====================================================================================================



/************************* ADD WATER PATCH *********************************/
//
// A water patch is 8x8 tiles.
// 

Boolean AddWaterPatch(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	y;

	x -= x % TERRAIN_POLYGON_SIZE;									// round down to nearest tile & center
	x += TERRAIN_POLYGON_SIZE/2;
	z -= z % TERRAIN_POLYGON_SIZE;
	z += TERRAIN_POLYGON_SIZE/2;

			/* GET Y COORD */
			
	if (itemPtr->parm[3] & 1)
		 y = GetTerrainHeightAtCoord_Planar(x,z)+WATER_Y_OFFSET;
	else
		y = FIXED_WATER_Y;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_WaterPatch;	
	gNewObjectDefinition.type = LEVEL0_MObjType_WaterPatch;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = y;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = STATUS_BIT_HIGHFILTER|STATUS_BIT_NOTRICACHE;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveWaterPatch;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	MakeObjectTransparent(newObj, .8);
	MakeObjectKeepBackfaces(newObj);								// no bother to remove these

	newObj->UndulationIndex = 1;

	gNumWaterPatches++;												// keep count of these
		
	return(true);													// item was added
}


/********************* MOVE WATER PATCH **********************/

static void MoveWaterPatch(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		Nano_DeleteObject(theNode);
		gNumWaterPatches--;
		return;
	}
	
			/* MAKE UNDULATE */
			
	theNode->UndulationIndex += gFramesPerSecondFrac*3.0f;
	theNode->Scale.y = sin(theNode->UndulationIndex)*.5f + .501f;
	UpdateObjectTransforms(theNode);
	
	
			
}


/******************** UPDATE WATER TEXTURE ANIMATION ************************/

void UpdateWaterTextureAnimation(void)
{
	if (gNumWaterPatches)  
	{
					/* MOVE UVS */
					
		QD3D_ScrollUVs(gObjectGroupList[LEVEL0_MGroupNum_WaterPatch][LEVEL0_MObjType_WaterPatch],
						gWaterU,gWaterV);
		
		gWaterU -= .04*gFramesPerSecondFrac;
		gWaterV += .08*gFramesPerSecondFrac;
	}
}


//===============================================================================================


/************************* ADD TREE *********************************/
//
// parm0 = tree type 0..5
//

Boolean AddTree(TerrainItemEntryType *itemPtr, long  x, long z)
{
int		i;
ObjNode	*newObj;
float	yoff,scale;
TQ3BoundingBox *bbox;
static const float scales[] =
{
	1.0,				// fern
	1.1,				// stickpalm
	1.0,				// bamboo
	4.0,				// cypress
	1.2,				// main palm
	1.3					// pine palm
};

	i =  itemPtr->parm[0];											// get tree type
	if ((i < 0) || (i > 5))											// verify it
		return(true);

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Tree;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Tree1 + i;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 100;
	gNewObjectDefinition.moveCall = MoveTree;
	gNewObjectDefinition.rot = RandomFloat()*PI2;
	gNewObjectDefinition.scale = scale = scales[i] + RandomFloat()*.5f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list

	bbox = &gObjectGroupBBoxList[newObj->Group][newObj->Type];
	yoff = bbox->min.y * scale;	
	newObj->TreeYOff = yoff;
	newObj->Coord.y -= yoff;					// adjust y so that bottom is on ground (trees all have centroid in middle)


			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	SetObjectCollisionBounds(newObj,bbox->max.y * scale,bbox->min.y * scale,
									-30,30,30,-30);

	UpdateObjectTransforms(newObj);
	return(true);									// item was added
}


/******************** MOVE TREE ***************************/

static void MoveTree(ObjNode *theNode)
{
float	y;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		Nano_DeleteObject(theNode);
		return;
	}

	y = GetTerrainHeightAtCoord_Planar(theNode->Coord.x,theNode->Coord.z) - theNode->TreeYOff;	// recalc y
	if (y != theNode->Coord.y)
	{
		theNode->Coord.y = y;
		CalcObjectBoxFromNode(theNode);
		UpdateObjectTransforms(theNode);
	}
}


/************************* ADD MUSHROOM *********************************/

Boolean AddMushroom(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	scale;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Mushroom;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Mushroom;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.moveCall = MoveStaticObject;
	gNewObjectDefinition.rot = RandomFloat()*PI2;
	gNewObjectDefinition.scale = scale = 1.0f + RandomFloat();
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list


			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	SetObjectCollisionBounds(newObj,newObj->Radius,0,-100,100,100,-100);

	return(true);									// item was added
}




/************************* ADD BOULDER *********************************/
//
// This boulder just sits there
//

Boolean AddBoulder(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	scale;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Boulder;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Boulder;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z) - 10.0f;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = STATUS_BIT_HIGHFILTER;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.moveCall = MoveStaticObject;
	gNewObjectDefinition.rot = RandomFloat()*PI2;
	gNewObjectDefinition.scale = scale = 1.0f + RandomFloat();
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list


			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	CreateCollisionTrianglesForObject(newObj);		// build triangle list


	return(true);									// item was added
}


/************************* ADD BUSH *********************************/
//
// Triceratops like to hide in these.
//

Boolean AddBush(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	yoff,scale;
TQ3BoundingBox *bbox;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Bush;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Bush;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.moveCall = MoveStaticObject;
	gNewObjectDefinition.rot = RandomFloat()*PI2;
	gNewObjectDefinition.scale = scale = 4.2f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list

	bbox = &gObjectGroupBBoxList[newObj->Group][newObj->Type];
	yoff = bbox->min.y * scale;	
	newObj->Coord.y -= yoff;					// adjust y so that bottom is on ground (trees all have centroid in middle)


			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	
	SetObjectCollisionBounds(newObj,120,-100,-100,100,100,-100);

	UpdateObjectTransforms(newObj);
	
	
			/* PUT TRICERATOPS IN THE BUSH */
			
	if (itemPtr->parm[3] & 1)
	{
		MakeTriceratops(newObj, x, z);
	}
			
	
	return(true);									// item was added
}

/********************** EXPLODE BUSH ************************/
//
// Called from Enemy_Tricer when he leaves the bush.
//

void ExplodeBush(ObjNode *theBush)
{
		/* VERIFY THAT THIS IS REALLY A BUSH */
		
	if (theBush->Type != LEVEL0_MObjType_Bush)
		return;
		
	if (theBush->Group != LEVEL0_MGroupNum_Bush)
		return;

	QD3D_ExplodeGeometry(theBush, 500, PARTICLE_MODE_BOUNCE|PARTICLE_MODE_HEAVYGRAVITY, 2, 1.0);
	
	Nano_DeleteObject(theBush);
}





/************************* ADD GAS VENT *********************************/

Boolean AddGasVent(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_GasVent;	
	gNewObjectDefinition.type = LEVEL0_MObjType_GasVent;
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = STATUS_BIT_NULLSHADER|STATUS_BIT_NOTRICACHE;
	gNewObjectDefinition.slot = SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall = MoveGasVent;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list

	MakeObjectTransparent(newObj,.7);
	MakeObjectKeepBackfaces(newObj);
	

	newObj->VentHasLimit = itemPtr->parm[3] & 1;		// get flag if has limit
	
			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_BONUS;
	newObj->CBits = CBITS_TOUCHABLE;
	
	SetObjectCollisionBounds(newObj,100, 0, -50, 50, 50, -50);

	gNumSteamVents++;

				/* START STEAM SOUND */				
				
	if (gSteamSoundChannel == -1)
		gSteamSoundChannel = PlayEffect_Parms(EFFECT_STEAM,1,kMiddleC);

	return(true);									// item was added
}


/********************* MOVE GAS VENT **********************/

static void MoveGasVent(ObjNode *theNode)
{
float	d;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		Nano_DeleteObject(theNode);
		gNumSteamVents--;
		if (gNumSteamVents == 0)
		{
			if (gSteamSoundChannel != -1)					// stop steam sound
			{
				StopAChannel(&gSteamSoundChannel);
			}		
		}
		return;
	}
	
			/* MAKE UNDULATE */
			
	theNode->Scale.y = RandomFloat()*.3 + 0.5;
	UpdateObjectTransforms(theNode);
	
	
		/* IF IS TOUCHING ME, THEN GET FUEL */
		
	
	if (gMyCoord.x > theNode->CollisionBoxes->left)
		if (gMyCoord.x < theNode->CollisionBoxes->right)
			if (gMyCoord.z < theNode->CollisionBoxes->front)
				if (gMyCoord.z > theNode->CollisionBoxes->back)
					if (gMyCoord.y < theNode->CollisionBoxes->top)
						if (gMyCoord.y > theNode->CollisionBoxes->bottom)
						{
							gFuel += (MAX_FUEL_CAPACITY / 5.0f) * gFramesPerSecondFrac;
							if (gFuel > MAX_FUEL_CAPACITY)
								gFuel = MAX_FUEL_CAPACITY;
								
							gInfobarUpdateBits |= UPDATE_FUEL;
							
									/* SEE IF VENT IS OUT OF GAS */
									
							if (theNode->VentHasLimit)
							{
								theNode->VentTimer += gFramesPerSecondFrac;
								if (theNode->VentTimer > 2.0f)
								{
									theNode->TerrainItemPtr = nil;			// never coming back
									Nano_DeleteObject(theNode);
									return;
								}
							}
						}			

		/* CALC DIST FOR SOUND MANAGER */
		
	d = CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gMyCoord.x, gMyCoord.z);
	if (d < gMinSteamDist)
		gMinSteamDist = d;

}



