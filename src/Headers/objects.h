//
// Object.h
//
#include "qd3d_support.h"

#define INVALID_NODE_FLAG	0xdeadbeef			// put into CType when node is deleted


#define	PLAYER_SLOT		500
#define	SLOT_OF_DUMB	3000
#define CAMERA_SLOT		(SLOT_OF_DUMB+1)
#define	INFOBAR_SLOT	(CAMERA_SLOT+1)

enum
{
	SKELETON_GENRE,
	DISPLAY_GROUP_GENRE,
	EVENT_GENRE
};


//========================================================

extern	void InitObjectManager(void);
extern	ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef);
extern	void MoveObjects(void);
extern	void DrawObjects(QD3DSetupOutputType *setupInfo);
extern	void DeleteAllObjects(void);
extern	void DeleteObject(ObjNode	*theNode);
extern	void GetObjectInfo(ObjNode *theNode);
extern	void UpdateObject(ObjNode *theNode);
extern	ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef);
void AttachGeometryToDisplayGroupObject(ObjNode* theNode, int numMeshes, TQ3TriMeshData** meshList);
extern	void CreateBaseGroup(ObjNode *theNode);
extern	void UpdateObjectTransforms(ObjNode *theNode);
extern	void MakeObjectKeepBackfaces(ObjNode *theNode);
extern	void DisposeObjectBaseGroup(ObjNode *theNode);
extern	void MakeObjectTransparent(ObjNode *theNode, float transPercent);

extern	void MoveStaticObject(ObjNode *theNode);

extern	void CalcNewTargetOffsets(ObjNode *theNode, float scale);

//===================


extern	void AllocateCollisionBoxMemory(ObjNode *theNode, short numBoxes);
extern	void CalcObjectBoxFromNode(ObjNode *theNode);
extern	void CalcObjectBoxFromGlobal(ObjNode *theNode);
extern	void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back);
extern	void UpdateShadow(ObjNode *theNode);
extern	void CheckAllObjectsInConeOfVision(void);
extern	ObjNode	*AttachShadowToObject(ObjNode *theNode, float scaleX, float scaleZ);
extern	void StartObjectStreamEffect(ObjNode *theNode, short effectNum);
extern	void StopObjectStreamEffect(ObjNode *theNode);
extern	void KeepOldCollisionBoxes(ObjNode *theNode);







