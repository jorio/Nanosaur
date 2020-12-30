// PATCH 3DMF MODELS.C
// (C) 2020 Iliyas Jorio
// This file is part of Nanosaur. https://github.com/jorio/nanosaur

/*
 * These functions tweak some models from the game
 * on a case-by-case basis in order to:
 *
 * - enhance performance by adding add an alpha test
 *   so they aren't considered transparent by Quesa
 *   (which would knock them off the fast rendering path)
 *
 * - make the game look better by removing unsightly seams
 *   by adding UV clamping
 */

/****************************/
/*    EXTERNALS             */
/****************************/

#include "Pomme.h"
#include "GamePatches.h"
#include <Quesa.h>
#include <QuesaGeometry.h>
#include <QuesaGroup.h>
#include <QuesaShader.h>
#include <misc.h>



/****************************/
/*    CONSTANTS             */
/****************************/

#define OBJTREE_FRONTIER_STACK_LENGTH 64

const TQ3Float32 gTextureAlphaThreshold = 0.501111f;		// We really want 0.5, but the weird number makes it easier to spot when debugging Quesa.


/****************************/
/*    STATIC FUNCTIONS      */
/****************************/



/********************** RUN CALLBACK FOR EACH TRIMESH ***********************/
//
// Convenience function that traverses the object graph and runs the given callback on all trimeshes.
//

static void ForEachTriMesh(
		TQ3Object root,
		void (*callback)(TQ3TriMeshData triMeshData, void* userData),
		void* userData,
		uint64_t triMeshMask)
{
	TQ3Object	frontier[OBJTREE_FRONTIER_STACK_LENGTH];
	int			top = 0;

	frontier[top] = root;

	unsigned long nFoundTriMeshes = 0;

	while (top >= 0)
	{
		TQ3Object obj = frontier[top];
		frontier[top] = nil;
		top--;

		if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry) &&
			Q3Geometry_GetType(obj) == kQ3GeometryTypeTriMesh)		// must be trimesh
		{
			if (triMeshMask & 1)
			{
				TQ3TriMeshData		triMeshData;
				TQ3Status			status;

				status = Q3TriMesh_GetData(obj, &triMeshData);		// get trimesh data
				if (!status) DoFatalAlert("Q3TriMesh_GetData failed!");

				callback(triMeshData, userData);

				Q3TriMesh_EmptyData(&triMeshData);
			}

			triMeshMask >>= 1;
			nFoundTriMeshes++;
		}
		else if (Q3Object_IsType(obj, kQ3ShapeTypeShader) &&
				 Q3Shader_GetType(obj) == kQ3ShaderTypeSurface)		// must be texture surface shader
		{
			DoAlert("Implement me?");
		}
		else if (Q3Object_IsType(obj, kQ3ShapeTypeGroup))			// SEE IF RECURSE SUB-GROUP
		{
			TQ3GroupPosition pos = nil;
			Q3Group_GetFirstPosition(obj, &pos);

			while (pos)												// scan all objects in group
			{
				TQ3Object child;
				Q3Group_GetPositionObject(obj, pos, &child);		// get object from group
				if (child)
				{
					top++;
					if (top >= OBJTREE_FRONTIER_STACK_LENGTH) DoFatalAlert("objtree frontier stack overflow");
					frontier[top] = child;
				}
				Q3Group_GetNextPosition(obj, &pos);
			}
		}

		if (obj != root)
		{
			Q3Object_Dispose(obj);						// dispose local ref
		}
	}

	if (nFoundTriMeshes > 8*sizeof(triMeshMask))
	{
		DoAlert("This group contains more trimeshes than can fit in the mask.");
	}
}


/**************************** QD3D: SET TEXTURE ALPHA THRESHOLD *******************************/
//
// Instructs Quesa to discard transparent texels so meshes don't get knocked off fast rendering path.
//
// The game has plenty of models with textures containing texels that are either fully-opaque
// or fully-transparent (e.g.: the ladybug's eyelashes and wings).
//
// Normally, textures that are not fully-opaque would make Quesa consider the entire mesh
// as having transparency. This would take the mesh out of the fast rendering path, because Quesa
// needs to depth sort the triangles in transparent meshes.
//
// In the case of textures that are either fully-opaque or fully-transparent, though, we don't
// need that extra depth sorting. The transparent texels can simply be discarded instead.

static void SetAlphaTest(TQ3TriMeshData triMeshData, void* userData_thresholdFloatPtr)
{
	// SEE IF HAS A TEXTURE
	if (Q3AttributeSet_Contains(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader))
	{
		TQ3SurfaceShaderObject	shader;
		TQ3TextureObject		texture;
		TQ3Mipmap				mipmap;
		TQ3Status				status;

		status = Q3AttributeSet_Get(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader, &shader);
		if (!status) DoFatalAlert("Q3AttributeSet_Get failed!");

		/* ADD ALPHA TEST ELEMENT */

		status = Q3Object_AddElement(shader, kQ3ElementTypeTextureShaderAlphaTest, userData_thresholdFloatPtr);
		if (!status) DoFatalAlert("Q3Object_AddElement failed!");

		/* GET MIPMAP & APPLY EDGE PADDING TO IMAGE */
		/* (TO AVOID BLACK SEAMS WHERE TEXELS ARE BEING DISCARDED) */

		status = Q3TextureShader_GetTexture(shader, &texture);
		if (!status) DoFatalAlert("Q3TextureShader_GetTexture failed!");

		status = Q3MipmapTexture_GetMipmap(texture, &mipmap);
		if (!status) DoFatalAlert("Q3MipmapTexture_GetMipmap failed!");

		if (mipmap.pixelType == kQ3PixelTypeARGB16 ||			// Edge padding only effective if image has alpha channel
			mipmap.pixelType == kQ3PixelTypeARGB32)
		{
//			ApplyEdgePadding(&mipmap);
		}

		/* DISPOSE REFS */

		Q3Object_Dispose(texture);
		Q3Object_Dispose(shader);
	}
}

/****************************/
/*    PUBLIC FUNCTIONS      */
/****************************/

void PatchSkeleton3DMF(const char* cName, TQ3Object newModel)
{
	// Discard transparent texels for performance
	ForEachTriMesh(newModel, SetAlphaTest, (void *) &gTextureAlphaThreshold, ~0ull);
}

void PatchGrouped3DMF(const char* cName, TQ3Object* objects, int nObjects)
{
	/*********************************************************************/
	/* DISCARD TRANSPARENT TEXELS FOR PERFORMANCE (ALPHA TESTING)        */
	/*********************************************************************/

	for (int i = 0; i < nObjects; i++)
	{
		ForEachTriMesh(objects[i], SetAlphaTest, (void *) &gTextureAlphaThreshold, ~0ull);
	}
}
