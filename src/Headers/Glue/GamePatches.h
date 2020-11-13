#pragma once

#include <Quesa.h>
#include "GLBackdrop.h"

#ifdef __cplusplus
extern "C" {
#endif

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

void SetBackdropClipRegion(int width, int height);

void ClearBackdrop(UInt32 argb);

void RenderBackdropQuad(int fit);

void DisposeBackdropTexture(void);

//-----------------------------------------------------------------------------
// Misc additions

void SetProModeSettings(int isPro);

OSErr MakePrefsFSSpec(const char* prefFileName, FSSpec* spec);

void DoSDLMaintenance(void);

void SetFullscreenMode(void);

#ifdef __cplusplus
}
#endif
