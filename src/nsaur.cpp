#include "PommeInternal.h"
#include <SDL.h>
#include <iostream>
#include <Quesa.h>
#include "Files/ArchiveVolume.h"


TQ3ViewObject				gView = nullptr;

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

	// Mount game archive as data volume
	short archiveVolumeID = Pomme::Files::MountArchiveAsVolume("nanosaur134.bin");
	gDataSpec.vRefNum = archiveVolumeID;

	// Use application resource file
	FSSpec applicationSpec = {};
	if (noErr != FSMakeFSSpec(0, gDataSpec.parID, ":Nanosaur\u2122", &applicationSpec)) {
	//if (noErr != FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Nanosaur#", &applicationSpec)) {
		throw std::exception("Can't find application resource file.");
	}
	UseResFile(FSpOpenResFile(&applicationSpec, fsRdPerm));

	// Start the game
	try {
		GameMain();
	} catch (Pomme::QuitRequest&) {
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	if (gView != NULL)
		Q3Object_Dispose(gView);

	// TODO: dispose SDL gl context

//	if (gDC != NULL)
//		ReleaseDC((HWND)gWindow, gDC);

//	DestroyWindow((HWND)gWindow);

	// Terminate Quesa
	Q3Exit();
	
	//Pomme::Shutdown();

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
