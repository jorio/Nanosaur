/****************************/
/*     TERRAIN.C           */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/


#include "QD3D.h"

#include "globals.h"
#include "objects.h"
#include "main.h"
#include "terrain.h"
#include "misc.h"
#include "file.h"
#include "3dmath.h"
#include "camera.h"
#include "mobjtypes.h"

#include "GamePatches.h"
#include "frustumculling.h"

#include <QD3DMath.h>
#include <stdlib.h>
#include <string.h>
#include "qd3d_geometry.h"
#include "renderer.h"

extern	TerrainItemEntryType	**gTerrainItemLookupTableX;
extern	Ptr			gTileFilePtr;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	TQ3Point3D		gMyCoord;

#define TILE_TEXTURE_INTERNAL_FORMAT	GL_RGB
#define TILE_TEXTURE_FORMAT				GL_BGRA_EXT
#define TILE_TEXTURE_TYPE				GL_UNSIGNED_SHORT_1_5_5_5_REV



/****************************/
/*  PROTOTYPES             */
/****************************/

static void ScrollTerrainUp(long superRow, long superCol);
static void ScrollTerrainDown(long superRow, long superCol);
static void ScrollTerrainLeft(void);
static void ScrollTerrainRight(long superCol, long superRow, long tileCol, long tileRow);
static short GetFreeSuperTileMemory(void);
static inline void ReleaseSuperTileObject(short superTileNum);
static void CalcNewItemDeleteWindow(void);
static float	GetTerrainHeightAtRowCol(long row, long col);
static void CreateSuperTileMemoryList(void);
static short	BuildTerrainSuperTile(long	startCol, long startRow);
static void DrawTileIntoMipmap(UInt16 tile, short row, short col, UInt16 *buffer);
static void BuildTerrainSuperTile_Flat(SuperTileMemoryType *, long startCol, long startRow);
static void ShrinkSuperTileTextureMap(const uint16_t* srcPtr, uint16_t* dstPtr, int targetSize);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ITEM_WINDOW		1				// # supertiles for item add window
#define	OUTER_SIZE		0				// size of border out of add window for delete window

enum
{
	SPLIT_ARBITRARY,
	SPLIT_BACKWARD,
	SPLIT_FORWARD
};



/**********************/
/*     VARIABLES      */
/**********************/

static UInt8	gHiccupEliminator = 0;

Ptr		gTerrainPtr = nil;								// points to terrain file data
UInt16	*gTileDataPtr;

UInt16	**gTerrainTextureLayer = nil;					// 2 dimensional array of UInt16s (allocated below)
UInt16	**gTerrainHeightMapLayer = nil;
UInt16	**gTerrainPathLayer = nil;

long	gTerrainTileWidth,gTerrainTileDepth;			// width & depth of terrain in tiles
long	gTerrainUnitWidth,gTerrainUnitDepth;			// width & depth of terrain in world units (see TERRAIN_POLYGON_SIZE)

long	gNumTerrainTextureTiles = 0;
Ptr		gTerrainHeightMapPtrs[MAX_HEIGHTMAP_TILES];

long	gNumSuperTilesDeep,gNumSuperTilesWide;	  		// dimensions of terrain in terms of supertiles
long	gCurrentSuperTileRow,gCurrentSuperTileCol;

// Source port enhancement: these indices were bytes, changed to shorts
// so we can have an active supertile area of 8x8 supertiles or more
static short	gTerrainScrollBuffer[MAX_SUPERTILES_DEEP][MAX_SUPERTILES_WIDE];		// 2D array which has index to supertiles for each possible supertile

short	gNumFreeSupertiles = 0;
static	SuperTileMemoryType		*gSuperTileMemoryList = NULL;


long	gTerrainItemDeleteWindow_Near,gTerrainItemDeleteWindow_Far,
		gTerrainItemDeleteWindow_Left,gTerrainItemDeleteWindow_Right;

TileAttribType	*gTileAttributes = nil;

UInt16		gTileFlipRotBits;
short		gTileAttribParm0;
Byte		gTileAttribParm1,gTileAttribParm2;

float		gSuperTileRadius;			// normal x/z radius

float		gUnitToPixel = 1.0/(TERRAIN_POLYGON_SIZE/TERRAIN_HMTILE_SIZE);

const float gOneOver_TERRAIN_POLYGON_SIZE = (1.0 / TERRAIN_POLYGON_SIZE);


			/* TILE SPLITTING TABLES */
			
					
					/* /  */
static	Byte			gTileTriangles1_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 6, 0, 1}, { 7, 1, 2}, { 8, 2, 3}, { 9, 3, 4}, {10, 4, 5} },
	{ {12, 6, 7}, {13, 7, 8}, {14, 8, 9}, {15, 9,10}, {16,10,11} },
	{ {18,12,13}, {19,13,14}, {20,14,15}, {21,15,16}, {22,16,17} },
	{ {24,18,19}, {25,19,20}, {26,20,21}, {27,21,22}, {28,22,23} },
	{ {30,24,25}, {31,25,26}, {32,26,27}, {33,27,28}, {34,28,29} }
};

static	Byte			gTileTriangles2_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 6, 1, 7}, { 7, 2, 8}, { 8, 3, 9}, { 9, 4,10}, {10, 5,11} },
	{ {12, 7,13}, {13, 8,14}, {14, 9,15}, {15,10,16}, {16,11,17} },
	{ {18,13,19}, {19,14,20}, {20,15,21}, {21,16,22}, {22,17,23} },
	{ {24,19,25}, {25,20,26}, {26,21,27}, {27,22,28}, {28,23,29} },
	{ {30,25,31}, {31,26,32}, {32,27,33}, {33,28,34}, {34,29,35} }
};

					/* \  */
static	Byte			gTileTriangles1_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 0, 7, 6}, { 1, 8, 7}, { 2, 9, 8}, { 3,10, 9}, { 4,11,10} },
	{ { 6,13,12}, { 7,14,13}, { 8,15,14}, { 9,16,15}, {10,17,16} },
	{ {12,19,18}, {13,20,19}, {14,21,20}, {15,22,21}, {16,23,22} },
	{ {18,25,24}, {19,26,25}, {20,27,26}, {21,28,27}, {22,29,28} },
	{ {24,31,30}, {25,32,31}, {26,33,32}, {27,34,33}, {28,35,34} }
};

static	Byte			gTileTriangles2_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3] =
{
	{ { 0, 1, 7}, { 1, 2, 8}, { 2, 3, 9}, { 3, 4,10}, { 4, 5,11} },
	{ { 6, 7,13}, { 7, 8,14}, { 8, 9,15}, { 9,10,16}, {10,11,17} },
	{ {12,13,19}, {13,14,20}, {14,15,21}, {15,16,22}, {16,17,23} },
	{ {18,19,25}, {19,20,26}, {20,21,27}, {21,22,28}, {22,23,29} },
	{ {24,25,31}, {25,26,32}, {26,27,33}, {27,28,34}, {28,29,35} }
};


TQ3Point3D		gWorkGrid[SUPERTILE_SIZE+1][SUPERTILE_SIZE+1];
UInt16			*gTempTextureBuffer = nil;

TQ3Vector3D		gRecentTerrainNormal;							// from _Planar

/****************** INIT TERRAIN MANAGER ************************/
//
// Only called at boot!
//

void InitTerrainManager(void)
{
 	gSuperTileRadius = sqrt(2) * (TERRAIN_SUPERTILE_UNIT_SIZE/2);

	CreateSuperTileMemoryList();
	ClearScrollBuffer();
	
	
			/* ALLOC TEMP TEXTURE BUFF */
			
	if (gTempTextureBuffer == nil)
		gTempTextureBuffer = (UInt16 *)AllocPtr(TEMP_TEXTURE_BUFF_SIZE * TEMP_TEXTURE_BUFF_SIZE * sizeof(UInt16));
}


/***************** CLEAR SCROLL BUFFER ************************/

void ClearScrollBuffer(void)
{
long	row,col;

	for (row = 0; row < MAX_SUPERTILES_DEEP; row++)
		for (col = 0; col < MAX_SUPERTILES_WIDE; col++)
			gTerrainScrollBuffer[row][col] = EMPTY_SUPERTILE;
			
	gHiccupEliminator = 0;
}





/***************** DISPOSE TERRAIN **********************/
//
// Deletes any existing terrain data
//

void DisposeTerrain(void)
{
	if (gTileFilePtr)
	{
		DisposePtr(gTileFilePtr);
		gTileFilePtr = nil;
	}
	
	if (gTerrainItemLookupTableX != nil)
	{
	  	DisposePtr((Ptr)gTerrainItemLookupTableX);
	  	gTerrainItemLookupTableX = nil;
	}

	if (gTerrainTextureLayer != nil)
	{
		DisposePtr((Ptr)gTerrainTextureLayer);
		gTerrainTextureLayer = nil;
	}

	if (gTerrainHeightMapLayer != nil)
	{
		DisposePtr((Ptr)gTerrainHeightMapLayer);
		gTerrainHeightMapLayer = nil;
	}

	if (gTerrainPathLayer != nil)
	{
		DisposePtr((Ptr)gTerrainPathLayer);
		gTerrainPathLayer = nil;
	}

	if (gTerrainPtr != nil)	   					    		// see if zap existing terrain
	{
		DisposePtr((Ptr)gTerrainPtr);
		gTerrainPtr = nil;
	}

	if (gSuperTileMemoryList != nil)						// release all supertiles (source port addition)
	{
		for (int i = 0; i < MAX_SUPERTILES; i++)
		{
			SuperTileMemoryType* superTile = &gSuperTileMemoryList[i];

			if (superTile->textureData)
			{
				DisposePtr((Ptr) superTile->textureData);
				superTile->textureData = nil;
			}

			if (superTile->glTextureName)
			{
				glDeleteTextures(1, &superTile->glTextureName);
				superTile->glTextureName = 0;
			}

			if (superTile->triMeshPtr)
			{
				Q3TriMeshData_Dispose(superTile->triMeshPtr);
				superTile->triMeshPtr = nil;
			}

			if (superTile->triMeshPtr2)
			{
				Q3TriMeshData_Dispose(superTile->triMeshPtr2);
				superTile->triMeshPtr2 = nil;
			}
		}
		DisposePtr((Ptr) gSuperTileMemoryList);
		gSuperTileMemoryList = nil;
	}
	gNumFreeSupertiles = 0;
}




/************** CREATE SUPERTILE MEMORY LIST ********************/
//
// This preallocates all of the memory that will ever be needed by all of
// the supertiles on the screen for POLY_FT3's and vertex list data.
//

static void CreateSuperTileMemoryList(void)
{
short					u,v,i,j;
TQ3Status				status;
TQ3Point3D				p[NUM_VERTICES_IN_SUPERTILE];
TQ3TriMeshTriangleData	newTriangle[NUM_POLYS_IN_SUPERTILE];
TQ3Vector3D				faceNormals[NUM_POLYS_IN_SUPERTILE];
TQ3Vector3D				vertexNormals[NUM_VERTICES_IN_SUPERTILE];
TQ3Param2D				uvs[NUM_VERTICES_IN_SUPERTILE];
TQ3Param2D				uvs2[4] = {{0,0}, {1,0}, {0,1}, {1,1}};

Ptr						blankTexPtr;


			/**********************************/
			/* ALLOCATE MEMORY FOR SUPERTILES */
			/**********************************/

	// Source port addition: the supertile list is now dynamically-allocated
	// so we can toggle between Nanosaur and Nanosaur Extreme at boot time.
	// Also, we allow more active supertiles than the original game by using
	// shorts instead of bytes for supertile indices (thereby limiting the max
	// amount of supertiles to 32767).

	GAME_ASSERT_MESSAGE(MAX_SUPERTILES <= 32767, "MAX_SUPERTILES too large. Try decreasing SUPERTILE_ACTIVE_RANGE.");

	GAME_ASSERT_MESSAGE(!gSuperTileMemoryList, "gSuperTileMemoryList already allocated.");

	gSuperTileMemoryList = (SuperTileMemoryType *) AllocPtr(sizeof(SuperTileMemoryType) * MAX_SUPERTILES);


			/**********************************/
			/* MAKE EMPTY TRIANGLE DATA LISTS */
			/**********************************/
			
				/* INIT POINT COORDS & NORMALS */
				
	for (i = 0; i < NUM_VERTICES_IN_SUPERTILE; i++)
	{
		p[i].x = p[i].y = p[i].z = 0.0f;										// clear point list
		vertexNormals[i].x = vertexNormals[i].z = 0;						// set dummy vertex normals (point up)
		vertexNormals[i].y = 1.0;
		Q3Vector3D_Normalize(&vertexNormals[i],&vertexNormals[i]);
	}
	
			/* INIT UV'S */
			
	i = 0;
	for (v = 0; v <= SUPERTILE_SIZE; v++)									// sets uv's 0.0 -> 1.0 for single texture map
	{
		for (u = 0; u <= SUPERTILE_SIZE; u++)
		{
			uvs[i].u = (float)u / (float)SUPERTILE_SIZE;
			uvs[i].v = (float)v / (float)SUPERTILE_SIZE;
			i++;
		}	
	}

			/* INIT FACES & FACE NORMALS */
				
	j = 0;
	for (i = 0; i < NUM_POLYS_IN_SUPERTILE; i++)							// create triangle list
	{
		if (i&1)
		{
			newTriangle[i].pointIndices[0] = gTileTriangles2_A[j/SUPERTILE_SIZE][j%SUPERTILE_SIZE][0];
			newTriangle[i].pointIndices[1] = gTileTriangles2_A[j/SUPERTILE_SIZE][j%SUPERTILE_SIZE][1];
			newTriangle[i].pointIndices[2] = gTileTriangles2_A[j/SUPERTILE_SIZE][j%SUPERTILE_SIZE][2];
			j++;
		}
		else
		{
			newTriangle[i].pointIndices[0] = gTileTriangles1_A[j/SUPERTILE_SIZE][j%SUPERTILE_SIZE][0];
			newTriangle[i].pointIndices[1] = gTileTriangles1_A[j/SUPERTILE_SIZE][j%SUPERTILE_SIZE][1];
			newTriangle[i].pointIndices[2] = gTileTriangles1_A[j/SUPERTILE_SIZE][j%SUPERTILE_SIZE][2];
		}
		
		faceNormals[i].x = faceNormals[i].z = 0;							// set dummy face normals (pointing up!)
		faceNormals[i].y = 1;			
		Q3Vector3D_Normalize(&faceNormals[i],&faceNormals[i]);
	}



			/********************************************/
			/* FOR EACH POSSIBLE SUPERTILE ALLOC MEMORY */
			/********************************************/

	gNumFreeSupertiles = MAX_SUPERTILES;

	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		blankTexPtr = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * sizeof(uint16_t));
		GAME_ASSERT(blankTexPtr);

		GLuint textureName = Render_LoadTexture(
				TILE_TEXTURE_INTERNAL_FORMAT,
				SUPERTILE_TEXMAP_SIZE,
				SUPERTILE_TEXMAP_SIZE,
				TILE_TEXTURE_FORMAT,
				TILE_TEXTURE_TYPE,
				blankTexPtr,
				kRendererTextureFlags_ClampBoth
				);

					/* SET DATA */

		gSuperTileMemoryList[i].mode = SUPERTILE_MODE_FREE;						// it's free for use


		gSuperTileMemoryList[i].textureData = (uint16_t*) blankTexPtr;
		gSuperTileMemoryList[i].glTextureName = textureName;

		// TODO NOQUESA: do we need face normals at all?

					/* CREATE THE TRIMESH OBJECT */

		TQ3TriMeshData* tmd = Q3TriMeshData_New(NUM_POLYS_IN_SUPERTILE, NUM_VERTICES_IN_SUPERTILE);
		GAME_ASSERT(tmd);

		gSuperTileMemoryList[i].triMeshPtr = tmd;

		memcpy(tmd->triangles,		newTriangle,	sizeof(tmd->triangles[0]) * NUM_POLYS_IN_SUPERTILE);
		memcpy(tmd->vertexUVs,		uvs,			sizeof(tmd->vertexUVs[0]) * NUM_VERTICES_IN_SUPERTILE);
		for (int pointIndex = 0; pointIndex < NUM_VERTICES_IN_SUPERTILE; pointIndex++)
		{
			tmd->points[pointIndex] = (TQ3Point3D) { 0, 0, 0 };					// clear point list
			tmd->vertexNormals[pointIndex] = (TQ3Vector3D) { 0, 1, 0 };			// set dummy vertex normals (point up)
		}
		// TODO: face normals?

		tmd->bBox.isEmpty = kQ3False;					// calc bounding box
		tmd->bBox.min.x = tmd->bBox.min.y = tmd->bBox.min.z = 0;
		tmd->bBox.max.x = tmd->bBox.max.y = tmd->bBox.max.z = TERRAIN_SUPERTILE_UNIT_SIZE;

		tmd->hasTexture = true;
		tmd->glTextureName = textureName;


			/****************************************************/
			/* NOW CREATE SECONDARY TRIMESH FOR FLAT SUPERTILES */
			/****************************************************/

		TQ3TriMeshData* tmdFlat = Q3TriMeshData_New(2, 4);			// only 2 triangles in a flat supertile, 4 vertices
		GAME_ASSERT(tmdFlat);

		gSuperTileMemoryList[i].triMeshPtr2 = tmdFlat;

				/* CREATE THE 2 TRIANGLES */
		// SOURCE PORT NOTE: changed first tri's winding to 0-2-1. Second tri doesn't need to change.
		tmdFlat->triangles[0].pointIndices[0] = 0;
		tmdFlat->triangles[0].pointIndices[1] = 2;
		tmdFlat->triangles[0].pointIndices[2] = 1;
		tmdFlat->triangles[1].pointIndices[0] = 1;
		tmdFlat->triangles[1].pointIndices[1] = 2;
		tmdFlat->triangles[1].pointIndices[2] = 3;

		for (int pointIndex = 0; pointIndex < 4; pointIndex++)
		{
			tmdFlat->points[pointIndex] = (TQ3Point3D) {0, 0, 0 };				// clear point list
			tmdFlat->vertexNormals[pointIndex] = (TQ3Vector3D) {0, 1, 0 };		// set dummy vertex normals (point up)
		}

		memcpy(tmdFlat->vertexUVs, uvs2, 4 * sizeof(tmdFlat->vertexUVs[0]));
		// TODO: face normals?

		tmdFlat->bBox.isEmpty = kQ3False;					// calc bounding box
		tmdFlat->bBox.min.x = tmdFlat->bBox.min.y = tmdFlat->bBox.min.z = 0;
		tmdFlat->bBox.max.x = tmdFlat->bBox.max.y = tmdFlat->bBox.max.z = TERRAIN_SUPERTILE_UNIT_SIZE;

		tmdFlat->hasTexture = true;
		tmdFlat->glTextureName = textureName;
	}
}


/***************** GET FREE SUPERTILE MEMORY *******************/
//
// Finds one of the preallocated supertile memory blocks and returns its index
// IT ALSO MARKS THE BLOCK AS USED
//
// OUTPUT: index into gSuperTileMemoryList
//

static short GetFreeSuperTileMemory(void)
{
short	i;

				/* SCAN FOR A FREE BLOCK */

	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		if (gSuperTileMemoryList[i].mode == SUPERTILE_MODE_FREE)
		{
			gSuperTileMemoryList[i].mode = SUPERTILE_MODE_USED;
			gNumFreeSupertiles--;			
			return(i);
		}
	}

	DoFatalAlert("No Free Supertiles!");
	return(-1);											// ERROR, NO FREE BLOCKS!!!! SHOULD NEVER GET HERE!
}


/******************* BUILD TERRAIN SUPERTILE *******************/
//
// Builds a new supertile which has scrolled on
//
// INPUT: startCol = starting column in map
//		  startRow = starting row in map
//
// OUTPUT: index to supertile
//

static short	BuildTerrainSuperTile(long	startCol, long startRow)
{
long	 			row,col,row2,col2,i;
short				superTileNum;
float				height,miny,maxy;
TQ3Vector3D			normals[SUPERTILE_SIZE+1][SUPERTILE_SIZE+1];
TQ3TriMeshData		*triMeshPtr;
TQ3Vector3D			*normalPtr,*vertexNormalList;
UInt16				tile;
short				h1,h2,h3,h4;
TQ3Point3D			*pointList;
TQ3TriMeshTriangleData	*triangleList;
TQ3StorageObject	mipmapStorage;
UInt32				validSize,bufferSize;
SuperTileMemoryType	*superTilePtr;

	superTileNum = GetFreeSuperTileMemory();					// get memory block for the data
	superTilePtr = &gSuperTileMemoryList[superTileNum];			// get ptr to it

	superTilePtr->hiccupTimer = (gHiccupEliminator++ & 0x7);	// set hiccup timer to aleiviate hiccup caused by massive texture uploading

	superTilePtr->row = startRow;								// remember row/col of data for dereferencing later
	superTilePtr->col = startCol;

	superTilePtr->coord.x = (startCol * TERRAIN_POLYGON_SIZE) + (TERRAIN_SUPERTILE_UNIT_SIZE/2);		// also remember world coords
	superTilePtr->coord.z = (startRow * TERRAIN_POLYGON_SIZE) + (TERRAIN_SUPERTILE_UNIT_SIZE/2);

	superTilePtr->left = (startCol * TERRAIN_POLYGON_SIZE);		// also save left/back coord
	superTilePtr->back = (startRow * TERRAIN_POLYGON_SIZE);

			/*********************************/
			/* CREATE TERRAIN MESH VERTICES  */
			/*********************************/

	miny = 1000000;
	maxy = -miny;
	
	for (row2 = 0; row2 <= SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;
		
		for (col2 = 0; col2 <= SUPERTILE_SIZE; col2++)
		{
			col = col2 + startCol;
			
			if ((row >= gTerrainTileDepth) || (col >= gTerrainTileWidth))			// check for edge vertices (off map array)
				height = 0;
			else
				height = GetTerrainHeightAtRowCol(row,col); 						// get pixel height here

			gWorkGrid[row2][col2].x = (col*TERRAIN_POLYGON_SIZE);
			gWorkGrid[row2][col2].z = (row*TERRAIN_POLYGON_SIZE);

			gWorkGrid[row2][col2].y = height;										// save height @ this tile's upper left corner
			
			if (height > maxy)														// keep track of min/max
				maxy = height;
			if (height < miny)
				miny = height;			
		}
	}

	superTilePtr->coord.y = (miny+maxy)/2;						// This y coord is not used to translate since the terrain has no translation matrix
																// Instead, this is used by the cone-of-vision routine for culling tests

			/* CALC RADIUS */
			
	height = (maxy-miny)/2;
	if (height > gSuperTileRadius)
		superTilePtr->radius = height;
	else						
		superTilePtr->radius = gSuperTileRadius;


			/* SEE IF IT'S A FLAT SUPER-TILE */
			
	if (maxy == miny)
	{
		superTilePtr->isFlat = true;
		BuildTerrainSuperTile_Flat(superTilePtr,startCol,startRow);
		return(superTileNum);
	}
	else
		superTilePtr->isFlat = false;
	


			/******************************/
			/* CALCULATE VERTEX NORMALS   */
			/******************************/

	for (row2 = 0; row2 <= SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;
		
		for (col2 = 0; col2 <= SUPERTILE_SIZE; col2++)
		{
			float	center_h,left_h,right_h,back_h,front_h;
			
			col = col2 + startCol;

					/* GET CENTER HEIGHT */
					
			center_h = gWorkGrid[row2][col2].y;

					/* GET LEFT HEIGHT */
					
			if (col2 == 0)
				left_h = GetTerrainHeightAtRowCol(row,col-1);
			else
				left_h = gWorkGrid[row2][col2-1].y;
				
					/* GET RIGHT HEIGHT */
			
			if (col2 == SUPERTILE_SIZE)
				right_h = GetTerrainHeightAtRowCol(row,col+1);
			else
				right_h = gWorkGrid[row2][col2+1].y;
			
					/* GET BACK HEIGHT */
			
			if (row2 == 0)
				back_h = GetTerrainHeightAtRowCol(row-1,col);
			else
				back_h = gWorkGrid[row2-1][col2].y;
			
					/* GET FRONT HEIGHT */
			
			if (row2 == SUPERTILE_SIZE)
				front_h = GetTerrainHeightAtRowCol(row+1,col);
			else
				front_h = gWorkGrid[row2+1][col2].y;
			
			
			normals[row2][col2].x = ((left_h - center_h) + (center_h - right_h)) * .01;
			normals[row2][col2].y = 1;
			normals[row2][col2].z = ((back_h - center_h) + (center_h - front_h)) * .01;
			
			Q3Vector3D_Normalize(&normals[row2][col2],&normals[row2][col2]);						// normalize the vertex normal
			
		}
	}

				/***************************************/
				/* DETERMINE SPLITTING ANGLE FOR TILES */
				/***************************************/

	for (row = 0; row < SUPERTILE_SIZE; row++)
	{
		for (col = 0; col < SUPERTILE_SIZE; col++)
		{
			h1 = gWorkGrid[row][col].y;												// get height of all 4 vertices (clockwise)
			h2 = gWorkGrid[row][col+1].y;
			h3 = gWorkGrid[row+1][col+1].y;
			h4 = gWorkGrid[row+1][col].y;


					/* QUICK CHECK FOR FLAT POLYS */

			if ((h1 == h2) && (h1 == h3) && (h1 == h4))								// see if all same level
			{
				CalcPlaneEquationOfTriangle(&superTilePtr->tilePlanes1[row][col],	// calc plane equation for both triangles
							 &gWorkGrid[row][col], &gWorkGrid[row][col+1],
							 &gWorkGrid[row+1][col+1]);
				superTilePtr->tilePlanes2[row][col] = superTilePtr->tilePlanes1[row][col];

				superTilePtr->splitAngle[row][col] = SPLIT_ARBITRARY;				// split doesnt matter
				continue;
			}


				/* SEE IF MANUAL FOLD-SPLIT */

			tile = GetPathTileNumAtRowCol(row + startRow, col + startCol);			// get path tile to see if manual fold
			if ((tile == PATH_TILE_FOLD_A) || (tile == PATH_TILE_FOLD_B))
			{
				if (tile == PATH_TILE_FOLD_A)
					superTilePtr->splitAngle[row][col] = SPLIT_FORWARD;				// use / splits
				else
					superTilePtr->splitAngle[row][col] = SPLIT_BACKWARD; 			// use \ splits
			}
			
			
					/* CALC FOLD-SPLIT */
			else
			{
				if (abs(h1-h3) < abs(h2-h4))
					superTilePtr->splitAngle[row][col] = SPLIT_BACKWARD; 			// use \ splits
				else
					superTilePtr->splitAngle[row][col] = SPLIT_FORWARD;				// use / splits
			}
			
			
				/* CALC PLANE EQUATION FOR EACH POLY */
				
			if (superTilePtr->splitAngle[row][col] == SPLIT_BACKWARD)				// if \ split
			{
				CalcPlaneEquationOfTriangle(&superTilePtr->tilePlanes1[row][col],	// calc plane equation for left triangle
							 &gWorkGrid[row][col], &gWorkGrid[row+1][col+1],
							 &gWorkGrid[row+1][col]);

				CalcPlaneEquationOfTriangle(&superTilePtr->tilePlanes2[row][col],	// calc plane equation for right triangle
							 &gWorkGrid[row][col], &gWorkGrid[row][col+1],
							 &gWorkGrid[row+1][col+1]);			
			}
			else																	// otherwise, / split
			{
				CalcPlaneEquationOfTriangle(&superTilePtr->tilePlanes1[row][col],	// calc plane equation for left triangle
							 &gWorkGrid[row][col], &gWorkGrid[row][col+1],
							 &gWorkGrid[row+1][col]);

				CalcPlaneEquationOfTriangle(&superTilePtr->tilePlanes2[row][col],	// calc plane equation for right triangle
							 &gWorkGrid[row][col+1], &gWorkGrid[row+1][col+1],
							 &gWorkGrid[row+1][col]);			
			}			
		}
	}

			/*********************************/
			/* CREATE TERRAIN MESH POLYGONS  */
			/*********************************/

					/* GET THE TRIMESH */

	triMeshPtr = gSuperTileMemoryList[superTileNum].triMeshPtr;					// get the triMesh
	GAME_ASSERT(triMeshPtr);

	pointList = triMeshPtr->points;												// get ptr to point/vertex list
	triangleList = triMeshPtr->triangles;										// get ptr to triangle index list
	vertexNormalList = triMeshPtr->vertexNormals;								// get ptr to vertex normals
	normalPtr = nil;	// TODO QUESA: FACE NORMALS!							// get ptr to face normals

	
			/* SET BOUNDING BOX */
			
	triMeshPtr->bBox.min.x = gWorkGrid[0][0].x;
	triMeshPtr->bBox.max.x = triMeshPtr->bBox.min.x+TERRAIN_SUPERTILE_UNIT_SIZE;
	triMeshPtr->bBox.min.y = miny;
	triMeshPtr->bBox.max.y = maxy;
	triMeshPtr->bBox.min.z = gWorkGrid[0][0].z;
	triMeshPtr->bBox.max.z = triMeshPtr->bBox.min.z + TERRAIN_SUPERTILE_UNIT_SIZE;
				

					/* SET VERTEX COORDS & NORMALS */
	
	i = 0;			
	for (row = 0; row < (SUPERTILE_SIZE+1); row++)
		for (col = 0; col < (SUPERTILE_SIZE+1); col++)
		{
			pointList[i] = gWorkGrid[row][col];									// copy from other list
			vertexNormalList[i] = normals[row][col];
			i++;
		}
	

				/* UPDATE TRIMESH DATA WITH NEW INFO */
			

	i = 0;			
	for (row2 = 0; row2 < SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;

		for (col2 = 0; col2 < SUPERTILE_SIZE; col2++)
		{

			col = col2 + startCol;

					/* ADD TILE TO PIXMAP */
					
			tile = gTerrainTextureLayer[row][col];									// get tile from map
			DrawTileIntoMipmap(tile,row2,col2,gTempTextureBuffer);		// draw into mipmap & return anim flag

					/* SET SPLITTING INFO */

			if (gSuperTileMemoryList[superTileNum].splitAngle[row2][col2] == SPLIT_BACKWARD)	// set coords & uv's based on splitting
			{
					/* \ */
				// SOURCE PORT NOTE: changed winding to 0-2-1 for both tris.
				triMeshPtr->triangles[i].pointIndices[0] = gTileTriangles1_B[row2][col2][0];
				triMeshPtr->triangles[i].pointIndices[1] = gTileTriangles1_B[row2][col2][2];
				triMeshPtr->triangles[i].pointIndices[2] = gTileTriangles1_B[row2][col2][1];
				i++;
				triMeshPtr->triangles[i].pointIndices[0] = gTileTriangles2_B[row2][col2][0];
				triMeshPtr->triangles[i].pointIndices[1] = gTileTriangles2_B[row2][col2][2];
				triMeshPtr->triangles[i].pointIndices[2] = gTileTriangles2_B[row2][col2][1];
				i++;
			}
			else
			{
					/* / */
				// SOURCE PORT NOTE: changed winding to 0-2-1 for both tris.
				triMeshPtr->triangles[i].pointIndices[0] = gTileTriangles1_A[row2][col2][0];
				triMeshPtr->triangles[i].pointIndices[1] = gTileTriangles1_A[row2][col2][2];
				triMeshPtr->triangles[i].pointIndices[2] = gTileTriangles1_A[row2][col2][1];
				i++;
				triMeshPtr->triangles[i].pointIndices[0] = gTileTriangles2_A[row2][col2][0];
				triMeshPtr->triangles[i].pointIndices[1] = gTileTriangles2_A[row2][col2][2];
				triMeshPtr->triangles[i].pointIndices[2] = gTileTriangles2_A[row2][col2][1];
				i++;
			}			
		}
	}


			/******************/
			/* UPDATE TEXTURE */
			/******************/

			/* GET MIPMAP BUFFER */

	// store a resized copy of the texture in the supertile's textureData buffer
	ShrinkSuperTileTextureMap(gTempTextureBuffer, superTilePtr->textureData, SUPERTILE_TEXMAP_SIZE);

				/* UPDATE THE TRIMESH */

	glBindTexture(GL_TEXTURE_2D, superTilePtr->glTextureName);
	glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0,
			0,
			SUPERTILE_TEXMAP_SIZE,
			SUPERTILE_TEXMAP_SIZE,
			TILE_TEXTURE_FORMAT,
			TILE_TEXTURE_TYPE,
			superTilePtr->textureData);
	CHECK_GL_ERROR();


	return(superTileNum);
}



/************************* BUILD TERRAIN SUPERTILE: FLAT ************************************/

static void BuildTerrainSuperTile_Flat(SuperTileMemoryType	*superTilePtr, long startCol, long startRow) 
{
TQ3TriMeshData		*triMeshPtr;
TQ3Point3D			*pointList;
TQ3StorageObject	mipmapStorage;
TQ3Status			status;
long				row2,col2,row,col;
UInt32				validSize,bufferSize;
UInt16				tile;
TQ3PlaneEquation	planeEq;

			/*********************************/
			/* CREATE TERRAIN MESH POLYGONS  */
			/*********************************/

					/* GET THE TRIMESH */

	triMeshPtr = superTilePtr->triMeshPtr2;
	pointList = triMeshPtr->points;												// get ptr to point/vertex list


					/* SET VERTEX COORDS */
	
	pointList[0] = gWorkGrid[0][0];
	pointList[1] = gWorkGrid[0][SUPERTILE_SIZE];
	pointList[2] = gWorkGrid[SUPERTILE_SIZE][0];
	pointList[3] = gWorkGrid[SUPERTILE_SIZE][SUPERTILE_SIZE];

	pointList[1].x += SUPERTILE_OVERLAP;										// overlap by n pixels
	pointList[2].z += SUPERTILE_OVERLAP;
	pointList[3].x += SUPERTILE_OVERLAP;
	pointList[3].z += SUPERTILE_OVERLAP;
	pointList[0].x -= SUPERTILE_OVERLAP;
	pointList[0].z -= SUPERTILE_OVERLAP;
	pointList[1].z -= SUPERTILE_OVERLAP;
	pointList[2].x -= SUPERTILE_OVERLAP;

			/* SET BOUNDING BOX */

	triMeshPtr->bBox.min.x = pointList[0].x;
	triMeshPtr->bBox.max.x = pointList[1].x;
	triMeshPtr->bBox.min.y = pointList[0].y;
	triMeshPtr->bBox.max.y = pointList[0].y;
	triMeshPtr->bBox.min.z = pointList[0].z;
	triMeshPtr->bBox.max.z = pointList[2].z;


	CalcPlaneEquationOfTriangle(&planeEq, &pointList[0], &pointList[1],&pointList[2]);// calc plane equation for entire supertile


				/* UPDATE TEXTURE MAP */
		
	for (row2 = 0; row2 < SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;

		for (col2 = 0; col2 < SUPERTILE_SIZE; col2++)
		{
			col = col2 + startCol;
			tile = gTerrainTextureLayer[row][col];									// get tile from map
			DrawTileIntoMipmap(tile,row2,col2,gTempTextureBuffer);					// draw into mipmap
			
			
			superTilePtr->tilePlanes1[row2][col2] = 								// calc plane equation for triangles
			superTilePtr->tilePlanes2[row2][col2] = planeEq;			
			superTilePtr->splitAngle[row2][col2] = SPLIT_ARBITRARY;					// set split to arbitrary (since there isn't any)
		}
	}

				/* UPDATE TEXTURE */

	// store a resized copy of the texture in the supertile's textureData buffer
	ShrinkSuperTileTextureMap(gTempTextureBuffer, superTilePtr->textureData, SUPERTILE_TEXMAP_SIZE);

	glBindTexture(GL_TEXTURE_2D, superTilePtr->glTextureName);
	glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0,
			0,
			SUPERTILE_TEXMAP_SIZE,
			SUPERTILE_TEXMAP_SIZE,
			TILE_TEXTURE_FORMAT,
			TILE_TEXTURE_TYPE,
			superTilePtr->textureData);
	CHECK_GL_ERROR();
}



/********************* DRAW TILE INTO MIPMAP *************************/

static void DrawTileIntoMipmap(UInt16 tile, short row, short col, UInt16 *buffer)
{
UInt16		texMapNum,flipRotBits;
UInt64		*tileData;
Byte		y;	
UInt64		*dPtr;
UInt16		*sPtr,*tileDataS;

			/* EXTRACT BITS INFO FROM TILE */
				
	flipRotBits = tile&(TILE_FLIPXY_MASK|TILE_ROTATE_MASK);		// get flip & rotate bits
	texMapNum = tile&TILENUM_MASK; 								// filter out texture #

	if (texMapNum >= gNumTerrainTextureTiles)					// make sure not illegal tile #
	{
//		DoFatalAlert("DrawTileIntoMipmap: illegal tile #");
		texMapNum = 0;
	}
				/* CALC PTRS */
				
	buffer += ((row * OREOMAP_TILE_SIZE) * (SUPERTILE_SIZE * OREOMAP_TILE_SIZE)) + (col * OREOMAP_TILE_SIZE);		// get dest
	tileData = (UInt64 *)(gTileDataPtr + (texMapNum * OREOMAP_TILE_SIZE * OREOMAP_TILE_SIZE));						// get src




	switch(flipRotBits)         											// set uv's based on flip & rot bits
	{
				/* NO FLIP & NO ROT */
					/* XYFLIP ROT 2 */

		case	0:
		case	TILE_FLIPXY_MASK | TILE_ROT2:
				dPtr = (UInt64 *)buffer;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					memcpy(dPtr, tileData, 8 * sizeof(UInt64));

					dPtr += (OREOMAP_TILE_SIZE/4)*SUPERTILE_SIZE;			// next line in dest
					tileData += (OREOMAP_TILE_SIZE/4);						// next line in src
				}
				break;

					/* FLIP X */
				/* FLIPY ROT 2 */

		case	TILE_FLIPX_MASK:
		case	TILE_FLIPY_MASK | TILE_ROT2:
				sPtr = (UInt16 *)buffer;
				tileDataS = (UInt16 *)tileData;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					for (int x = 0; x < 32; x++)
						sPtr[x] = tileDataS[31-x];

					sPtr += OREOMAP_TILE_SIZE * SUPERTILE_SIZE;		// next line in dest
					tileDataS += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

					/* FLIP Y */
				/* FLIPX ROT 2 */

		case	TILE_FLIPY_MASK:
		case	TILE_FLIPX_MASK | TILE_ROT2:
				dPtr = (UInt64 *)buffer;
				tileData += (OREOMAP_TILE_SIZE*(OREOMAP_TILE_SIZE-1)*2)/8;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					memcpy(dPtr, tileData, 8 * sizeof(uint64_t));

					dPtr += (OREOMAP_TILE_SIZE/4)*SUPERTILE_SIZE;			// next line in dest
					tileData -= (OREOMAP_TILE_SIZE*2/8);					// next line in src
				}
				break;


				/* FLIP XY */
				/* NO FLIP ROT 2 */

		case	TILE_FLIPXY_MASK:
		case	TILE_ROT2:
				sPtr = (UInt16 *)buffer;
				tileDataS = (UInt16 *)(tileData + (OREOMAP_TILE_SIZE*(OREOMAP_TILE_SIZE-1)*2)/8);
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					for (int x = 0; x < 32; x++)
						sPtr[x] = tileDataS[31-x];

					sPtr += (OREOMAP_TILE_SIZE/4)*SUPERTILE_SIZE*4;			// next line in dest
					tileDataS -= (OREOMAP_TILE_SIZE*2/2);					// next line in src
				}
				break;

				/* NO FLIP ROT 1 */
				/* FLIP XY ROT 3 */

		case	TILE_ROT1:
		case	TILE_FLIPXY_MASK | TILE_ROT3:
				sPtr = (UInt16 *)buffer + (OREOMAP_TILE_SIZE-1);			// draw to right col from top row of src
				tileDataS = (UInt16 *)tileData;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					for (int x = 0; x < 32; x++)
						sPtr[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*x] = tileDataS[x];
						
					sPtr--;											// prev col in dest
					tileDataS += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

				/* NO FLIP ROT 3 */
				/* FLIP XY ROT 1 */

		case	TILE_ROT3:
		case	TILE_FLIPXY_MASK | TILE_ROT1:
				sPtr = (UInt16 *)buffer;
				tileDataS = (UInt16 *)tileData;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					for (int x = 0; x < 32; x++)
						sPtr[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*(31-x)] = tileDataS[x];		// backwards

					sPtr++;											// next col in dest
					tileDataS += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

				/* FLIP X ROT 1 */
				/* FLIP Y ROT 3 */

		case	TILE_FLIPX_MASK | TILE_ROT1:
		case	TILE_FLIPY_MASK | TILE_ROT3:
				sPtr = (UInt16 *)buffer + (OREOMAP_TILE_SIZE-1);
				tileDataS = (UInt16 *)tileData;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					for (int x = 0; x < 32; x++)
						sPtr[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*(31-x)] = tileDataS[x];		// backwards

					sPtr--;											// next col in dest
					tileDataS += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;

				/* FLIP X ROT 3 */
				/* FLIP Y ROT 1 */

		case	TILE_FLIPX_MASK | TILE_ROT3:
		case	TILE_FLIPY_MASK | TILE_ROT1:
				sPtr = (UInt16 *)buffer;							// draw to right col from top row of src
				tileDataS = (UInt16 *)tileData;
				for (y =  0; y < OREOMAP_TILE_SIZE; y++)
				{
					for (int x = 0; x < 32; x++)
						sPtr[OREOMAP_TILE_SIZE*SUPERTILE_SIZE*x] = tileDataS[x];

					sPtr++;											// prev col in dest
					tileDataS += OREOMAP_TILE_SIZE;					// next line in src
				}
				break;
	}


}





/************ SHRINK SUPERTILE TEXTURE MAP ********************/
//
// Shrinks a 160x160 src texture to a 160x160, 128x128, or 64x64 dest texture
//

static void ShrinkSuperTileTextureMap(const uint16_t* srcPtr, uint16_t* dstPtr, int dstSize)
{
#if ((SUPERTILE_SIZE * OREOMAP_TILE_SIZE) != 160)
	ReWrite this!
#endif

	switch (dstSize)
	{
		case 64:
			for (int y = 0; y < 64; y++)
			{
				for (int x = 0; x < 64; x += 2)
				{
					dstPtr[0] = srcPtr[0];
					dstPtr[1] = srcPtr[2];
					dstPtr += 2;
					srcPtr += 5;
				}

				srcPtr += SUPERTILE_SIZE * OREOMAP_TILE_SIZE;					// skip every other line

				if (y % 2 == 0)													// skip another line
					srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE;
			}
			break;

		case 128:
			for (int y = 0; y < 128; y++)
			{
				for (int x = 0; x < 128; x += 4)
				{
					dstPtr[0] = srcPtr[0];
					dstPtr[1] = srcPtr[1];
					dstPtr[2] = srcPtr[2];
					dstPtr[3] = srcPtr[3];
					dstPtr += 4;
					srcPtr += 5;
				}

				if (y % 4 == 3)													// skip every fourth line
					srcPtr +=  SUPERTILE_SIZE * OREOMAP_TILE_SIZE;
			}
			break;

		case 160:
			memcpy(dstPtr, srcPtr, 160 * 160 * sizeof(UInt16));
			break;

		default:
			DoFatalAlert("ShrinkSuperTileTextureMap: Unsupported target size");
			break;
	}
}


/******************* RELEASE SUPERTILE OBJECT *******************/
//
// Deactivates the terrain object and releases its memory block
//

static inline void ReleaseSuperTileObject(short superTileNum)
{
	gSuperTileMemoryList[superTileNum].mode = SUPERTILE_MODE_FREE;		// it's free!
	gNumFreeSupertiles++;
}

/********************* DRAW TERRAIN **************************/
//
// This is the main call to update the screen.  It draws all ObjNode's and the terrain itself
//

void DrawTerrain(QD3DSetupOutputType *setupInfo)
{
				/* DRAW SUPERTILE TRIMESHES */

	for (int i = 0; i < MAX_SUPERTILES; i++)
	{
		if (gSuperTileMemoryList[i].mode != SUPERTILE_MODE_USED)		// if supertile is being used, then draw it
			continue;
		
#if !TWO_MEG_VERSION		
		if (gSuperTileMemoryList[i].hiccupTimer != 0)					// see if this supertile is still in hiccup prevention mode
		{
			gSuperTileMemoryList[i].hiccupTimer--;
			continue;
		}
#endif		

		if (!IsSphereInFrustum_XZ(										// make sure it's visible
				&gSuperTileMemoryList[i].coord,
				1.25f*gSuperTileMemoryList[i].radius))
		{
			continue;
		}

			/* DRAW THE TRIMESH IN THIS SUPERTILE */

		TQ3TriMeshData* mesh = gSuperTileMemoryList[i].isFlat
				? gSuperTileMemoryList[i].triMeshPtr2
				: gSuperTileMemoryList[i].triMeshPtr;

		Render_DrawTriMeshList(1, &mesh, nil, nil);
	}


		/* DRAW OBJECTS */
		
	DrawObjects(setupInfo);												// draw objNodes
	QD3D_DrawParticles(setupInfo);
}


/***************** GET TERRAIN HEIGHT AT COORD ******************/
//
// Given a world x/z coord, return the y coord based on height map
//
// INPUT: x/z = world coords 
//
// OUTPUT: y = world y coord
//

float	GetTerrainHeightAtCoord(float x, float z)
{
long	row,col,offx,offy;
short	height[2][2];
UInt16	tileNum,tile,flipBits;
float	heightf,fracX,fracZ;
long	newX[2][2],newZ[2][2];
UInt8	h,v;


			/* CALC X/Z FOR 4 PIXELS */
			
	newX[0][1] = newX[1][1] = (newX[0][0] = newX[1][0] = x) + (TERRAIN_POLYGON_SIZE/TERRAIN_HMTILE_SIZE);		// calc integer version of x/z coords
	newZ[1][0] = newZ[1][1] = (newZ[0][0] = newZ[0][1] = z) + (TERRAIN_POLYGON_SIZE/TERRAIN_HMTILE_SIZE);
	
	
	for (v = 0; v < 2; v++)
		for (h = 0; h < 2; h++)
		{
			if ((h ==1) && (v == 1))										// skip [1][1] since we only need 3 samples not all 4
				continue;
				
			if ((newX[v][h] < 0) || (newZ[v][h] < 0))						// see if out of bounds
				return(0);
			if ((newX[v][h] >= gTerrainUnitWidth) || (newZ[v][h] >= gTerrainUnitDepth))
				return(0);

			col = (float)newX[v][h]*gOneOver_TERRAIN_POLYGON_SIZE;			// calc map row/col that the coord lies on
			row = (float)newZ[v][h]*gOneOver_TERRAIN_POLYGON_SIZE;

			tile = gTerrainHeightMapLayer[row][col];						// get height tile from map
			tileNum = tile&TILENUM_MASK;  									// filter out tile #
			flipBits = tile&TILE_FLIPXY_MASK;								// filter out flip bits
				
			offx = ((newX[v][h]%TERRAIN_POLYGON_SIZE)*TERRAIN_HMTILE_SIZE)*gOneOver_TERRAIN_POLYGON_SIZE;	// see how far into 32x32 tile we are
			if (flipBits & TILE_FLIPX_MASK)									// see if flipped X
				offx = TERRAIN_HMTILE_SIZE-1-offx;

			offy = ((newZ[v][h]%TERRAIN_POLYGON_SIZE)*TERRAIN_HMTILE_SIZE)*gOneOver_TERRAIN_POLYGON_SIZE;
			if (flipBits & TILE_FLIPY_MASK)									// see if flipped Y
				offy = TERRAIN_HMTILE_SIZE-1-offy;
						
			height[v][h] =  *(gTerrainHeightMapPtrs[tileNum]+(offy*TERRAIN_HMTILE_SIZE)+offx);	// get pixel value from height map data
		}
	
	
			/* CALC WEIGHTED Y */
		
	fracX = x*gUnitToPixel - (float)((long)(x*gUnitToPixel));
	fracZ = z*gUnitToPixel - (float)((long)(z*gUnitToPixel));

	if (height[0][0] != height[1][0])											// see if z interpolate
		heightf = ((float)height[0][0] * (1-fracZ)) + ((float)height[1][0] * fracZ);
	else
	if (height[0][0] != height[0][1])											// see if x interpolate
		heightf = ((float)height[0][0] * (1-fracX)) + ((float)height[0][1] * fracX);
	else
		heightf = height[0][0];

	heightf *= 	HEIGHT_EXTRUDE_FACTOR;	

	return(heightf);
 }


/***************** GET TERRAIN HEIGHT AT COORD: PLANAR ******************/
//
// Given a world x/z coord, return the y coord based on the plane equation
// of the supertile's polygons beneath it.
//
// INPUT: x/z = world coords 
//
// OUTPUT: y = world y coord
//

float	GetTerrainHeightAtCoord_Planar(float x, float z)
{
short			superTileX,superTileZ;
long			xi,zi;
UInt16			col,row;
short			superTileNum;
SuperTileMemoryType	*superTilePtr;
TQ3PlaneEquation	*planeEq;

	xi = x;
	zi = z;

			/* CALC SUPERTILE ROW/COL OF COORD */
			
	superTileX = xi / (TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE);			// calc x
	if ((superTileX < 0) ||	(superTileX >= gNumSuperTilesWide))			// verify bounds
		return(0);

	superTileZ = zi / (TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE);			// calc z
	if ((superTileZ < 0) ||	(superTileZ >= gNumSuperTilesDeep))			// verify bounds
		return(0);


				/* GET THE SUPERTILE # */
					
	superTileNum = gTerrainScrollBuffer[superTileZ][superTileX];
	if (superTileNum == EMPTY_SUPERTILE)								// if no supertile for this area, then just do the rough calc
		return(GetTerrainHeightAtCoord_Quick(x,z));

	superTilePtr = &gSuperTileMemoryList[superTileNum];					// get ptr to the supertile

	xi -= superTilePtr->left;											// calc offset into supertile
	zi -= superTilePtr->back;

	col = xi / TERRAIN_POLYGON_SIZE;									// see which tile we're on
	row = zi / TERRAIN_POLYGON_SIZE;			


			/*******************************************/
			/* DETERMINE WHICH TRIANGLE OF TILE TO USE */
			/*******************************************/

			/* IF ARBITRARY, THEN COPLANAR AND CAN USE EITHER ONE */
				
	if (superTilePtr->splitAngle[row][col] == SPLIT_ARBITRARY)
	{
		planeEq = &superTilePtr->tilePlanes1[row][col];
		gRecentTerrainNormal = planeEq->normal;								// remember the normal here
		return (IntersectionOfYAndPlane(x,z,planeEq));						// return intersection y
	}	
	else
	{
		xi -= (col * TERRAIN_POLYGON_SIZE);									// calc offset into this tile
		zi -= (row * TERRAIN_POLYGON_SIZE);
		
					/* HANDLE A FORWARD SPLIT */
					
		if (superTilePtr->splitAngle[row][col] == SPLIT_FORWARD)
		{
			if (xi < (TERRAIN_POLYGON_SIZE-zi))
			{
				planeEq = &superTilePtr->tilePlanes1[row][col];
				gRecentTerrainNormal = planeEq->normal;						// remember the normal here
				return (IntersectionOfYAndPlane(x,z,planeEq));
			}
			else
			{
				planeEq = &superTilePtr->tilePlanes2[row][col];
				gRecentTerrainNormal = planeEq->normal;						// remember the normal here
				return (IntersectionOfYAndPlane(x,z,planeEq));
			}
		}
		
					/* HANDLE A BACKWARDS SPLIT */
		else
		{
			if (xi < zi)
			{
				planeEq = &superTilePtr->tilePlanes1[row][col];
				gRecentTerrainNormal = planeEq->normal;						// remember the normal here
				return (IntersectionOfYAndPlane(x,z,planeEq));
			}
			else
			{
				planeEq = &superTilePtr->tilePlanes2[row][col];
				gRecentTerrainNormal = planeEq->normal;						// remember the normal here
				return (IntersectionOfYAndPlane(x,z,planeEq));
			}
		}
	}
 }



/***************** GET TERRAIN HEIGHT AT COORD_QUICK ******************/
//
// Same as above, but does not do interpolation thus should not be used for moving objects.
//
// Also note that this version takes x/z inputs as longs, not floats like above.
//

float	GetTerrainHeightAtCoord_Quick(long x, long z)
{
long	row,col,offx,offy;
short	height;
UInt16	tileNum,tile,flipBits;
Ptr		pixelPtr;
float	heightf;

	gRecentTerrainNormal.x = 0;								// normal is unknown here
	gRecentTerrainNormal.y = 1.0;
	gRecentTerrainNormal.z = 0;

	if ((x < 0) || (z < 0))										// see if out of bounds
		return(0);
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return(0);

	col = (float)x*gOneOver_TERRAIN_POLYGON_SIZE;	 			// calc map row/col that the coord lies on
	row = (float)z*gOneOver_TERRAIN_POLYGON_SIZE;

	tile = gTerrainHeightMapLayer[row][col];					// get height tile from map
	tileNum = tile&TILENUM_MASK;  								// filter out tile #
	flipBits = tile&TILE_FLIPXY_MASK;							// filter out flip bits


	offx = ((x%TERRAIN_POLYGON_SIZE)*TERRAIN_HMTILE_SIZE)*gOneOver_TERRAIN_POLYGON_SIZE;	// see how far into 32x32 tile we are
	offy = ((z%TERRAIN_POLYGON_SIZE)*TERRAIN_HMTILE_SIZE)*gOneOver_TERRAIN_POLYGON_SIZE;

	if (flipBits & TILE_FLIPX_MASK)							// see if flipped X
		offx = TERRAIN_HMTILE_SIZE-1-offx;

	if (flipBits & TILE_FLIPY_MASK)							// see if flipped Y
		offy = TERRAIN_HMTILE_SIZE-1-offy;


	pixelPtr = gTerrainHeightMapPtrs[tileNum];				// point to that tile's pixel data
	pixelPtr += (offy*TERRAIN_HMTILE_SIZE)+offx;			// calc index into tile data for the pixel we want

	height = *pixelPtr;										// get pixel value
	heightf = height*HEIGHT_EXTRUDE_FACTOR; 				// scale it to a useable y coordinate

	return(heightf);
 }



/***************** GET TERRAIN HEIGHT AT ROW/COL ******************/
//
// INPUT: row/col= map row/col
//
// OUTPUT: y = world y coord
//

static float	GetTerrainHeightAtRowCol(long row, long col)
{
long	offx,offy;
unsigned char	height;
UInt16	tileNum,tile,flipBits;
Ptr		pixelPtr;
float	heightf;

	if (row < 0)
		return(0);
	if (row >= gTerrainTileDepth)
		return(0);
	if (col < 0)
		return(0);
	if (col >= gTerrainTileWidth)
		return(0);

	tile = gTerrainHeightMapLayer[row][col];				// get height data from map
	tileNum = tile&TILENUM_MASK; 							// filter out tile #
	flipBits = tile&TILE_FLIPXY_MASK;						// filter out flip bits

	pixelPtr = gTerrainHeightMapPtrs[tileNum];				// point to that tile's pixel data

	if (flipBits & TILE_FLIPX_MASK)							// see if flipped X
		offx = TERRAIN_HMTILE_SIZE-1;
	else
		offx = 0;

	if (flipBits & TILE_FLIPY_MASK)							// see if flipped Y
		offy = TERRAIN_HMTILE_SIZE-1;
	else
		offy = 0;

	pixelPtr += (offy*TERRAIN_HMTILE_SIZE)+offx;			// calc index into tile data for the pixel we want

	height = *pixelPtr++;									// get pixel value
	heightf = (float)height * HEIGHT_EXTRUDE_FACTOR; 		// scale it to a useable y coordinate

	return(heightf);
}




/***************** GET SUPERTILE INFO ******************/
//
// Given a world x/z coord, return some supertile info
//
// INPUT: x/y = world x/y coords
// OUTPUT: row/col in tile coords and supertile coords
//

void GetSuperTileInfo(long x, long z, long *superCol, long *superRow, long *tileCol, long *tileRow)
{
long	row,col;


//	if ((x < 0) || (y < 0))									// see if out of bounds
//		return;
//	if ((x >= gTerrainUnitWidth) || (y >= gTerrainUnitDepth))
//		return;

	col = x * (1.0/TERRAIN_SUPERTILE_UNIT_SIZE);					// calc supertile relative row/col that the coord lies on
	row = z * (1.0/TERRAIN_SUPERTILE_UNIT_SIZE);

	*superRow = row;										// return which supertile relative row/col it is
	*superCol = col;
	*tileRow = row*SUPERTILE_SIZE;							// return which tile row/col the super tile starts on
	*tileCol = col*SUPERTILE_SIZE;
}


/******************** DO MY TERRAIN UPDATE ********************/

void DoMyTerrainUpdate(void)
{
long	x,y;
long	superCol,superRow,tileCol,tileRow;


			/* CALC PIXEL COORDS OF FAR LEFT SUPER TILE */

	x = gMyCoord.x-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	y = gMyCoord.z-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);


			/* SEE IF WE'VE SCROLLED A WHOLE SUPERTILE YET */

	GetSuperTileInfo(x,y,&superCol,&superRow,&tileCol,&tileRow); 		// get supertile coord info


		// NOTE: DO VERTICAL FIRST!!!!

				/* SEE IF SCROLLED UP */

	if (superRow > gCurrentSuperTileRow)
	{
		if (superRow > (gCurrentSuperTileRow+1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled up > 1 tile!");
		ScrollTerrainUp(superRow,superCol);
		gCurrentSuperTileRow = superRow;
	}
	else
				/* SEE IF SCROLLED DOWN */

	if (superRow < gCurrentSuperTileRow)
	{
		if (superRow < (gCurrentSuperTileRow-1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled down < -1 tile!");
		ScrollTerrainDown(superRow,superCol);
		gCurrentSuperTileRow = superRow;
	}

			/* SEE IF SCROLLED LEFT */

	if (superCol > gCurrentSuperTileCol)
	{
		if (superCol > (gCurrentSuperTileCol+1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled left > 1 tile!");
		ScrollTerrainLeft();
	}
	else
				/* SEE IF SCROLLED RIGHT */

	if (superCol < gCurrentSuperTileCol)
	{
		if (superCol < (gCurrentSuperTileCol-1))						// check for overload scroll
			DoFatalAlert("DoMyTerrainUpdate: scrolled right < -1 tile!");
		ScrollTerrainRight(superCol,superRow,tileCol,tileRow);
		gCurrentSuperTileCol = superCol;
	}

	CalcNewItemDeleteWindow();							// recalc item delete window

}


/****************** CALC NEW ITEM DELETE WINDOW *****************/

static void CalcNewItemDeleteWindow(void)
{
long	temp,temp2;

				/* CALC LEFT SIDE OF WINDOW */

	temp = gCurrentSuperTileCol*(TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE);		// convert to 3space coords
	temp2 = temp - (ITEM_WINDOW+OUTER_SIZE)*         	   			  	// factor window left
			(TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE);
	gTerrainItemDeleteWindow_Left = temp2;


				/* CALC RIGHT SIDE OF WINDOW */

	temp += SUPERTILE_DIST_WIDE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE;				// calc offset to right side
	temp += (ITEM_WINDOW+OUTER_SIZE)*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE; 	// factor window right
	gTerrainItemDeleteWindow_Right = temp;


				/* CALC FAR SIDE OF WINDOW */

	temp = gCurrentSuperTileRow*(TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE);				// convert to 3space coords
	temp2 = temp-(ITEM_WINDOW+OUTER_SIZE)*TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE;	// factor window top/back
	gTerrainItemDeleteWindow_Far = temp2;


				/* CALC NEAR SIDE OF WINDOW */

	temp += SUPERTILE_DIST_DEEP*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE;		// calc offset to bottom side
	temp += (ITEM_WINDOW+OUTER_SIZE)*                				// factor window bottom/front
			TERRAIN_POLYGON_SIZE*SUPERTILE_SIZE;
	gTerrainItemDeleteWindow_Near = temp;
}



/********************** SCROLL TERRAIN UP *************************/
//
// INPUT: superRow = new supertile row #
//

static void ScrollTerrainUp(long superRow, long superCol)
{
long	col,i,bottom,left,right;
short	superTileNum;
long	tileRow,tileCol;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

			/* PURGE OLD TOP ROW */

	if ((gCurrentSuperTileRow < gNumSuperTilesDeep) && (gCurrentSuperTileRow >= 0))	// check if off map
	{
		for (i = 0; i < SUPERTILE_DIST_WIDE; i++)
		{
			col = gCurrentSuperTileCol+i;
			if (col >= gNumSuperTilesWide)											// check if off map
				break;
			if (col < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[gCurrentSuperTileRow][col];			// get supertile for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);						 		// free the supertile
				gTerrainScrollBuffer[gCurrentSuperTileRow][col] = EMPTY_SUPERTILE;
			}
		}
	}


		/* CREATE NEW BOTTOM ROW */

	superRow += SUPERTILE_DIST_DEEP-1;		 				   				// calc row # of bottom supertile row
	tileRow = superRow * SUPERTILE_SIZE;  									// calc row # of bottom tile row

	if (superRow >= gNumSuperTilesDeep)										// see if off map
		return;
	if (superRow < 0)
		return;

	tileCol = gCurrentSuperTileCol * SUPERTILE_SIZE;						// calc col # bot left tile col

	for (col = gCurrentSuperTileCol; col < (gCurrentSuperTileCol + SUPERTILE_DIST_WIDE); col++)
	{
		if (col >= gNumSuperTilesWide)
			goto check_items;
		if (col < 0)
			goto next;

		if (gTerrainScrollBuffer[superRow][col] == EMPTY_SUPERTILE)					// see if something is already there
		{
			if ((tileCol >= 0) && (tileCol < gTerrainTileWidth))
			{
				superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 					// make new terrain object
				gTerrainScrollBuffer[superRow][col] = superTileNum;						// save into scroll buffer array
			}
		}
next:
		tileCol += SUPERTILE_SIZE;
	}

check_items:

			/* ADD ITEMS ON BOTTOM */

	bottom = superRow+ITEM_WINDOW;
	left = superCol-ITEM_WINDOW;
	right = superCol+SUPERTILE_DIST_WIDE-1+ITEM_WINDOW;

	if (left < 0)
		left = 0;
	else
	if (left >= gNumSuperTilesWide)
		return;

	if (right < 0)
		return;
	if (right >= gNumSuperTilesWide)
		right = gNumSuperTilesWide-1;

	if (bottom >= gNumSuperTilesDeep)
		return;
	if (bottom < 0)
		return;

	ScanForPlayfieldItems(bottom,bottom,left,right);
}


/********************** SCROLL TERRAIN DOWN *************************/
//
// INPUT: superRow = new supertile row #
//

static void ScrollTerrainDown(long superRow, long superCol)
{
long	col,i,row,top,left,right;
short	superTileNum;
long	tileRow,tileCol;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

			/* PURGE OLD BOTTOM ROW */

	row = gCurrentSuperTileRow+SUPERTILE_DIST_DEEP-1;						// calc supertile row # for bottom row

	if ((row < gNumSuperTilesDeep) && (row >= 0))							// check if off map
	{
		for (i = 0; i < SUPERTILE_DIST_WIDE; i++)
		{
			col = gCurrentSuperTileCol+i;
			if (col >= gNumSuperTilesWide)									// check if off map
				break;
			if (col < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[row][col];					// get supertile for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);	  						// free the terrain object
				gTerrainScrollBuffer[row][col] = EMPTY_SUPERTILE;
			}
		}
	}


				/* CREATE NEW TOP ROW */

	if (superRow < 0) 														// see if off map
		return;
	if (superRow >= gNumSuperTilesDeep)
		return;

	tileCol = gCurrentSuperTileCol * SUPERTILE_SIZE;						// calc col # bot left tile col
	tileRow = superRow * SUPERTILE_SIZE;  									// calc col # bot left tile col

	for (col = gCurrentSuperTileCol; col < (gCurrentSuperTileCol + SUPERTILE_DIST_WIDE); col++)
	{
		if (col >= gNumSuperTilesWide)										// see if off map
			goto check_items;
		if (col < 0)
			goto next;

		if (gTerrainScrollBuffer[superRow][col] == EMPTY_SUPERTILE)					// see if something is already there
		{
			if ((tileCol >= 0) && (tileCol < gTerrainTileWidth))
			{
				superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 				// make new terrain object
				gTerrainScrollBuffer[superRow][col] = superTileNum;					// save into scroll buffer array
			}
		}
next:
		tileCol += SUPERTILE_SIZE;
	}

check_items:

			/* ADD ITEMS ON TOP */

	top = superRow-ITEM_WINDOW;
	left = superCol-ITEM_WINDOW;
	right = superCol+SUPERTILE_DIST_WIDE-1+ITEM_WINDOW;

	if (left < 0)
		left = 0;
	else
	if (left >= gNumSuperTilesWide)
		return;

	if (right < 0)
		return;
	else
	if (right >= gNumSuperTilesWide)
		right = gNumSuperTilesWide-1;

	if (top >= gNumSuperTilesDeep)
		return;
	if (top < 0)
		return;

	ScanForPlayfieldItems(top,top,left,right);

}


/********************** SCROLL TERRAIN LEFT *************************/
//
// Assumes gCurrentSuperTileCol & gCurrentSuperTileRow are in current positions, will do gCurrentSuperTileCol++ at end of routine.
//

static void ScrollTerrainLeft(void)
{
long	row,top,bottom,right;
short	superTileNum;
long 	tileCol,tileRow,newSuperCol;
long	bottomRow;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

	bottomRow = gCurrentSuperTileRow + SUPERTILE_DIST_DEEP;								// calc bottom row (+1)


			/* PURGE OLD LEFT COL */

	if ((gCurrentSuperTileCol < gNumSuperTilesWide) && (gCurrentSuperTileCol >= 0))		// check if on map
	{
		for (row = gCurrentSuperTileRow; row < bottomRow; row++)
		{
			if (row >= gNumSuperTilesDeep)												// check if off map
				break;
			if (row < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[row][gCurrentSuperTileCol]; 			// get supertile for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);									// free the terrain object
				gTerrainScrollBuffer[row][gCurrentSuperTileCol] = EMPTY_SUPERTILE;
			}
		}
	}


		/* CREATE NEW RIGHT COL */

	newSuperCol = gCurrentSuperTileCol+SUPERTILE_DIST_WIDE;	   					// calc col # of right supertile col
	tileCol = newSuperCol * SUPERTILE_SIZE; 		 							// calc col # of right tile col
	tileRow = gCurrentSuperTileRow * SUPERTILE_SIZE;

	if (newSuperCol >= gNumSuperTilesWide)										// see if off map
		goto exit;
	if (newSuperCol < 0)
		goto exit;

	for (row = gCurrentSuperTileRow; row < bottomRow; row++)
	{
		if (row >= gNumSuperTilesDeep)
			break;
		if (row < 0)
			goto next;

		if (gTerrainScrollBuffer[row][newSuperCol] == EMPTY_SUPERTILE)			// make sure nothing already here
		{
			superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 				// make new terrain object
			gTerrainScrollBuffer[row][newSuperCol] = superTileNum;				// save into scroll buffer array
		}
next:
		tileRow += SUPERTILE_SIZE;
	}


			/* ADD ITEMS ON RIGHT */

	top = gCurrentSuperTileRow-ITEM_WINDOW;
	bottom = gCurrentSuperTileRow + (SUPERTILE_DIST_DEEP-1) + ITEM_WINDOW;
	right = newSuperCol+ITEM_WINDOW;

	if (right < 0)
		goto exit;
	if (right >= gNumSuperTilesWide)
		goto exit;

	if (top < 0)
		top = 0;
//		goto exit;
	else
	if (top >= gNumSuperTilesDeep)
		return;

//	if (bottom >= gNumSuperTilesDeep)
//		goto exit;
	if (bottom < 0)
		goto exit;

	ScanForPlayfieldItems(top,bottom,right,right);
	
exit:
    gCurrentSuperTileCol++;	
}


/********************** SCROLL TERRAIN RIGHT *************************/

static void ScrollTerrainRight(long superCol, long superRow, long tileCol, long tileRow)
{
long	col,i,row;
short	superTileNum;
long	top,bottom,left;

	gHiccupEliminator = 0;															// reset this when about to make a new row/col of supertiles

			/* PURGE OLD RIGHT ROW */

	col = gCurrentSuperTileCol+SUPERTILE_DIST_WIDE-1;						// calc supertile col # for right col

	if ((col < gNumSuperTilesWide) && (col >= 0))							// check if off map
	{
		for (i = 0; i < SUPERTILE_DIST_DEEP; i++)
		{
			row = gCurrentSuperTileRow+i;
			if (row >= gNumSuperTilesDeep)									// check if off map
				break;
			if (row < 0)
				continue;

			superTileNum = gTerrainScrollBuffer[row][col];						// get terrain object for that spot
			if (superTileNum != EMPTY_SUPERTILE)
			{
				ReleaseSuperTileObject(superTileNum);		 					// free the terrain object
				gTerrainScrollBuffer[row][col] = EMPTY_SUPERTILE;
			}
		}
	}

		/* CREATE NEW LEFT ROW */

	if (superCol < 0) 														// see if off map
		return;
	if (superCol >= gNumSuperTilesWide)
		return;

	for (row = gCurrentSuperTileRow; row < (gCurrentSuperTileRow + SUPERTILE_DIST_DEEP); row++)
	{
		if (row >= gNumSuperTilesDeep)										// see if off map
			goto check_items;
		if (row < 0)
			goto next;

		if (gTerrainScrollBuffer[row][superCol] == EMPTY_SUPERTILE)						// see if something is already there
		{
			if ((tileRow >= 0) && (tileRow < gTerrainTileDepth))
			{
				superTileNum = BuildTerrainSuperTile(tileCol,tileRow); 				// make new terrain object
				gTerrainScrollBuffer[row][superCol] = superTileNum;					// save into scroll buffer array
			}
		}
next:
		tileRow += SUPERTILE_SIZE;
	}

			/* ADD ITEMS ON LEFT */

check_items:
	top = superRow-ITEM_WINDOW;
	bottom = superRow + (SUPERTILE_DIST_DEEP-1)+ITEM_WINDOW;
	left = superCol-ITEM_WINDOW;

	if (left < 0)
		return;
	if (left >= gNumSuperTilesWide)
		return;

	if (top >= gNumSuperTilesDeep)
		return;
	if (top < 0)
		top = 0;

//	if (bottom >= gNumSuperTilesDeep)
//		bottom = gNumSuperTilesDeep-1;
	if (bottom < 0)
		return;

	ScanForPlayfieldItems(top,bottom,left,left);
}


/**************** PRIME INITIAL TERRAIN ***********************/

void PrimeInitialTerrain(void)
{
long	i,w;


	w = SUPERTILE_DIST_WIDE+ITEM_WINDOW+1;

	gCurrentSuperTileCol -= w;								// start left and scroll into position

	for (i=0; i < w; i++)
	{
		ScrollTerrainLeft();
		CalcNewItemDeleteWindow();							// recalc item delete window
	}	
}


/***************** GET TILE ATTRIBS ******************/
//
// Given a world x/z coord, return the attribs there
// NOTE: does it by calculating the row/col and then calling other routine.
//
// INPUT: x/z = world coords in INTEGER format
//
// OUTPUT: attribs
//

UInt16	GetTileAttribs(long x, long z)
{
long	row,col;

	if ((x < 0) || (z < 0))										// see if out of bounds
		return(0);
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return(0);

	col = (float)x*gOneOver_TERRAIN_POLYGON_SIZE;	 							// calc map row/col that the coord lies on
	row = (float)z*gOneOver_TERRAIN_POLYGON_SIZE;

	return(GetTileAttribsAtRowCol(row,col));
 }



/******************** GET TILE ATTRIBS AT ROW COL *************************/
//
// OUTPUT: 	attrib bits
//			gTileAttribParm0..2 = tile attribs
//  		gTileFlipRotBits = flip/rot bits of tile
//

UInt16	GetTileAttribsAtRowCol(short row, short col)
{
UInt16	tile,texMapNum,attribBits;

	tile = gTerrainTextureLayer[row][col];						// get tile data from map
	texMapNum = tile&TILENUM_MASK; 										// filter out texture #
	gTileFlipRotBits = tile&(TILE_FLIPXY_MASK|TILE_ROTATE_MASK);		// get flip & rotate bits

	attribBits = gTileAttributes[texMapNum].bits;				// get attribute bits
	gTileAttribParm0 = gTileAttributes[texMapNum].parm0;		// and parameters
	gTileAttribParm1 = gTileAttributes[texMapNum].parm1;
	gTileAttribParm2 = gTileAttributes[texMapNum].parm2;


	return(attribBits);
}



/******************** GET TILE COLLISION BITS AT ROW COL *************************/
//
// Reads the tile on the *** PATH *** layer and converts it into a top/bottom/left/right bit field.
//
// INPUT:	checkAlt = also check seconday collision tiles (for enemies usually)
//

UInt16	GetTileCollisionBitsAtRowCol(short row, short col, Boolean checkAlt)
{
UInt16	tile;

	tile = gTerrainPathLayer[row][col];						// get path data from map
	tile = tile&TILENUM_MASK;							   		// filter out tile # 
	if (tile == 0)
		return(0);

			/* CHECK PRIMARY COLLISION TILES */
			
	switch(tile)
	{
	 	case	PATH_TILE_SOLID_ALL:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOP:
				return(SIDE_BITS_TOP);

		case	PATH_TILE_SOLID_RIGHT:
				return(SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOM:
				return(SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFT:
				return(SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPBOTTOM:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFTRIGHT:
				return(SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHT:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMRIGHT:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMLEFT:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFT:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFTRIGHT:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHTBOTTOM:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_BOTTOMLEFTRIGHT:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPBOTTOMLEFT:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_BOTTOM);
	}
	
	
				/* CHECK SECONDARY */
	if (checkAlt)
	{
		switch(tile)
		{
		 	case	PATH_TILE_SOLID_ALL2:
					return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOP2:
					return(SIDE_BITS_TOP);
	
			case	PATH_TILE_SOLID_RIGHT2:
					return(SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_BOTTOM2:
					return(SIDE_BITS_BOTTOM);
	
			case	PATH_TILE_SOLID_LEFT2:
					return(SIDE_BITS_LEFT);
	
			case	PATH_TILE_SOLID_TOPBOTTOM2:
					return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM);
	
			case	PATH_TILE_SOLID_LEFTRIGHT2:
					return(SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOPRIGHT2:
					return(SIDE_BITS_TOP|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_BOTTOMRIGHT2:
					return(SIDE_BITS_BOTTOM|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_BOTTOMLEFT2:
					return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT);
	
			case	PATH_TILE_SOLID_TOPLEFT2:
					return(SIDE_BITS_TOP|SIDE_BITS_LEFT);
	
			case	PATH_TILE_SOLID_TOPLEFTRIGHT2:
					return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOPRIGHTBOTTOM2:
					return(SIDE_BITS_TOP|SIDE_BITS_RIGHT|SIDE_BITS_BOTTOM);
	
			case	PATH_TILE_SOLID_BOTTOMLEFTRIGHT2:
					return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);
	
			case	PATH_TILE_SOLID_TOPBOTTOMLEFT2:
					return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_BOTTOM);
		}
	}

	return(0);
}


/******************** GET TILE COLLISION BITS AT ROW COL 2 *************************/
//
// Version #2 here only checks the secondary collision tiles.
//

UInt16	GetTileCollisionBitsAtRowCol2(short row, short col)
{
UInt16	tile;

	tile = gTerrainPathLayer[row][col];						// get path data from map
	tile = tile&TILENUM_MASK; 							  		// filter out tile #
	if (tile == 0)
		return(0);

	switch(tile)
	{
	 	case	PATH_TILE_SOLID_ALL2:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOP2:
				return(SIDE_BITS_TOP);

		case	PATH_TILE_SOLID_RIGHT2:
				return(SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOM2:
				return(SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFT2:
				return(SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPBOTTOM2:
				return(SIDE_BITS_TOP|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_LEFTRIGHT2:
				return(SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHT2:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMRIGHT2:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_BOTTOMLEFT2:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFT2:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT);

		case	PATH_TILE_SOLID_TOPLEFTRIGHT2:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPRIGHTBOTTOM2:
				return(SIDE_BITS_TOP|SIDE_BITS_RIGHT|SIDE_BITS_BOTTOM);

		case	PATH_TILE_SOLID_BOTTOMLEFTRIGHT2:
				return(SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT);

		case	PATH_TILE_SOLID_TOPBOTTOMLEFT2:
				return(SIDE_BITS_TOP|SIDE_BITS_LEFT|SIDE_BITS_BOTTOM);
	}
	return(0);
}




