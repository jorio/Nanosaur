//
// triggers.h
//


#define		TriggerSides	Special[5]
#define		TriggerType		Flag[5]

enum
{
	TRIGTYPE_POWERUP,
	TRIGTYPE_CRYSTAL,
	TRIGTYPE_STEPSTONE
};



extern	Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side);
extern	Boolean AddPowerUp(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddCrystal(TerrainItemEntryType *itemPtr, long  x, long z);
extern	Boolean AddStepStone(TerrainItemEntryType *itemPtr, long  x, long z);
extern	void ExplodeCrystal(ObjNode *theNode);

