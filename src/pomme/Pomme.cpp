#include <iostream>

#include "Pomme.h"

//-----------------------------------------------------------------------------
// Our own utils

void ImplementMe(const char* fn, std::string msg, int severity) {
	if (severity >= 0) {
		std::cerr << "ImplementMe[" << severity << "] " << fn;
		if (!msg.empty())
			std::cerr << ": " << msg;
		std::cerr << "\n";
	}
	
	if (severity >= 1) {
		std::stringstream ss;
		ss << "TODO" << severity << ": " << fn << "()\n";
		if (!msg.empty())
			ss << "Message: " << msg << "\n";
		if (IDOK != MessageBoxA(NULL, ss.str().c_str(), "ImplementMe", MB_ICONWARNING | MB_OKCANCEL)) {
			exit(1);
		}
	}

	if (severity >= 2) {
		exit(1);
	}
}

//-----------------------------------------------------------------------------
// FSSpec

OSErr FSMakeFSSpec(short vRefNum, long dirID, const char* fileName, FSSpec* spec) {
	TODO();
	return unimpErr;
}

short FSpOpenResFile(const FSSpec* spec, char permission) {
	TODO();
	return -1;
}

OSErr FSpOpenDF(const FSSpec* spec, char permission, short* refNum) {
	TODO();
	return unimpErr;
}

OSErr FSpOpenRF(const FSSpec* spec, char permission, short* refNum) {
	TODO();
	return unimpErr;
}

OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) {
	TODO();
	return unimpErr;
}

OSErr FSpDelete(const FSSpec* spec) {
	TODO();
	return unimpErr;
}

OSErr ResolveAlias(const FSSpec* spec, AliasHandle alias, FSSpec* target, Boolean* wasChanged) {
	TODO();
	return unimpErr;
}

//-----------------------------------------------------------------------------
// File I/O

OSErr FSRead(short refNum, long* count, Ptr buffPtr) {
	TODO();
	return unimpErr;
}

OSErr FSWrite(short refNum, long* count, Ptr buffPtr) {
	TODO();
	return unimpErr;
}

OSErr FSClose(short refNum) {
	TODO();
	return unimpErr;
}

OSErr GetEOF(short refNum, long* logEOF) {
	TODO();
	return unimpErr;
}

OSErr SetEOF(short refNum, long logEOF) {
	TODO();
	return unimpErr;
}

//-----------------------------------------------------------------------------
// Resource file management

OSErr ResError(void) {
	TODO();
	return unimpErr;
}

void UseResFile(short refNum) {
	TODO();
}

short CurResFile() {
	TODO();
	return -1;
}

void CloseResFile(short refNum) {
	TODO();
}

int Count1Resources(ResType theType) {
	TODO();
	return 0;
}

Handle GetResource(ResType theType, short theID) {
	TODO();
	return nil;
}

void ReleaseResource(Handle theResource) {
	TODO();
}

void RemoveResource(Handle theResource) {
	TODO();
}

void AddResource(Handle theData, ResType theType, short theID, const char* name) {
	TODO();
}

void WriteResource(Handle theResource) {
	TODO();
}

void DetachResource(Handle theResource) {
	TODO();
}

//-----------------------------------------------------------------------------
// QuickDraw 2D

// pictureID = resource ID for a 'PICT' resource
PicHandle GetPicture(short pictureID) {
	TODO2("pictureID" << pictureID);  return nil;
}

void DisposeGWorld(GWorldPtr offscreenGWorld) {
	TODO();
}

//-----------------------------------------------------------------------------
// QuickDraw 3D

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* fs) {
	TODO();
	return nil;
}

//-----------------------------------------------------------------------------
// Misc

void ExitToShell() {
	exit(0);
}

void SysBeep(short duration) {
#ifdef WIN32
	MessageBeep(0);
#else
	TODOMINOR();
#endif
}

void FlushEvents(short, short) {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Input

void GetKeys(KeyMap) {
	TODOMINOR();
}

Boolean Button(void) {
	TODOMINOR();
	return false;
}

//-----------------------------------------------------------------------------
// No-op memory junk

void MaxApplZone(void) {
	// No-op
}

void MoreMasters(void) {
	// No-op
}

Size CompactMem(Size) {
	// No-op
	// TODO: what should we actually return?
	return 0;
}

Size CompactMemSys(Size) {
	// No-op
	// TODO: what should we actually return?
	return 0;
}

void PurgeMem(Size) {
	// No-op
}

void PurgeMemSys(Size) {
	// No-op
}

Size MaxMem(Size*) {
	// No-op
	// TODO: what should we actually return?
	return 0;
}

void HNoPurge(Handle) {
	// No-op
}

void HLock(Handle) {
	// No-op
}

void HLockHi(Handle) {
	// No-op
}

void NoPurgePixels(PixMapHandle) {
	// No-op
}

//-----------------------------------------------------------------------------
// Memory: Handle

Handle NewHandleClear(Size byteCount) {
	TODOFATAL();
	return nil;
}

Handle TempNewHandle(Size byteCount, OSErr* err) {
	TODOFATAL();
	return nil;
}

Size GetHandleSize(Handle) {
	TODOFATAL();
	return 0;
}

void SetHandleSize(Handle handle, Size byteCount) {
	TODOFATAL();
}

void DisposeHandle(Handle) {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Memory: Ptr

Ptr NewPtr(Size byteCount) {
	TODOFATAL();
	return nil;
}

Ptr NewPtrSys(Size byteCount) {
	TODOFATAL();
	return nil;
}

void DisposePtr(Ptr p) {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Memory: BlockMove

void BlockMove(const void* srcPtr, void* destPtr, Size byteCount) {
	TODOFATAL();
}

void BlockMoveData(const void* srcPtr, void* destPtr, Size byteCount) {
	TODOFATAL();
}

//-----------------------------------------------------------------------------
// Time Manager

void GetDateTime(unsigned long* secs) {
	TODOMINOR();
}

void Microseconds(UnsignedWide*) {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Mouse cursor

void InitCursor() {
	TODOMINOR();
}

void HideCursor() {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Sound Manager

short GetDefaultOutputVolume(long*) {
	TODOMINOR();
	return 0;
}

void SetDefaultOutputVolume(long) {
	TODOMINOR();
}

OSErr SndNewChannel(SndChannelPtr* chan, short synth, long init, /*ProcPtr*/void* userRoutine) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndChannelStatus(SndChannelPtr chan, short theLength, SCStatusPtr theStatus) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndDoImmediate(SndChannelPtr chan, const SndCommand* cmd) {
	TODOMINOR();
	return unimpErr;
}

OSErr GetSoundHeaderOffset(SndListHandle sndHandle, long* offset) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndStartFilePlay(SndChannelPtr chan, short fRefNum, short resNum, long bufferSize, Ptr theBuffer, /*AudioSelectionPtr*/ void* theSelection, FilePlayCompletionUPP theCompletion, Boolean async) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndPauseFilePlay(SndChannelPtr chan) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndStopFilePlay(SndChannelPtr chan, Boolean quietNow) {
	TODOMINOR();
	return unimpErr;
}

//-----------------------------------------------------------------------------
// Fade

void MakeFadeEvent(Boolean fadeIn) {
	TODOMINOR2("fadeIn=" << fadeIn);
}

void GammaFadeOut() {
	TODOMINOR();
}

void GammaFadeIn() {
	TODOMINOR();
}

//-----------------------------------------------------------------------------
// Our own init

void InitFakeMac() {
	std::cout << "fake mac inited\n";
}
