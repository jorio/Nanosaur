#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>
#include <strstream>
#include "PommeInternal.h"
#include "cmixer.h"

#define LOG POMME_GENLOG(POMME_DEBUG_SOUND, "SOUN")

static SndChannelPtr headChan = nullptr;
static int nManagedChans = 0;

// Internal channel info
struct ChannelEx {
	SndChannelPtr prevChan;
	bool macChannelStructAllocatedByPomme;
	cmixer::WavStream* stream;
};

static inline ChannelEx& GetEx(SndChannelPtr chan)
{
	return *(ChannelEx*)chan->firstMod;
}

static inline cmixer::WavStream* GetMixSource(SndChannelPtr chan)
{
	return GetEx(chan).stream;
}

static inline SndChannelPtr* NextOf(SndChannelPtr chan)
{
	return &chan->nextChan;
}

static inline SndChannelPtr* PrevOf(SndChannelPtr chan)
{
	return &GetEx(chan).prevChan;
}

static void Link(SndChannelPtr chan)
{
	if (!headChan) {
		*NextOf(chan) = nullptr;
	}
	else {
		assert(nullptr == *PrevOf(headChan));
		*PrevOf(headChan) = chan;
		*NextOf(chan) = headChan;
	}

	headChan = chan;
	*PrevOf(chan) = nullptr;

	nManagedChans++;
}

static void Unlink(SndChannelPtr chan)
{
	if (headChan == chan)
		headChan = *NextOf(chan);

	if (*PrevOf(chan))
		*NextOf(*PrevOf(chan)) = *NextOf(chan);

	*PrevOf(chan) = nullptr;
	*NextOf(chan) = nullptr;

	nManagedChans--;
}

//-----------------------------------------------------------------------------
// Sound Manager

OSErr GetDefaultOutputVolume(long* stereoLevel)
{
	unsigned short g = (unsigned short)(cmixer::GetMasterGain() / 256.0);
	*stereoLevel = (g << 16) | g;
	return noErr;
}

// See IM:S:2-139, "Controlling Volume Levels".
OSErr SetDefaultOutputVolume(long stereoLevel)
{
	unsigned short left  = 0xFFFF & stereoLevel;
	unsigned short right = 0xFFFF & (stereoLevel >> 16);
	if (right != left)
		TODOMINOR2("setting different volumes for left & right is not implemented");
	LOG << left / 256.0 << "\n";
	cmixer::SetMasterGain(left / 256.0);
	return noErr;
}

// IM:S:2-127
OSErr SndNewChannel(SndChannelPtr* chan, short synth, long init, SndCallBackProcPtr userRoutine)
{
	if (synth != sampledSynth) {
		TODO2("unimplemented synth type " << sampledSynth);
		return unimpErr;
	}

	//---------------------------
	// Do allocs

	bool allocatedByPomme = false;

	ChannelEx* impl = new ChannelEx;
	*impl = {};

	if (!*chan) {
		*chan = new SndChannel;
		(**chan) = {};
		impl->macChannelStructAllocatedByPomme = true;
	}
	else {
		impl->macChannelStructAllocatedByPomme = false;
	}

	(**chan).firstMod = (Ptr)impl;

	//---------------------------
	// Set up

	Link(*chan);	// Link chan into our list of managed chans
	(**chan).callBack = userRoutine;

	//---------------------------
	// Done

	LOG << "New channel created, total managed channels = " << nManagedChans << "\n";

	return noErr;
}

// IM:S:2-129
OSErr SndDisposeChannel(SndChannelPtr chan, Boolean quietNow)
{
	Unlink(chan);

	auto* ex = &GetEx(chan);

	bool alsoDeleteMacStruct = ex->macChannelStructAllocatedByPomme;
	delete ex;
	chan->firstMod = nullptr;

	if (alsoDeleteMacStruct) {
		delete chan;
	}

	TODOMINOR2("issue flushCmd, quietCmd, etc. (IM: S ");

	return noErr;
}

OSErr SndChannelStatus(SndChannelPtr chan, short theLength, SCStatusPtr theStatus)
{
	memset(theStatus, 0, sizeof(SCStatus));

	auto& ex = GetEx(chan);

	if (ex.stream) {
		theStatus->scChannelPaused = ex.stream->GetState() == cmixer::CM_STATE_PAUSED;
		theStatus->scChannelBusy   = ex.stream->GetState() == cmixer::CM_STATE_PLAYING;
	}

	return noErr;
}

static void ProcessSoundCmd(SndChannelPtr chan, const Ptr sndhdr)
{
	// Install a sampled sound as a voice in a channel. If the high bit of the
	// command is set, param2 is interpreted as an offset from the beginning of
	// the 'snd ' resource containing the command to the sound header. If the
	// high bit is not set, param2 is interpreted as a pointer to the sound
	// header.

	cmixer::WavStream** stream = &GetEx(chan).stream;

	if (*stream) {
		(**stream).Stop();
		delete *stream;
		*stream = nullptr;
	}

	// PACKED RECORD
	std::istrstream f0(sndhdr, 22+42);
	Pomme::BigEndianIStream f(f0);

	if (f.Read<SInt32>() != 0) {
		TODOFATAL2("expected 0 at the beginning of an snd");
	}

	SInt32	mysteryNumber	= f.Read<SInt32>(); // the meaning of this is decided by the encoding just a bit later
	Fixed	fixedSampleRate	= f.Read<Fixed>();
	UInt32	loopStart		= f.Read<UInt32>();
	UInt32	loopEnd			= f.Read<UInt32>();
	Byte	encoding		= f.Read<Byte>();
	Byte	baseFrequency	= f.Read<Byte>(); // 0-127, see Table 2-2, IM:S:2-43

	int		sampleRate		= (((unsigned)fixedSampleRate) >> 16) & 0xFFFF;

	switch (encoding) {
	case 0x00: // stdSH - standard sound header - IM:S:2-104
	{
		SInt32 nBytes = mysteryNumber; // first field meant nBytes

		// noncompressed sample data (8-bit mono) from this point on
		auto here = sndhdr + f.Tell();

		*stream = new cmixer::WavStream(sampleRate, 8, 1, std::vector<char>(here, here + nBytes));
		break;
	}

	case 0xFF: // extSH - extended sound header - IM:S:2-106
	{
		SInt32 nChannels = mysteryNumber; // first field meant nChannels

		// fields that follow baseFrequency
		SInt32 nFrames = f.Read<SInt32>();
		f.Skip(22);
		SInt16 bitDepth = f.Read<SInt16>();
		f.Skip(14);

		int nBytes = nChannels * nFrames * bitDepth / 8;

		// noncompressed sample data (big endian) from this point on
		auto here = sndhdr + f.Tell();

		// TODO: Get rid of gratuitous buffer copy
		// TODO: Get rid of gratuitous new
		*stream = new cmixer::WavStream(sampleRate, bitDepth, nChannels, std::vector<char>(here, here + nBytes));
		(**stream).bigEndian = true;
		break;
	}

	case 0xFE: // cmpSH - compressed sound header - IM:S:2-108
	{
		SInt32 nChannels = mysteryNumber; // first field meant nChannels

		// fields that follow baseFrequency
		SInt32 nCompressedChunks = f.Read<SInt32>();
		f.Skip(14);
		OSType format = f.Read<OSType>();
		f.Skip(20);
		
		// compressed sample data from this point on
		auto here = sndhdr + f.Tell();

		switch (format) {
		case 'ima4':
		{
			auto nBytes = 34 * nChannels * nCompressedChunks;
			// TODO: Get rid of gratuitous buffer copy
			auto decoded = Pomme::Sound::DecodeIMA4(std::vector<Byte>(here, here + nBytes), nChannels);
			// TODO: Get rid of gratuitous buffer copy
			*stream = new cmixer::WavStream(sampleRate, 16, nChannels,
				std::vector<char>((char*)decoded.data(), (char*)(decoded.data() + decoded.size())));
			break;
		}

		default:
			TODOFATAL2("unsupported snd compression format " << format);
		}
		break;
	}

	default:
		TODOFATAL2("unsupported snd header encoding " << (int)encoding);
	}

	if (*stream)
		(**stream).Play();
}

OSErr SndDoImmediate(SndChannelPtr chan, const SndCommand* cmd)
{
	//4,3,80,42
	switch (cmd->cmd & 0x7FFF) {
	case nullCmd:
		break;

	case quietCmd:
		if (GetMixSource(chan))
			GetMixSource(chan)->Stop();
		break;

	case soundCmd:
		LOG << "ProcessSoundCmd " << cmd->cmd << "(" << cmd->param1 << "," << cmd->param2 << ")\n";
		ProcessSoundCmd(chan, cmd->ptr);
		break;

	default:
		TODOMINOR2(cmd->cmd << "(" << cmd->param1 << "," << cmd->param2 << ")");
	}

	return noErr;
}

template<typename T>
static void Expect(const T a, const T b, const char* msg)
{
	if (a != b)
		throw std::exception(msg);
}

// IM:S:2-58 "MyGetSoundHeaderOffset"
OSErr GetSoundHeaderOffset(SndListHandle sndHandle, long* offset)
{
	std::istrstream f0((Ptr)*sndHandle, GetHandleSize((Handle)sndHandle));
	Pomme::BigEndianIStream f(f0);

	// Skip everything before sound commands
	Expect<SInt16>(1, f.Read<SInt16>(), "'snd ' format");
	Expect<SInt16>(1, f.Read<SInt16>(), "'snd ' modifier count");
	Expect<SInt16>(5, f.Read<SInt16>(), "'snd ' sampledSynth");
	UInt32 initBits = f.Read<UInt32>();

	SInt16 nCmds = f.Read<SInt16>();
	LOG << nCmds << " commands\n";
	for (; nCmds >= 1; nCmds--) {
		// todo: ReadStruct<SndCommand>, packfmt = Hhl (noalign)
		UInt16 cmd = f.Read<UInt16>();
		SInt16 param1 = f.Read<SInt16>();
		SInt32 param2 = f.Read<SInt32>();
		cmd &= 0x7FFF; // See IM:S:2-75
		// When a sound command contained in an 'snd ' resource has associated sound data,
		// the high bit of the command is set. This changes the meaning of the param2 field of the
		// command from a pointer to a location in RAM to an offset value that specifies the offset
		// in bytes from the resource's beginning to the location of the associated sound data (such
		// as a sampled sound header). 
		LOG << "command " << cmd << "\n";
		if (cmd == bufferCmd || cmd == soundCmd) {
			LOG << "offset found! " << param2 << "\n";
			*offset = param2;
			return noErr;
		}
	}
	
	LOG << "didn't find offset in snd resource\n";
	return badFormat;
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

	if (!chan) {
		if (async) // async requires passing in a channel
			return badChannel;
		TODO2("nullptr chan for sync play, check IM:S:1-37");
		return unimpErr;
	}

	if (theSelection) {
		TODO2("audio selection record not implemented");
		return unimpErr;
	}

	Pomme::Sound::AudioClip clip = Pomme::Sound::ReadAIFF(Pomme::Files::GetStream(fRefNum));

	auto& impl = GetEx(chan);

	if (impl.stream) {
		delete impl.stream;
		impl.stream = nullptr;
	}

	// TODO: don't use new/delete
	// TODO: get rid of the gratuitous buffer copy
	impl.stream = new cmixer::WavStream(clip.sampleRate, 16, clip.nChannels, std::vector<char>(clip.pcmData));

	impl.stream->Play();

	if (!async) {
		while (impl.stream->GetState() != cmixer::CM_STATE_STOPPED) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		theCompletion(chan);
		delete impl.stream;
		impl.stream = nullptr;
		return noErr;
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
