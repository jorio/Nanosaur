/****************************/
/*   	COLLISION.c		    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <QD3DGeometry.h>
#include <QD3DTransform.h>
#include <QD3DGroup.h>
#include "globals.h"
#include "objects.h"
#include "collision.h"
#include "3dmath.h"
#include "triggers.h"
#include "terrain.h"
#include "misc.h"
#include "pickups.h"

 
extern	TQ3Point3D	gCoord;
extern	TQ3Vector3D	gDelta;
extern	TQ3Matrix4x4	gWorkMatrix;
extern	ObjNode		*gFirstNodePtr;
extern	long	gTerrainUnitDepth,gTerrainUnitWidth;
extern	float		gFramesPerSecond,gFramesPerSecondFrac;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void AddBGCollisions(ObjNode *theNode, float realDX, float realDZ, UInt32 cType);
static void AllocateCollisionTriangleMemory(ObjNode *theNode, long numTriangles);
static void GetTrianglesFromTriMesh(TQ3Object obj);
static void ScanForTriangles_Recurse(TQ3Object obj);
static void AddTriangleCollision(ObjNode *thisNode, float x, float y, float z, float oldX, float oldZ, long bottomSide, long oldBottomSide);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_COLLISIONS				60
#define	MAX_TEMP_COLL_TRIANGLES		300


/****************************/
/*    VARIABLES             */
/****************************/


CollisionRec	gCollisionList[MAX_COLLISIONS];
short			gNumCollisions = 0;
Byte			gTotalSides;

short				gNumCollTriangles;
CollisionTriangleType	gCollTriangles[MAX_TEMP_COLL_TRIANGLES];
TQ3BoundingBox		gCollTrianglesBBox;


/******************* COLLISION DETECT *********************/
//
// NOTE: assumes baseNode only has 1 collision box.
//

void CollisionDetect(ObjNode *baseNode,unsigned long CType)
{
ObjNode 	*thisNode;
long		sideBits,cBits;
float		relDX,relDY,relDZ;						// relative deltas
float		realDX,realDY,realDZ;					// real deltas
float		x,y,z,oldX,oldZ;
short		numBaseBoxes,targetNumBoxes,target;
CollisionBoxType *baseBoxList;
CollisionBoxType *targetBoxList;
long		leftSide,rightSide,frontSide,backSide,bottomSide,topSide,oldBottomSide;

	gNumCollisions = 0;								// clear list
	gTotalSides = 0;

			/* GET BASE BOX INFO */
			
	numBaseBoxes = baseNode->NumCollisionBoxes;
	if (numBaseBoxes == 0)
		return;
	baseBoxList = baseNode->CollisionBoxes;

	leftSide = baseBoxList->left;
	rightSide = baseBoxList->right;
	frontSide = baseBoxList->front;
	backSide = baseBoxList->back;
	bottomSide = baseBoxList->bottom;
	topSide = baseBoxList->top;
	oldBottomSide = baseBoxList->oldBottom;


	x = gCoord.x;									// copy coords into registers
	y = gCoord.y;
	z = gCoord.z;

		/* DETERMINE REAL DELTA FROM PREVIOUS POSITION */

	realDX = x - (oldX = baseNode->OldCoord.x);
	realDY = y - baseNode->OldCoord.y;
	realDZ = z - (oldZ = baseNode->OldCoord.z);


			/****************************/		
			/* SCAN AGAINST ALL OBJECTS */
			/****************************/		
		
	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;
		
		if (!(thisNode->CType & CType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;		
				
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;
	
		if (thisNode == baseNode)								// dont collide against itself
			goto next;
	
		if (baseNode->ChainNode == thisNode)					// don't collide against its own chained object
			goto next;
			
				/*************************************/		
				/* FIRST DO COLLISION TRIANGLE CHECK */
				/*************************************/		
			
		if (thisNode->CollisionTriangles)
		{
			AddTriangleCollision(thisNode, x, y, z, oldX, oldZ, bottomSide, oldBottomSide);
		}
			
				/******************************/		
				/* NOW DO COLLISION BOX CHECK */
				/******************************/		
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// see if target has any boxes
		if (targetNumBoxes)
		{
			targetBoxList = thisNode->CollisionBoxes;
		
		
				/******************************************/
				/* CHECK BASE BOX AGAINST EACH TARGET BOX */
				/*******************************************/
				
			for (target = 0; target < targetNumBoxes; target++)
			{
						/* DO RECTANGLE INTERSECTION */
			
				if (rightSide < targetBoxList[target].left)
					continue;
					
				if (leftSide > targetBoxList[target].right)
					continue;
					
				if (frontSide < targetBoxList[target].back)
					continue;
					
				if (backSide > targetBoxList[target].front)
					continue;
					
				if (bottomSide > targetBoxList[target].top)
					continue;

				if (topSide < targetBoxList[target].bottom)
					continue;
					
									
						/* THERE HAS BEEN A COLLISION SO CHECK WHICH SIDE PASSED THRU */
			
				sideBits = 0;
				cBits = thisNode->CBits;					// get collision info bits
								
				if (cBits & CBITS_TOUCHABLE)				// if it's generically touchable, then add it without side info
					goto got_sides;
				
				relDX = realDX - thisNode->Delta.x;			// calc relative deltas
				relDY = realDY - thisNode->Delta.y;				
				relDZ = realDZ - thisNode->Delta.z;				
			

								/* CHECK FRONT COLLISION */
			
				if ((cBits & SIDE_BITS_BACK) && (relDZ > 0))						// see if target has solid back & we are going relatively +Z
				{
					if (baseBoxList->oldFront < targetBoxList[target].oldBack)		// get old & see if already was in target (if so, skip)
					{
						if ((baseBoxList->front >= targetBoxList[target].back) &&	// see if currently in target
							(baseBoxList->front <= targetBoxList[target].front))
							sideBits = SIDE_BITS_FRONT;
					}
				}
				else			
								/* CHECK BACK COLLISION */
			
				if ((cBits & SIDE_BITS_FRONT) && (relDZ < 0))						// see if target has solid front & we are going relatively -Z
				{
					if (baseBoxList->oldBack > targetBoxList[target].oldFront)		// get old & see if already was in target	
						if ((baseBoxList->back <= targetBoxList[target].front) &&	// see if currently in target
							(baseBoxList->back >= targetBoxList[target].back))
							sideBits = SIDE_BITS_BACK;
				}

		
								/* CHECK RIGHT COLLISION */
			
			
				if ((cBits & SIDE_BITS_LEFT) && (relDX > 0))						// see if target has solid left & we are going relatively right
				{
					if (baseBoxList->oldRight < targetBoxList[target].oldLeft)		// get old & see if already was in target	
						if ((baseBoxList->right >= targetBoxList[target].left) &&	// see if currently in target
							(baseBoxList->right <= targetBoxList[target].right))
							sideBits |= SIDE_BITS_RIGHT;
				}
				else

							/* CHECK COLLISION ON LEFT */

				if ((cBits & SIDE_BITS_RIGHT) && (relDX < 0))						// see if target has solid right & we are going relatively left
				{
					if (baseBoxList->oldLeft > targetBoxList[target].oldRight)		// get old & see if already was in target	
						if ((baseBoxList->left <= targetBoxList[target].right) &&	// see if currently in target
							(baseBoxList->left >= targetBoxList[target].left))
							sideBits |= SIDE_BITS_LEFT;
				}	

								/* CHECK TOP COLLISION */
			
			
				if ((cBits & SIDE_BITS_BOTTOM) && (relDY > 0))						// see if target has solid bottom & we are going relatively up
				{
					if (baseBoxList->oldTop < targetBoxList[target].oldBottom)		// get old & see if already was in target	
						if ((baseBoxList->top >= targetBoxList[target].bottom) &&	// see if currently in target
							(baseBoxList->top <= targetBoxList[target].top))
							sideBits |= SIDE_BITS_TOP;
				}
				else

							/* CHECK COLLISION ON BOTTOM */

				if ((cBits & SIDE_BITS_TOP) && (relDY < 0))							// see if target has solid top & we are going relatively down
				{
					if (baseBoxList->oldBottom > targetBoxList[target].oldTop)		// get old & see if already was in target	
					{
						if ((baseBoxList->bottom <= targetBoxList[target].top) &&	// see if currently in target
							(baseBoxList->bottom >= targetBoxList[target].bottom))
						{
							sideBits |= SIDE_BITS_BOTTOM;
						}
					}
				}	

								 /* SEE IF ANYTHING TO ADD */
										 							 
				if (!sideBits)														// see if anything actually happened
					continue;

						/* ADD TO COLLISION LIST */
got_sides:
				gCollisionList[gNumCollisions].baseBox = 0;
				gCollisionList[gNumCollisions].targetBox = target;
				gCollisionList[gNumCollisions].sides = sideBits;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;	
				gTotalSides |= sideBits;											// remember total of this
			}
		}
next:	
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);


				/*******************************/
				/*   DO BACKGROUND COLLISIONS  */
				/*******************************/
				//
				// it's important to do these last so that any
				// collision will take final priority!
				//

	if (CType & CTYPE_BGROUND)														// see if do BG collision
		AddBGCollisions(baseNode,realDX,realDZ, CType);


	if (gNumCollisions > MAX_COLLISIONS)											// see if overflowed (memory corruption ensued)
		DoFatalAlert("CollisionDetect: gNumCollisions > MAX_COLLISIONS");
}


/***************** HANDLE COLLISIONS ********************/
//
// This is a generic collision handler.  Takes care of
// all processing.
//
// INPUT:  cType = CType bit mask for collision matching
//
// OUTPUT: totalSides
//

Byte HandleCollisions(ObjNode *theNode, unsigned long	cType)
{
Byte		totalSides;
short		i;
float		originalX,originalY,originalZ;
long		offset,maxOffsetX,maxOffsetZ,maxOffsetY;
float		offXSign,offZSign,offYSign;
Byte		base,target;
ObjNode		*targetObj = nil;
CollisionBoxType *baseBoxPtr = nil, *targetBoxPtr = nil;
long		leftSide,rightSide,frontSide,backSide,bottomSide;
CollisionBoxType *boxList = nil;

	theNode->PlatformNode = nil;							// assume not on any platforms

	CalcObjectBoxFromGlobal(theNode);						// calc current collision box

	CollisionDetect(theNode,cType);							// get collision info
	
	originalX = gCoord.x;									// remember starting coords
	originalY = gCoord.y;									
	originalZ = gCoord.z;								
	totalSides = 0;
	maxOffsetX = maxOffsetZ = maxOffsetY = 0;
	offXSign = offYSign = offZSign = 0;

			/* GET BASE BOX INFO */
			
	if (theNode->NumCollisionBoxes == 0)					// it's gotta have a collision box
		return(0);
	boxList = theNode->CollisionBoxes;
	leftSide = boxList->left;
	rightSide = boxList->right;
	frontSide = boxList->front;
	backSide = boxList->back;
	bottomSide = boxList->bottom;


			/* SCAN THRU ALL RETURNED COLLISIONS */	
	
	for (i=0; i < gNumCollisions; i++)						// handle all collisions
	{
		totalSides |= gCollisionList[i].sides;				// keep sides info
		base = gCollisionList[i].baseBox;					// get collision box index for base & target
		target = gCollisionList[i].targetBox;
		targetObj = gCollisionList[i].objectPtr;			// get ptr to target objnode
		
		baseBoxPtr = boxList + base;						// calc ptrs to base & target collision boxes
		if (targetObj)
		{
			targetBoxPtr = targetObj->CollisionBoxes;	
			targetBoxPtr += target;
		}
		
				/*********************************************/
				/* HANDLE ANY SPECIAL OBJECT COLLISION TYPES */
				/*********************************************/
				
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
				/* HANDLE TRIGGERS */
		
			if ((targetObj->CType & CTYPE_TRIGGER) && (cType & CTYPE_TRIGGER))	// target must be trigger and we must have been looking for them as well
			{
				if (!HandleTrigger(targetObj,theNode,gCollisionList[i].sides))	// returns false if handle as non-solid trigger
					gCollisionList[i].sides = 0;
					
				maxOffsetX = gCoord.x - originalX;								// see if trigger caused a move
				maxOffsetZ = gCoord.z - originalZ;				
			}		
		}

		if (gCollisionList[i].type == COLLISION_TYPE_TILE)
		{
			
					/**************************/
					/* HANDLE TILE COLLISIONS */
					/**************************/

			if (gCollisionList[i].sides & SIDE_BITS_BACK)						// SEE IF HIT BACK
			{
				offset = TERRAIN_POLYGON_SIZE-(backSide%TERRAIN_POLYGON_SIZE);	// see how far over it went
				maxOffsetZ = offset;
				offZSign = 1;
				gDelta.z = 0;													// stop my dz
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_FRONT)						// SEE IF HIT FRONT
			{
				offset = (frontSide%TERRAIN_POLYGON_SIZE)+1; 					// see how far over it went
				maxOffsetZ = offset;
				offZSign = -1;
				gDelta.z = 0;													// stop my dz
				
			}

			if (gCollisionList[i].sides & SIDE_BITS_LEFT)						// SEE IF HIT LEFT
			{
				offset = TERRAIN_POLYGON_SIZE-(leftSide%TERRAIN_POLYGON_SIZE);	// see how far over it went
				maxOffsetX = offset;
				offXSign = 1;
				gDelta.x = 0;													// stop my dx
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_RIGHT)						// SEE IF HIT RIGHT
			{
				offset = (rightSide%TERRAIN_POLYGON_SIZE)+1;					// see how far over it went
				maxOffsetX = offset;
				offXSign = -1;
				gDelta.x = 0;													// stop my dx
			}

		}
		else

					/********************************/
					/* HANDLE OBJECT COLLISIONS 	*/	
					/********************************/

		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			if (gCollisionList[i].sides & SIDE_BITS_BACK)						// SEE IF BACK HIT
			{
				offset = (targetBoxPtr->front - baseBoxPtr->back)+1;			// see how far over it went
				if (offset > maxOffsetZ)
				{
					maxOffsetZ = offset;
					offZSign = 1;
				}
				gDelta.z = 0;
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_FRONT)						// SEE IF FRONT HIT
			{
				offset = (baseBoxPtr->front - targetBoxPtr->back)+1;			// see how far over it went
				if (offset > maxOffsetZ)
				{
					maxOffsetZ = offset;
					offZSign = -1;
				}
				gDelta.z = 0;
			}

			if (gCollisionList[i].sides & SIDE_BITS_LEFT)						// SEE IF HIT LEFT
			{
				offset = (targetBoxPtr->right - baseBoxPtr->left)+1;			// see how far over it went
				if (offset > maxOffsetX)
				{
					maxOffsetX = offset;
					offXSign = 1;
				}
				gDelta.x = 0;
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_RIGHT)						// SEE IF HIT RIGHT
			{
				offset = (baseBoxPtr->right - targetBoxPtr->left)+1;			// see how far over it went
				if (offset > maxOffsetX)
				{
					maxOffsetX = offset;
					offXSign = -1;
				}
				gDelta.x = 0;
			}

			if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)						// SEE IF HIT BOTTOM
			{
				offset = (targetBoxPtr->top - baseBoxPtr->bottom)+1;			// see how far over it went
				if (offset > maxOffsetY)
				{
					maxOffsetY = offset;
					offYSign = 1;
				}
				gDelta.y = 0; 
				theNode->PlatformNode = targetObj;								// ** remember what it's on! **
			}
			else
			if (gCollisionList[i].sides & SIDE_BITS_TOP)						// SEE IF HIT TOP
			{
				offset = (baseBoxPtr->top - targetBoxPtr->bottom)+1;			// see how far over it went
				if (offset > maxOffsetY)
				{
					maxOffsetY = offset;
					offYSign = -1;
				}
				gDelta.y =0;						
			}
		}
		
					/********************************/
					/* HANDLE TRIANGLE COLLISIONS 	*/	
					/********************************/

		else
		if (gCollisionList[i].type == COLLISION_TYPE_TRIANGLE)
		{
					/* ADJUST Y */
					
			originalY = gCollisionList[i].planeIntersectY - theNode->BottomOff + 1;
			maxOffsetY = 0;					
			gDelta.y = -1;													// keep some force down			
		}
	}
			/* ADJUST MAX AMOUNTS */
			
	gCoord.x = originalX + (float)maxOffsetX * offXSign;			
	gCoord.z = originalZ + (float)maxOffsetZ * offZSign;
	gCoord.y = originalY + (maxOffsetY * (int)offYSign);	// y is special - we do some additional rouding to avoid the jitter problem
			
	return(totalSides);
}




/****************** IS POINT IN PICKUP COLLISION SPHERE ************************/
//
// Does simple radius collision with thePt to see if it's inside the
// collision box of a CTYPE_PICKUP object.
//
// OUTPUT: node of obj, or nil == none found.
//

ObjNode *IsPointInPickupCollisionSphere(TQ3Point3D *thePt)
{
ObjNode	*thisNode;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (!(thisNode->CType & CTYPE_PICKUP))					// only care about Pickup type objects
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;
				
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;
	
				/* SEE IF POINT WITHIN BOUNDING SPHERE (PLUS A LITTLE) OF PICKUP OBJ */
				
		if (CalcQuickDistance(thePt->x,thePt->z,thisNode->Coord.x,thisNode->Coord.z) <= thisNode->PickUpCollisionRadius)
			return(thisNode);
				
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);
	
	return(nil);												// nothing found;
}



/**************** ADD BG COLLISIONS *********************/

static void AddBGCollisions(ObjNode *theNode, float realDX, float realDZ, UInt32 cType)
{
short		oldRow,left,right,oldCol,back,front;
short		count,num;
UInt16		bits;
CollisionBoxType *boxList;
float		leftSide,rightSide,frontSide,backSide;
float		oldLeftSide,oldRightSide,oldFrontSide,oldBackSide;
Boolean		checkAltTiles = cType & CTYPE_BGROUND2;		// see if check alt tiles also

			/* GET BASE BOX INFO */
			
	if (theNode->NumCollisionBoxes == 0)				// it's gotta have a collision box
		return;
	boxList = theNode->CollisionBoxes;
	leftSide = boxList->left;
	rightSide = boxList->right;
	frontSide = boxList->front;
	backSide = boxList->back;

	oldLeftSide = boxList->oldLeft;
	oldRightSide = boxList->oldRight;
	oldFrontSide = boxList->oldFront;
	oldBackSide = boxList->oldBack;


				/* SEE IF OFF MAP */

	if (frontSide >= gTerrainUnitDepth)	  			// see if front is off of map
		return;
	if (backSide < 0)				 				// see if back is off of map
		return;
	if (rightSide >= gTerrainUnitWidth)				// see if right is off of map
		return;
	if (leftSide < 0)								// see if left is off of map
		return;


				/**************************/
				/* CHECK FRONT/BACK SIDES */
				/**************************/

	if (realDZ > 0)												// see if front (+z)
	{
				/* SEE IF FRONT SIDE HIT BACKGROUND */

		oldRow = oldFrontSide*gOneOver_TERRAIN_POLYGON_SIZE;	// get old front side & see if in same row as current
		front = frontSide*gOneOver_TERRAIN_POLYGON_SIZE;		// calc front row
		if (front == oldRow)									// if in same row as before,then skip cuz didn't change
			goto check_x;

		left = leftSide*gOneOver_TERRAIN_POLYGON_SIZE; 			// calc left column
		right = rightSide*gOneOver_TERRAIN_POLYGON_SIZE;	  	// calc right column
		count = num = (right-left)+1;							// calc # of tiles wide to check.  num = original count value

		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(front,left,checkAltTiles);	// get collision info at this path tile
			if (bits & SIDE_BITS_TOP)							// see if tile solid on back
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_FRONT;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_FRONT;
				goto check_x;
			}
			left++;												// next column ->
		}
	}
	else
	if (realDZ < 0)												// see if away
	{
						/* SEE IF TOP SIDE HIT BACKGROUND */

		oldRow = oldBackSide*gOneOver_TERRAIN_POLYGON_SIZE;	  	// get old back side & see if in same row as current
		back = backSide*gOneOver_TERRAIN_POLYGON_SIZE;
		if (back == oldRow)										// if in same row as before,then skip
			goto check_x;

		left = leftSide*gOneOver_TERRAIN_POLYGON_SIZE; 			// calc left column
		right = rightSide*gOneOver_TERRAIN_POLYGON_SIZE;	  	// calc right column
		count = num = (right-left)+1;							// calc # of tiles wide to check.  num = original count value


		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(back,left,checkAltTiles); 	// get collision info for this tile
			if (bits & SIDE_BITS_BOTTOM)						// see if tile solid on front
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_BACK;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_BACK;
				goto check_x;
			}
			left++;												// next column ->
		}
	}

				/************************/
				/* DO HORIZONTAL CHECK  */
				/************************/

check_x:

	if (realDX > 0)												// see if right
	{
				/* SEE IF RIGHT SIDE HIT BACKGROUND */

		oldCol = oldRightSide*gOneOver_TERRAIN_POLYGON_SIZE; 	// get old right side & see if in same col as current
		right = rightSide*gOneOver_TERRAIN_POLYGON_SIZE;
		if (right == oldCol)				 					// if in same col as before,then skip
			return;

		back = backSide*gOneOver_TERRAIN_POLYGON_SIZE;
		front = frontSide*gOneOver_TERRAIN_POLYGON_SIZE;
		count = num = (front-back)+1;	 						// calc # of tiles high to check.  num = original count value


		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(back,right,checkAltTiles); 	// get collision info for this tile
			if (bits & SIDE_BITS_LEFT)							// see if tile solid on left
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_RIGHT;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_RIGHT;
				return;
			}
			back++;												// next row
		}
	}
	else
	if (realDX < 0)												// see if left
	{
						/* SEE IF LEFT SIDE HIT BACKGROUND */

		oldCol = oldLeftSide*gOneOver_TERRAIN_POLYGON_SIZE;  	// get old left side & see if in same col as current
		left = leftSide*gOneOver_TERRAIN_POLYGON_SIZE;
		if (left == oldCol)										// if in same col as before,then skip
			return;

		back = backSide*gOneOver_TERRAIN_POLYGON_SIZE;
		front = frontSide*gOneOver_TERRAIN_POLYGON_SIZE;
		count = num = (front-back)+1;							// calc # of tiles high to check.  num = original count value


		for (; count > 0; count--)
		{
			bits = GetTileCollisionBitsAtRowCol(back,left,checkAltTiles); 	// get collision info for this tile
			if (bits & SIDE_BITS_RIGHT)							// see if tile solid on right
			{
				gCollisionList[gNumCollisions].sides = SIDE_BITS_LEFT;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TILE;
				gNumCollisions++;
				gTotalSides |= SIDE_BITS_LEFT;
				return;
			}
			back++;										// next row
		}
	}
}


/*==========================================================================================*/
/*============================== COLLISION TRIANGLES =======================================*/
/*==========================================================================================*/

#pragma mark ---------- COLLISION TRIANGLE GENERATION ----------------

/******************* CREATE COLLISION TRIANGES FOR OBJECT *************************/
//
// Scans an object's geometry and creates a list of collision triangles.
// The valid triangles are those who's normals face upward (y>0).
//

void CreateCollisionTrianglesForObject(ObjNode *theNode)
{
short	i;

	if (theNode->BaseGroup == nil)
		return;
		
			/* CREATE TEMPORARY TRIANGLE LIST */
			
	gNumCollTriangles = 0;							// clear counter
	Q3Matrix4x4_SetIdentity(&gWorkMatrix);			// init to identity matrix
	gCollTrianglesBBox.min.x =						// init bounding box
	gCollTrianglesBBox.min.y =
	gCollTrianglesBBox.min.z = 1000000;
	gCollTrianglesBBox.max.x =
	gCollTrianglesBBox.max.y =
	gCollTrianglesBBox.max.z = -1000000;
	
	ScanForTriangles_Recurse(theNode->BaseGroup);
	
	
		/* ALLOC MEM & COPY TEMP LIST INTO REAL LIST */
			
	AllocateCollisionTriangleMemory(theNode, gNumCollTriangles);					// alloc memory
	
	theNode->CollisionTriangles->numTriangles = gNumCollTriangles;					// set # triangles
	
				/* WIDEN & COPY BBOX */
				
	theNode->CollisionTriangles->bBox.min.x = gCollTrianglesBBox.min.x - 50;
	theNode->CollisionTriangles->bBox.min.y = gCollTrianglesBBox.min.y - 50;
	theNode->CollisionTriangles->bBox.min.z = gCollTrianglesBBox.min.z - 50;

	theNode->CollisionTriangles->bBox.max.x = gCollTrianglesBBox.max.x + 50;
	theNode->CollisionTriangles->bBox.max.y = gCollTrianglesBBox.max.y + 50;
	theNode->CollisionTriangles->bBox.max.z = gCollTrianglesBBox.max.z + 50;
	
	for (i=0; i < gNumCollTriangles; i++)
		theNode->CollisionTriangles->triangles[i] = gCollTriangles[i];				// copy each triangle
	
}


/****************** SCAN FOR TRIANGLES - RECURSE ***********************/

static void ScanForTriangles_Recurse(TQ3Object obj)
{
TQ3GroupPosition	position;
TQ3Object   		object,baseGroup;
TQ3ObjectType		oType;
TQ3Matrix4x4  		stashMatrix,transform;

				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		Q3Matrix4x4_Multiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
	else

				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(obj);									// get geometry type
		if (oType == kQ3GeometryTypeTriMesh)
		{
			GetTrianglesFromTriMesh(obj);
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
 		baseGroup = obj;
  		stashMatrix = gWorkMatrix;										// push matrix
  		for (Q3Group_GetFirstPosition(obj, &position); position != nil;
  			 Q3Group_GetNextPosition(obj, &position))					// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != NULL)
   			{
    			ScanForTriangles_Recurse(object);						// sub-recurse this object
    			Q3Object_Dispose(object);								// dispose local ref
   			}
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix  		
	}
}


/************************ GET TRIANGLES FROM TRIMESH ****************************/

static void GetTrianglesFromTriMesh(TQ3Object obj)
{
TQ3TriMeshData		triMeshData;
short				v,t,i0,i1,i2;
TQ3Point3D			*points;
float				nX,nY,nZ,x,y,z;
TQ3PlaneEquation 	*plane;
TQ3Vector3D			vv;
float				m00,m01,m02,m10,m11,m12,m20,m21,m22,m30,m31,m32;


			/* GET DATA */
			
	Q3TriMesh_GetData(obj,&triMeshData);							// get trimesh data	
		
	
		/* TRANSFORM ALL POINTS */
			
	points = &triMeshData.points[0];								// point to points list
	
	m00 = gWorkMatrix.value[0][0];	m01 = gWorkMatrix.value[0][1];	m02 = gWorkMatrix.value[0][2];	// load matrix into registers
	m10 = gWorkMatrix.value[1][0];	m11 = gWorkMatrix.value[1][1];	m12 = gWorkMatrix.value[1][2];
	m20 = gWorkMatrix.value[2][0];	m21 = gWorkMatrix.value[2][1];	m22 = gWorkMatrix.value[2][2];
	m30 = gWorkMatrix.value[3][0];	m31 = gWorkMatrix.value[3][1];	m32 = gWorkMatrix.value[3][2];
	
	for (v = 0; v < triMeshData.numPoints; v++)						// scan thru all verts
	{	
		x = (m00*points[v].x) + (m10*points[v].y) + (m20*points[v].z) + m30;			// transform x value
		y = (m01*points[v].x) + (m11*points[v].y) + (m21*points[v].z) + m31;			// transform y
		points[v].z = (m02*points[v].x) + (m12*points[v].y) + (m22*points[v].z) + m32;	// transform z
		points[v].x = x;
		points[v].y = y;	
	}
	
				/* CHECK EACH FACE */
						
	for (t = 0; t < triMeshData.numTriangles; t++)					// scan thru all faces
	{
		i0 = triMeshData.triangles[t].pointIndices[0];				// get indecies of 3 points
		i1 = triMeshData.triangles[t].pointIndices[1];			
		i2 = triMeshData.triangles[t].pointIndices[2];
		
		Q3Point3D_CrossProductTri(&points[i0],&points[i1],&points[i2], &vv);	// calc face normal		
		nY = vv.y;

		if (nY == 0.0f)												// cant have y=0 since will result in divide by 0 later
			nY = 0.0001f;

		nX = vv.x;
		nZ = vv.z;

			/*****************************/
			/* WE GOT A USEABLE TRIANGLE */
			/*****************************/

		x = points[i0].x;											// get coords of 1st pt
		y = points[i0].y;	
		z = points[i0].z;	
		
				/* COPY VERTEX COORDS */
				
		gCollTriangles[gNumCollTriangles].verts[0] = points[i0];	// keep coords of the 3 vertices
		gCollTriangles[gNumCollTriangles].verts[1] = points[i1];
		gCollTriangles[gNumCollTriangles].verts[2] = points[i2];
		plane = &gCollTriangles[gNumCollTriangles].planeEQ;			// get ptr to temporary collision triangle list

				/* SAVE FACE NORMAL */
										
		FastNormalizeVector(nX, nY, nZ, &plane->normal);			// normalize & save into plane equation

				/* CALC PLANE CONSTANT */

		plane->constant = 	(plane->normal.x * x) +					// calc dot product for plane constant
						  	(plane->normal.y * y) +
							(plane->normal.z * z);			
							
				/* UPDATE BOUNDING BOX */
					
		if (x < gCollTrianglesBBox.min.x)							// check all 3 vertices to update bbox
			gCollTrianglesBBox.min.x = x;
		if (y < gCollTrianglesBBox.min.y)
			gCollTrianglesBBox.min.y = y;
		if (z < gCollTrianglesBBox.min.z)
			gCollTrianglesBBox.min.z = z;
		if (x > gCollTrianglesBBox.max.x)
			gCollTrianglesBBox.max.x = x;
		if (y > gCollTrianglesBBox.max.y)
			gCollTrianglesBBox.max.y = y;
		if (z > gCollTrianglesBBox.max.z)
			gCollTrianglesBBox.max.z = z;

		if (points[i1].x < gCollTrianglesBBox.min.x)
			gCollTrianglesBBox.min.x = points[i1].x;
		if (points[i1].y < gCollTrianglesBBox.min.y)
			gCollTrianglesBBox.min.y = points[i1].y;
		if (points[i1].z < gCollTrianglesBBox.min.z)
			gCollTrianglesBBox.min.z = points[i1].z;
		if (points[i1].x > gCollTrianglesBBox.max.x)
			gCollTrianglesBBox.max.x = points[i1].x;
		if (points[i1].y > gCollTrianglesBBox.max.y)
			gCollTrianglesBBox.max.y = points[i1].y;
		if (points[i1].z > gCollTrianglesBBox.max.z)
			gCollTrianglesBBox.max.z = points[i1].z;

		if (points[i2].x < gCollTrianglesBBox.min.x)
			gCollTrianglesBBox.min.x = points[i2].x;
		if (points[i2].y < gCollTrianglesBBox.min.y)
			gCollTrianglesBBox.min.y = points[i2].y;
		if (points[i2].z < gCollTrianglesBBox.min.z)
			gCollTrianglesBBox.min.z = points[i2].z;
		if (points[i2].x > gCollTrianglesBBox.max.x)
			gCollTrianglesBBox.max.x = points[i2].x;
		if (points[i2].y > gCollTrianglesBBox.max.y)
			gCollTrianglesBBox.max.y = points[i2].y;
		if (points[i2].z > gCollTrianglesBBox.max.z)
			gCollTrianglesBBox.max.z = points[i2].z;
			
		gNumCollTriangles++;										// inc counter				
		if (gNumCollTriangles > MAX_TEMP_COLL_TRIANGLES)
			DoFatalAlert("GetTrianglesFromTriMesh: too many triangles in list!");							
	}
	
			/* CLEANUP */
			
	Q3TriMesh_EmptyData(&triMeshData);
}


/**************** ALLOCATE COLLISION TRIANGLE MEMORY *******************/

static void AllocateCollisionTriangleMemory(ObjNode *theNode, long numTriangles)
{
			/* FREE OLD STUFF */
	
	if (theNode->CollisionTriangles)
		DisposeCollisionTriangleMemory(theNode);

	if (numTriangles == 0)
		DoFatalAlert("AllocateCollisionTriangleMemory: numTriangles = 0?");
	
			/* ALLOC MEMORY */
			
	theNode->CollisionTriangles = (TriangleCollisionList *)AllocPtr(sizeof(TriangleCollisionList));				// alloc main block
	if (theNode->CollisionTriangles == nil)
		DoFatalAlert("AllocateCollisionTriangleMemory: Alloc failed!");
	
	theNode->CollisionTriangles->triangles = (CollisionTriangleType *)AllocPtr(sizeof(CollisionTriangleType) * numTriangles);	// alloc triangle array

	theNode->CollisionTriangles->numTriangles = numTriangles;						// set #	
}


/******************** DISPOSE COLLISION TRIANGLE MEMORY ***********************/

void DisposeCollisionTriangleMemory(ObjNode *theNode)
{
	if (theNode->CollisionTriangles == nil)
		return;

	if (theNode->CollisionTriangles->triangles)							// nuke triangle list
		DisposePtr((Ptr)theNode->CollisionTriangles->triangles);

	DisposePtr((Ptr)theNode->CollisionTriangles);						// nuke collision data

	theNode->CollisionTriangles = nil;									// clear ptr
}

#pragma mark ------------ POINT COLLISION -----------------

/****************** IS POINT IN POLY ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInPoly2D(float pt_x, float pt_y, Byte numVerts, TQ3Point2D *polypts)
{
Byte 		oldquad,newquad;
float 		thispt_x,thispt_y,lastpt_x,lastpt_y;
signed char	wind;										// current winding number 
Byte		i;

			/************************/
			/* INIT STARTING VALUES */
			/************************/
			
	wind = 0;
    lastpt_x = polypts[numVerts-1].x;  					// get last point's coords  
    lastpt_y = polypts[numVerts-1].y;    
    
	if (lastpt_x < pt_x)								// calc quadrant of the last point
	{
    	if (lastpt_y < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (lastpt_y < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}
    

			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/
    
    for (i=0; i<numVerts; i++)
    {
   			/* GET THIS POINT INFO */
    			
		thispt_x = polypts[i].x;						// get this point's coords
		thispt_y = polypts[i].y;

		if (thispt_x < pt_x)							// calc quadrant of this point
		{
	    	if (thispt_y < pt_y)
	    		newquad = 2;
	 		else
	 			newquad = 1;
	 	}
	 	else
	    {
	    	if (thispt_y < pt_y)
	    		newquad = 3;
	 		else
	 			newquad = 0;
		}

				/* SEE IF QUADRANT CHANGED */
				
        if (oldquad != newquad)
        {
			if (((oldquad+1)&3) == newquad)				// see if advanced
            	wind++;
			else
        	if (((newquad+1)&3) == oldquad)				// see if backed up
				wind--;
    		else
			{
				float	a,b;
				
             		/* upper left to lower right, or upper right to lower left.
             		   Determine direction of winding  by intersection with x==0. */
                                             
    			a = (lastpt_y - thispt_y) * (pt_x - lastpt_x);			
                b = lastpt_x - thispt_x;
                a += lastpt_y * b;
                b *= pt_y;

				if (a > b)
                	wind += 2;
 				else
                	wind -= 2;
    		}
  		}
  		
  				/* MOVE TO NEXT POINT */
  				
   		lastpt_x = thispt_x;
   		lastpt_y = thispt_y;
   		oldquad = newquad;
	}
	

	return(wind); 										// non zero means point in poly
}





/****************** IS POINT IN TRIANGLE ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2)
{
Byte 		oldquad,newquad;
float		m;
signed char	wind;										// current winding number 

			/*********************/
			/* DO TRIVIAL REJECT */
			/*********************/
			
	m = x0;												// see if to left of triangle							
	if (x1 < m)
		m = x1;
	if (x2 < m)
		m = x2;
	if (pt_x < m)
		return(false);

	m = x0;												// see if to right of triangle							
	if (x1 > m)
		m = x1;
	if (x2 > m)
		m = x2;
	if (pt_x > m)
		return(false);

	m = y0;												// see if to back of triangle							
	if (y1 < m)
		m = y1;
	if (y2 < m)
		m = y2;
	if (pt_y < m)
		return(false);

	m = y0;												// see if to front of triangle							
	if (y1 > m)
		m = y1;
	if (y2 > m)
		m = y2;
	if (pt_y > m)
		return(false);


			/*******************/
			/* DO WINDING TEST */
			/*******************/
			
		/* INIT STARTING VALUES */
			
    
	if (x2 < pt_x)								// calc quadrant of the last point
	{
    	if (y2 < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}
    

			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

	wind = 0;
    

//=============================================
			
	if (x0 < pt_x)									// calc quadrant of this point
	{
    	if (y0 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y0 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y2 - y0) * (pt_x - x2);			
            b = x2 - x0;
            a += y2 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
				
	oldquad = newquad;

//=============================================

	if (x1 < pt_x)							// calc quadrant of this point
	{
    	if (y1 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y1 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y0 - y1) * (pt_x - x0);			
            b = x0 - x1;
            a += y0 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
			
	oldquad = newquad;

//=============================================
			
	if (x2 < pt_x)							// calc quadrant of this point
	{
    	if (y2 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y1 - y2) * (pt_x - x1);			
            b = x1 - x2;
            a += y1 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
	
	return(wind); 										// non zero means point in poly
}






/******************** DO SIMPLE POINT COLLISION *********************************/
//
// OUTPUT: # collisions detected
//

short DoSimplePointCollision(TQ3Point3D *thePoint, UInt32 cType)
{
ObjNode	*thisNode;
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;
		
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;
	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;
	
	
			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/
			
		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */
		
			if (thePoint->x < targetBoxList[target].left)
				continue;
				
			if (thePoint->x > targetBoxList[target].right)
				continue;
				
			if (thePoint->z < targetBoxList[target].back)
				continue;
				
			if (thePoint->z > targetBoxList[target].front)
				continue;
				
			if (thePoint->y > targetBoxList[target].top)
				continue;

			if (thePoint->y < targetBoxList[target].bottom)
				continue;
				
								
					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;	
		}
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}



/*************************** DO TRIANGLE COLLISION **********************************/

void DoTriangleCollision(ObjNode *theNode, unsigned long CType)
{
ObjNode	*thisNode;
float	x,y,z,oldX,oldY,oldZ;

	x = gCoord.x;	y = gCoord.y; 	z = gCoord.z;
	oldX = theNode->OldCoord.x;
	oldY = theNode->OldCoord.y;
	oldZ = theNode->OldCoord.z;

	gNumCollisions = 0;											// clear list
	gTotalSides = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->CollisionTriangles == nil)				// must be triangles here
			goto next;
		
		if (!(thisNode->CType & CType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;		
				
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;
	
		if (thisNode == theNode)								// dont collide against itself
			goto next;
	
		if (theNode->ChainNode == thisNode)						// don't collide against its own chained object
			goto next;

				/* SINCE NOTHING HIT, ADD TRIANGLE COLLISIONS */
				
		AddTriangleCollision(thisNode, x, y, z, oldX, oldZ, y, oldY);
next:	
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);

}


/******************* ADD TRIANGLE COLLISION ********************************/

static void AddTriangleCollision(ObjNode *thisNode, float x, float y, float z, float oldX, float oldZ,
						 long bottomSide, long oldBottomSide)
{
	if (thisNode->CollisionTriangles)
	{
		TriangleCollisionList	*collisionRec = thisNode->CollisionTriangles;
		CollisionTriangleType	*triangleList;
		short					numTriangles,i;
		TQ3Point3D 				intersectPt;
		
				/* FIRST CHECK IF INSIDE BOUNDING BOX */
				//
				// note: the bbox must be larger than the true bounds of the geometry
				//		for this to work correctly.  Since it's possible that in 1 frame of
				//		anim animation an object has gone thru a polygon and out of the bbox
				//		we enlarge the bbox (earlier) and cross our fingers that this will be sufficient.
				//
		
		if (x < collisionRec->bBox.min.x)
			return;
		if (x > collisionRec->bBox.max.x)
			return;
		if (y < collisionRec->bBox.min.y)
			return;
		if (bottomSide > collisionRec->bBox.max.y)				// see if bottom is still above top of bbox
			return;
		if (z < collisionRec->bBox.min.z)
			return;
		if (z > collisionRec->bBox.max.z)
			return;

					
			/**************************************************************/
			/* WE'RE IN THE BBOX, SO NOW SEE IF WE INTERSECTED A TRIANGLE */
			/**************************************************************/
		
		numTriangles = collisionRec->numTriangles;				// get # triangles to check
		triangleList = collisionRec->triangles;					// get pointer to triangles
		
		for (i = 0; i < numTriangles; i++)
		{
			float	keepY,intersectY;
			long	minDist;
			Boolean	validate;

						/* SEE WHERE LINE INTERSECTS PLANE */
						
			if (!IntersectionOfLineSegAndPlane(&triangleList[i].planeEQ,
											 x, bottomSide, z, oldX, oldBottomSide, oldZ,
											&intersectPt))
			{
				continue;
			}
										
						/* SEE IF INTERSECT PT IS INSIDE THE TRIANGLE */
							
			if (!IsPointInTriangle(intersectPt.x, intersectPt.z, triangleList[i].verts[0].x,		// see if intersec pt is inside this triangle
										 triangleList[i].verts[0].z,	triangleList[i].verts[1].x,
										 triangleList[i].verts[1].z, triangleList[i].verts[2].x,
										 triangleList[i].verts[2].z))
			{
				continue;
			}


				/**************************************************************************************/
				/* WE INTERSECTED A SPECIFIC TRIANGLE, BUT NOW SEE WHICH TRIANGLE WE WANT TO STAND ON */
				/**************************************************************************************/
				//
				// The triangle check above is only to determine if a hit actually occurred.  We don't use that
				// collision info for the final result, however.  Instead, we find the triangle closest to us and
				// do a simple y-collision check on it.
				//

			minDist = 10000000;
			keepY = bottomSide;
			validate = false;
			for (i = 0; i < numTriangles; i++)
			{
				float	dist;
				TQ3PlaneEquation *planeEQ = &triangleList[i].planeEQ;

				if (planeEQ->normal.y <= 0.0f)												// cant stand on triangles facing down
					continue;
				
				if (IsPointInTriangle(x, z, triangleList[i].verts[0].x,						// see if original coord is inside this triangle
									 triangleList[i].verts[0].z, triangleList[i].verts[1].x,
									 triangleList[i].verts[1].z, triangleList[i].verts[2].x,
									 triangleList[i].verts[2].z))
				{
					intersectY = IntersectionOfYAndPlane(x, z, planeEQ);					// calc y coord
					
					dist = fabs(intersectY - bottomSide);									// calc dist to this y coord
					
					if (dist < minDist)														// see if closest so far
					{
						minDist = dist;
						keepY = intersectY;
						validate = true;
					}
				}
			}
			
			if (validate)
			{
						/* ADD TO COLLISION LIST */

				gCollisionList[gNumCollisions].planeIntersectY = keepY;
				gCollisionList[gNumCollisions].baseBox = 0;
				gCollisionList[gNumCollisions].targetBox = i;
				gCollisionList[gNumCollisions].sides = SIDE_BITS_BOTTOM;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_TRIANGLE;
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;	
				gTotalSides |= SIDE_BITS_BOTTOM;
			}
			
			break;
			
		}
	}
}
























