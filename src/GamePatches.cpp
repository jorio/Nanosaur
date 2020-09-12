#include <QD3DMath.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "PommeInternal.h"
#include "input.h"

extern TQ3Matrix4x4 gCameraWorldToViewMatrix;
extern TQ3Matrix4x4 gCameraViewToFrustumMatrix;
extern long gNodesDrawn;
extern SDL_Window* gSDLWindow;
extern float	gFramesPerSecond;

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


/**************** DRAW PICTURE TO SCREEN ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
//
//
// INPUT: myFSSpec = spec of image file
//

OSErr DrawPictureToScreen(FSSpec* spec, short x, short y)
{
	short refNum;
	
	OSErr error = FSpOpenDF(spec, fsRdPerm, &refNum);
	if (noErr != error) {
		TODO2("Couldn't open picture: " << Pascal2C(spec->name));
		return error;
	}
	
	auto& stream = Pomme::Files::GetStream(refNum);
	auto pict = Pomme::Graphics::ReadPICT(stream, true);
	FSClose(refNum);
	
	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(Pomme::Graphics::GetScreenPort());
	Pomme::Graphics::DrawARGBPixmap(x, y, pict);
	SetPort(oldPort);
	
	return noErr;
}

//-----------------------------------------------------------------------------
// Fade

void DumpGLPixels(const char* outFN)
{
	int width = 640;
	int height = 480;
	SDL_GetWindowSize(gSDLWindow, &width, &height);
	auto buf = std::vector<char>(width * height * 3);
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buf.data());
	
	std::ofstream out(outFN, std::ios::out | std::ios::binary);
	short TGAhead[] = { 0, 2, 0, 0, 0, 0, width, height, 24 };
	out.write(reinterpret_cast<char*>(&TGAhead), sizeof(TGAhead));
	out.write(buf.data(), buf.size());
	
	printf("Screenshot saved to %s\n", outFN);
}

//-----------------------------------------------------------------------------
// SDL maintenance

static struct {
	UInt32 lastUpdateAt = 0;
	const UInt32 updateInterval = 250;
	UInt32 frameAccumulator = 0;
	char titleBuffer[1024];
} debugText;

void DoSDLMaintenance()
{
	static int holdFramerateCap = 0;

	// Cap frame rate.
	if (gFramesPerSecond > 200 || holdFramerateCap > 0) {
		SDL_Delay(5);
		// Keep framerate cap for a while to avoid jitter in game physics
		holdFramerateCap = 10;
	} else {
		holdFramerateCap--;
	}
	
#if _DEBUG
	UInt32 now = SDL_GetTicks();
	UInt32 ticksElapsed = now - debugText.lastUpdateAt;
	if (ticksElapsed >= debugText.updateInterval) {
		float fps = 1000 * debugText.frameAccumulator / (float)ticksElapsed;
		snprintf(debugText.titleBuffer, 1024, "nsaur - %d fps - %d nodes drawn", (int)round(fps), gNodesDrawn);
		SDL_SetWindowTitle(gSDLWindow, debugText.titleBuffer);
		debugText.frameAccumulator = 0;
		debugText.lastUpdateAt = now;
	}
	debugText.frameAccumulator++;
#endif

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			throw Pomme::QuitRequest();
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				throw Pomme::QuitRequest();
				break;
			case SDL_WINDOWEVENT_RESIZED:
				printf("Window Resized!!\n");
				break;
			}
			break;
		}
	}
}