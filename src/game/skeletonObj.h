//
// SkeletonObj.h
//

#define 	MAX_SKELETON_TYPES	6

enum
{
	SKELETON_TYPE_PTERA,
	SKELETON_TYPE_REX,
	SKELETON_TYPE_STEGO,
	SKELETON_TYPE_DEINON,
	SKELETON_TYPE_TRICER,
	SKELETON_TYPE_SPITTER
				// NOTE: Check MAX_SKELETON_TYPES above
};




//===============================

extern	ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef);
extern	void DisposeSkeletonObjectMemory(SkeletonDefType *skeleton);
extern	void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton);
extern	void InitSkeletonManager(void);
extern	void LoadASkeleton(Byte num);
extern	void FreeSkeletonFile(Byte skeletonType);
extern	void FreeAllSkeletonFiles(short skipMe);
extern	void FreeSkeletonBaseData(SkeletonObjDataType *data);


