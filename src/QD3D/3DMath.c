/****************************/
/*   	3D MATH.C		    */
/* (c)1996 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static inline float MaskAngle(float angle);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


/********************* CALC X ANGLE FROM POINT TO POINT **********************/

float CalcXAngleFromPointToPoint(float fromY, float fromZ, float toY, float toZ)
{
float	xangle,zdiff;

	zdiff = toZ-fromZ;									// calc diff in Z
	zdiff = fabs(zdiff);								// get abs value

	xangle = atan2(-zdiff,toY-fromY) + (PI/2.0f);
	
			/* KEEP BETWEEN 0 & 2*PI */
			
	return(MaskAngle(xangle));
}


/********************* CALC Y ANGLE FROM POINT TO POINT **********************/
//
// Returns an unsigned result from 0 -> 6.28
//

float	CalcYAngleFromPointToPoint(float fromX, float fromZ, float toX, float toZ)
{
float	yangle;

	yangle = PI2 - MaskAngle(atan2(fromZ-toZ,fromX-toX) - (PI/2.0f));

	return(yangle);
}


/**************** CALC Y ANGLE BETWEEN VECTORS ******************/
//
// This simply returns the angle BETWEEN 2 3D vectors, NOT the angle FROM one point to
// another.
//

float	CalcYAngleBetweenVectors(TQ3Vector3D *v1, TQ3Vector3D *v2)
{
float	dot,angle;

	v1->y = v2->y = 0;

	Q3Vector3D_Normalize(v1,v1);	// make sure they're normalized
	Q3Vector3D_Normalize(v2,v2);

	dot = Q3Vector3D_Dot(v1,v2);	// the dot product is the cosine of the angle between the 2 vectors
	angle = acos(dot);				// get arc-cosine to get the angle out of it
	return(angle);
}


/**************** CALC ANGLE BETWEEN VECTORS 2D ******************/

float	CalcAngleBetweenVectors2D(TQ3Vector2D *v1, TQ3Vector2D *v2)
{
float	dot,angle;

	Q3Vector2D_Normalize(v1,v1);	// make sure they're normalized
	Q3Vector2D_Normalize(v2,v2);

	dot = Q3Vector2D_Dot(v1,v2);	// the dot product is the cosine of the angle between the 2 vectors
	angle = acos(dot);				// get arc-cosine to get the angle out of it
	return(angle);
}


/**************** CALC ANGLE BETWEEN VECTORS 3D ******************/

float	CalcAngleBetweenVectors3D(TQ3Vector3D *v1, TQ3Vector3D *v2)
{
float	dot,angle;

	Q3Vector3D_Normalize(v1,v1);	// make sure they're normalized
	Q3Vector3D_Normalize(v2,v2);

	dot = Q3Vector3D_Dot(v1,v2);	// the dot product is the cosine of the angle between the 2 vectors
	angle = acos(dot);				// get arc-cosine to get the angle out of it
	return(angle);
}




/******************** MASK ANGLE ****************************/
//
// Given an arbitrary angle, it limits it to between 0 and 2*PI
//

inline float MaskAngle(float angle)
{
	bool neg = angle < 0;

	int n = (int)(angle * (1.0f/ PI2));		// see how many times it wraps fully
	angle -= (float)n * PI2;				// subtract # wrappings to get just the remainder

	if (neg)
		angle += PI2;

	return angle;
}


/************ TURN OBJECT TOWARD TARGET ****************/
//
// INPUT:	theNode = object
//			x/z = target coords to aim towards
//			turnSpeed = turn speed, 0 == just return angle between
//
// OUTPUT:  remaining angle to target rot
//

float TurnObjectTowardTarget(ObjNode *theNode, float x, float z, float turnSpeed, Boolean useOffsets)
{
float	desiredRotY,diff;
float	currentRotY;
float	adjustedTurnSpeed;

	if (useOffsets)
	{
		x += theNode->TargetOff.x;										// offset coord
		z += theNode->TargetOff.y;
	}

	desiredRotY = CalcYAngleFromPointToPoint(theNode->Coord.x, theNode->Coord.z, x, z);		// calc angle directly at target
	currentRotY = theNode->Rot.y;															// get current angle

	if (turnSpeed != 0.0f)
	{
		adjustedTurnSpeed = turnSpeed * gFramesPerSecondFrac;		// calc actual turn speed


		diff = desiredRotY - currentRotY;							// calc diff from current to desired
		if (diff > adjustedTurnSpeed)								// (+) rotation
		{
			if (diff > PI)											// see if shorter to go backward
			{
			   currentRotY -= adjustedTurnSpeed;
			}
			else
			{
			   	currentRotY += adjustedTurnSpeed;
			}
		}
		else
		if (diff < -adjustedTurnSpeed)
		{
			if (diff < -PI)											// see if shorter to go forward
			{
			   currentRotY += adjustedTurnSpeed;
			}
			else
			{
			   currentRotY -= adjustedTurnSpeed;
			}
		}
		else
		{
			currentRotY = desiredRotY;
		}
		currentRotY = MaskAngle(currentRotY);						// keep from looping too bad

		theNode->Rot.y = currentRotY;								// update node
	}

	return(fabs(desiredRotY - currentRotY));
}



/************* CALC QUICK DISTANCE ****************/
//
// Does cheezeball quick distance calculation on 2 2D points.
//

float CalcQuickDistance(float x1, float y1, float x2, float y2)
{
float	diffX,diffY;

	diffX = __fabs(x1-x2);
	diffY = __fabs(y1-y2);

	if (diffX > diffY)
	{
		return(diffX + (0.375f*diffY));			// same as (3*diffY)/8
	}
	else
	{
		return(diffY + (0.375f*diffX));
	}

}




/******************** CALC POINT ON OBJECT *************************/
//
// Uses the input ObjNode's BaseTransformMatrix to transform the input point.
//

void CalcPointOnObject(ObjNode *theNode, TQ3Point3D *inPt, TQ3Point3D *outPt)
{
	Q3Point3D_Transform(inPt, &theNode->BaseTransformMatrix, outPt);
}

/***************** CALC FACE NORMAL *********************/
//
// Returns the normal vector off the face defined by 3 points.
//

void CalcFaceNormal(TQ3Point3D *p1, TQ3Point3D *p2, TQ3Point3D *p3, TQ3Vector3D *normal)
{
TQ3Vector3D	v1,v2;

	v1.x = (p1->x - p3->x);
	v1.y = (p1->y - p3->y);
	v1.z = (p1->z - p3->z);
	v2.x = (p2->x - p3->x);
	v2.y = (p2->y - p3->y);
	v2.z = (p2->z - p3->z);

	Q3Vector3D_Cross(&v1, &v2, normal);				// cross product == vector perpendicular to 2 other vectors
	Q3Vector3D_Normalize(normal,normal);
}


/******************* SET QUICK XYZ-ROTATION MATRIX ************************/
//
// Does a quick precomputation to calculate an XYZ rotation matrix
//
//
// cy*cz					cy*sz					-sy			0
// (sx*sy*cz)+(cx*-sz)		(sx*sy*sz)+(cx*cz)		sx*cy		0
// (cx*sy*cz)+(-sx*-sz)		(cx*sy*sz)+(-sx*cz)		cx*cy		0
// 0						0						0			1
//

void SetQuickRotationMatrix_XYZ(TQ3Matrix4x4 *m, float rx, float ry, float rz)
{
float	sx,cx,sy,sz,cy,cz,sxsy,cxsy;

	sx = sin(rx);
	sy = sin(ry);
	sz = sin(rz);
	cx = cos(rx);
	cy = cos(ry);
	cz = cos(rz);
	
	sxsy = sx*sy;
	cxsy = cx*sy;
	
	m->value[0][0] = cy*cz;					m->value[0][1] = cy*sz; 				m->value[0][2] = -sy; 	m->value[0][3] = 0;
	m->value[1][0] = (sxsy*cz)+(cx*-sz);	m->value[1][1] = (sxsy*sz)+(cx*cz);		m->value[1][2] = sx*cy;	m->value[1][3] = 0;
	m->value[2][0] = (cxsy*cz)+(-sx*-sz);	m->value[2][1] = (cxsy*sz)+(-sx*cz);	m->value[2][2] = cx*cy;	m->value[2][3] = 0;
	m->value[3][0] = 0;						m->value[3][1] = 0;						m->value[3][2] = 0;		m->value[3][3] = 1;
}



/******************* CALC PLANE EQUATION OF TRIANGLE ********************/
//
// input points should be clockwise!
//

void CalcPlaneEquationOfTriangle(TQ3PlaneEquation *plane, TQ3Point3D *p3, TQ3Point3D *p2, TQ3Point3D *p1)
{
float	pq_x,pq_y,pq_z;
float	pr_x,pr_y,pr_z;
float	p1x,p1y,p1z;

	p1x = p1->x;									// get point #1
	p1y = p1->y;
	p1z = p1->z;

	pq_x = p1x - p2->x;								// calc vector pq
	pq_y = p1y - p2->y;
	pq_z = p1z - p2->z;

	pr_x = p1->x - p3->x;							// calc vector pr
	pr_y = p1->y - p3->y;
	pr_z = p1->z - p3->z;


			/* CALC CROSS PRODUCT FOR THE FACE'S NORMAL */
			
	plane->normal.x = (pq_y * pr_z) - (pq_z * pr_y);				// cross product of edge vectors = face normal
	plane->normal.y = ((pq_z * pr_x) - (pq_x * pr_z));
	plane->normal.z = (pq_x * pr_y) - (pq_y * pr_x);

	FastNormalizeVector(plane->normal.x,plane->normal.y, plane->normal.z, &plane->normal);
	

		/* CALC DOT PRODUCT FOR PLANE CONSTANT */

	plane->constant = 	((plane->normal.x * p1x) +
						(plane->normal.y * p1y) +
						(plane->normal.z * p1z));
}




/************************** VECTORS ARE CLOSE ENOUGH ****************************/

Boolean VectorsAreCloseEnough(TQ3Vector3D *v1, TQ3Vector3D *v2)
{
	if (fabs(v1->x - v2->x) < 0.02f)			// WARNING: If change, must change in BioOreoPro also!!
		if (fabs(v1->y - v2->y) < 0.02f)
			if (fabs(v1->z - v2->z) < 0.02f)
				return(true);

	return(false);
}

/************************** POINTS ARE CLOSE ENOUGH ****************************/

Boolean PointsAreCloseEnough(TQ3Point3D *v1, TQ3Point3D *v2)
{
	if (__fabs(v1->x - v2->x) < 0.001f)		// WARNING: If change, must change in BioOreoPro also!!
		if (__fabs(v1->y - v2->y) < 0.001f)
			if (__fabs(v1->z - v2->z) < 0.001f)
				return(true);

	return(false);
}



/******************** FAST NORMALIZE VECTOR ***********************/
//
// My special version here will use the frsqrte opcode if available.
// It does Newton-Raphson refinement to get a good result.
//

void FastNormalizeVector(float vx, float vy, float vz, TQ3Vector3D *outV)
{
#if 1
	TQ3Vector3D input = { vx, vy, vz };
	Q3Vector3D_Normalize(&input, outV);
#else
float	temp;
float	isqrt, temp1, temp2;		
	
	temp = vx * vx;
	temp += vy * vy;
	temp += vz * vz;
	
	isqrt = __frsqrte (temp);					// isqrt = first approximation of 1/sqrt() 
	temp1 = temp * (float)(-.5);				// temp1 = -a / 2 		
	temp2 = isqrt * isqrt;						// temp2 = sqrt^2
	temp1 *= isqrt;								// temp1 = -a * sqrt / 2 
	isqrt *= (float)(3.0/2.0);					// isqrt = 3 * sqrt / 2 
	temp = isqrt + temp1 * temp2;				// isqrt = (3 * sqrt - a * sqrt^3) / 2 
	
	outV->x = vx * temp;						// return results
	outV->y = vy * temp;
	outV->z = vz * temp;
#endif
}


#pragma mark ....intersect line & plane....

/******************** INTERSECT PLANE & LINE SEGMENT ***********************/
//
// Returns true if the input line segment intersects the plane.
//

Boolean IntersectionOfLineSegAndPlane(TQ3PlaneEquation *plane, float v1x, float v1y, float v1z,
								 float v2x, float v2y, float v2z, TQ3Point3D *outPoint)
{
int		a,b;
float	r;
float	nx,ny,nz,planeConst;
float	vBAx, vBAy, vBAz, dot, lam;

	nx = plane->normal.x;
	ny = plane->normal.y;
	nz = plane->normal.z;
	planeConst = plane->constant;
	
	
		/* DETERMINE SIDENESS OF VERT1 */
		
	r = -planeConst;
	r += (nx * v1x) + (ny * v1y) + (nz * v1z);
	a = (r < 0.0f) ? 1 : 0;

		/* DETERMINE SIDENESS OF VERT2 */
		
	r = -planeConst;
	r += (nx * v2x) + (ny * v2y) + (nz * v2z);
	b = (r < 0.0f) ? 1 : 0;

		/* SEE IF LINE CROSSES PLANE (INTERSECTS) */

	if (a == b)
	{
		return(false);
	}


		/****************************************************/
		/* LINE INTERSECTS, SO CALCULATE INTERSECTION POINT */
		/****************************************************/
			
				/* CALC LINE SEGMENT VECTOR BA */
				
	vBAx = v2x - v1x;
	vBAy = v2y - v1y;
	vBAz = v2z - v1z;
	
			/* DOT OF PLANE NORMAL & LINE SEGMENT VECTOR */
			
	dot = (nx * vBAx) + (ny * vBAy) + (nz * vBAz);
	
			/* IF VALID, CALC INTERSECTION POINT */
			
	if (dot)
	{
		lam = planeConst;
		lam -= (nx * v1x) + (ny * v1y) + (nz * v1z);		// calc dot product of plane normal & 1st vertex
		lam /= dot;											// div by previous dot for scaling factor
		
		outPoint->x = v1x + (lam * vBAx);					// calc intersect point
		outPoint->y = v1y + (lam * vBAy);
		outPoint->z = v1z + (lam * vBAz);
		return(true);
	}
	
		/* IF DOT == 0, THEN LINE IS PARALLEL TO PLANE THUS NO INTERSECTION */
		
	else
		return(false);
}




/*********** INTERSECTION OF Y AND PLANE FUNCTION ********************/
//
// INPUT:	
//			x/z		:	xz coords of point
//			p		:	ptr to the plane
//
//
// *** IMPORTANT-->  This function does not check for divides by 0!! As such, there should be no
//					"vertical" polygons (polys with normal->y == 0).
//

float IntersectionOfYAndPlane_Func(float x, float z, TQ3PlaneEquation *p)
{
	return	((p->constant - ((p->normal.x * x) + (p->normal.z * z))) / p->normal.y);
}
















