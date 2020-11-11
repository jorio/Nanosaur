//
// 3DMath.h
//



extern	float CalcXAngleFromPointToPoint(float fromY, float fromZ, float toY, float toZ);
extern	float	CalcYAngleFromPointToPoint(float fromX, float fromZ, float toX, float toZ);
extern	float TurnObjectTowardTarget(ObjNode *theNode, float x, float z, float turnSpeed, Boolean useOffsets);
extern	float CalcQuickDistance(float x1, float y1, float x2, float y2);
extern	float	CalcYAngleBetweenVectors(TQ3Vector3D *v1, TQ3Vector3D *v2);
extern	float	CalcAngleBetweenVectors2D(TQ3Vector2D *v1, TQ3Vector2D *v2);
extern	float	CalcAngleBetweenVectors3D(TQ3Vector3D *v1, TQ3Vector3D *v2);
extern	void CalcPointOnObject(ObjNode *theNode, TQ3Point3D *inPt, TQ3Point3D *outPt);
extern	void CalcFaceNormal(TQ3Point3D *p1, TQ3Point3D *p2, TQ3Point3D *p3, TQ3Vector3D *normal);
extern	void SetQuickRotationMatrix_XYZ(TQ3Matrix4x4 *m, float rx, float ry, float rz);
extern	void CalcPlaneEquationOfTriangle(TQ3PlaneEquation *plane, TQ3Point3D *, TQ3Point3D *, TQ3Point3D *);
extern	Boolean IntersectionOfLineSegAndPlane(TQ3PlaneEquation *plane, float v1x, float v1y, float v1z,
								 float v2x, float v2y, float v2z, TQ3Point3D *outPoint);

extern	Boolean VectorsAreCloseEnough(TQ3Vector3D *v1, TQ3Vector3D *v2);
extern	Boolean PointsAreCloseEnough(TQ3Point3D *v1, TQ3Point3D *v2);
extern	void FastNormalizeVector(float vx, float vy, float vz, TQ3Vector3D *outV);
extern	float IntersectionOfYAndPlane_Func(float x, float z, TQ3PlaneEquation *p);


/*********** INTERSECTION OF Y AND PLANE ********************/
//
// INPUT:	
//			x/z		:	xz coords of point
//			p		:	ptr to the plane
//
//
// *** IMPORTANT-->  This function does not check for divides by 0!! As such, there should be no
//					"vertical" polygons (polys with normal->y == 0).
//

#define IntersectionOfYAndPlane(_x, _z, _p)	(((_p)->constant - (((_p)->normal.x * _x) + ((_p)->normal.z * _z))) / (_p)->normal.y)


