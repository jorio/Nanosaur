#pragma once

// SOURCE PORT EXTRAS

#define USE_BUGGY_CULLING false

Boolean IsSphereInConeOfVision(TQ3Point3D* coord, float radius, float hither, float yon);

char GetTypedKey(void);

//-----------------------------------------------------------------------------
// Fade

void MakeFadeEvent(Boolean fadeIn);

void GammaFadeOut();

void GammaFadeIn();

//-----------------------------------------------------------------------------
// Backdrop

void AllocBackdropTexture(void);

void RenderBackdropQuad(bool forceSwapBuffers = false);

void DisposeBackdropTexture(void);




void DumpGWorldToGWorld(GWorldPtr thisWorld, GWorldPtr destWorld, Rect* srcRect, Rect* destRect);

