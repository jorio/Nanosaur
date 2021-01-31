#pragma once

#include "GLBackdrop.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Reimplementations of game functions

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

//-----------------------------------------------------------------------------
// Patch 3DMF models

#if 0 // TODO: noquesa
void PatchSkeleton3DMF(const char* cName, TQ3Object newModel);

void PatchGrouped3DMF(const char* cName, TQ3Object* objects, int nObjects);

void ApplyEdgePadding(const TQ3Mipmap* mipmap);
#endif

#ifdef __cplusplus
}
#endif
