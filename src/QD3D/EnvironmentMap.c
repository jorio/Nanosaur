/****************************/
/* 	ENVIRONMENTMAP.C   		*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DMath.h>

#include "globals.h"
#include "qd3d_support.h"
#include "misc.h"
#include "environmentmap.h"
#include "objects.h"
#include "3dmath.h"

extern	QD3DSetupOutputType		*gGameViewInfoPtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

/****************************/
/*    CONSTANTS             */
/****************************/

#define ENVMAP_MAX_VERTICES_PER_MESH	5000

/*********************/
/*    VARIABLES      */
/*********************/

TQ3Vector3D				gEnvMapNormals[ENVMAP_MAX_VERTICES_PER_MESH];
TQ3Param2D				gEnvMapUVs[ENVMAP_MAX_VERTICES_PER_MESH];

/****************************** ENVIRONMENT MAP TRI MESH *****************************************/
//
// NOTE:  This assumes no face normals, thus if there are face normals, they won't be transformed and weird things will happen.
//
// After calling this function, draw your trimesh using:
//		- gEnvMapNormals as vertex normals (instead of the trimesh's actual normals)
//		- gEnvMapUVs as texture coordinates (instead of the trimesh's actual UVs)

void EnvironmentMapTriMesh(
		const TQ3TriMeshData *triMeshDataPtr,
		const TQ3Matrix4x4* transform)
{
TQ3Matrix4x4		invTranspose;
const TQ3Point3D*	camCoord = &gGameViewInfoPtr->cameraPlacement.cameraLocation;

	GAME_ASSERT(transform);

				/* GET TRIMESH INFO */

	int numVertices = triMeshDataPtr->numPoints;						// get # verts
	GAME_ASSERT(numVertices <= ENVMAP_MAX_VERTICES_PER_MESH);

				/* TRANSFORM VERTEX NORMALS */

	Q3Matrix4x4_Invert(transform, &invTranspose);						// calc inverse-transpose matrix
	Q3Matrix4x4_Transpose(&invTranspose, &invTranspose);

	GAME_ASSERT(triMeshDataPtr->vertexNormals);
	for (int vertNum = 0; vertNum < numVertices; vertNum++)				// transform all normals
	{
		Q3Vector3D_Transform(&triMeshDataPtr->vertexNormals[vertNum], &invTranspose, &gEnvMapNormals[vertNum]);
		Q3Vector3D_Normalize(&gEnvMapNormals[vertNum], &gEnvMapNormals[vertNum]);
	}

				/* TRANSFORM FACE NORMALS */

#if 0	// NOQUESA: no face normals in structure
	if (triMeshDataPtr->faceNormals)
	{
		for (long faceNum = 0; faceNum < numFaces; faceNum++)				// transform all normals
		{
			Q3Vector3D_Transform(&triMeshDataPtr->faceNormals[faceNum], &invTranspose, &normals2[faceNum]);
			Q3Vector3D_Normalize(&normals2[faceNum], &normals2[faceNum]);
		}
	}
#endif

		/****************************/
		/* CALC UVS FOR EACH VERTEX */
		/****************************/	

	if (triMeshDataPtr->hasTexture)											// only if has UV's
	{
		for (int vertNum = 0; vertNum < numVertices; vertNum++)
		{
			const TQ3Point3D* point = &triMeshDataPtr->points[vertNum];
			const TQ3Vector3D* surfaceNormal = &gEnvMapNormals[vertNum];

					/* CALC VECTOR TO VERTEX */

			TQ3Vector3D eyeVector;
			Q3Vector3D_Transform((TQ3Vector3D*)point, transform, &eyeVector);
			eyeVector.x -= camCoord->x;
			eyeVector.y -= camCoord->y;
			eyeVector.z -= camCoord->z;

					/* REFLECT VECTOR AROUND VERTEX NORMAL */
			// Reflection vector: N(2(N.V)) - V
			// N - Surface Normal
			// V - View Direction
			float dot = 2.0f * Q3Vector3D_Dot(surfaceNormal, &eyeVector);	// compute 2(N.V)
			TQ3Vector3D reflectedVector =
			{
				surfaceNormal->x * dot - eyeVector.x,
				surfaceNormal->y * dot - eyeVector.y,
				surfaceNormal->z * dot - eyeVector.z,
			};
			Q3Vector3D_Normalize(&reflectedVector, &reflectedVector);		// normalize result

						/* CALC UV */

			gEnvMapUVs[vertNum].u = (reflectedVector.x * .5f) + .5f;
			gEnvMapUVs[vertNum].v = (reflectedVector.y * .5f) + .5f;
		}
	}
}
