//
// affectors.h
//

enum
{
	AFFECTORTYPE_ZIGZAG,
	AFFECTORTYPE_TWIRLY
};


#define	AffectorType	Flag[5]

#define AFFECTOR_COLLISION_RANGE	4

//=================================================


extern	Boolean HandleAffector(ObjNode *affectorNode, ObjNode *affecteeNode);
extern	void AddZigZag(ItemEntryType *itemPtr);
