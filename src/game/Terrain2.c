/****************************/
/*   	TERRAIN2.C 	        */
/****************************/


#include "globals.h"
#include "terrain.h"
#include "misc.h"
#include "main.h"
#include "objects.h"
#include "mobjtypes.h"
#include "items.h"
#include "pickups.h"
#include "triggers.h"
#include "3dmath.h"
#include "enemy.h"
#include "mytraps.h"
#include "timeportal.h"
#include "structformats.h"

/***************/
/* EXTERNALS   */
/***************/

extern	NewObjectDefinitionType	gNewObjectData;
extern	long	gTerrainTileWidth,gTerrainTileDepth;
extern	Ptr		gTerrainPtr;
extern	long	gTerrainItemDeleteWindow_Near,gTerrainItemDeleteWindow_Far,
				gTerrainItemDeleteWindow_Left,gTerrainItemDeleteWindow_Right;
extern	ObjNode		*gThisNodePtr;
extern	UInt32		gControllerA;
extern	long		gDX,gDY,gDZ,gX,gY,gZ;
extern	long		gMyStartX,gMyStartZ;
extern	long	gNumTerrainTextureTiles;
extern	long	*gTerrainTexturePtrs;
extern	long	gNumSuperTilesDeep,gNumSuperTilesWide,gCurrentActiveBuildingID;
extern	Boolean		gLevelHasHeightMaps[];
extern	long	gTerrainUnitWidth,gTerrainUnitDepth;
extern	UInt16	**gTerrainHeightMapLayer,**gTerrainPathLayer;
extern	TQ3Point3D	gCoord,gMyCoord;
extern	Byte		gMyStartAim;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void PreloadAllTiles(void);
static void RestoreItemList(void);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

short	  				gNumTerrainItems;
TerrainItemEntryType	**gTerrainItemLookupTableX = nil;
TerrainItemEntryType 	*gMasterItemList = nil;

Ptr						gMaxItemAddress;					// addr of last item in current item list


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_ITEM_NUM	19					// for error checking!

Boolean (*gTerrainItemAddRoutines[])(TerrainItemEntryType *, long, long) =
{
		NilAdd,								// My Start Coords
		AddPowerUp,							// 1: PowerUp
		AddEnemy_Tricer,					// 2: Triceratops enemy
		AddEnemy_Rex,						// 3: Rex enemy
		AddLavaPatch,						// 4: Lava patch
		AddEgg,								// Egg
		AddGasVent,							// 6:	Gas vent
		AddEnemy_Ptera,						// 7: Pteranodon enemy
		AddEnemy_Stego,						// 8:  Stegosaurus enemy
		AddTimePortal,						// 9: time portal
		AddTree,							// 10: tree
		AddBoulder,							// 11: boulder
		AddMushroom,						// 12: mushroom
		AddBush,							// 13: bush
		AddWaterPatch,						// 14: water patch
		AddCrystal,							// 15: crystal
		AddEnemy_Spitter,					// 16: spitter enemy
		AddStepStone,						// 17: step stone
		AddRollingBoulder,					// 18: rolling boulder
		AddSporePod							// 19: spore pod
};


/********************* BUILD TERRAIN ITEM LIST ***********************/
//
// Build sorted lists of terrain items
//

void BuildTerrainItemList(void)
{
long			offset;
SInt32			*longPtr;
long			col,itemCol,itemNum,nextCol,prevCol;
TerrainItemEntryType *lastPtr;

			/* ALLOC MEMORY FOR LOOKUP TABLE */

	if (gTerrainItemLookupTableX != nil)
		DisposePtr((Ptr)gTerrainItemLookupTableX);
	gTerrainItemLookupTableX = (TerrainItemEntryType **)NewPtr(sizeof(TerrainItemEntryType *)*gNumSuperTilesWide);


					/* GET BASIC INFO */

	offset = *((SInt32 *)(gTerrainPtr+12));							// get offset to OBJECT_LIST
	longPtr = (SInt32  *)(gTerrainPtr+offset);	  					// get pointer to OBJECT_LIST

	SInt32 numTerrainItemsAsLong = *longPtr++;						// get # items in file
	ByteswapInts(sizeof(SInt32), 1, &numTerrainItemsAsLong);
	gNumTerrainItems = numTerrainItemsAsLong;
	if (gNumTerrainItems == 0)
		return;

	gMasterItemList = (TerrainItemEntryType *)longPtr;				// point to items in file
	ByteswapStructs(STRUCTFORMAT_TerrainItemEntryType, sizeof(TerrainItemEntryType), gNumTerrainItems, gMasterItemList);


				/* BUILD HORIZ LOOKUP TABLE */

	gMaxItemAddress = (Ptr)&gMasterItemList[gNumTerrainItems-1]; 	// remember addr of last item

	lastPtr = &gMasterItemList[0];
	nextCol = 0;													// start @ col 0
	prevCol = -1;

	for (itemNum = 0; itemNum < gNumTerrainItems; itemNum++)
	{
		itemCol = gMasterItemList[itemNum].x / (SUPERTILE_SIZE*OREOMAP_TILE_SIZE);	// get column of item (supertile relative)
		if (itemCol >= gNumSuperTilesWide)
		{
			DoAlert("Warning! Item off right side of universe!");
			goto trail;
		}
		else
		if (itemCol < 0)
		{
			DoAlert("Warning! Item off left side of universe!");
			goto trail;
		}

		if (itemCol < prevCol)										// see if ERROR - list not sorted correctly!!!
			DoAlert("Error! ObjectList not sorted right!");

		if (itemCol > prevCol)										// see if changed
		{
			for (col = nextCol; col <= itemCol; col++)				// filler pointers
				gTerrainItemLookupTableX[col] = &gMasterItemList[itemNum];

			prevCol = itemCol;
			nextCol = itemCol+1;
			lastPtr = &gMasterItemList[itemNum];
		}
	}
trail:
	for (col = nextCol; col < gNumSuperTilesWide; col++)				// set trailing column pointers
	{
		gTerrainItemLookupTableX[col] = lastPtr;
	}
	
	
			/*********************/
			/* BUILD LINKED LIST */
			/*********************/
			
		/* MANUALLY BUILD 1ST & LAST NODES */
			
	gMasterItemList[0].prevItemIdx = -1;													// 1st has no prev
	gMasterItemList[0].nextItemIdx = 1;														// link to next

	gMasterItemList[gNumTerrainItems-1].prevItemIdx = gNumTerrainItems-2;					// 1st has no prev
	gMasterItemList[gNumTerrainItems-1].nextItemIdx = -1;									// last has no next
	
	
			/* LINK ALL THE OTHERS */
				
	for (itemNum = 1; itemNum < (gNumTerrainItems-1); itemNum++)
	{
		gMasterItemList[itemNum].prevItemIdx = itemNum - 1;
		gMasterItemList[itemNum].nextItemIdx = itemNum + 1;
	}		
	
}



/******************** FIND MY START COORD ITEM *******************/
//
// Scans thru item list for item type #14 which is a teleport reciever / start coord,
// or scans for receiving teleporter (#11) if gTeleportInfo.activateFlag is set.
//

void FindMyStartCoordItem(void)
{
long	i;

				/* SCAN FOR "START COORD" ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
		if (gMasterItemList[i].type == MAP_ITEM_MYSTARTCOORD)							// see if it's a MyStartCoord item
		{
			gMyCoord.x = gMyStartX = (gMasterItemList[i].x * MAP2UNIT_VALUE);	// convert to world coords
			gMyCoord.z = gMyStartZ = gMasterItemList[i].y * MAP2UNIT_VALUE;
			gMyStartAim = gMasterItemList[i].parm[0];							// get aim 0..7
			return;
		}

	DoAlert("No Start Coord or Teleporter Item Found!");

	gMyStartX = 0;
	gMyStartZ = 0;
}



/****************** SCAN FOR PLAYFIELD ITEMS *******************/
//
// Given this range, scan for items.  Coords are in supertile relative row/col values.
//

void ScanForPlayfieldItems(long top, long bottom, long left, long right)
{
TerrainItemEntryType *itemPtr;
long			type;
Boolean			flag;
long			maxX,minX,maxY,minY;
long			realX,realZ;

	if (gNumTerrainItems == 0)
		return;

	itemPtr = gTerrainItemLookupTableX[left];					// get pointer to 1st item at this X

	minX = left*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);
	maxX = right*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);			// calc min/max coords to be in range (map relative)
	maxX += (SUPERTILE_SIZE*OREOMAP_TILE_SIZE)-1;

	minY = top*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);
	maxY = bottom*(SUPERTILE_SIZE*OREOMAP_TILE_SIZE);
	maxY += (SUPERTILE_SIZE*OREOMAP_TILE_SIZE)-1;

	while ((itemPtr->x >= minX) && (itemPtr->x <= maxX)) 		// check all items in this column range
	{
		if ((itemPtr->y >= minY) && (itemPtr->y <= maxY))		// & this row range
		{
					/* ADD AN ITEM */

			if (!(itemPtr->flags&ITEM_FLAGS_INUSE))				// see if item available
			{
				type = itemPtr->type;							// get item #
				if (type > MAX_ITEM_NUM)						// error check!
				{
					DoAlert("Illegal Map Item Type!");
					ShowSystemErr(type);
				}
				else
				{
					realX = itemPtr->x * MAP2UNIT_VALUE;		// calc & pass 3-space coords
					realZ = itemPtr->y * MAP2UNIT_VALUE;
			
					flag = gTerrainItemAddRoutines[type](itemPtr,realX, realZ); 	// call item's ADD routine
					if (flag)
						itemPtr->flags |= ITEM_FLAGS_INUSE;		// set in-use flag
				}
			}
		}
		if (itemPtr->nextItemIdx < 0)							// see if that was the end of the linked list
			break;
		else
			itemPtr = &gMasterItemList[itemPtr->nextItemIdx];	// point to next item in linked list
	}
}


/******************** NIL ADD ***********************/
//
// nothing add
//

Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z)
{
	return(false);
}


/***************** TRACK TERRAIN ITEM ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem(ObjNode *theNode)
{
long	x,z;

	x = (theNode->Coord.x);
	z = (theNode->Coord.z);

	if (x < gTerrainItemDeleteWindow_Left)
		return(true);
	if (x > gTerrainItemDeleteWindow_Right)
		return(true);
	if (z < gTerrainItemDeleteWindow_Far)
		return(true);
	if (z > gTerrainItemDeleteWindow_Near)
		return(true);

	return(false);
}

/***************** TRACK TERRAIN ITEM FAR ******************/
//
// Returns true if theNode is out of range
//
// INPUT: range = INTEGER range to add to delete window
//

Boolean TrackTerrainItem_Far(ObjNode *theNode, long range)
{
long	x,z;

	x = (theNode->Coord.x);
	z = (theNode->Coord.z);

	if (x < (gTerrainItemDeleteWindow_Left-range))
		return(true);
	if (x > (gTerrainItemDeleteWindow_Right+range))
		return(true);
	if (z < (gTerrainItemDeleteWindow_Far + range))
		return(true);
	if (z > (gTerrainItemDeleteWindow_Near - range))
		return(true);

	return(false);
}

/***************** GET PATH TILE NUM ******************/
//
// Given a world x/z coord, return the path tile # there.
// NOTE: does it by calculating the row/col and then calling other routine.
//
// INPUT: x/z = world coords
//
// OUTPUT: tile #
//

UInt16	GetPathTileNum(float x, float z)
{
long	row,col,y;

	y = z;

	if ((x < 0) || (y < 0))										// see if out of bounds
		return(0);
	if ((x >= gTerrainUnitWidth) || (y >= gTerrainUnitDepth))
		return(0);

	col = x*gOneOver_TERRAIN_POLYGON_SIZE;	 							// calc map row/col that the coord lies on
	row = y*gOneOver_TERRAIN_POLYGON_SIZE;

	return(GetPathTileNumAtRowCol(row,col));
 }


/***************** GET PATH TILE NUM AT ROW COL ******************/
//
// Given a row,col coord, return the path tile # there
//
// INPUT: ROW/COL = world coords
//
// OUTPUT: tile #
//

UInt16	GetPathTileNumAtRowCol(long row, long col)
{
UInt16 tile;

	tile = gTerrainPathLayer[row][col];							// get path data from map
	tile = tile&TILENUM_MASK; 							  		// filter out tile # 

	return(tile);
 }

/*************************** ROTATE ON TERRAIN ***************************/
//
// Rotates an object's x & z such that it's lying on the terrain.
//
// INPUT:	theNode = the object to rotate
//			sideOff = distance from origin to side
//			endOff = distance from origin to ends
//

void RotateOnTerrain(ObjNode *theNode, float sideOff, float endOff)
{
TQ3Point3D	front,back,left,right;
float	rotY;
float	sinRot,cosRot;
TQ3Matrix4x4	*matrix = &theNode->BaseTransformMatrix;
TQ3Vector3D		lookAt,upVector,theXAxis;

	Q3Matrix4x4_SetIdentity(matrix);											// init the matrix
	
	rotY = theNode->Rot.y;
	
	
			/* CALC TERRAIN HEIGHT IN FRONT,BACK,LEFT & RIGHT */

	sinRot = sin(rotY)*endOff;
	cosRot = cos(rotY)*endOff;

	front.x = gCoord.x - sinRot;
	front.z = gCoord.z - cosRot;
	front.y = GetTerrainHeightAtCoord(front.x, front.z);

	back.x = gCoord.x + sinRot;
	back.z = gCoord.z + cosRot;
	back.y = GetTerrainHeightAtCoord(back.x, back.z);


	sinRot = sin(rotY+(PI/2))*sideOff;
	cosRot = cos(rotY+(PI/2))*sideOff;

	right.x = gCoord.x + sinRot;
	right.z = gCoord.z + cosRot;
	right.y = GetTerrainHeightAtCoord(right.x, right.z);

	left.x = gCoord.x - sinRot;
	left.z = gCoord.z - cosRot;
	left.y = GetTerrainHeightAtCoord(left.x, left.z);


			/* CALC LOOK-AT VECTOR */

	{
		// Source port fix: flipped sign so the quad points the correct way
		lookAt.x = back.x-front.x;
		lookAt.y = back.y-front.y;
		lookAt.z = back.z-front.z;
		Q3Vector3D_Normalize(&lookAt,&lookAt);
	}



	matrix->value[2][0] = lookAt.x;
	matrix->value[2][1] = lookAt.y;
	matrix->value[2][2] = lookAt.z;
	

			/* CALC UP VECTOR */

	{
		theXAxis.x = right.x - left.x;							// first calc left->right vector
		theXAxis.y = right.y - left.y;	
		theXAxis.z = right.z - left.z;	
		Q3Vector3D_Normalize(&theXAxis,&theXAxis);
	}

	Q3Vector3D_Cross(&theXAxis,&lookAt, &upVector);				// cross product to get upVector

	matrix->value[1][0] = upVector.x;
	matrix->value[1][1] = upVector.y;
	matrix->value[1][2] = upVector.z;


		/* CALC THE X-AXIS VECTOR */
		
	matrix->value[0][0] = theXAxis.x;
	matrix->value[0][1] = theXAxis.y;
	matrix->value[0][2] = theXAxis.z;

		/* POP IN THE TRANSLATE INTO THE MATRIX */
			
	matrix->value[3][0] = theNode->Coord.x;
	matrix->value[3][1] = theNode->Coord.y;
	matrix->value[3][2] = theNode->Coord.z;


			/* SET SCALE IF ANY */
			
	if ((theNode->Scale.x != 1) || (theNode->Scale.y != 1) || (theNode->Scale.z != 1))		// see if ignore scale
	{
		TQ3Matrix4x4	matrix2;
		
		Q3Matrix4x4_SetScale(&matrix2, theNode->Scale.x,		// make scale matrix
								 theNode->Scale.y,			
								 theNode->Scale.z);
		Q3Matrix4x4_Multiply(&matrix2, matrix, matrix);

	}

	SetObjectTransformMatrix(theNode);							// update the matrix

}























