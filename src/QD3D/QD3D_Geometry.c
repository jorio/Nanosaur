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

float		gBoomForce,gParticleDecaySpeed;
Byte		gParticleMode;
long		gParticleDensity;

long				gNumParticles = 0;
ParticleType		gParticles[MAX_PARTICLES];

static RenderModifiers kParticleRenderingMods;



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
		TQ3TriMeshData* mesh = meshList[i];
		for (int v = 0; v < mesh->numPoints; v++)						// scan thru all verts
		{
			TQ3Point3D tmPoint = mesh->points[v];						// get point

			float dist = Q3Vector3D_Length((TQ3Vector3D*) &tmPoint);	// calc dist
			if (dist > maxRadius)
				maxRadius = dist;
		}
	}

	return maxRadius;
}

//===================================================================================================
//===================================================================================================
//===================================================================================================

#pragma mark =========== particle explosion ==============


/********************** QD3D: INIT PARTICLES **********************/

void QD3D_InitParticles(void)
{
	gNumParticles = 0;

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		ParticleType* particle = &gParticles[i];
		particle->isUsed = false;
		particle->mesh = Q3TriMeshData_New(1, 3, kQ3TriMeshDataFeatureVertexUVs | kQ3TriMeshDataFeatureVertexNormals);
		for (int v = 0; v < 3; v++)
			particle->mesh->triangles[0].pointIndices[v] = v;
	}

	Render_SetDefaultModifiers(&kParticleRenderingMods);
	kParticleRenderingMods.statusBits |= STATUS_BIT_KEEPBACKFACES;			// draw both faces on particles
}


/********************* FIND FREE PARTICLE ***********************/
//
// OUTPUT: -1 == none free found
//

static inline long FindFreeParticle(void)
{
long	i;

	if (gNumParticles >= MAX_PARTICLES)
		return(-1);

	for (i = 0; i < MAX_PARTICLES; i++)
		if (gParticles[i].isUsed == false)
			return(i);

	return(-1);
}


/****************** QD3D: EXPLODE GEOMETRY ***********************/
//
// Given any object as input, breaks up all polys into separate objNodes &
// calculates velocity et.al.
//

void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed)
{
	if (theNode->CType == INVALID_NODE_FLAG)				// make sure the node is valid
		return;


	gBoomForce = boomForce;
	gParticleMode = particleMode;
	gParticleDensity = particleDensity;
	gParticleDecaySpeed = particleDecaySpeed;


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

			/*******************************/
			/* SCAN THRU ALL TRIMESH FACES */
			/*******************************/
					
	for (int t = 0; t < inMesh->numTriangles; t += gParticleDensity)	// scan thru all faces
	{
				/* GET FREE PARTICLE INDEX */

		long particleIndex = FindFreeParticle();
		if (particleIndex == -1)													// see if all out
			break;

		ParticleType* particle = &gParticles[particleIndex];
		TQ3TriMeshData* pMesh = particle->mesh;

		const uint16_t* ind = inMesh->triangles[t].pointIndices;						// get indices of 3 points

				/*********************/
				/* INIT TRIMESH DATA */
				/*********************/

				/* DO POINTS */

		for (int v = 0; v < 3; v++)
		{
			Q3Point3D_Transform(&inMesh->points[ind[v]], transform, &pMesh->points[v]);		// transform points
		}

		centerPt.x = (pMesh->points[0].x + pMesh->points[1].x + pMesh->points[2].x) * 0.3333f;		// calc center of polygon
		centerPt.y = (pMesh->points[0].y + pMesh->points[1].y + pMesh->points[2].y) * 0.3333f;
		centerPt.z = (pMesh->points[0].z + pMesh->points[1].z + pMesh->points[2].z) * 0.3333f;

		for (int v = 0; v < 3; v++)
		{
			pMesh->points[v].x -= centerPt.x;											// offset coords to be around center
			pMesh->points[v].y -= centerPt.y;
			pMesh->points[v].z -= centerPt.z;
		}

		pMesh->bBox.min = pMesh->bBox.max = centerPt;
		pMesh->bBox.isEmpty = kQ3False;

				/* DO VERTEX NORMALS */

		for (int v = 0; v < 3; v++)
		{
			Q3Vector3D_Transform(&inMesh->vertexNormals[ind[v]], transform, &pMesh->vertexNormals[v]);		// transform normals
			Q3Vector3D_Normalize(&pMesh->vertexNormals[v], &pMesh->vertexNormals[v]);						// normalize normals
		}

				/* DO VERTEX UV'S */

		pMesh->texturingMode = inMesh->texturingMode;
		if (inMesh->texturingMode != kQ3TexturingModeOff)				// see if also has UV
		{
			for (int v = 0; v < 3; v++)									// get vertex u/v's
			{
				pMesh->vertexUVs[v] = inMesh->vertexUVs[ind[v]];
			}
			pMesh->glTextureName = inMesh->glTextureName;
			pMesh->internalTextureID = inMesh->internalTextureID;
		}


				/* DO VERTEX COLORS */

		pMesh->diffuseColor = inMesh->diffuseColor;

		pMesh->hasVertexColors = inMesh->hasVertexColors;				// has per-vertex colors?
		if (inMesh->hasVertexColors)
		{
			for (int v = 0; v < 3; v++)									// get per-vertex colors
			{
				pMesh->vertexColors[v] = inMesh->vertexColors[ind[v]];
			}
		}


			/*********************/
			/* SET PHYSICS STUFF */
			/*********************/

		particle->coord = centerPt;
		particle->rot = (TQ3Vector3D) {0,0,0};
		particle->scale = 1.0f;
		
		particle->coordDelta.x = (RandomFloat() - 0.5f) * gBoomForce;
		particle->coordDelta.y = (RandomFloat() - 0.5f) * gBoomForce;
		particle->coordDelta.z = (RandomFloat() - 0.5f) * gBoomForce;
		if (gParticleMode & PARTICLE_MODE_UPTHRUST)
			particle->coordDelta.y += 1.5f * gBoomForce;

		particle->rotDelta.x = (RandomFloat() - 0.5f) * 4.0f;			// random rotation deltas
		particle->rotDelta.y = (RandomFloat() - 0.5f) * 4.0f;
		particle->rotDelta.z = (RandomFloat() - 0.5f) * 4.0f;

		particle->decaySpeed = gParticleDecaySpeed;
		particle->mode = gParticleMode;

				/* SET VALID & INC COUNTER */

		particle->isUsed = true;
		gNumParticles++;
	}
}


/************************** QD3D: MOVE PARTICLES ****************************/

void QD3D_MoveParticles(void)
{
float	ty,y,fps,x,z;
long	i;
TQ3Matrix4x4	matrix,matrix2;

	if (gNumParticles == 0)												// quick check if any particles at all
		return;

	fps = gFramesPerSecondFrac;

	for (i=0; i < MAX_PARTICLES; i++)
	{
		if (!gParticles[i].isUsed)										// source port fix
			continue;

		ParticleType* particle = &gParticles[i];

				/* ROTATE IT */

		particle->rot.x += particle->rotDelta.x * fps;
		particle->rot.y += particle->rotDelta.y * fps;
		particle->rot.z += particle->rotDelta.z * fps;
					
					/* MOVE IT */
					
		if (particle->mode & PARTICLE_MODE_HEAVYGRAVITY)
			particle->coordDelta.y -= fps * GRAVITY_CONSTANT/2;		// gravity
		else
			particle->coordDelta.y -= fps * GRAVITY_CONSTANT/3;		// gravity
			
		x = (particle->coord.x += particle->coordDelta.x * fps);
		y = (particle->coord.y += particle->coordDelta.y * fps);
		z = (particle->coord.z += particle->coordDelta.z * fps);
		
		
					/* SEE IF BOUNCE */
					
		ty = GetTerrainHeightAtCoord_Quick(x,z);				// get terrain height here
		if (y <= ty)
		{
			if (particle->mode & PARTICLE_MODE_BOUNCE)
			{
				particle->coord.y  = ty;
				particle->coordDelta.y *= -0.5;
				particle->coordDelta.x *= 0.9;
				particle->coordDelta.z *= 0.9;
			}
			else
				goto del;
		}
		
					/* SCALE IT */
					
		particle->scale -= particle->decaySpeed * fps;
		if (particle->scale <= 0.0f)
		{
				/* DEACTIVATE THIS PARTICLE */
	del:	
			particle->isUsed = false;
			gNumParticles--;
			continue;
		}

			/***************************/
			/* UPDATE TRANSFORM MATRIX */
			/***************************/
			

				/* SET SCALE MATRIX */

		Q3Matrix4x4_SetScale(&particle->matrix, particle->scale,	particle->scale, particle->scale);
	
					/* NOW ROTATE IT */

		Q3Matrix4x4_SetRotate_XYZ(&matrix, particle->rot.x, particle->rot.y, particle->rot.z);
		Q3Matrix4x4_Multiply(&particle->matrix,&matrix, &matrix2);
	
					/* NOW TRANSLATE IT */

		Q3Matrix4x4_SetTranslate(&matrix, particle->coord.x, particle->coord.y, particle->coord.z);
		Q3Matrix4x4_Multiply(&matrix2,&matrix, &particle->matrix);
	}
}


/************************* QD3D: DRAW PARTICLES ****************************/

void QD3D_DrawParticles(QD3DSetupOutputType *setupInfo)
{
	if (gNumParticles == 0)												// quick check if any particles at all
		return;

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		ParticleType* particle = &gParticles[i];
		if (particle->isUsed)
		{
			Render_SubmitMesh(particle->mesh, &particle->matrix, &kParticleRenderingMods, &particle->coord);
		}
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
