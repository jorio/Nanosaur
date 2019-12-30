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
#include "timeportal.h"
#include "mobjtypes.h"
#include "qd3d_geometry.h"
#include "collision.h"
#include "3dmath.h"
#include "infobar.h"
#include "sound2.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	TQ3Point3D			gCoord,gMyCoord;
extern	TQ3Vector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	short				gNumTerrainItems;
extern	TerrainItemEntryType 	*gMasterItemList;
extern	unsigned long 	gInfobarUpdateBits;
extern	Boolean				gWonGameFlag,gGameOverFlag;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveTimePortal(ObjNode *theNode);
static void FindTimePortals(void);
static void MoveTimePortalRing(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TIME_PORTALS	8

#define	PORTAL_ACTIVE_TIME		(LEVEL_DURATION/gNumTimePortals)		// even spread for level duration
		
#define	PORTAL_YOFF			250.0f
#define	RING_START_SCALE	5.0f
#define	RING_START_SCALE_EXIT	(5.0 * 1.5)


/*********************/
/*    VARIABLES      */
/*********************/

short			gNumTimePortals = 0;
TimePortalType	gTimePortalList[MAX_TIME_PORTALS];


/******************** INIT TIME PORTALS ***********************/

void InitTimePortals(void)
{
			/* CREATE THE TIME PORTAL LIST & INIT STRUCTS */
			
	FindTimePortals();
		
}


/******************** FIND TIME PORTALS *******************/
//
// Finds the time portals in the map & creates a sorted list
//

static void FindTimePortals(void)
{
long	i,n;

	gNumTimePortals = 0;

				/* SCAN FOR TIME PORTAL ITEM ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
		if (gMasterItemList[i].type == MAP_ITEM_TIMEPORTAL)
		{
			n = gMasterItemList[i].parm[0];										// parm0 = portal #
			gTimePortalList[n].coord.x = gMasterItemList[i].x * MAP2UNIT_VALUE;	// convert to world coords
			gTimePortalList[n].coord.y = gMasterItemList[i].y * MAP2UNIT_VALUE;
			gNumTimePortals++;
		}
}


/********************** ADD TIME PORTAL **************************/

Boolean AddTimePortal(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	newObj = MakeTimePortal(PORTAL_TYPE_EGG,x,z);
	newObj->TerrainItemPtr = itemPtr;					// keep ptr to item list
	return(true);
}



/************************* MAKE TIME PORTAL *********************************/
//
// INPUT: 	portalType = egg or my way in/out
//			x/z = coords for my portal (ignored for eggs)
//

ObjNode *MakeTimePortal(Byte portalType, float x, float z)
{
ObjNode	*newObj;

			/*******************************/
			/* MAKE INVISIBLE EMITTER UNIT */
			/*******************************/
			
	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.coord.x = x;
	gNewObjectDefinition.coord.y = GetTerrainHeightAtCoord_Planar(x,z);
	gNewObjectDefinition.coord.z = z;
	gNewObjectDefinition.flags = STATUS_BIT_ALWAYSCULL;
	gNewObjectDefinition.slot = 100;
	gNewObjectDefinition.moveCall = MoveTimePortal;
	newObj = MakeNewObject(&gNewObjectDefinition);
	if (newObj)
	{
		newObj->Radius = 1000;								// for culling
		
		newObj->Kind = portalType;							// remember which type of portal this is

				/* SET COLLISION INFO */
				
		newObj->CType = CTYPE_PORTAL;
		newObj->CBits = CBITS_TOUCHABLE;
		
		SetObjectCollisionBounds(newObj,500,0,-60,60,60,-60);

		newObj->SpecialF[0] = 0;						// ring emitter timer
	}

	return(newObj);									// item was added
}


/****************** MOVE TIME PORTAL *********************/

static void MoveTimePortal(ObjNode *theNode)
{
ObjNode	*newObj;
float	fps = gFramesPerSecondFrac;
float	y;

				/* SEE IF OUT OF RANGE */

	if (theNode->TerrainItemPtr)											// only if from terrain
		if (TrackTerrainItem(theNode))						
		{
			Nano_DeleteObject(theNode);
			return;
		}

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)							// see if not visible
		return;

	y = GetTerrainHeightAtCoord_Planar(theNode->Coord.x,theNode->Coord.z);	// recalc y
	if (y != theNode->Coord.y)
	{
		theNode->Coord.y = y;
		CalcObjectBoxFromNode(theNode);
	}

			/* SEE IF EMIT A PULSE RING */
			
	theNode->SpecialF[0] += fps;
	if (theNode->SpecialF[0] > .3f)
	{		
		theNode->SpecialF[0] = 0;											// reset timer
		
		gNewObjectDefinition.group = GLOBAL_MGroupNum_TimePortalRing;		// make ring object
		gNewObjectDefinition.type = GLOBAL_MObjType_TimePortalRing;	
		gNewObjectDefinition.coord.x = theNode->Coord.x;
		
		if (theNode->Kind == PORTAL_TYPE_ENTER)
			gNewObjectDefinition.coord.y = theNode->Coord.y + 400;
		else
			gNewObjectDefinition.coord.y = theNode->Coord.y + 15;
		
		gNewObjectDefinition.coord.z = theNode->Coord.z;
		gNewObjectDefinition.flags = 0;
		gNewObjectDefinition.slot = SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall = MoveTimePortalRing;
		gNewObjectDefinition.rot = 0;
		
		switch(theNode->Kind)
		{
			case	PORTAL_TYPE_ENTER:
					gNewObjectDefinition.scale = 1.0;	
					break;
					
			case	PORTAL_TYPE_EXIT:
					gNewObjectDefinition.scale = RING_START_SCALE_EXIT;
					break;

			case	PORTAL_TYPE_EGG:
					gNewObjectDefinition.scale = RING_START_SCALE;
					break;					
		}
			
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		if (newObj)
		{
			newObj->SpecialF[4] = theNode->Coord.y;				// remember floor y
			newObj->Kind = theNode->Kind;
		
			if (theNode->Kind == PORTAL_TYPE_ENTER)
			{
				newObj->Delta.y = -400;
				newObj->Health = 1;
			}
			else
				newObj->Delta.y = 50;
			MakeObjectKeepBackfaces(newObj);
			MakeObjectTransparent(newObj,1);
		}
	}
	
			/* SEE IF EXIT PORTAL IS DONE */
			
	if (theNode->Kind == PORTAL_TYPE_EXIT)
	{
		theNode->SpecialF[1] += fps;
		if (theNode->SpecialF[1] > 5)
		{
			gGameOverFlag = gWonGameFlag = true;
		}
	}		
}



/****************** MOVE TIME PORTAL RING *********************/

static void MoveTimePortalRing(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
	
	
	switch(theNode->Kind)
	{					/*********/
						/* ENTER */
						/*********/
				
		case	PORTAL_TYPE_ENTER:
		
						/* SEE IF GROW IT */
								
				if (theNode->Coord.y < (theNode->SpecialF[4]+30))
				{
					theNode->Coord.y -= fps * 20.0f;
					
					theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 5.0f;
					theNode->Health -= fps * .6f;
					if (theNode->Health <= 0.0f)
					{
						Nano_DeleteObject(theNode);
						return;
					}
						
				}
				
						/* DOWN TUBE */
				else
				{
					theNode->Delta.y += 200.0f * fps;						// climb down
					theNode->Coord.y += theNode->Delta.y * fps;
					
					theNode->Health -= fps * 0.3f;						// decay it
					if (theNode->Health <= 0.0f)
					{
						Nano_DeleteObject(theNode);
						return;
					}		
				}
				break;
				
				
					/*******/				
					/* EGG */
					/*******/
								
		case	PORTAL_TYPE_EGG:
		
						/* SEE IF SHRINK IT */
								
				if (theNode->Scale.x != 1.0f)
				{
					theNode->Coord.y += fps * 20.0f;
					
					theNode->Scale.x = theNode->Scale.y = theNode->Scale.z -= fps * 5.0f;
					if (theNode->Scale.x < 1.0f)
						theNode->Scale.x = theNode->Scale.y = theNode->Scale.z = 1.0f;	
						
					theNode->Health = (RING_START_SCALE - theNode->Scale.x) / (RING_START_SCALE-1);
				}
				
						/* CLIMB TUBE */
				else
				{
					theNode->Delta.y += 250.0f * fps;						// climb up
					theNode->Coord.y += theNode->Delta.y * fps;
					
					theNode->Health -= fps * 0.6f;						// decay it
					if (theNode->Health <= 0.0f)
					{
						Nano_DeleteObject(theNode);
						return;
					}		
				}
				break;
				
					/********/
					/* EXIT */
					/********/
				
		case	PORTAL_TYPE_EXIT:
						/* SEE IF SHRINK IT */
								
				if (theNode->Scale.x != 2.0f)
				{
					theNode->Coord.y += fps * 30.0f;
					
					theNode->Scale.x = theNode->Scale.y = theNode->Scale.z -= fps * 5.0f;
					if (theNode->Scale.x < 2.0f)
						theNode->Scale.x = theNode->Scale.y = theNode->Scale.z = 2.0f;	
						
					theNode->Health = (RING_START_SCALE_EXIT - theNode->Scale.x) / (RING_START_SCALE_EXIT-1);
				}
				
						/* CLIMB TUBE */
				else
				{
					theNode->Delta.y += 270.0f * fps;						// climb up
					theNode->Coord.y += theNode->Delta.y * fps;
					
					theNode->Health -= fps * 0.3f;						// decay it
					if (theNode->Health <= 0.0f)
					{
						Nano_DeleteObject(theNode);
						return;
					}		
				}
				break;
	}

	MakeObjectTransparent(theNode,theNode->Health);	
	UpdateObjectTransforms(theNode);
}


/********************** FIND CLOSEST PORTAL **********************/

short	FindClosestPortal(void)
{
int	i;
float	min = 100000000,d;
short	close;

	for (i =0; i < gNumTimePortals; i++)
	{
		d = CalcQuickDistance(gTimePortalList[i].coord.x,gTimePortalList[i].coord.y,gMyCoord.x, gMyCoord.z);
		if (d < min)
		{
			min = d;
			close = i;
		}
	}

	return(close);
}







