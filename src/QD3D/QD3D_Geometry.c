/****************************/
/*   	QD3D GEOMETRY.C	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void ExplodeTriMesh(const TQ3TriMeshData *mesh, const TQ3Matrix4x4* transform);


/****************************/
/*    CONSTANTS             */
/****************************/

static const TQ3Matrix4x4 kIdentity4x4 =
{{
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1},
}};


/*********************/
/*    VARIABLES      */
/*********************/

float		gBoomForce,gShardDecaySpeed;
Byte		gShardMode;
int			gShardDensity;

static RenderModifiers	gShardRenderMods;
static ShardType		gShardMemory[MAX_SHARDS];
static Pool				*gShardPool = NULL;



/*************** QD3D: CALC OBJECT BOUNDING BOX ************************/

void QD3D_CalcObjectBoundingBox(int numMeshes, TQ3TriMeshData** meshList, TQ3BoundingBox* boundingBox)
{
	GAME_ASSERT(numMeshes);
	GAME_ASSERT(meshList);
	GAME_ASSERT(boundingBox);

	boundingBox->isEmpty = true;
	boundingBox->min = (TQ3Point3D) { 0, 0, 0 };
	boundingBox->max = (TQ3Point3D) { 0, 0, 0 };

	for (int i = 0; i < numMeshes; i++)
	{
		TQ3TriMeshData* mesh = meshList[i];
		for (int v = 0; v < mesh->numPoints; v++)
		{
			TQ3Point3D p = mesh->points[v];

			if (boundingBox->isEmpty)
			{
				boundingBox->isEmpty = false;
				boundingBox->min = p;
				boundingBox->max = p;
			}
			else
			{
				if (p.x < boundingBox->min.x) boundingBox->min.x = p.x;
				if (p.y < boundingBox->min.y) boundingBox->min.y = p.y;
				if (p.z < boundingBox->min.z) boundingBox->min.z = p.z;

				if (p.x > boundingBox->max.x) boundingBox->max.x = p.x;
				if (p.y > boundingBox->max.y) boundingBox->max.y = p.y;
				if (p.z > boundingBox->max.z) boundingBox->max.z = p.z;
			}
		}
	}
}



/****************** QD3D: CALC OBJECT RADIUS ***********************/
//
// Given any object as input, calculates the radius based on the farthest TriMesh vertex.
//

float QD3D_CalcObjectRadius(int numMeshes, TQ3TriMeshData** meshList)
{
	float maxRadius = 0;

	for (int i = 0; i < numMeshes; i++)
	{
		const TQ3Point3D* points = meshList[i]->points;
		for (int v = 0; v < meshList[i]->numPoints; v++)	// scan thru all verts
		{
			TQ3Vector3D vector = {points[v].x, points[v].y, points[v].z};
			float dist = Q3Vector3D_Length(&vector);		// calc dist
			if (dist > maxRadius)
				maxRadius = dist;
		}
	}

	return maxRadius;
}

//===================================================================================================
//===================================================================================================
//===================================================================================================

#pragma mark =========== shard explosion ==============


/********************** QD3D: INIT SHARDS **********************/

void QD3D_InitShards(void)
{
	GAME_ASSERT(!gShardPool);
	gShardPool = Pool_New(MAX_SHARDS);

	for (int i = 0; i < MAX_SHARDS; i++)
	{
		ShardType* shard = &gShardMemory[i];
		shard->mesh = Q3TriMeshData_New(1, 3, kQ3TriMeshDataFeatureVertexUVs | kQ3TriMeshDataFeatureVertexNormals);
		for (int v = 0; v < 3; v++)
			shard->mesh->triangles[0].pointIndices[v] = v;
	}

	Render_SetDefaultModifiers(&gShardRenderMods);
	gShardRenderMods.statusBits |= STATUS_BIT_KEEPBACKFACES;			// draw both faces on shards
}


void QD3D_DisposeShards(void)
{
	GAME_ASSERT(gShardPool);
	Pool_Free(gShardPool);
	gShardPool = NULL;

	for (int i = 0; i < MAX_SHARDS; i++)
	{
		ShardType* shard = &gShardMemory[i];
		if (shard->mesh)
		{
			Q3TriMeshData_Dispose(shard->mesh);
			shard->mesh = nil;
		}
	}
}


/****************** QD3D: EXPLODE GEOMETRY ***********************/
//
// Given any object as input, breaks up all polys into separate objNodes &
// calculates velocity et.al.
//

void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte shardMode, int shardDensity, float shardDecaySpeed)
{
	if (theNode->CType == INVALID_NODE_FLAG)				// make sure the node is valid
		return;


	gBoomForce = boomForce;
	gShardMode = shardMode;
	gShardDensity = shardDensity;
	gShardDecaySpeed = shardDecaySpeed;


			/* SEE IF EXPLODING SKELETON OR PLAIN GEOMETRY */

	const TQ3Matrix4x4* transform;

	if (theNode->Genre == SKELETON_GENRE)
	{
		transform = &kIdentity4x4;							// init to identity matrix (skeleton vertices are pre-transformed)
	}
	else
	{
		transform = &theNode->BaseTransformMatrix;			// static object: set pos/rot/scale from its base transform matrix
	}


			/* EXPLODE EACH TRIMESH INDIVIDUALLY */

	for (int i = 0; i < theNode->NumMeshes; i++)
	{
		ExplodeTriMesh(theNode->MeshList[i], transform);
	}
}


/********************** EXPLODE TRIMESH *******************************/

static void ExplodeTriMesh(const TQ3TriMeshData *inMesh, const TQ3Matrix4x4* transform)
{
TQ3Point3D			centerPt = {0,0,0};

	GAME_ASSERT(gShardPool);

			/*******************************/
			/* SCAN THRU ALL TRIMESH FACES */
			/*******************************/
					
	for (int t = 0; t < inMesh->numTriangles; t += gShardDensity)	// scan thru all faces
	{
				/* GET FREE SHARD INDEX */

		int shardIndex = Pool_AllocateIndex(gShardPool);
		if (shardIndex < 0)													// see if all out
			break;

		ShardType* shard = &gShardMemory[shardIndex];
		TQ3TriMeshData* sMesh = shard->mesh;

		const uint32_t* ind = inMesh->triangles[t].pointIndices;						// get indices of 3 points

				/*********************/
				/* INIT TRIMESH DATA */
				/*********************/

				/* DO POINTS */

		for (int v = 0; v < 3; v++)
		{
			Q3Point3D_Transform(&inMesh->points[ind[v]], transform, &sMesh->points[v]);		// transform points
		}

		centerPt.x = (sMesh->points[0].x + sMesh->points[1].x + sMesh->points[2].x) * 0.3333f;		// calc center of polygon
		centerPt.y = (sMesh->points[0].y + sMesh->points[1].y + sMesh->points[2].y) * 0.3333f;
		centerPt.z = (sMesh->points[0].z + sMesh->points[1].z + sMesh->points[2].z) * 0.3333f;

		for (int v = 0; v < 3; v++)
		{
			sMesh->points[v].x -= centerPt.x;											// offset coords to be around center
			sMesh->points[v].y -= centerPt.y;
			sMesh->points[v].z -= centerPt.z;
		}

		sMesh->bBox.min = sMesh->bBox.max = centerPt;
		sMesh->bBox.isEmpty = kQ3False;

				/* DO VERTEX NORMALS */

		for (int v = 0; v < 3; v++)
		{
			Q3Vector3D_Transform(&inMesh->vertexNormals[ind[v]], transform, &sMesh->vertexNormals[v]);		// transform normals
			Q3Vector3D_Normalize(&sMesh->vertexNormals[v], &sMesh->vertexNormals[v]);						// normalize normals
		}

				/* DO VERTEX UV'S */

		sMesh->texturingMode = inMesh->texturingMode;
		if (inMesh->texturingMode != kQ3TexturingModeOff)				// see if also has UV
		{
			for (int v = 0; v < 3; v++)									// get vertex u/v's
			{
				sMesh->vertexUVs[v] = inMesh->vertexUVs[ind[v]];
			}
			sMesh->glTextureName = inMesh->glTextureName;
			sMesh->internalTextureID = inMesh->internalTextureID;
		}


				/* DO VERTEX COLORS */

		sMesh->diffuseColor = inMesh->diffuseColor;

		sMesh->hasVertexColors = inMesh->hasVertexColors;				// has per-vertex colors?
		if (inMesh->hasVertexColors)
		{
			for (int v = 0; v < 3; v++)									// get per-vertex colors
			{
				sMesh->vertexColors[v] = inMesh->vertexColors[ind[v]];
			}
		}


			/*********************/
			/* SET PHYSICS STUFF */
			/*********************/

		shard->coord = centerPt;
		shard->rot = (TQ3Vector3D) {0,0,0};
		shard->scale = 1.0f;
		
		shard->coordDelta.x = (RandomFloat() - 0.5f) * gBoomForce;
		shard->coordDelta.y = (RandomFloat() - 0.5f) * gBoomForce;
		shard->coordDelta.z = (RandomFloat() - 0.5f) * gBoomForce;
		if (gShardMode & SHARD_MODE_UPTHRUST)
			shard->coordDelta.y += 1.5f * gBoomForce;

		shard->rotDelta.x = (RandomFloat() - 0.5f) * 4.0f;			// random rotation deltas
		shard->rotDelta.y = (RandomFloat() - 0.5f) * 4.0f;
		shard->rotDelta.z = (RandomFloat() - 0.5f) * 4.0f;

		shard->decaySpeed = gShardDecaySpeed;
		shard->mode = gShardMode;
	}
}


/************************** QD3D: MOVE SHARDS ****************************/

void QD3D_MoveShards(void)
{
float	ty,y,fps,x,z;
TQ3Matrix4x4	matrix,matrix2;

	if (!gShardPool || Pool_Empty(gShardPool))						// quick check if any shards at all
		return;

	fps = gFramesPerSecondFrac;

	for (int i = Pool_First(gShardPool); i >= 0; )
	{
		int nextIndex = Pool_Next(gShardPool, i);

		ShardType* shard = &gShardMemory[i];

				/* ROTATE IT */

		shard->rot.x += shard->rotDelta.x * fps;
		shard->rot.y += shard->rotDelta.y * fps;
		shard->rot.z += shard->rotDelta.z * fps;
					
					/* MOVE IT */
					
		if (shard->mode & SHARD_MODE_HEAVYGRAVITY)
			shard->coordDelta.y -= fps * GRAVITY_CONSTANT/2;		// gravity
		else
			shard->coordDelta.y -= fps * GRAVITY_CONSTANT/3;		// gravity
			
		x = (shard->coord.x += shard->coordDelta.x * fps);
		y = (shard->coord.y += shard->coordDelta.y * fps);
		z = (shard->coord.z += shard->coordDelta.z * fps);

					/* SEE IF BOUNCE */
					
		ty = GetTerrainHeightAtCoord_Quick(x,z);				// get terrain height here
		if (y <= ty)
		{
			if (shard->mode & SHARD_MODE_BOUNCE)
			{
				shard->coord.y  = ty;
				shard->coordDelta.y *= -0.5;
				shard->coordDelta.x *= 0.9;
				shard->coordDelta.z *= 0.9;
			}
			else
				goto del;	// nuke it
		}
		
					/* SCALE IT */
					
		shard->scale -= shard->decaySpeed * fps;
		if (shard->scale <= 0.0f)
		{
			goto del;		// nuke it
		}

			/***************************/
			/* UPDATE TRANSFORM MATRIX */
			/***************************/
			

				/* SET SCALE MATRIX */

		Q3Matrix4x4_SetScale(&shard->matrix, shard->scale,	shard->scale, shard->scale);
	
					/* NOW ROTATE IT */

		Q3Matrix4x4_SetRotate_XYZ(&matrix, shard->rot.x, shard->rot.y, shard->rot.z);
		Q3Matrix4x4_Multiply(&shard->matrix,&matrix, &matrix2);
	
					/* NOW TRANSLATE IT */

		Q3Matrix4x4_SetTranslate(&matrix, shard->coord.x, shard->coord.y, shard->coord.z);
		Q3Matrix4x4_Multiply(&matrix2,&matrix, &shard->matrix);

		goto next;

del:
		Pool_ReleaseIndex(gShardPool, i);

next:
		i = nextIndex;
	}
}


/************************* QD3D: DRAW SHARDS ****************************/

void QD3D_DrawShards(QD3DSetupOutputType *setupInfo)
{
	(void) setupInfo;

	if (!gShardPool)												// quick check if any shards at all
		return;

	for (int i = Pool_First(gShardPool); i >= 0; i = Pool_Next(gShardPool, i))
	{
		ShardType* shard = &gShardMemory[i];
		Render_SubmitMesh(shard->mesh, &shard->matrix, &gShardRenderMods, &shard->coord);
	}
}



//============================================================================================
//============================================================================================
//============================================================================================






/****************** QD3D: SCROLL UVs ***********************/
//
// Given any object as input this will scroll any u/v coordinates by the given amount
//

void QD3D_ScrollUVs(int numMeshes, TQ3TriMeshData** meshList, float rawDeltaU, float rawDeltaV)
{
	for (int i = 0; i < numMeshes; i++)
	{
		TQ3TriMeshData* mesh = meshList[i];

				/* SEE IF HAS A TEXTURE */

		if (mesh->texturingMode == kQ3TexturingModeOff)
			continue;

		GAME_ASSERT(mesh->vertexUVs);

		for (int j = 0; j < mesh->numPoints; j++)
		{
			mesh->vertexUVs[j].u += rawDeltaU;
			mesh->vertexUVs[j].v -= rawDeltaV;
		}
	}
}
