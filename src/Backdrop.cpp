#include <SDL_opengl.h>
#include <SDL_video.h>
#include <SDL_events.h>
#include <PommeInternal.h>
#include "game/Structs.h"
#include "windows_nano.h"

extern SDL_Window* gSDLWindow;
extern PrefsType gGamePrefs;

static GLuint backdropTexture = -1;
static bool backdropTextureAllocated = false;
static bool backdropPillarbox = false;

static SDL_GLContext exclusiveGLContext = nullptr;
static bool exclusiveGLContextValid = false;

constexpr const bool ALLOW_BACKDROP_TEXTURE = true;

void SetWindowGamma(int percent)
{
	SDL_SetWindowBrightness(gSDLWindow, percent/100.0f);
}

static UInt32* GetBackdropPixPtr()
{
	return (UInt32*)GetPixBaseAddr(GetGWorldPixMap(Pomme::Graphics::GetScreenPort()));
}

void AllocBackdropTexture()
{
	if (!ALLOW_BACKDROP_TEXTURE) {
		return;
	}

	if (backdropTextureAllocated) {
		return;
	}
	
	backdropTextureAllocated = true;
	
	glGenTextures(1, &backdropTexture);
	auto error = glGetError();
	if (error != 0) {
		TODOMINOR2("couldn't alloc backdrop texture: " << error);
		return;
	}

	glBindTexture(GL_TEXTURE_2D, backdropTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, GetBackdropPixPtr());
	Pomme_SetPortDirty(false);
}

void DisposeBackdropTexture()
{
	if (!ALLOW_BACKDROP_TEXTURE) {
		return;
	}

	printf("[BACKDROP] disposed\n");
	
	if (backdropTextureAllocated) {
		glDeleteTextures(1, &backdropTexture);
		backdropTextureAllocated = false;
	}
}

void EnableBackdropPillarboxing(Boolean pillarbox)
{
	backdropPillarbox = pillarbox;
}

void RenderBackdropQuad()
{
	if (!ALLOW_BACKDROP_TEXTURE) {
		return;
	}

	if (!backdropTextureAllocated) {
		return;
	}

	unsigned char* pixPtr = (unsigned char*)GetBackdropPixPtr();
	
	int windowWidth, windowHeight;
	GLint vp[4];

	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	glGetIntegerv(GL_VIEWPORT, vp);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, windowWidth, windowHeight, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	if (backdropPillarbox) {
		float clearR = pixPtr[1] / 255.0f;
		float clearG = pixPtr[2] / 255.0f;
		float clearB = pixPtr[3] / 255.0f;
		glClearColor(clearR, clearG, clearB, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glBindTexture(GL_TEXTURE_2D, backdropTexture);

	if (Pomme_IsPortDirty())
	{
		glTexSubImage2D(GL_TEXTURE_2D,
			0, 0, 0, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT,
			GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixPtr);
		Pomme_SetPortDirty(false);
	}

	if (gGamePrefs.highQualityTextures) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	
	int quadLeft = 0;
	int quadRight = windowWidth;
	if (backdropPillarbox) {
		const float ratio = (float) windowWidth / windowHeight;
		const float fourThirds = 4.0f / 3.0f;
		if (ratio > fourThirds) {
			int pillarboxedWidth = fourThirds * windowWidth / ratio;
			quadLeft = (windowWidth / 2) - (pillarboxedWidth / 2);
			quadRight = quadLeft + pillarboxedWidth;
		}
	}

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0.0, 0.0); glVertex2i(quadLeft, 0);
	glTexCoord2f(1.0, 0.0); glVertex2i(quadRight, 0);
	glTexCoord2f(0.0, 1.0); glVertex2i(quadLeft, windowHeight);
	glTexCoord2f(1.0, 1.0); glVertex2i(quadRight, windowHeight);
	glEnd();

	if (exclusiveGLContextValid) { // in exclusive GL mode, force swap buffer
		SDL_GL_SwapWindow(gSDLWindow);
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glEnable(GL_SCISSOR_TEST);

	glViewport(vp[0], vp[1], (GLsizei)vp[2], (GLsizei)vp[3]);
}

void ExclusiveOpenGLMode_Begin()
{
	if (!ALLOW_BACKDROP_TEXTURE)
		return;

	if (exclusiveGLContextValid)
		throw std::runtime_error("already in exclusive GL mode");

	exclusiveGLContext = SDL_GL_CreateContext(gSDLWindow);
	exclusiveGLContextValid = true;
	SDL_GL_MakeCurrent(gSDLWindow, exclusiveGLContext);

	AllocBackdropTexture();
}

void ExclusiveOpenGLMode_End()
{
	if (!ALLOW_BACKDROP_TEXTURE)
		return;

	if (!exclusiveGLContextValid)
		throw std::runtime_error("not in exclusive GL mode");

	DisposeBackdropTexture();

	exclusiveGLContextValid = false;
	SDL_GL_DeleteContext(exclusiveGLContext);
	exclusiveGLContext = nullptr;
}
