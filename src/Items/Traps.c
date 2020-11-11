/****************************/
/*   	TRAPS.C			    */
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
#include "mytraps.h"
#include "collision.h"
#include "3dmath.h"
#include "qd3d_geometry.h"
#include "effects.h"
#include "sound2.h"

extern	float				gFramesPerSecondFrac;
extern	TQ3Point3D			gCoord,gMyCoord;
extern	TQ3Vector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	TQ3Vector3D			gRecentTerrainNormal;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveRollingBoulder(ObjNode *theNode);
static void MoveSporePod(ObjNode *theNode);
static void PodShootSpores(ObjNode *thePod);
static void MoveASpore(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	POD_BURST_DIST		300
#define	NUM_SPORES			20

#define	BOULDER_DIST		1200

/*********************/
/*    VARIABLES      */
/*********************/

#define	BoulderIsActive	Flag[0]

#define	PodUndulation	SpecialF[0]
#define	PodBaseScale	SpecialF[1]
#define	SporeInvisible	Flag[0]
#define	SporeTimer		SpecialF[0]

/************************* ADD ROLLING BOULDER *********************************/
//
// This boulder rolls after me
//

Boolean AddRollingBoulder(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	scale;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Boulder2;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Boulder2;	
	gNewObjectDefinition.scale = scale = 3.0;
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z) + 30.0f*scale;
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 50;
	gNewObjectDefinition.moveCall = MoveRollingBoulder;
	gNewObjectDefinition.rot = RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list


			/* SET COLLISION INFO */
			
	newObj->Damage = 0;			
	newObj->CType = CTYPE_MISC|CTYPE_BLOCKSHADOW;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,30.0f*scale,-30.0f*scale,-30.0f*scale,30.0f*scale,30.0f*scale,-30.0f*scale);
	return(true);								// item was added
}


/*********************** MOVE ROLLING BOULDER *****************************/

static void MoveRollingBoulder(ObjNode *theNode)
{
float	y,fps = gFramesPerSecondFrac;
float	oldX,oldZ,delta;
	
			/* SEE IF GONE */
			
	if (TrackTerrainItem(theNode))								// check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/*********************/
		/* BOULDER IS ACTIVE */
		/*********************/
		
	if (theNode->BoulderIsActive)
	{
		GetObjectInfo(theNode);
		oldX = gCoord.x;
		oldZ = gCoord.z;
		
					/* DO GRAVITY & FRICTION */
									
		gDelta.y += -(GRAVITY_CONSTANT)*fps;		// add gravity
		ApplyFrictionToDeltas(30*fps, &gDelta);		// apply motion friction
		
		
					/* MOVE IT */
					
		gCoord.y += gDelta.y*fps;					// move it
		gCoord.x += gDelta.x*fps;
		gCoord.z += gDelta.z*fps;
		
		y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);		// get y here
		if ((gCoord.y+theNode->BottomOff) < y)						// see if bottom below/on ground
		{
			gCoord.y = y-theNode->BottomOff;
			
			if (gDelta.y < 0.0f)									// was falling?
			{
				gDelta.y = -gDelta.y * .8f;							// bounce it
				if (fabs(gDelta.y) < 1.0f)							// when gets small enough, just make zero
					gDelta.y = 0;
				else				
				if (gDelta.y > 30.0f)
				{
					float	d;
					int		volume;
					
					if (gDelta.y > 400.0f)								// make sure doesnt bounce too much
						gDelta.y = 400.0f;
						
					d = CalcQuickDistance(gCoord.x, gCoord.z, gMyCoord.x, gMyCoord.z);	// calc volume based on distance
					volume = FULL_CHANNEL_VOLUME - (long)(d * .15f);
					if (volume > 0)
						PlayEffect_Parms(EFFECT_ROCKSLAM,volume,kMiddleC-10+(MyRandomLong()&3));
				}
			}
			else
				gDelta.y = 0;										// hit while going up slope, so zero delta y
			

		}

			/* DEAL WITH SLOPES */
			
		if (gRecentTerrainNormal.y < .95f)							// if fairly flat, then no sliding effect
		{	
			gDelta.x += gRecentTerrainNormal.x * fps * 900.0f;
			gDelta.z += gRecentTerrainNormal.z * fps * 900.0f;
		}		
		
		delta = sqrt(gDelta.x * gDelta.x + gDelta.z * gDelta.z) * fps;
		theNode->Coord = gCoord;
		TurnObjectTowardTarget(theNode, oldX, oldZ, delta * .5f, false);	
		theNode->Rot.x += delta *.01f;
		
		
				/* DAMAGE IS BASED ON SPEED */
		
		theNode->Damage = (delta / fps) / 2700.0f;		
		
		
				/* IF MOVING, NOT SOLID */
			
		if (delta > 10.0f)
			theNode->CBits = CBITS_TOUCHABLE;
		else
			theNode->CBits = CBITS_ALLSOLID;
					
					
				/* DO COLLISION DETECTION */
				
		HandleCollisions(theNode,CTYPE_MISC);
					
					
	}
	
	
			/**********************/
			/* BOULDER IS WAITING */
			/**********************/
	else
	{
		float	d;
		TQ3Point2D p1,p2;
		
		GetObjectInfo(theNode);
		
		gCoord.y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z) + 30* theNode->Scale.x; // update y
		
		p1.x = gCoord.x;
		p1.y = gCoord.z;
		p2.x = gMyCoord.x;
		p2.y = gMyCoord.z;
		
		d = Q3Point2D_Distance(&p1,&p2);
		if (d < BOULDER_DIST)
		{
			theNode->CType |= CTYPE_HURTME|CTYPE_HURTENEMY;
			theNode->BoulderIsActive = true;
			
			d = 200.0/d;
			gDelta.x = (p2.x - p1.x) * d;
			gDelta.z = (p2.y - p1.y) * d;			
		}	
	}
	
					/* UPDATE */
					
	UpdateObject(theNode);
}


//====================================================================================================



/************************* ADD SPORE POD *********************************/

Boolean AddSporePod(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group = LEVEL0_MGroupNum_Pod;	
	gNewObjectDefinition.type = LEVEL0_MObjType_Pod;	
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = 100;
	gNewObjectDefinition.moveCall = MoveSporePod;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;			// keep ptr to item list

	newObj->PodUndulation = RandomFloat()*PI2;
	newObj->PodBaseScale = newObj->Scale.y;

			/* SET COLLISION INFO */
			
	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,20,0,-20,20,20,-20);
	return(true);								// item was added
}


/***************** MOVE SPORE POD *******************/

static void MoveSporePod(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	d,y;

		/* UPDATE Y */
		
	y = GetTerrainHeightAtCoord_Planar(theNode->Coord.x,theNode->Coord.z);	// recalc y
	if (y != theNode->Coord.y)
	{
		theNode->Coord.y = y;
		CalcObjectBoxFromNode(theNode);
	}


		/* SEE IF GONE */
		
	if (TrackTerrainItem(theNode))								// check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


		/* SEE IF BURST */

	d = Q3Point3D_Distance(&theNode->Coord,&gMyCoord);
	if (d < POD_BURST_DIST)
	{
		PodShootSpores(theNode);
		QD3D_ExplodeGeometry(theNode, 120, PARTICLE_MODE_UPTHRUST, 3, 1.0);
		theNode->TerrainItemPtr = nil;						// never coming back
		DeleteObject(theNode);		
		PlayEffect_Parms(EFFECT_EXPLODE,FULL_CHANNEL_VOLUME,kMiddleC-5);	// play sound		
		return;
	}


		/* DO POD UNDULATION */
		
	theNode->PodUndulation += fps*2.5f;
	theNode->Scale.y = theNode->PodBaseScale + (sin(theNode->PodUndulation) * .1);


	UpdateObjectTransforms(theNode);
}


/***************** POD SHOOT SPORES *********************/

static void PodShootSpores(ObjNode *thePod)
{
ObjNode	*newObj;
short	i;

	for (i = 0; i < NUM_SPORES; i++)
	{
		gNewObjectDefinition.group = LEVEL0_MGroupNum_Pod;	
		gNewObjectDefinition.type = LEVEL0_MObjType_Spore;	
		gNewObjectDefinition.coord = thePod->Coord;
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.slot = 100;
		gNewObjectDefinition.moveCall = MoveASpore;
		gNewObjectDefinition.rot = 0;
		gNewObjectDefinition.scale = 1.2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj)
		{
			newObj->Delta.y = 300.0f + (RandomFloat()*200.0f);
			newObj->Delta.x = (RandomFloat()-.5f) * 350.0f;
			newObj->Delta.z = (RandomFloat()-.5f) * 350.0f;
			
			newObj->CType = CTYPE_HURTME;
			newObj->CBits = CBITS_TOUCHABLE;
			SetObjectCollisionBounds(newObj,10,-10,-10,10,10,-10);			
			newObj->Damage = .05f;
		}
	}
}


/******************** MOVE A SPORE *************************/

static void MoveASpore(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;
ObjNode	*puff;


	if (theNode->SporeInvisible)
	{
		theNode->SporeTimer += fps;
		if (theNode->SporeTimer > 1)
		{
			DeleteObject(theNode);
			return;		
		}	
	}
	else
	{
		GetObjectInfo(theNode);
		
				/* MOVE IT */
				
		gDelta.y -= (GRAVITY_CONSTANT/2.5f) * fps;
		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;


			/* SEE IF HIT GROUND */
				
		y = GetTerrainHeightAtCoord_Planar(gCoord.x, gCoord.z);		// get y here
		if (gCoord.y <= y)
		{
			PlayEffect_Parms(EFFECT_EXPLODE,100,kMiddleC-5+(MyRandomLong()&0xf));	// play sound
		
			theNode->SporeInvisible = true;							// hide for now
			theNode->StatusBits |= STATUS_BIT_HIDDEN;
			theNode->LeftOff *= 3;									// make collision box bigger
			theNode->RightOff *= 3;
			theNode->TopOff *= 3;
			theNode->BottomOff *= 3;
			theNode->FrontOff *= 3;
			theNode->BackOff *= 3;			
			gDelta.x = gDelta.y = gDelta.z = 0;						// stop deltas for future collision tests
			
			puff = MakeSmokePuff(gCoord.x, y, gCoord.z, .5);
			puff->Delta.x = puff->Delta.y = puff->Delta.z = 0;
			puff->SpecialF[0] = 1.5;								// decay rate
			return;
		}
		UpdateObject(theNode);
	}
}








