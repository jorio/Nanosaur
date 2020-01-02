#include <iostream>

#include "Pomme.h"
#include "PommeInternal.h"

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

std::string Pomme::FourCCString(FourCharCode t) {
	char b[5];
	*(ResType*)b = t;
#if !(TARGET_RT_BIGENDIAN)
	std::reverse(b, b + 4);
#endif
	// replace non-ascii with '?'
	for (int i = 0; i < 4; i++) {
		char c = b[i];
		if (c < ' ' || c > '~') b[i] = '?';
	}
	b[4] = '\0';
	return b;
}

//-----------------------------------------------------------------------------
// FSSpec

OSErr FSpOpenDF(const FSSpec* spec, char permission, short* refNum) {
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

long GetResourceSizeOnDisk(Handle theResource) {
	TODO();
	return -1;
}

long SizeResource(Handle theResource) {
	return GetResourceSizeOnDisk(theResource);
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

void NumToString(long theNum, Str255 theString) {
	std::stringstream ss;
	ss << theNum;
	theString = Str255(ss.str().c_str());
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

UInt32 TickCount() {
	TODOMINOR();
	return 0;
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

char* Pascal2C(const char* pstr) {
	static char cstr[256];
	memcpy(cstr, &pstr[1], pstr[0]);
	cstr[pstr[0]] = '\0';
	return cstr;
}

void Pomme::Init() {
	Pomme::InitFiles("._Nanosaur™");
	std::cout << "fake mac inited\n";
}
