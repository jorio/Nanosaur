#include <QD3DMath.h>

extern TQ3Matrix4x4 gCameraWorldToViewMatrix;
extern TQ3Matrix4x4 gCameraViewToFrustumMatrix;

Boolean IsSphereInConeOfVision(TQ3Point3D* coord, float radius, float hither, float yon)
{
	TQ3Point3D world;
	TQ3Point3D p;
	TQ3Point3D r;

	Q3Point3D_Transform(coord, &gCameraWorldToViewMatrix, &world);

			/* SEE IF BEHIND CAMERA */

	if (world.z >= -hither)
	{
		if (world.z - radius > -hither)							// is entire sphere behind camera?
			return false;

			/* PARTIALLY BEHIND */

		world.z -= radius;										// move edge over hither plane so cone calc will work
	}
	else
	{
			/* SEE IF BEYOND YON PLANE */

		if (world.z + radius < -yon)							// see if too far away
			return false;
	}

	/*****************************/
	/* SEE IF WITHIN VISION CONE */
	/*****************************/

			/* TRANSFORM WORLD COORD & RADIUS */

	Q3Point3D_Transform(&world, &gCameraViewToFrustumMatrix, &p);

	r = { radius, radius, world.z };
	Q3Point3D_Transform(&r, &gCameraViewToFrustumMatrix, &r);


			/* SEE IF SPHERE "WOULD BE" OUT OF BOUNDS */

	if ((p.x + r.x) < -1.0f) return false;
	if ((p.x - r.x) >  1.0f) return false;
	if ((p.y + r.y) < -1.0f) return false;
	if ((p.y - r.y) >  1.0f) return false;

	return true;
}



