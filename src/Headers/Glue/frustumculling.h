#pragma once

enum
{
	kFrustumPlaneRight	= 0,
	kFrustumPlaneLeft	= 1,
	kFrustumPlaneTop	= 2,
	kFrustumPlaneBottom	= 3,
	kFrustumPlaneNear	= 4,
	kFrustumPlaneFar	= 5,
};

void UpdateFrustumPlanes(TQ3ViewObject viewObject);

bool IsSphereInFrustum_XZ(const TQ3Point3D* sphereWorldOrigin, float sphereRadius);

bool IsSphereInFrustum_XYZ(const TQ3Point3D* sphereWorldOrigin, float sphereRadius);
