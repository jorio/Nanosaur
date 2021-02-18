/*

Adapted from Quesa's math routines.
Original copyright notice below:

Copyright (c) 1999-2020, Quesa Developers. All rights reserved.

For the current release of Quesa, please see:
	<https://github.com/jwwalker/Quesa>

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

o Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

o Redistributions in binary form must reproduce the above copyright notice, this list of conditions
and the following disclaimer in the documentation and/or other materials provided with the
distribution.

o Neither the name of Quesa nor the names of its contributors may be used to endorse or promote
products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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

#define Q3Math_DegreesToRadians(_x)             ((TQ3Float32) ((_x) *  kQ3Pi / 180.0f))
#define Q3Math_RadiansToDegrees(_x)             ((TQ3Float32) ((_x) * 180.0f / kQ3Pi))
#define Q3Math_Min(_x,_y)                       ((_x) <= (_y) ? (_x) : (_y))
#define Q3Math_Max(_x,_y)                       ((_x) >= (_y) ? (_x) : (_y))

#define __Q3Memory_Clear(ptr, size) memset((ptr), 0, (size))

#define __Q3Float_Swap(_a, _b)		\
	do {							\
		float _temp;				\
									\
		_temp = (_a);				\
		(_a)  = (_b);				\
		(_b)  = _temp;				\
	} while (0)

//-----------------------------------------------------------------------------
#pragma mark Q3Point2D

static inline float Q3Point2D_DistanceSquared(
		const TQ3Point2D *p1,
		const TQ3Point2D *p2)
{
	return		((p1->x - p2->x) * (p1->x - p2->x))
			+	((p1->y - p2->y) * (p1->y - p2->y));
}

static inline float Q3Point2D_Distance(
		const TQ3Point2D *p1,
		const TQ3Point2D *p2)
{
	return sqrtf(Q3Point2D_DistanceSquared(p1, p2));
}

//-----------------------------------------------------------------------------
#pragma mark Q3Point3D

static inline float Q3Point3D_DistanceSquared(
		const TQ3Point3D *p1,
		const TQ3Point3D *p2)
{
	return		((p1->x - p2->x) * (p1->x - p2->x))
			+	((p1->y - p2->y) * (p1->y - p2->y))
			+	((p1->z - p2->z) * (p1->z - p2->z));
}

static inline float Q3Point3D_Distance(
		const TQ3Point3D *p1,
		const TQ3Point3D *p2)
{
	return sqrtf(Q3Point3D_DistanceSquared(p1, p2));
}

static inline void Q3Point3D_CrossProductTri(
		const TQ3Point3D* p1,
		const TQ3Point3D* p2,
		const TQ3Point3D* p3,
		TQ3Point3D* result)
{
	float v1_x = p2->x - p1->x;
	float v1_y = p2->y - p1->y;
	float v1_z = p2->z - p1->z;

	float v2_x = p3->x - p2->x;
	float v2_y = p3->y - p2->y;
	float v2_z = p3->z - p2->z;

	result->x = (v1_y * v2_z) - (v1_z * v2_y);
	result->y = (v1_z * v2_x) - (v1_x * v2_z);
	result->z = (v1_x * v2_y) - (v1_y * v2_x);
}

static inline TQ3Vector3D* Q3Vector3D_Transform(
		const TQ3Vector3D *vector3D,
		const TQ3Matrix4x4 *matrix4x4,
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

	return result;
}

static inline TQ3Point3D* Q3Point3D_Transform(
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

	return result;
}

static inline TQ3Point3D* Q3Point3D_TransformAffine(
		const TQ3Point3D *point3D,
		const TQ3Matrix4x4 *matrix4x4,
		TQ3Point3D *result)
{
	// Save input to avoid problems when result is same as input
	float x = point3D->x;
	float y = point3D->y;
	float z = point3D->z;

#define M(x,y) matrix4x4->value[x][y]
	result->x = x*M(0,0) + y*M(1,0) + z*M(2,0) + M(3,0);
	result->y = x*M(0,1) + y*M(1,1) + z*M(2,1) + M(3,1);
	result->z = x*M(0,2) + y*M(1,2) + z*M(2,2) + M(3,2);
#undef M

	return(result);
}

void Q3Point3D_To3DTransformArray(
		const TQ3Point3D		*inPoints3D,
		const TQ3Matrix4x4		*matrix4x4,
		TQ3Point3D				*outPoints3D,
		TQ3Uns32				numPoints);

//-----------------------------------------------------------------------------
#pragma mark Q3RationalPoint3D

static inline TQ3RationalPoint3D* Q3RationalPoint3D_Transform(
		const TQ3RationalPoint3D *rationalPoint3D,
		const TQ3Matrix3x3 *matrix3x3,
		TQ3RationalPoint3D *result)
{
	// Save input to avoid problems when result is same as input
	float x = rationalPoint3D->x;
	float y = rationalPoint3D->y;
	float w = rationalPoint3D->w;

#define M(x,y) matrix3x3->value[x][y]
	result->x = x*M(0,0) + y*M(1,0) + w*M(2,0);
	result->y = x*M(0,1) + y*M(1,1) + w*M(2,1);
	result->w = x*M(0,2) + y*M(1,2) + w*M(2,2);
#undef M

	return(result);
}

//-----------------------------------------------------------------------------
#pragma mark Q3Vector2D

static inline float Q3Vector2D_LengthSquared(
		const TQ3Vector2D* v)
{
	return (v->x * v->x) + (v->y * v->y);
}

static inline float Q3Vector2D_Length(
		const TQ3Vector2D* v)
{
	return sqrtf(Q3Vector2D_LengthSquared(v));
}

static inline void Q3Vector2D_Scale(
		const TQ3Vector2D* v1,
		float s,
		TQ3Vector2D* result)
{
	result->x = v1->x * s;
	result->y = v1->y * s;
}

static inline void Q3Vector2D_Normalize(
		const TQ3Vector2D* v1,
		TQ3Vector2D* result)
{
	// Normalization performance trick: usually, adding kQ3MinFloat to the length
	// makes no difference, but it prevents division by zero without a branch.
	float theLength = Q3Vector2D_Length(v1) + kQ3MinFloat;
	Q3Vector2D_Scale(v1, 1.0f / theLength, result);
}

//-----------------------------------------------------------------------------
#pragma mark Q3Vector3D

static inline float Q3Vector3D_LengthSquared(
		const TQ3Vector3D* v)
{
	return (v->x * v->x) + (v->y * v->y) + (v->z * v->z);
}

static inline float Q3Vector3D_Length(
		const TQ3Vector3D* v)
{
	return sqrtf(Q3Vector3D_LengthSquared(v));
}

static inline float Q3Vector2D_Dot(
		const TQ3Vector2D *v1,
		const TQ3Vector2D *v2)
{
	return (v1->x * v2->x) + (v1->y * v2->y);
}

static inline void Q3Vector3D_Cross(
		const TQ3Vector3D* v1,
		const TQ3Vector3D* v2,
		TQ3Vector3D* result)
{
	float rx = (v1->y * v2->z) - (v1->z * v2->y);
	float ry = (v1->z * v2->x) - (v1->x * v2->z);
	float rz = (v1->x * v2->y) - (v1->y * v2->x);

	result->x = rx;
	result->y = ry;
	result->z = rz;
}

static inline float Q3Vector3D_Dot(
		const TQ3Vector3D *v1,
		const TQ3Vector3D *v2)
{
	return		(v1->x * v2->x)
			+	(v1->y * v2->y)
			+	(v1->z * v2->z);
}

static inline void Q3Vector3D_Scale(
		const TQ3Vector3D *v1,
		float scale,
		TQ3Vector3D *result)
{
	result->x = v1->x * scale;
	result->y = v1->y * scale;
	result->z = v1->z * scale;
}

static inline void Q3Vector3D_Normalize(
		const TQ3Vector3D* v1,
		TQ3Vector3D* result)
{
	float theLength = Q3Vector3D_Length(v1) + kQ3MinFloat;
	Q3Vector3D_Scale(v1, 1.0f / theLength, result);
}

//-----------------------------------------------------------------------------
#pragma mark Q3Matrix3x3

static inline TQ3Matrix3x3* Q3Matrix3x3_SetIdentity(
		TQ3Matrix3x3 *matrix3x3)
{
	__Q3Memory_Clear(matrix3x3, sizeof(TQ3Matrix3x3));

#define M(x,y) matrix3x3->value[x][y]
	M(0,0) = 1.0f;
	M(1,1) = 1.0f;
	M(2,2) = 1.0f;
#undef M

	return matrix3x3;
}

static inline TQ3Matrix3x3* Q3Matrix3x3_SetTranslate(
		TQ3Matrix3x3* matrix3x3,
		float xTrans,
		float yTrans)
{
	__Q3Memory_Clear(matrix3x3, sizeof(TQ3Matrix3x3));

#define M(x,y) matrix3x3->value[x][y]
	M(0,0) = 1.0f;

	M(1,1) = 1.0f;

	M(2,0) = xTrans;
	M(2,1) = yTrans;
	M(2,2) = 1.0f;
#undef M

	return matrix3x3;
}

//-----------------------------------------------------------------------------
#pragma mark Q3Matrix4x4

static inline TQ3Matrix4x4* Q3Matrix4x4_SetIdentity(
		TQ3Matrix4x4 *matrix4x4)
{
	__Q3Memory_Clear(matrix4x4, sizeof(TQ3Matrix4x4));

#define M(x,y) matrix4x4->value[x][y]

	M(0,0) = 1.0f;
	M(1,1) = 1.0f;
	M(2,2) = 1.0f;
	M(3,3) = 1.0f;

#undef M

	return matrix4x4;
}

static inline TQ3Matrix4x4* Q3Matrix4x4_SetScale(
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

static inline TQ3Matrix4x4* Q3Matrix4x4_SetTranslate(
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

static inline TQ3Matrix4x4* Q3Matrix4x4_Multiply(
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

static inline TQ3Matrix4x4* Q3Matrix4x4_SetRotate_X(
		TQ3Matrix4x4 *matrix4x4,
		float angle)
{
	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);

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

static inline TQ3Matrix4x4* Q3Matrix4x4_SetRotate_Y(
		TQ3Matrix4x4 *matrix4x4,
		float angle)
{
	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);

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

static inline TQ3Matrix4x4* Q3Matrix4x4_SetRotate_Z(
		TQ3Matrix4x4 *matrix4x4,
		float angle)
{
	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);

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

static inline TQ3Matrix4x4* Q3Matrix4x4_SetRotate_XYZ(
		TQ3Matrix4x4 *matrix4x4,
		float xAngle,
		float yAngle,
		float zAngle)
{
	float cosX = cosf(xAngle);
	float sinX = sinf(xAngle);
	float cosY = cosf(yAngle);
	float sinY = sinf(yAngle);
	float cosZ = cosf(zAngle);
	float sinZ = sinf(zAngle);

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

TQ3Matrix4x4* Q3Matrix4x4_Transpose(
		const TQ3Matrix4x4 *matrix4x4,
		TQ3Matrix4x4 *result);

TQ3Matrix4x4* Q3Matrix4x4_Invert(
		const TQ3Matrix4x4 *inMatrix,
		TQ3Matrix4x4 *result);

#ifdef __cplusplus
}
#endif
