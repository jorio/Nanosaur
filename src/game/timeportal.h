//
// timeportal.h
//


enum
{
	PORTAL_TYPE_EGG,
	PORTAL_TYPE_ENTER,
	PORTAL_TYPE_EXIT
};

extern	void InitTimePortals(void);
extern	ObjNode *MakeTimePortal(Byte portalType, float x, float z);
extern	Boolean AddTimePortal(TerrainItemEntryType *itemPtr, long  x, long z);
extern	short	FindClosestPortal(void);
