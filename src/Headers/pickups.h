//
// pickups.h
//

#define	PickUpCollisionRadius	SpecialF[5]

extern	Boolean AddEgg(TerrainItemEntryType *itemPtr, long  x, long z);
extern	void TryToDoPickUp(ObjNode *theNode, Byte limbNum);
extern	void DropItem(ObjNode *theNode);
extern	void GetAllEggsCheat(void);

