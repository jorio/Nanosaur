//
// skeletonJoints.h
//

enum
{
	SKELETON_LIMBTYPE_PILLOW,
	SKELETON_LIMBTYPE_LEFTHAND,
	SKELETON_LIMBTYPE_HEAD
};



#define	NO_LIMB_MESH		(-1)			// indicates that this joint's limb mesh does not exist
#define	NO_PREVIOUS_JOINT	(-1)

//===========================================

void UpdateJointTransforms(SkeletonObjDataType *skeleton, int jointNum);
void FindCoordOfJoint(ObjNode *theNode, int jointNum, TQ3Point3D *outPoint);
void FindCoordOnJoint(ObjNode *theNode, int jointNum, const TQ3Point3D *inPoint, TQ3Point3D *outPoint);
void FindJointFullMatrix(ObjNode *theNode, int jointNum, TQ3Matrix4x4 *outMatrix);

