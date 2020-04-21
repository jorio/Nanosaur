#pragma once

// SOURCE PORT EXTRAS

#define USE_BUGGY_CULLING false

Boolean IsSphereInConeOfVision(TQ3Point3D* coord, float radius, float hither, float yon);

char GetTypedKey(void);

//-----------------------------------------------------------------------------
// Backdrop

void SetWindowGamma(int percent);

void ExclusiveOpenGLMode_Begin(void);

void ExclusiveOpenGLMode_End(void);

void AllocBackdropTexture(void);

void RenderBackdropQuad(void);

void DisposeBackdropTexture(void);
