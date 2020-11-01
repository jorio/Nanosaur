#pragma once

#include "Pomme.h"

#ifdef PRO_MODE
	#define ARCHIVE_NAME "nanosaurextreme134.bin"
	#define APP_FILE_INSIDE_ARCHIVE = u8"Nanosaur\u2122 Extreme"
#else
	#define ARCHIVE_NAME "nanosaur134.bin"
	#define APP_FILE_INSIDE_ARCHIVE u8"Nanosaur\u2122"
#endif

#if EMBED_DATA
bool FindEmbeddedGameData(FSSpec* dataSpec);
#else
void SetGameDataPathFromArgs(int argc, const char** argv);
bool FindGameData(FSSpec* dataSpec);
void WriteDataLocationSetting();
#endif
