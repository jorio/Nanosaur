#include <iostream>
#include "PommeInternal.h"

#define LOG POMME_GENLOG(POMME_DEBUG_SOUND, "SOUN")

using namespace Pomme;

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
	return noErr;
}

OSErr GetSoundHeaderOffset(SndListHandle sndHandle, long* offset) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndStartFilePlay(SndChannelPtr chan, short fRefNum, short resNum, long bufferSize, Ptr theBuffer, /*AudioSelectionPtr*/ void* theSelection, FilePlayCompletionUPP theCompletion, Boolean async) {
	TODOMINOR();
	return noErr;
}

OSErr SndPauseFilePlay(SndChannelPtr chan) {
	TODOMINOR();
	return unimpErr;
}

OSErr SndStopFilePlay(SndChannelPtr chan, Boolean quietNow) {
	TODOMINOR();
	return unimpErr;
}

NumVersion SndSoundManagerVersion() {
	NumVersion v;
	v.majorRev = 3;
	v.minorAndBugRev = 9;
	v.stage = 0x80;
	v.nonRelRev = 0;
	return v;
}

//-----------------------------------------------------------------------------
// Init Sound Manager

void Pomme::Sound::Init()
{
	cmixer::InitWithSDL();
}
