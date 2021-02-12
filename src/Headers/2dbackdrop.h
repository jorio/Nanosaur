#pragma once

//-----------------------------------------------------------------------------
// Backdrop

enum
{
	BACKDROP_FILL = 0,
	BACKDROP_PILLARBOX = 1,
	BACKDROP_LETTERBOX = 2,
	BACKDROP_CLEAR_BLACK = 4,
	BACKDROP_FIT = BACKDROP_PILLARBOX | BACKDROP_LETTERBOX,
};

void SetWindowGamma(int percent);

void Backdrop_AllocTexture(void);

void Backdrop_DisposeTexture(void);

void Backdrop_Draw(void);

void Backdrop_SetClipRegion(int width, int height);

void Backdrop_Clear(UInt32 argb);

void Backdrop_Render(int fit);
