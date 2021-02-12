// 2D BACKDROP.C
// (C)2021 Iliyas Jorio
// This file is part of Nanosaur. https://github.com/jorio/nanosaur

#include <SDL.h>
#include <Pomme.h>
#include "2dbackdrop.h"
#include "structs.h"
#include "windows_nano.h"
#include "renderer.h"
#include "misc.h"

extern	SDL_Window*		gSDLWindow;
extern	PrefsType		gGamePrefs;
extern	UInt32*const	gCoverWindowPixPtr;

extern	int 			gWindowWidth;
extern	int 			gWindowHeight;

static	GLuint			glBackdropTextureName = 0;

void SetWindowGamma(int percent)
{
	SDL_SetWindowBrightness(gSDLWindow, percent / 100.0f);
}

void Backdrop_AllocTexture()
{
	glBackdropTextureName = Render_LoadTexture(
			GL_RGBA,
			GAME_VIEW_WIDTH,
			GAME_VIEW_HEIGHT,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8,
			gCoverWindowPixPtr,
			kRendererTextureFlags_None
	);

	ClearPortDamage();
}

void Backdrop_Clear(UInt32 argb)
{
	UInt32 bgra = Byteswap32(&argb);

	UInt32* backdropPixPtr = gCoverWindowPixPtr;

	for (int i = 0; i < GAME_VIEW_WIDTH * GAME_VIEW_HEIGHT; i++)
	{
		*(backdropPixPtr++) = bgra;
	}

	GrafPtr port;
	GetPort(&port);
	DamagePortRegion(&port->portRect);
}

void Backdrop_DisposeTexture()
{
	if (glBackdropTextureName != 0)
	{
		glDeleteTextures(1, &glBackdropTextureName);
		glBackdropTextureName = 0;
	}
}

void Backdrop_SetClipRegion(int regionWidth, int regionHeight)
{
#if 0
	glBackdrop->SetClipRegion(regionWidth, regionHeight);
#endif
}

void Backdrop_Draw(void)
{
	if (glBackdropTextureName == 0)
		return;

	glBindTexture(GL_TEXTURE_2D, glBackdropTextureName);

	// If the screen port has dirty pixels ("damaged"), update the texture
	if (IsPortDamaged())
	{
		Rect damageRect;
		GetPortDamageRegion(&damageRect);

		// Set unpack row length to 640
		GLint pUnpackRowLength;
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pUnpackRowLength);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, GAME_VIEW_WIDTH);

		glTexSubImage2D(
				GL_TEXTURE_2D,
				0,
				damageRect.left,
				damageRect.top,
				damageRect.right - damageRect.left,
				damageRect.bottom - damageRect.top,
				GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8,
				gCoverWindowPixPtr + (damageRect.top * GAME_VIEW_WIDTH + damageRect.left));
		CHECK_GL_ERROR();

		// Restore unpack row length
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pUnpackRowLength);

		ClearPortDamage();
	}

	glViewport(0, 0, gWindowWidth, gWindowHeight);
	Render_Enter2D();
	Render_Draw2DFullscreenQuad();
	Render_Exit2D();
}

