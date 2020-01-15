#include <thread>
#include <chrono>
#include <iostream>
#include "PommeInternal.h"
#include "cmixer.h"

#define LOG POMME_GENLOG(POMME_DEBUG_SOUND, "SOUN")

static cmixer::WavStream* wip_ugly_stream = nullptr;

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

OSErr SndStartFilePlay(
	SndChannelPtr						chan,	
	short								fRefNum,
	short								resNum,
	long								bufferSize,
	Ptr									theBuffer,
	/*AudioSelectionPtr*/ void*			theSelection,
	FilePlayCompletionUPP				theCompletion,
	Boolean								async)
{
	if (resNum != 0) {
		TODO2("playing snd resource not implemented yet, resource " << resNum);
		return unimpErr;
	}

	Pomme::Sound::AudioClip clip = Pomme::Sound::ReadAIFF(Pomme::Files::GetStream(fRefNum));

	if (wip_ugly_stream) {
		delete wip_ugly_stream;
		wip_ugly_stream = nullptr;
	}

	// TODO: 1 stream per channel
	// TODO: don't use new/delete
	// TODO: get rid of the gratuitous buffer copy
	wip_ugly_stream = new cmixer::WavStream(clip.sampleRate, 16, clip.nChannels, std::vector<char>(clip.pcmData));

	wip_ugly_stream->Play();

	if (!async) {
		while (wip_ugly_stream->GetState() != cmixer::CM_STATE_STOPPED) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	TODOMINOR2("plug completion callback");

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
