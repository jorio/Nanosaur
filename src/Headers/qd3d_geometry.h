//
// qd3d_geometry.h
//

#include "qd3d_support.h"


enum
{
	PARTICLE_MODE_BOUNCE = (1),
	PARTICLE_MODE_UPTHRUST = (1<<1),
	PARTICLE_MODE_HEAVYGRAVITY = (1<<2)
};


#define	MAX_PARTICLES		150

typedef struct
{
	Boolean					isUsed;
	TQ3Vector3D				rot,rotDelta;
	TQ3Point3D				coord,coordDelta;
	float					decaySpeed,scale;
	Byte					mode;
	TQ3Matrix4x4			matrix;
	TQ3TriMeshData			*mesh;
}ParticleType;

float QD3D_CalcObjectRadius(int numMeshes, TQ3TriMeshData** meshList);
void QD3D_CalcObjectBoundingBox(int numMeshes, TQ3TriMeshData** meshList, TQ3BoundingBox* boundingBox);
extern	void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed);
extern	void QD3D_ReplaceGeometryTexture(TQ3Object obj, TQ3SurfaceShaderObject theShader);
extern	void QD3D_ScrollUVs(TQ3Object theObject, float du, float dv);
extern	void QD3D_InitParticles(void);
extern	void QD3D_MoveParticles(void);
extern	void QD3D_DrawParticles(QD3DSetupOutputType *setupInfo);


