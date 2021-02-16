#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Reimplementations of game functions

char GetTypedKey(void);

void DoQualityDialog(void);

//-----------------------------------------------------------------------------
// Misc additions

void SetProModeSettings(int isPro);

void DoSDLMaintenance(void);

void SetFullscreenMode(void);

#ifdef __cplusplus
}
#endif
