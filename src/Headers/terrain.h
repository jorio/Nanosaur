//
// Terrain.h
//

#ifndef TERRAIN_H
#define TERRAIN_H

#include "qd3d_support.h"

#define	MAP_ITEM_MYSTARTCOORD		0				// map item # for my start coords
#define	MAP_ITEM_TIMEPORTAL			9				// map item # for TIME PORTAL

#if SOURCE_PORT_ENHANCEMENTS
#define TERRAIN_160X160_TEXTURES	true			// source port addition - use 160x160 textures as stored in terrain files instead of shrinking to 128x128 or 64x64
#endif

extern	const float	gOneOver_TERRAIN_POLYGON_SIZE;

		/* PATH TILES */

enum
{
	PATH_TILE_PATHUP = 1,
	PATH_TILE_PATHUPRIGHT,
	PATH_TILE_PATHRIGHT,
	PATH_TILE_PATHDOWNRIGHT,
	PATH_TILE_PATHDOWN,
	PATH_TILE_PATHDOWNLEFT,
	PATH_TILE_PATHLEFT,
	PATH_TILE_PATHUPLEFT,
	PATH_TILE_SOLID_ALL,
	PATH_TILE_SOLID_TOP,
	PATH_TILE_SOLID_RIGHT,
	PATH_TILE_SOLID_BOTTOM,
	PATH_TILE_SOLID_LEFT,
	PATH_TILE_SOLID_TOPBOTTOM,
	PATH_TILE_SOLID_LEFTRIGHT,
	PATH_TILE_SOLID_TOPRIGHT,
	PATH_TILE_SOLID_BOTTOMRIGHT,
	PATH_TILE_SOLID_BOTTOMLEFT,
	PATH_TILE_SOLID_TOPLEFT,
	PATH_TILE_SOLID_TOPLEFTRIGHT,
	PATH_TILE_SOLID_TOPRIGHTBOTTOM,
	PATH_TILE_SOLID_BOTTOMLEFTRIGHT,
	PATH_TILE_SOLID_TOPBOTTOMLEFT,

	PATH_TILE_SOLID_ALL2,
	PATH_TILE_SOLID_TOP2,
	PATH_TILE_SOLID_RIGHT2,
	PATH_TILE_SOLID_BOTTOM2,
	PATH_TILE_SOLID_LEFT2,
	PATH_TILE_SOLID_TOPBOTTOM2,
	PATH_TILE_SOLID_LEFTRIGHT2,
	PATH_TILE_SOLID_TOPRIGHT2,
	PATH_TILE_SOLID_BOTTOMRIGHT2,
	PATH_TILE_SOLID_BOTTOMLEFT2,
	PATH_TILE_SOLID_TOPLEFT2,
	PATH_TILE_SOLID_TOPLEFTRIGHT2,
	PATH_TILE_SOLID_TOPRIGHTBOTTOM2,
	PATH_TILE_SOLID_BOTTOMLEFTRIGHT2,
	PATH_TILE_SOLID_TOPBOTTOMLEFT2,

	PATH_TILE_PATHREVERSE,
	PATH_TILE_HURT,
	PATH_TILE_FOLD_A,
	PATH_TILE_FOLD_B,
	PATH_TILE_SLOPEFORCE

};

		/* SUPER TILE MODES */

enum
{
	SUPERTILE_MODE_FREE,
	SUPERTILE_MODE_USED
};

#if TWO_MEG_VERSION
#define	SUPERTILE_TEXMAP_SIZE	64						// the width & height of a supertile's texture
#elif TERRAIN_160X160_TEXTURES 
#define SUPERTILE_TEXMAP_SIZE	160
#else
#define	SUPERTILE_TEXMAP_SIZE	128						// the width & height of a supertile's texture
#endif

#define	OREOMAP_TILE_SIZE		32 						// pixel w/h of texture tile
#define TERRAIN_HMTILE_SIZE		32						// pixel w/h of heightmap tile
#define	TERRAIN_POLYGON_SIZE	140						// size in world units of terrain polygon

#define	TERRAIN_POLYGON_SIZE_Frac	((float)1.0/(float)TERRAIN_POLYGON_SIZE)

#define	SUPERTILE_SIZE			5  						// size of a super-tile / terrain object zone

#if SOURCE_PORT_ENHANCEMENTS
#define SUPERTILE_OVERLAP		0.0
#else
#define SUPERTILE_OVERLAP		1.0
#endif

#define	TEMP_TEXTURE_BUFF_SIZE	(OREOMAP_TILE_SIZE * SUPERTILE_SIZE)

#define	MAP2UNIT_VALUE			((float)TERRAIN_POLYGON_SIZE/OREOMAP_TILE_SIZE)	//value to xlate Oreo map pixel coords to 3-space unit coords

#define	TERRAIN_SUPERTILE_UNIT_SIZE	(SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE)	// world unit size of a supertile

#ifdef PRO_MODE
#define	SUPERTILE_ACTIVE_RANGE		4 						// distance to watch supertiles
#else
#define	SUPERTILE_ACTIVE_RANGE		3 						// distance to watch supertiles
#endif

#define	SUPERTILE_DIST_WIDE		(SUPERTILE_ACTIVE_RANGE*2+1)
#define	SUPERTILE_DIST_DEEP		(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ACTIVE_RANGE)
#define	MAX_SUPERTILES			(SUPERTILE_DIST_WIDE*SUPERTILE_DIST_DEEP)

// Source port addition: we allow more active supertiles than the original game
// by using shorts instead of bytes for supertile indices.
#if MAX_SUPERTILES>32767
	#error "Active supertile range too large"
#endif

#define	EMPTY_SUPERTILE		-1

#define	NUM_POLYS_IN_SUPERTILE	(SUPERTILE_SIZE * SUPERTILE_SIZE * 2)					// 2 triangles per tile

#define	MAX_TILE_ANIMS			32
#define	MAX_TERRAIN_TILES		((300*3)+1)							// 10x15 * 3pages + 1 blank/black

#define	MAX_TERRAIN_WIDTH		400
#define	MAX_TERRAIN_DEPTH		400

#define	MAX_SUPERTILES_WIDE		(MAX_TERRAIN_WIDTH/SUPERTILE_SIZE)
#define	MAX_SUPERTILES_DEEP		(MAX_TERRAIN_DEPTH/SUPERTILE_SIZE)

#define	NUM_VERTICES_IN_SUPERTILE	((SUPERTILE_SIZE+1)*(SUPERTILE_SIZE+1))				// # vertices in a supertile

#define	MAX_HEIGHTMAP_TILES		300						// 15x20 tiles per page


//=====================================================================


struct SuperTileMemoryType
{
	Byte				mode;									// free, used, etc.
	Byte				hiccupTimer;							// timer to delay drawing to avoid hiccup of texture upload
	UInt16				row,col;								// supertile's map position
	TQ3Point3D			coord;									// world coords
	long				left,back;								// integer coords of back/left corner
	TQ3GeometryObject	triMesh;								// trimeshes for the supertile
	TQ3Point3D			vectorList[NUM_VERTICES_IN_SUPERTILE];
	Byte				splitAngle[SUPERTILE_SIZE][SUPERTILE_SIZE];		// 0 = /  1 = \ .
	TQ3PlaneEquation	tilePlanes1[SUPERTILE_SIZE][SUPERTILE_SIZE];		// plane equation for each tile poly A
	TQ3PlaneEquation	tilePlanes2[SUPERTILE_SIZE][SUPERTILE_SIZE];		// plane equation for each tile poly B
	
	float				radius;									// radius of this supertile
	
	TQ3GeometryObject	triMesh2;								// for 4  point flat supertiles
	Boolean				isFlat;
};
typedef struct SuperTileMemoryType SuperTileMemoryType;


typedef struct TileAttribType
{
	UInt16	bits;
	short	parm0;
	Byte	parm1,parm2;
	short	undefined;
}TileAttribType;

enum
{
	TILE_ATTRIB_MAKEDUST	=	1<<4,
	TILE_ATTRIB_LAVA		=	1<<5,
	TILE_ATTRIB_WATER		=	1<<7
};

#define	TILE_ATTRIB_ALLSOLID	(TILE_ATTRIB_TOPSOLID+TILE_ATTRIB_BOTTOMSOLID+TILE_ATTRIB_LEFTSOLID+TILE_ATTRIB_RIGHTSOLID)

#define	TILENUM_MASK		0x0fff					// b0000111111111111 = mask to filter out tile #
#define	TILE_FLIPX_MASK		(1<<15)
#define	TILE_FLIPY_MASK		(1<<14)
#define	TILE_FLIPXY_MASK 	(TILE_FLIPY_MASK|TILE_FLIPX_MASK)
#define	TILE_ROTATE_MASK	((1<<13)|(1<<12))
#define	TILE_ROT1			(1<<12)
#define	TILE_ROT2			(2<<12)
#define	TILE_ROT3			(3<<12)

#define	HEIGHT_EXTRUDE_FACTOR 4.0f


		/* TERRAIN ITEM FLAGS */

enum
{
	ITEM_FLAGS_INUSE	=	(1)
};


//=====================================================================




extern 	void DisposeTerrain(void);
extern	void DrawTerrain(QD3DSetupOutputType *setupInfo);
extern 	float	GetTerrainHeightAtCoord(float x, float z);
extern	float	GetTerrainHeightAtCoord_Quick(long x, long z);
extern 	UInt16	GetTileAttribsAtRowCol(short row, short col);
extern	UInt16	GetTileCollisionBitsAtRowCol(short row, short col, Boolean checkAlt);
extern 	UInt16	GetTileCollisionBitsAtRowCol2(short row, short col);
extern	UInt16	GetTileAttribs(long x, long z);
extern	void GetSuperTileInfo(long x, long z, long *superCol, long *superRow, long *tileCol, long *tileRow);
extern	void InitTerrainManager(void);
extern	void ClearScrollBuffer(void);
extern	float	GetTerrainHeightAtCoord_Planar(float x, float z);



extern 	void BuildTerrainItemList(void);
extern 	void ScanForPlayfieldItems(long top, long bottom, long left, long right);
extern 	Boolean TrackTerrainItem(ObjNode *theNode);
extern 	Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z);
extern	void PrimeInitialTerrain(void);
extern 	void FindMyStartCoordItem(void);
extern 	Boolean TrackTerrainItem_Far(ObjNode *theNode, long range);
extern 	UInt16	GetPathTileNum(float x, float z);
extern 	void MakeBackupOfItemList(void);
extern	UInt16	GetPathTileNumAtRowCol(long row, long col);
extern	void RotateOnTerrain(ObjNode *theNode, float sideOff, float endOff);
extern	void DoMyTerrainUpdate(void);




#endif






