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
