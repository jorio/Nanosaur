#include "PommeInternal.h"
#include <SDL.h>
#include <iostream>
#include "Files/ArchiveVolume.h"

#ifndef USE_ARCHIVE
	#define USE_ARCHIVE 1
#endif

#if PRO_MODE
	constexpr const char* ARCHIVE_NAME = "nanosaurextreme134.bin";
	#if USE_ARCHIVE
		constexpr const char* APPLICATION_FILE = ":Nanosaur# Extreme";
	#else
		constexpr const char* APPLICATION_FILE = ":Nanosaur\u2122 Extreme";
	#endif
#else
	constexpr const char* ARCHIVE_NAME = "nanosaur134.bin";
	#if USE_ARCHIVE
		constexpr const char* APPLICATION_FILE = ":Nanosaur#";
	#else
		constexpr const char* APPLICATION_FILE = ":Nanosaur\u2122";
	#endif
#endif

// bare minimum from Windows.c to satisfy externs in game code
WindowPtr gCoverWindow = nullptr;
UInt32* gCoverWindowPixPtr = nullptr;

extern FSSpec gDataSpec;

void GameMain(void);
void RegisterUnpackableTypes(void);

int CommonMain(int argc, const char** argv)
{
	// Start our "machine"
	Pomme::Init("Nanosaur");

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*)GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Register format strings to unpack the structs
	RegisterUnpackableTypes();

#if USE_ARCHIVE
	// Mount game archive as data volume
	short archiveVolumeID = Pomme::Files::MountArchiveAsVolume(ARCHIVE_NAME);
	gDataSpec.vRefNum = archiveVolumeID;
#endif

	// Use application resource file
	FSSpec applicationSpec = {};
	if (noErr != FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, APPLICATION_FILE, &applicationSpec)) {
		throw std::runtime_error("Can't find application resource file: " + std::string(APPLICATION_FILE));
	}
	UseResFile(FSpOpenResFile(&applicationSpec, fsRdPerm));
	Pomme::Graphics::SetWindowIconFromIcl8Resource(128);

	// Start the game
	try {
		GameMain();
	} catch (Pomme::QuitRequest&) {
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	Pomme::Shutdown();

	return 0;
}

int main(int argc, char** argv)
{
	std::string uncaught;

	try {
		return CommonMain(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex) {
		uncaught = ex.what();
	}
	catch (...) {
		uncaught = "unknown";
	}

	std::cerr << "Uncaught exception: " << uncaught << "\n";
	SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
	return 1;
}
