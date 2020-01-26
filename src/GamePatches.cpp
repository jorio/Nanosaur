#include <QD3DMath.h>

#include "input.h"

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


char GetTypedKey(void)
{
	static const char KCHR_US_LOWER[] = "asdfhgzxcv\0bqweryt123465=97-80]ou[ip\rlj'k;\\,/nm.\t `\x08\0\x1B\0\0\0\0\0\0\0\0\0\0\0.\0*\0+\0\0\0\0\0/\n\0-\0\0=01234567\089\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x7F\0\0\0\0\0\x1C\x1D\x1F\x1E";
	static const char KCHR_US_UPPER[] = "ASDFHGZXCV\0BQWERYT!@#$^%+(&_*)}OU{IP\rLJ\"K:|<?NM>\t ~\x08\0\x1B\0\0\0\0\0\0\0\0\0\0\0.\0*\0+\0\0\0\0\0/\n\0-\0\0=01234567\089\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x7F\0\0\0\0\0\x1C\x1D\x1F\x1E";

	ReadKeyboard_Real();

	for (int i = 0; i < sizeof(KCHR_US_LOWER); i++) {
		if (!GetNewKeyState_Real(i))
			continue;

		char c = '\0';

		if (GetKeyState_Real(kVK_Shift) || GetKeyState_Real(kVK_RightShift))
			c = KCHR_US_UPPER[i];
		else
			c = KCHR_US_LOWER[i];

		if (c == '\0')
			continue;

		return c;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Fade

void MakeFadeEvent(Boolean fadeIn)
{
	TODOMINOR2("fadeIn=" << (int)fadeIn);
}

void GammaFadeOut()
{
	TODOMINOR();
}

void GammaFadeIn()
{
	TODOMINOR();
}