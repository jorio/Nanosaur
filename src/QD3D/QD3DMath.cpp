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

#include "QD3DMath.h"

void Q3Point3D_To3DTransformArray(
		const TQ3Point3D		*inPoints3D,
		const TQ3Matrix4x4		*matrix4x4,
		TQ3Point3D				*outPoints3D,
		TQ3Uns32				numPoints)
{
	TQ3Uns32 i;

	// In the common case of the last column of the matrix being (0, 0, 0, 1),
	// we can avoid some divisions and conditionals inside the loop.
	if ( (matrix4x4->value[3][3] == 1.0f) &&
		 (matrix4x4->value[0][3] == 0.0f) &&
		 (matrix4x4->value[1][3] == 0.0f) &&
		 (matrix4x4->value[2][3] == 0.0f) )
	{
		for (i = 0; i < numPoints; ++i)
		{
			Q3Point3D_TransformAffine( inPoints3D, matrix4x4, outPoints3D );
			inPoints3D++;
			outPoints3D++;
		}
	}
	else
	{
		// Transform the points - will be in-lined in release builds
		for (i = 0; i < numPoints; ++i)
		{
			Q3Point3D_Transform(inPoints3D, matrix4x4, outPoints3D);
			inPoints3D++;
			outPoints3D++;
		}
	}
}

//-----------------------------------------------------------------------------
#pragma mark Q3Matrix4x4

TQ3Matrix4x4* Q3Matrix4x4_Transpose(
		const TQ3Matrix4x4 *matrix4x4,
		TQ3Matrix4x4 *result)
{
	int i, j;

	if (result != matrix4x4)
	{
		for (i = 0; i < 4; ++i)
			for (j = 0; j < 4; ++j)
				result->value[i][j] = matrix4x4->value[j][i];
	}
	else
	{
		__Q3Float_Swap(result->value[1][0], result->value[0][1]);
		__Q3Float_Swap(result->value[2][0], result->value[0][2]);
		__Q3Float_Swap(result->value[3][0], result->value[0][3]);
		__Q3Float_Swap(result->value[2][1], result->value[1][2]);
		__Q3Float_Swap(result->value[3][1], result->value[1][3]);
		__Q3Float_Swap(result->value[2][3], result->value[3][2]);
	}
	return(result);
}


//=============================================================================
//		e3matrix4x4_extract3x3 : Select the upper left 3x3 of a 4x4 matrix
//-----------------------------------------------------------------------------
static void e3matrix4x4_extract3x3( const TQ3Matrix4x4& in4x4, TQ3Matrix3x3& out3x3 )
{
	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; ++col)
		{
			out3x3.value[row][col] = in4x4.value[row][col];
		}
	}
}

//=============================================================================
//		e3matrix3x3_invert : Transforms the given 3x3 matrix into its inverse.
//-----------------------------------------------------------------------------
//      Note :  This function uses Gauss-Jordon elimination with full pivoting
//              to transform the given matrix to the identity matrix while
//              transforming the identity matrix to the inverse. As the given
//              matrix is reduced to 1's and 0's column-by-column, the inverse
//              matrix is created in its place column-by-column.
//
//              See Press, et al., "Numerical Recipes in C", 2nd ed., pp. 32 ff.
//-----------------------------------------------------------------------------
static void e3matrix3x3_invert(TQ3Matrix3x3* a)
{
#define A(x,y) a->value[x][y]

	TQ3Int32 irow = 0, icol = 0;
	TQ3Int32 i, j, k;       // *** WARNING: 'k' must be a SIGNED integer ***
	float big, element;
	TQ3Int32 ipiv[3], indxr[3], indxc[3];

	// Initialize ipiv: ipiv[j] is 0 (1) if row/column j has not (has) been pivoted
	for (j = 0; j < 3; ++j)
		ipiv[j] = 0;

	// Loop over 3 pivots
	for (k = 0; k < 3; ++k)
	{
		// Search unpivoted part of matrix for largest element to pivot on
		big = -1.0f;
		for (i = 0; i < 3; ++i)
		{
			if (ipiv[i])
				continue;

			for (j = 0; j < 3; ++j)
			{
				if (ipiv[j])
					continue;

				// Calculate absolute value of current element
				element = A(i,j);
				if (element < 0.0f)
					element = -element;

				// Compare current element to largest element so far
				if (element > big)
				{
					big = element;
					irow = i;
					icol = j;
				}
			}
		}

		// If largest element is 0, the matrix is singular
		// (If there are "nan" values in the matrix, "big" may still be -1.0.)
		if (big <= 0.0f)
		{
			throw std::runtime_error("e3matrix3x3_invert: non-invertible matrix");
			return;
		}

		// Mark pivot row and column
		++ipiv[icol];
		indxr[k] = irow;
		indxc[k] = icol;

		// If necessary, exchange rows to put pivot element on diagonal
		if (irow != icol)
		{
			for (j = 0; j < 3; ++j)
				__Q3Float_Swap(A(irow,j), A(icol,j));
		}

		// Divide pivot row by pivot element
		//
		// Note: If we were dividing by the same element many times, it would
		// make sense to multiply by its inverse. Since we divide by a given
		// elemen only 3 (4) times for a 3x3 (4x4) matrix, it doesn't make sense
		// to pay for the extra floating-point operation.
		element = A(icol,icol);
		A(icol,icol) = 1.0f;    // overwrite original matrix with inverse
		for (j = 0; j < 3; ++j)
			A(icol,j) /= element;

		// Reduce other rows
		for (i = 0; i < 3; ++i)
		{
			if (i == icol)
				continue;

			element = A(i,icol);
			A(i,icol) = 0.0f; // overwrite original matrix with inverse
			for (j = 0; j < 3; ++j)
				A(i,j) -= A(icol,j)*element;
		}
	}

	// Permute columns
	for (k = 3; --k >= 0; )     // *** WARNING: 'k' must be a SIGNED integer ***
	{
		if (indxr[k] != indxc[k])
		{
			for (i = 0; i < 3; ++i)
				__Q3Float_Swap(A(i,indxr[k]), A(i,indxc[k]));
		}
	}

#undef A
}

//=============================================================================
//		e3matrix4x4_invert : Transforms the given 4x4 matrix into its inverse.
//-----------------------------------------------------------------------------
//      Note :  This function uses Gauss-Jordon elimination with full pivoting
//              to transform the given matrix to the identity matrix while
//              transforming the identity matrix to the inverse. As the given
//              matrix is reduced to 1's and 0's column-by-column, the inverse
//              matrix is created in its place column-by-column.
//
//              See Press, et al., "Numerical Recipes in C", 2nd ed., pp. 32 ff.
//-----------------------------------------------------------------------------
static void e3matrix4x4_invert(TQ3Matrix4x4* a)
{
#define A(x,y) a->value[x][y]

	TQ3Int32 irow = 0, icol = 0;
	TQ3Int32 i, j, k;       // *** WARNING: 'k' must be a SIGNED integer ***
	float big, element;
	TQ3Int32 ipiv[4], indxr[4], indxc[4];

	// Initialize ipiv: ipiv[j] is 0 (1) if row/column j has not (has) been pivoted
	for (j = 0; j < 4; ++j)
		ipiv[j] = 0;

	// Loop over 4 pivots
	for (k = 0; k < 4; ++k)
	{
		// Search unpivoted part of matrix for largest element to pivot on
		big = -1.0f;
		for (i = 0; i < 4; ++i)
		{
			if (ipiv[i])
				continue;

			for (j = 0; j < 4; ++j)
			{
				if (ipiv[j])
					continue;

				// Calculate absolute value of current element
				element = A(i,j);
				if (element < 0.0f)
					element = -element;

				// Compare current element to largest element so far
				if (element > big)
				{
					big = element;
					irow = i;
					icol = j;
				}
			}
		}

		// If largest element is 0, the matrix is singular
		// (If there are "nan" values in the matrix, "big" may still be -1.0.)
		if (big <= 0.0f)
		{
			throw std::runtime_error("e3matrix4x4_invert: non-invertible matrix");
			return;
		}

		// Mark pivot row and column
		++ipiv[icol];
		indxr[k] = irow;
		indxc[k] = icol;

		// If necessary, exchange rows to put pivot element on diagonal
		if (irow != icol)
		{
			for (j = 0; j < 4; ++j)
				__Q3Float_Swap(A(irow,j), A(icol,j));
		}

		// Divide pivot row by pivot element
		//
		// Note: If we were dividing by the same element many times, it would
		// make sense to multiply by its inverse. Since we divide by a given
		// element only 3 (4) times for a 3x3 (4x4) matrix, it doesn't make sense
		// to pay for the extra floating-point operation.
		element = A(icol,icol);
		A(icol,icol) = 1.0f;    // overwrite original matrix with inverse
		for (j = 0; j < 4; ++j)
			A(icol,j) /= element;

		// Reduce other rows
		for (i = 0; i < 4; ++i)
		{
			if (i == icol)
				continue;

			element = A(i,icol);
			A(i,icol) = 0.0f; // overwrite original matrix with inverse
			for (j = 0; j < 4; ++j)
				A(i,j) -= A(icol,j)*element;
		}
	}

	// Permute columns
	for (k = 4; --k >= 0; )     // *** WARNING: 'k' must be a SIGNED integer ***
	{
		if (indxr[k] != indxc[k])
		{
			for (i = 0; i < 4; ++i)
				__Q3Float_Swap(A(i,indxr[k]), A(i,indxc[k]));
		}
	}

#undef A
}

TQ3Matrix4x4* Q3Matrix4x4_Invert(
		const TQ3Matrix4x4 *matrix4x4,
		TQ3Matrix4x4 *result)
{
	if (result != matrix4x4)
		*result = *matrix4x4;

	// The 4x4 matrices used in 3D graphics often have a last column of
	// (0, 0, 0, 1).  In that case, we want the inverse to have exactly the same
	// last column, and we can compute the inverse with fewer floating point
	// multiplies and divides.  The inverse of the matrix
	//		A 0
	//		v 1
	// (where A is 3x3 and v is 1x3) is
	//		inv(A)			0
	//		-v * inv(A)		1	.
	if ( (result->value[3][3] == 1.0f) && (result->value[0][3] == 0.0f) &&
		 (result->value[1][3] == 0.0f) && (result->value[2][3] == 0.0f) )
	{
		TQ3Matrix3x3	upperLeft;
		e3matrix4x4_extract3x3( *result, upperLeft );
		int	i, j;

		e3matrix3x3_invert( &upperLeft );

		for (i = 0; i < 3; ++i)
		{
			for (j = 0; j < 3; ++j)
			{
				result->value[i][j] = upperLeft.value[i][j];
			}
		}

		TQ3RationalPoint3D	v = {
				result->value[3][0], result->value[3][1], result->value[3][2]
		};
		Q3RationalPoint3D_Transform( &v, &upperLeft, &v );

		result->value[3][0] = -v.x;
		result->value[3][1] = -v.y;
		result->value[3][2] = -v.w;
	}
	else
	{
		e3matrix4x4_invert(result);
	}

	return(result);
}
