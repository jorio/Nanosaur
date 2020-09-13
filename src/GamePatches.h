#pragma once

// SOURCE PORT EXTRAS

#define USE_BUGGY_CULLING false

//-----------------------------------------------------------------------------
// Reimplementations of game functions

Boolean IsSphereInConeOfVision(TQ3Point3D* coord, float radius, float hither, float yon);

char GetTypedKey(void);

void DoQualityDialog(void);

//-----------------------------------------------------------------------------
// Backdrop

void SetWindowGamma(int percent);

void ExclusiveOpenGLMode_Begin(void);

void ExclusiveOpenGLMode_End(void);

void AllocBackdropTexture(void);

void EnableBackdropPillarboxing(Boolean pillarbox);

void RenderBackdropQuad(void);

void DisposeBackdropTexture(void);

//-----------------------------------------------------------------------------
// Misc additions

void DoSDLMaintenance(void);

void SetFullscreenMode(void);
