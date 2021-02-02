//
// collision.h
//

enum
{
	COLLISION_TYPE_OBJ,						// box
	COLLISION_TYPE_TRIANGLE,				// poly
	COLLISION_TYPE_TILE
};
							



					/* COLLISION STRUCTURES */
struct CollisionRec
{
	Byte			baseBox,targetBox;
	unsigned short	sides;
	Byte			type;
	ObjNode			*objectPtr;			// object that collides with (if object type)
	float			planeIntersectY;	// where intersected triangle
};
typedef struct CollisionRec CollisionRec;



//=================================


extern	void CollisionDetect(ObjNode *baseNode,unsigned long CType);
extern	Byte HandleCollisions(ObjNode *theNode, unsigned long	cType);
extern	ObjNode *IsPointInPickupCollisionSphere(TQ3Point3D *thePt);
extern	Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
extern	short DoSimplePointCollision(TQ3Point3D *thePoint, UInt32 cType);
extern	void DisposeCollisionTriangleMemory(ObjNode *theNode);
extern	void CreateCollisionTrianglesForObject(ObjNode *theNode);
extern	void DoTriangleCollision(ObjNode *theNode, unsigned long CType);



