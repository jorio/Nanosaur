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
	uint16_t		sides;
	Byte			type;
	ObjNode			*objectPtr;			// object that collides with (if object type)
	float			planeIntersectY;	// where intersected triangle
};
typedef struct CollisionRec CollisionRec;



//=================================


void CollisionDetect(ObjNode *baseNode, uint32_t cType);
Byte HandleCollisions(ObjNode *theNode, uint32_t cType);
ObjNode *IsPointInPickupCollisionSphere(TQ3Point3D *thePt);
Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
int DoSimplePointCollision(TQ3Point3D *thePoint, uint32_t cType);
void DisposeCollisionTriangleMemory(ObjNode *theNode);
void CreateCollisionTrianglesForObject(ObjNode *theNode);
void DoTriangleCollision(ObjNode *theNode, uint32_t cType);
