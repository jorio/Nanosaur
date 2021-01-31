#pragma once

#include "QD3D.h"
#include <math.h>
#include <string.h>



#ifdef FLT_EPSILON
    #define kQ3RealZero                         (FLT_EPSILON)
#else
    #define kQ3RealZero                         ((TQ3Float32) 1.19209290e-07)
#endif

#ifdef FLT_MAX
    #define kQ3MaxFloat                         (FLT_MAX)
#else
    #define kQ3MaxFloat                         ((TQ3Float32) 3.40282347e+38)
#endif

#ifdef FLT_MIN
    #define kQ3MinFloat                         (FLT_MIN)
#else
    #define kQ3MinFloat                         ((TQ3Float32) 1.17549e-38)
#endif

#define kQ3Pi                                   ((TQ3Float32) 3.1415926535898)
#define kQ32Pi                                  ((TQ3Float32) (2.0 * 3.1415926535898))
#define kQ3PiOver2                              ((TQ3Float32) (3.1415926535898 / 2.0))
#define kQ33PiOver2                             ((TQ3Float32) (3.0 * 3.1415926535898 / 2.0))


#define __Q3Memory_Clear(ptr, size) memset((ptr), 0, (size))


#define __Q3FastVector2D_Set(_v, _x, _y)									\
	do																		\
		{																	\
		(_v)->x = (_x);														\
		(_v)->y = (_y);														\
		}																	\
	while (0)

#define __Q3FastVector3D_Set(_v, _x, _y, _z)								\
	do																		\
		{																	\
		(_v)->x = (_x);														\
		(_v)->y = (_y);														\
		(_v)->z = (_z);														\
		}																	\
	while (0)

#define __Q3FastPoint2D_Set(_p, _x, _y)										\
	do																		\
		{																	\
		(_p)->x = (_x);														\
		(_p)->y = (_y);														\
		}																	\
	while (0)

#define __Q3FastParam2D_Set(_p, _u, _v)										\
	do																		\
		{																	\
		(_p)->u = (_u);														\
		(_p)->v = (_v);														\
		}																	\
	while (0)

#define __Q3FastRationalPoint3D_Set(_p, _x, _y, _w)							\
	do																		\
		{																	\
		(_p)->x = (_x);														\
		(_p)->y = (_y);														\
		(_p)->w = (_w);														\
		}																	\
	while (0)

#define __Q3FastPoint3D_Set(_p, _x, _y, _z)									\
	do																		\
		{																	\
		(_p)->x = (_x);														\
		(_p)->y = (_y);														\
		(_p)->z = (_z);														\
		}																	\
	while (0)

#define __Q3FastRationalPoint4D_Set(_p, _x, _y, _z, _w)						\
	do																		\
		{																	\
		(_p)->x = (_x);														\
		(_p)->y = (_y);														\
		(_p)->z = (_z);														\
		(_p)->w = (_w);														\
		}																	\
	while (0)

#define __Q3FastPolarPoint_Set(_p, _r, _theta)								\
	do																		\
		{																	\
		(_p)->r     = (_r);													\
		(_p)->theta = (_theta);												\
		}																	\
	while (0)

#define __Q3FastSphericalPoint_Set(_p, _rho, _theta, _phi)					\
	do																		\
		{																	\
		(_p)->rho   = (_rho);												\
		(_p)->theta = (_theta);												\
		(_p)->phi   = (_phi);												\
		}																	\
	while (0)

#define __Q3FastVector2D_To3D(_v1, _v2)										\
	do																		\
		{																	\
		(_v2)->x = (_v1)->x;												\
		(_v2)->y = (_v1)->y;												\
		(_v2)->z = 1.0f;													\
		}																	\
	while (0)


#define __Q3FastVector2D_ToRationalPoint3D(_v, _p)							\
	do																		\
		{																	\
		(_p)->x = (_v)->x;													\
		(_p)->y = (_v)->y;													\
		(_p)->w = 0.0f;														\
		}																	\
	while (0)

#define __Q3FastVector3D_To2D(_v1, _v2)										\
	do																		\
		{																	\
		float invZ = 1.0f / (_v1)->z;										\
		(_v2)->x = (_v1)->x * invZ;											\
		(_v2)->y = (_v1)->y * invZ;											\
		}																	\
	while (0)

#define __Q3FastRationalPoint3D_ToVector2D(_p, _v)							\
	do																		\
		{																	\
		(_v)->x = (_p)->x;													\
		(_v)->y = (_p)->y;													\
		}																	\
	while (0)

#define __Q3FastVector3D_ToRationalPoint4D(_v, _p)							\
	do																		\
		{																	\
		(_p)->x = (_v)->x;													\
		(_p)->y = (_v)->y;													\
		(_p)->z = (_v)->z;													\
		(_p)->w = 0.0f;														\
		}																	\
	while (0)

#define __Q3FastRationalPoint4D_ToVector3D(_p, _v)							\
	do																		\
		{																	\
		(_v)->x = (_p)->x;													\
		(_v)->y = (_p)->y;													\
		(_v)->z = (_p)->z;													\
		}																	\
	while (0)

#define __Q3FastPoint2D_To3D(_p1, _p2)										\
	do																		\
		{																	\
		(_p2)->x = (_p1)->x;												\
		(_p2)->y = (_p1)->y;												\
		(_p2)->w = 1.0f;													\
		}																	\
	while (0)

#define __Q3FastRationalPoint3D_To2D(_p1, _p2)								\
	do																		\
		{																	\
		float invW = 1.0f / (_p1)->w;										\
		(_p2)->x = (_p1)->x * invW;											\
		(_p2)->y = (_p1)->y * invW;											\
		}																	\
	while (0)

#define __Q3FastPoint3D_To4D(_p1, _p2)										\
	do																		\
		{																	\
		(_p2)->x = (_p1)->x;												\
		(_p2)->y = (_p1)->y;												\
		(_p2)->z = (_p1)->z;												\
		(_p2)->w = 1.0f;													\
		}																	\
	while (0)

#define __Q3FastRationalPoint4D_To3D(_p1, _p2)								\
	do																		\
		{																	\
		float invW = 1.0f / (_p1)->w;										\
		(_p2)->x = (_p1)->x * invW;											\
		(_p2)->y = (_p1)->y * invW;											\
		(_p2)->z = (_p1)->z * invW;											\
		}																	\
	while (0)

#define __Q3FastPolarPoint_ToPoint2D(_p1, _p2)								\
	do																		\
		{																	\
		(_p2)->x = (_p1)->r * ((float) cos((_p1)->theta));					\
		(_p2)->y = (_p1)->r * ((float) sin((_p1)->theta));					\
		}																	\
	while (0)

#define __Q3FastVector2D_Dot(_v1, _v2)										\
	(																		\
		((_v1)->x * (_v2)->x) +												\
		((_v1)->y * (_v2)->y)												\
	)																		\

#define __Q3FastVector3D_Dot(_v1, _v2)										\
	(																		\
		((_v1)->x * (_v2)->x) +												\
		((_v1)->y * (_v2)->y) +												\
		((_v1)->z * (_v2)->z)												\
	)																		\

#define __Q3FastVector2D_Cross(_v1, _v2)									\
	(																		\
		((_v1)->x * (_v2)->y) -												\
		((_v1)->y * (_v2)->x)												\
	)																		\

#define __Q3FastPoint2D_CrossProductTri(_p1, _p2, _p3)						\
	(																		\
		(((_p2)->x - (_p1)->x) * ((_p3)->x - (_p2)->x)) -					\
		(((_p2)->y - (_p1)->y) * ((_p3)->y - (_p2)->y))						\
	)																		\

#define __Q3FastVector3D_Cross(_v1, _v2, _r)								\
	do																		\
		{																	\
		float rx = ((_v1)->y * (_v2)->z) - ((_v1)->z * (_v2)->y);			\
        float ry = ((_v1)->z * (_v2)->x) - ((_v1)->x * (_v2)->z);			\
        float rz = ((_v1)->x * (_v2)->y) - ((_v1)->y * (_v2)->x);			\
        																	\
        (_r)->x = rx;														\
        (_r)->y = ry;														\
        (_r)->z = rz;														\
		}																	\
	while (0)

#define __Q3FastPoint3D_CrossProductTri(_p1, _p2, _p3, _r)					\
	do																		\
		{																	\
		float _v1_x = (_p2)->x - (_p1)->x;									\
		float _v1_y = (_p2)->y - (_p1)->y;									\
		float _v1_z = (_p2)->z - (_p1)->z;									\
																			\
		float _v2_x = (_p3)->x - (_p2)->x;									\
		float _v2_y = (_p3)->y - (_p2)->y;									\
		float _v2_z = (_p3)->z - (_p2)->z;									\
																			\
		(_r)->x = (_v1_y * _v2_z) - (_v1_z * _v2_y);						\
        (_r)->y = (_v1_z * _v2_x) - (_v1_x * _v2_z);						\
        (_r)->z = (_v1_x * _v2_y) - (_v1_y * _v2_x);						\
		}																	\
	while (0)

#define __Q3FastVector2D_Length(_v)											\
	(																		\
		(float) sqrtf(__Q3FastVector2D_LengthSquared(_v))					\
	)																		\

#define __Q3FastVector2D_LengthSquared(_v)									\
	(																		\
		((_v)->x * (_v)->x) +												\
		((_v)->y * (_v)->y)													\
	)																		\

#define __Q3FastVector3D_Length(_v)											\
	(																		\
		(float) sqrtf(__Q3FastVector3D_LengthSquared(_v))					\
	)																		\

#define __Q3FastVector3D_LengthSquared(_v)									\
	(																		\
		((_v)->x * (_v)->x) +												\
		((_v)->y * (_v)->y)	+												\
		((_v)->z * (_v)->z)													\
	)																		\

#define __Q3FastPoint2D_Distance(_p1, _p2)									\
	(																		\
		(float) sqrtf(__Q3FastPoint2D_DistanceSquared(_p1, _p2))				\
	)																		\

#define __Q3FastPoint2D_DistanceSquared(_p1, _p2)							\
	(																		\
		(((_p1)->x - (_p2)->x) * ((_p1)->x - (_p2)->x))	+					\
		(((_p1)->y - (_p2)->y) * ((_p1)->y - (_p2)->y))						\
	)																		\

#define __Q3FastParam2D_Distance(_p1, _p2)									\
	(																		\
		__Q3FastPoint2D_Distance((const TQ3Point2D*) _p1,					\
							     (const TQ3Point2D*) _p2)					\
	)																		\

#define __Q3FastParam2D_DistanceSquared(_p1, _p2)							\
	(																		\
		__Q3FastPoint2D_DistanceSquared((const TQ3Point2D*) _p1,			\
									    (const TQ3Point2D*) _p2)			\
	)																		\

#define __Q3FastRationalPoint3D_Distance(_p1, _p2)							\
	(																		\
		(float) sqrtf(__Q3FastRationalPoint3D_DistanceSquared(_p1, _p2))		\
	)																		\

#define __Q3FastRationalPoint3D_DistanceSquared(_p1, _p2)					\
	(																		\
		(((_p1)->x/(_p1)->w - (_p2)->x/(_p2)->w) * ((_p1)->x/(_p1)->w - (_p2)->x/(_p2)->w))	+	\
		(((_p1)->y/(_p1)->w - (_p2)->y/(_p2)->w) * ((_p1)->y/(_p1)->w - (_p2)->y/(_p2)->w))		\
	)																		\

#define __Q3FastPoint3D_Distance(_p1, _p2)									\
	(																		\
		(float) sqrtf(__Q3FastPoint3D_DistanceSquared(_p1, _p2))				\
	)																		\

#define __Q3FastPoint3D_DistanceSquared(_p1, _p2)							\
	(																		\
		(((_p1)->x - (_p2)->x) * ((_p1)->x - (_p2)->x))	+					\
		(((_p1)->y - (_p2)->y) * ((_p1)->y - (_p2)->y))	+					\
		(((_p1)->z - (_p2)->z) * ((_p1)->z - (_p2)->z))						\
	)																		\

#define __Q3FastRationalPoint4D_Distance(_p1, _p2)							\
	(																		\
		(float) sqrtf(__Q3FastRationalPoint4D_DistanceSquared(_p1, _p2))		\
	)																		\

#define __Q3FastRationalPoint4D_DistanceSquared(_p1, _p2)					\
	(																		\
		(((_p1)->x/(_p1)->w - (_p2)->x/(_p2)->w) * ((_p1)->x/(_p1)->w - (_p2)->x/(_p2)->w))	+	\
		(((_p1)->y/(_p1)->w - (_p2)->y/(_p2)->w) * ((_p1)->y/(_p1)->w - (_p2)->y/(_p2)->w))	+	\
		(((_p1)->z/(_p1)->w - (_p2)->z/(_p2)->w) * ((_p1)->z/(_p1)->w - (_p2)->z/(_p2)->w))		\
	)																		\

#define __Q3FastVector2D_Negate(_v1, _v2)									\
	do																		\
		{																	\
		(_v2)->x = -(_v1)->x;												\
		(_v2)->y = -(_v1)->y;												\
		}																	\
	while (0)

#define __Q3FastVector3D_Negate(_v1, _v2)									\
	do																		\
		{																	\
		(_v2)->x = -(_v1)->x;												\
		(_v2)->y = -(_v1)->y;												\
		(_v2)->z = -(_v1)->z;												\
		}																	\
	while (0)

#define __Q3FastVector2D_Scale(_v1, _s, _v2)								\
	do																		\
		{																	\
		(_v2)->x = (_v1)->x * (_s);											\
		(_v2)->y = (_v1)->y * (_s);											\
		}																	\
	while (0)

#define __Q3FastVector3D_Scale(_v1, _s, _v2)								\
	do																		\
		{																	\
		(_v2)->x = (_v1)->x * (_s);											\
		(_v2)->y = (_v1)->y * (_s);											\
		(_v2)->z = (_v1)->z * (_s);											\
		}																	\
	while (0)

// Normalization performance trick:  usually, adding kQ3MinFloat to the length
// makes no difference, but it prevents division by zero without a branch.
#define __Q3FastVector2D_Normalize(_v1, _v2)								\
	do																		\
	{																		\
		float theLength = __Q3FastVector2D_Length(_v1) + kQ3MinFloat;		\
		__Q3FastVector2D_Scale(_v1, 1.0f / theLength, _v2);					\
	}																		\
	while (0)

#define __Q3FastVector3D_Normalize(_v1, _v2)								\
	do																		\
	{																		\
		float theLength = __Q3FastVector3D_Length(_v1) + kQ3MinFloat;		\
		__Q3FastVector3D_Scale(_v1, 1.0f / theLength, _v2);					\
	}																		\
	while (0)


#define __Q3FastVector2D_Add(_v1, _v2, _r)									\
	do																		\
		{																	\
		(_r)->x = (_v1)->x + (_v2)->x;										\
		(_r)->y = (_v1)->y + (_v2)->y;										\
		}																	\
	while (0)

#define __Q3FastVector3D_Add(_v1, _v2, _r)									\
	do																		\
		{																	\
		(_r)->x = (_v1)->x + (_v2)->x;										\
		(_r)->y = (_v1)->y + (_v2)->y;										\
		(_r)->z = (_v1)->z + (_v2)->z;										\
		}																	\
	while (0)

#define __Q3FastVector2D_Subtract(_v1, _v2, _r)								\
	do																		\
		{																	\
		(_r)->x = (_v1)->x - (_v2)->x;										\
		(_r)->y = (_v1)->y - (_v2)->y;										\
		}																	\
	while (0)

#define __Q3FastVector3D_Subtract(_v1, _v2, _r)								\
	do																		\
		{																	\
		(_r)->x = (_v1)->x - (_v2)->x;										\
		(_r)->y = (_v1)->y - (_v2)->y;										\
		(_r)->z = (_v1)->z - (_v2)->z;										\
		}																	\
	while (0)

#define __Q3FastPoint2D_Vector2D_Add(_p, _v, _r)							\
	do																		\
		{																	\
		(_r)->x = (_p)->x + (_v)->x;										\
		(_r)->y = (_p)->y + (_v)->y;										\
		}																	\
	while (0)

#define __Q3FastParam2D_Vector2D_Add(_p, _v, _r)							\
	do																		\
		{																	\
		(_r)->u = (_p)->u + (_v)->x;										\
		(_r)->v = (_p)->v + (_v)->y;										\
		}																	\
	while (0)

#define __Q3FastPoint3D_Vector3D_Add(_p, _v, _r)							\
	do																		\
		{																	\
		(_r)->x = (_p)->x + (_v)->x;										\
		(_r)->y = (_p)->y + (_v)->y;										\
		(_r)->z = (_p)->z + (_v)->z;										\
		}																	\
	while (0)

#define __Q3FastPoint2D_Vector2D_Subtract(_p, _v, _r)						\
	do																		\
		{																	\
		(_r)->x = (_p)->x - (_v)->x;										\
		(_r)->y = (_p)->y - (_v)->y;										\
		}																	\
	while (0)

#define __Q3FastParam2D_Vector2D_Subtract(_p, _v, _r)						\
	do																		\
		{																	\
		(_r)->u = (_p)->u - (_v)->x;										\
		(_r)->v = (_p)->v - (_v)->y;										\
		}																	\
	while (0)

#define __Q3FastPoint3D_Vector3D_Subtract(_p, _v, _r)						\
	do																		\
		{																	\
		(_r)->x = (_p)->x - (_v)->x;										\
		(_r)->y = (_p)->y - (_v)->y;										\
		(_r)->z = (_p)->z - (_v)->z;										\
		}																	\
	while (0)

#define __Q3FastPoint2D_Subtract(_p1, _p2, _r)								\
	do																		\
		{																	\
		(_r)->x = (_p1)->x - (_p2)->x;										\
		(_r)->y = (_p1)->y - (_p2)->y;										\
		}																	\
	while (0)

#define __Q3FastParam2D_Subtract(_p1, _p2, _r)								\
	do																		\
		{																	\
		(_r)->x = (_p1)->u - (_p2)->u;										\
		(_r)->y = (_p1)->v - (_p2)->v;										\
		}																	\
	while (0)

#define __Q3FastPoint3D_Subtract(_p1, _p2, _r)								\
	do																		\
		{																	\
		(_r)->x = (_p1)->x - (_p2)->x;										\
		(_r)->y = (_p1)->y - (_p2)->y;										\
		(_r)->z = (_p1)->z - (_p2)->z;										\
		}																	\
	while (0)

#define __Q3FastPoint2D_RRatio(_p1, _p2, _r1, _r2, _result)					\
	do																		\
		{																	\
		float frac   = (_r2) / ((_r1) + (_r2));								\
		(_result)->x = (_p1)->x + (frac * ((_p2)->x - (_p1)->x));			\
		(_result)->y = (_p1)->y + (frac * ((_p2)->y - (_p1)->y));			\
		}																	\
	while (0)

#define __Q3FastParam2D_RRatio(_p1, _p2, _r1, _r2, _result)					\
	do																		\
		{																	\
		float frac   = (_r2) / ((_r1) + (_r2));								\
		(_result)->u = (_p1)->u + (frac * ((_p2)->u - (_p1)->u));			\
		(_result)->v = (_p1)->v + (frac * ((_p2)->v - (_p1)->v));			\
		}																	\
	while (0)

#define __Q3FastPoint3D_RRatio(_p1, _p2, _r1, _r2, _result)					\
	do																		\
		{																	\
		float frac   = (_r2) / ((_r1) + (_r2));								\
		(_result)->x = (_p1)->x + (frac * ((_p2)->x - (_p1)->x));			\
		(_result)->y = (_p1)->y + (frac * ((_p2)->y - (_p1)->y));			\
		(_result)->z = (_p1)->z + (frac * ((_p2)->z - (_p1)->z));			\
		}																	\
	while (0)

#define __Q3FastRationalPoint4D_RRatio(_p1, _p2, _r1, _r2, _result)			\
	do																		\
		{																	\
		TQ3Point3D	_pt1, _pt2;												\
		__Q3FastRationalPoint4D_To3D( _p1, &_pt1 );							\
		__Q3FastRationalPoint4D_To3D( _p2, &_pt2 );							\
		__Q3FastPoint3D_RRatio( &_pt1, &_pt2, _r1, _r2, _result );			\
		(_result)->w = 1;													\
		}																	\
	while (0)

#define __Q3FastQuaternion_Set(_q, _w, _x, _y, _z)							\
	do																		\
		{																	\
		(_q)->w = (_w);														\
		(_q)->x = (_x);														\
		(_q)->y = (_y);														\
		(_q)->z = (_z);														\
		}																	\
	while (0)

#define __Q3FastQuaternion_SetIdentity(_q)									\
	do																		\
		{																	\
		(_q)->w = 1.0f;														\
		(_q)->x = 0.0f;														\
		(_q)->y = 0.0f;														\
		(_q)->z = 0.0f;														\
		}																	\
	while (0)

#define __Q3FastQuaternion_Copy(_q1, _q2)									\
	do																		\
		{																	\
		*(_q2) = *(_q1);													\
		}																	\
	while (0)

#define __Q3FastQuaternion_Dot(_q1, _q2)									\
	(																		\
		((_q1)->w * (_q2)->w) +												\
		((_q1)->x * (_q2)->x) +												\
		((_q1)->y * (_q2)->y) +												\
		((_q1)->z * (_q2)->z)												\
	)																		\

#define __Q3FastQuaternion_Normalize(_q1, _q2)								\
	do																		\
		{																	\
		float qDot    = __Q3FastQuaternion_Dot(_q1, _q1);					\
		float qFactor = 1.0f / (float) sqrt(qDot);							\
																			\
		(_q2)->w = (_q1)->w * qFactor;										\
		(_q2)->x = (_q1)->x * qFactor;										\
		(_q2)->y = (_q1)->y * qFactor;										\
		(_q2)->z = (_q1)->z * qFactor;										\
		}																	\
	while (0)

#define __Q3FastQuaternion_Invert(_q1, _q2)									\
	do																		\
		{																	\
		(_q2)->w =  (_q1)->w;												\
		(_q2)->x = -(_q1)->x;												\
		(_q2)->y = -(_q1)->y;												\
		(_q2)->z = -(_q1)->z;												\
		}																	\
	while (0)

#define __Q3FastBoundingBox_Reset(_b)										\
	do																		\
		{																	\
		(_b)->min.x =														\
		(_b)->min.y =														\
		(_b)->min.z =														\
		(_b)->max.x =														\
		(_b)->max.y =														\
		(_b)->max.z = 0.0f;													\
		(_b)->isEmpty = kQ3True;											\
		}																	\
	while (0)

#define __Q3FastBoundingBox_Set(_b, _min, _max, _isEmpty)					\
	do																		\
		{																	\
		(_b)->min     = *(_min);											\
		(_b)->max     = *(_max);											\
		(_b)->isEmpty =  (_isEmpty);										\
		}																	\
	while (0)

#define __Q3FastBoundingBox_Copy(_b1, _b2)									\
	do																		\
		{																	\
		*(_b2) = *(_b1);													\
		}																	\
	while (0)

#define __Q3FastBoundingSphere_Reset(_s)									\
	do																		\
		{																	\
		(_s)->origin.x =													\
		(_s)->origin.y =													\
		(_s)->origin.z =													\
		(_s)->radius   = 0.0f;												\
		(_s)->isEmpty = kQ3True;											\
		}																	\
	while (0)

#define __Q3FastBoundingSphere_Set(_s, _origin, _radius, _isEmpty)			\
	do																		\
		{																	\
		(_s)->origin  = *(_origin);											\
		(_s)->radius  =  (_radius);											\
		(_s)->isEmpty =  (_isEmpty);										\
		}																	\
	while (0)

#define __Q3FastBoundingSphere_Copy(_s1, _s2)								\
	do																		\
		{																	\
		*(_s2) = *(_s1);													\
		}																	\
	while (0)

static float Q3Point2D_Distance(const TQ3Point2D *p1, const TQ3Point2D *p2)
{
	return __Q3FastPoint2D_Distance(p1, p2);
}

static void Q3Vector2D_Normalize(
		const TQ3Vector2D* v1,
		TQ3Vector2D* result)
{
	__Q3FastVector2D_Normalize(v1, result);
}

static float Q3Vector2D_Dot(
		const TQ3Vector2D *v1,
		const TQ3Vector2D *v2)
{
	return __Q3FastVector2D_Dot(v1, v2);
}

static void Q3Vector3D_Cross(
		const TQ3Vector3D* v1,
		const TQ3Vector3D* v2,
		TQ3Vector3D* result)
{
	__Q3FastVector3D_Cross(v1, v2, result);
}

static float Q3Vector3D_Dot(
		const TQ3Vector3D *v1,
		const TQ3Vector3D *v2)
{
	return __Q3FastVector3D_Dot(v1, v2);
}

static void Q3Vector3D_Normalize(
		const TQ3Vector3D* v1,
		TQ3Vector3D* result)
{
	__Q3FastVector3D_Normalize(v1, result);
}

static void Q3Point3D_CrossProductTri(
		const TQ3Point3D* p1,
		const TQ3Point3D* p2,
		const TQ3Point3D* p3,
		TQ3Point3D* result)
{
	__Q3FastPoint3D_CrossProductTri(p1, p2, p3, result);
}

static
TQ3Vector3D *
Q3Vector3D_Transform(const TQ3Vector3D *vector3D, const TQ3Matrix4x4 *matrix4x4,
					 TQ3Vector3D *result)
{
	// Save input to avoid problems when result is same as input
	float x = vector3D->x;
	float y = vector3D->y;
	float z = vector3D->z;

#define M(x,y) matrix4x4->value[x][y]
	result->x = x*M(0,0) + y*M(1,0) + z*M(2,0);
	result->y = x*M(0,1) + y*M(1,1) + z*M(2,1);
	result->z = x*M(0,2) + y*M(1,2) + z*M(2,2);
#undef M

	return(result);
}


static
TQ3Matrix3x3 *
Q3Matrix3x3_SetTranslate(TQ3Matrix3x3 *matrix3x3, float xTrans, float yTrans)
{
	__Q3Memory_Clear(matrix3x3, sizeof(TQ3Matrix3x3));

#define M(x,y) matrix3x3->value[x][y]

	M(0,0) = 1.0f;

	M(1,1) = 1.0f;

	M(2,0) = xTrans;
	M(2,1) = yTrans;
	M(2,2) = 1.0f;

#undef M

	return(matrix3x3);
}



static
TQ3Matrix4x4 *
Q3Matrix4x4_SetIdentity(TQ3Matrix4x4 *matrix4x4)
{
	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) = 1.0f;
	M(1,1) = 1.0f;
	M(2,2) = 1.0f;
	M(3,3) = 1.0f;

#undef M

	return(matrix4x4);
}


static TQ3Matrix4x4 *
Q3Matrix4x4_SetScale(
		TQ3Matrix4x4 *matrix4x4,
		float xScale,
		float yScale,
		float zScale)
{
	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) = xScale;
	M(1,1) = yScale;
	M(2,2) = zScale;
	M(3,3) = 1.0f;

#undef M

	return(matrix4x4);
}


static
TQ3Matrix4x4 *
Q3Matrix4x4_SetTranslate(
		TQ3Matrix4x4 *matrix4x4,
		float xTrans,
		float yTrans,
		float zTrans)
{
	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) = 1.0f;

	M(1,1) = 1.0f;

	M(2,2) = 1.0f;

	M(3,0) = xTrans;
	M(3,1) = yTrans;
	M(3,2) = zTrans;
	M(3,3) = 1.0f;

#undef M

	return(matrix4x4);
}


static
TQ3Matrix4x4 *
Q3Matrix4x4_Multiply(
		const TQ3Matrix4x4 *m1,
		const TQ3Matrix4x4 *m2,
		TQ3Matrix4x4 *result)
{
	// If result is alias of input, output to temporary
	TQ3Matrix4x4 temp;
	TQ3Matrix4x4* output = (result == m1 || result == m2 ? &temp : result);

#define A(x,y)	m1->value[x][y]
#define B(x,y)	m2->value[x][y]
#define M(x,y)	output->value[x][y]

	M(0,0) = A(0,0)*B(0,0) + A(0,1)*B(1,0) + A(0,2)*B(2,0) + A(0,3)*B(3,0);
	M(0,1) = A(0,0)*B(0,1) + A(0,1)*B(1,1) + A(0,2)*B(2,1) + A(0,3)*B(3,1);
	M(0,2) = A(0,0)*B(0,2) + A(0,1)*B(1,2) + A(0,2)*B(2,2) + A(0,3)*B(3,2);
	M(0,3) = A(0,0)*B(0,3) + A(0,1)*B(1,3) + A(0,2)*B(2,3) + A(0,3)*B(3,3);

	M(1,0) = A(1,0)*B(0,0) + A(1,1)*B(1,0) + A(1,2)*B(2,0) + A(1,3)*B(3,0);
	M(1,1) = A(1,0)*B(0,1) + A(1,1)*B(1,1) + A(1,2)*B(2,1) + A(1,3)*B(3,1);
	M(1,2) = A(1,0)*B(0,2) + A(1,1)*B(1,2) + A(1,2)*B(2,2) + A(1,3)*B(3,2);
	M(1,3) = A(1,0)*B(0,3) + A(1,1)*B(1,3) + A(1,2)*B(2,3) + A(1,3)*B(3,3);

	M(2,0) = A(2,0)*B(0,0) + A(2,1)*B(1,0) + A(2,2)*B(2,0) + A(2,3)*B(3,0);
	M(2,1) = A(2,0)*B(0,1) + A(2,1)*B(1,1) + A(2,2)*B(2,1) + A(2,3)*B(3,1);
	M(2,2) = A(2,0)*B(0,2) + A(2,1)*B(1,2) + A(2,2)*B(2,2) + A(2,3)*B(3,2);
	M(2,3) = A(2,0)*B(0,3) + A(2,1)*B(1,3) + A(2,2)*B(2,3) + A(2,3)*B(3,3);

	M(3,0) = A(3,0)*B(0,0) + A(3,1)*B(1,0) + A(3,2)*B(2,0) + A(3,3)*B(3,0);
	M(3,1) = A(3,0)*B(0,1) + A(3,1)*B(1,1) + A(3,2)*B(2,1) + A(3,3)*B(3,1);
	M(3,2) = A(3,0)*B(0,2) + A(3,1)*B(1,2) + A(3,2)*B(2,2) + A(3,3)*B(3,2);
	M(3,3) = A(3,0)*B(0,3) + A(3,1)*B(1,3) + A(3,2)*B(2,3) + A(3,3)*B(3,3);

#undef A
#undef B
#undef M

	if (output == &temp)
		*result = temp;

	return(result);
}

static
TQ3Matrix4x4 *
Q3Matrix4x4_SetRotate_X(TQ3Matrix4x4 *matrix4x4, float angle)
{
	float cosAngle = (float) cosf(angle);
	float sinAngle = (float) sinf(angle);

	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) =  1.0f;

	M(1,1) =  cosAngle;
	M(1,2) =  sinAngle;

	M(2,1) = -sinAngle;
	M(2,2) =  cosAngle;

	M(3,3) =  1.0f;

#undef M

	return(matrix4x4);
}

static
TQ3Matrix4x4 *
Q3Matrix4x4_SetRotate_Y(TQ3Matrix4x4 *matrix4x4, float angle)
{
	float cosAngle = (float) cosf(angle);
	float sinAngle = (float) sinf(angle);

	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) =  cosAngle;
	M(0,2) = -sinAngle;

	M(1,1) =  1.0f;

	M(2,0) =  sinAngle;
	M(2,2) =  cosAngle;

	M(3,3) =  1.0f;

#undef M

	return(matrix4x4);
}


static
TQ3Matrix4x4 *
Q3Matrix4x4_SetRotate_Z(TQ3Matrix4x4 *matrix4x4, float angle)
{
	float cosAngle = (float) cosf(angle);
	float sinAngle = (float) sinf(angle);

	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) =  cosAngle;
	M(0,1) =  sinAngle;

	M(1,0) = -sinAngle;
	M(1,1) =  cosAngle;

	M(2,2) =  1.0f;

	M(3,3) =  1.0f;

#undef M

	return(matrix4x4);
}



static
TQ3Matrix4x4 *
Q3Matrix4x4_SetRotate_XYZ(
		TQ3Matrix4x4 *matrix4x4,
		float xAngle,
		float yAngle,
		float zAngle)
{
	float cosX = (float) cosf(xAngle);
	float sinX = (float) sinf(xAngle);
	float cosY = (float) cosf(yAngle);
	float sinY = (float) sinf(yAngle);
	float cosZ = (float) cosf(zAngle);
	float sinZ = (float) sinf(zAngle);

	float sinXsinY = sinX*sinY;
	float cosXsinY = cosX*sinY;

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) =  cosY*cosZ;
	M(0,1) =  cosY*sinZ;
	M(0,2) = -sinY;
	M(0,3) =  0.0f;

	M(1,0) =  sinXsinY*cosZ - cosX*sinZ;
	M(1,1) =  sinXsinY*sinZ + cosX*cosZ;
	M(1,2) =  sinX*cosY;
	M(1,3) =  0.0f;

	M(2,0) =  cosXsinY*cosZ + sinX*sinZ;
	M(2,1) =  cosXsinY*sinZ - sinX*cosZ;
	M(2,2) =  cosX*cosY;
	M(2,3) =  0.0f;

	M(3,0) =  0.0f;
	M(3,1) =  0.0f;
	M(3,2) =  0.0f;
	M(3,3) =  1.0f;

#undef M

	return(matrix4x4);
}


//-----------------------------------------------------------------------------
static
TQ3Point3D *
Q3Point3D_Transform(
		const TQ3Point3D *point3D,
		const TQ3Matrix4x4 *matrix4x4,
		TQ3Point3D *result)
{
	// Save input to avoid problems when result is same as input
	float x = point3D->x;
	float y = point3D->y;
	float z = point3D->z;
	float neww;

#define M(x,y) matrix4x4->value[x][y]
	result->x = x*M(0,0) + y*M(1,0) + z*M(2,0) + M(3,0);
	result->y = x*M(0,1) + y*M(1,1) + z*M(2,1) + M(3,1);
	result->z = x*M(0,2) + y*M(1,2) + z*M(2,2) + M(3,2);
	neww = x*M(0,3) + y*M(1,3) + z*M(2,3) + M(3,3);
#undef M

	if (neww == 0.0f)
	{
//		E3ErrorManager_PostError( kQ3ErrorInfiniteRationalPoint, kQ3False );
		neww = 1.0f;
	}

	if (neww != 1.0f)
	{
		float invw = 1.0f / neww;
		result->x *= invw;
		result->y *= invw;
		result->z *= invw;
	}

	return(result);
}


static
float
Q3Point3D_Distance(
		const TQ3Point3D *p1,
		const TQ3Point3D *p2)
{
	return __Q3FastPoint3D_Distance(p1, p2);
}





