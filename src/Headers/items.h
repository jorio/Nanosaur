//
// items.h
//

#define	LAVA_Y_OFFSET	50
#define	FIXED_LAVA_Y	305

#define	WATER_Y_OFFSET	50
#define	FIXED_WATER_Y	210


extern	Boolean AddLavaPatch(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddTree(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddBoulder(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddMushroom(TerrainItemEntryType *itemPtr, long  x, long z);
extern	void InitItemsManager(void);
extern	void UpdateLavaTextureAnimation(void);
extern	Boolean AddBush(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddNest(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddGasVent(TerrainItemEntryType *itemPtr, long  x, long z);
extern	void ExplodeBush(ObjNode *theBush);
extern	void UpdateWaterTextureAnimation(void);
extern	Boolean AddWaterPatch(TerrainItemEntryType *itemPtr, long  x, long z);

