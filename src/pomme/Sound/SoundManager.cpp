#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>
#include <strstream>
#include "PommeInternal.h"
#include "Sound/cmixer.h"

#define LOG POMME_GENLOG(POMME_DEBUG_SOUND, "SOUN")
#define LOG_NOPREFIX POMME_GENLOG_NOPREFIX(POMME_DEBUG_SOUND)

static SndChannelPtr headChan = nullptr;
static int nManagedChans = 0;
static double midiNoteFrequencies[128];

// Internal channel info
struct ChannelEx {
	SndChannelPtr prevChan;
	bool macChannelStructAllocatedByPomme;
	cmixer::WavStream* stream;
	FilePlayCompletionProcPtr onComplete;

	Byte baseNote = kMiddleC;
	Byte playbackNote = kMiddleC;
	double pitchMult = 1;

	void Recycle() {
		if (stream) {
			// TODO: don't use new/delete
			delete stream;
			stream = nullptr;
		}
		baseNote = kMiddleC;
		playbackNote = kMiddleC;
		pitchMult = 1;
	}

	void ApplyPitch() {
		if (!stream) return;
		double baseFreq = midiNoteFrequencies[baseNote];
		double playbackFreq = midiNoteFrequencies[playbackNote];
		stream->SetPitch(pitchMult * playbackFreq / baseFreq);
	}
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
// MIDI note utilities

// Note: these names are according to IM:S:2-43.
// These names won't match real-world names.
// E.g. for note 67 (A 440Hz), this will return "A6", whereas the real-world
// convention for that note is "A4".
static std::string GetMidiNoteName(int i)
{
	static const char* gamme[12] = { "A","A#","B","C","C#","D","D#","E","F","F#","G","G#" };

	int octave = 1 + (i + 3) / 12;
	int semitonesFromA = (i + 3) % 12;

	std::stringstream ss;
	ss << gamme[semitonesFromA] << octave;
	return ss.str();
}

static void InitMidiFrequencyTable()
{
	// powers of twelfth root of two
	double gamme[12];
	gamme[0] = 1.0;
	for (int i = 1; i < 12; i++)
		gamme[i] = gamme[i - 1] * 1.059630943592952646;

	for (int i = 0; i < 128; i++) {
		int octave = 1 + (i + 3) / 12; // A440 and middle C are in octave 7
		int semitone = (i + 3) % 12; // halfsteps up from A in current octave
		if (octave < 7)
			midiNoteFrequencies[i] = gamme[semitone] * 440.0 / (1 << (7 - octave)); // 440/(2**octaveDiff)
		else
			midiNoteFrequencies[i] = gamme[semitone] * 440.0 * (1 << (octave - 7)); // 440*(2**octaveDiff)
		//LOG << i << "\t" << GetMidiNoteName(i) << "\t" << midiNoteFrequencies[i] << "\n";
	}
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
	impl->baseNote = kMiddleC;

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

	LOG << "New channel created, init = $" << std::hex << init << std::dec << ", total managed channels = " << nManagedChans << "\n";

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

	GetEx(chan).Recycle();

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
	GetEx(chan).baseNote = baseFrequency;

	LOG << sampleRate << "Hz, " << GetMidiNoteName(baseFrequency) << ", loop " << loopStart << "->" << loopEnd << ", ";

	switch (encoding) {
	case 0x00: // stdSH - standard sound header - IM:S:2-104
	{
		SInt32 nBytes = mysteryNumber; // first field meant nBytes

		// noncompressed sample data (8-bit mono) from this point on
		auto here = sndhdr + f.Tell();

		LOG_NOPREFIX << "stdSH: 8-bit mono, " << nBytes << " frames\n";

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

		LOG_NOPREFIX << "extSH: " << bitDepth << "-bit " << (nChannels == 1? "mono": "stereo") << ", " << nFrames << " frames\n";

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

		if (format == 0) {
			// Assume MACE-3. It should've been set in the init options in the snd pre-header,
			// but Nanosaur doesn't actually init the sound channels for MACE-3. So I guess the Mac
			// assumes by default that any unspecified compression is MACE-3.
			// If it wasn't MACE-3, it would've been caught by GetSoundHeaderOffset.
			format = 'MAC3';
		}
		
		// compressed sample data from this point on
		auto here = sndhdr + f.Tell();

		LOG_NOPREFIX << "cmpSH: " << Pomme::FourCCString(format) << " " << (nChannels == 1 ? "mono" : "stereo") << ", " << nCompressedChunks << " ck, ";

		switch (format) {
		case 'ima4':
		{
			int nBytes = 34 * nChannels * nCompressedChunks;
			// TODO: Get rid of gratuitous buffer copy
			auto decoded = Pomme::Sound::DecodeIMA4(std::vector<Byte>(here, here + nBytes), nChannels);
			LOG_NOPREFIX << nBytes << " B (" << nBytes/nChannels << "), " << decoded.size() / nChannels << " frames\n";
			// TODO: Get rid of gratuitous buffer copy
			*stream = new cmixer::WavStream(sampleRate, 16, nChannels,
				std::vector<char>((char*)decoded.data(), (char*)(decoded.data() + decoded.size())));
			break;
		}

		case 'MAC3':
		{
			int nBytes = 2 * nChannels * nCompressedChunks;
			// TODO: Get rid of gratuitous buffer copy
			auto decoded = Pomme::Sound::DecodeMACE3(std::vector<Byte>(here, here + nBytes), nChannels);
			LOG_NOPREFIX << nBytes << " B (" << nBytes / nChannels << "), " << decoded.size() / nChannels << " frames\n";
			// TODO: Get rid of gratuitous buffer copy
			*stream = new cmixer::WavStream(sampleRate, 16, nChannels,
				std::vector<char>((char*)decoded.data(), (char*)(decoded.data() + decoded.size())));
			break;
		}

		default:
			TODOFATAL2("unsupported snd compression format " << Pomme::FourCCString(format));
		}
		break;
	}

	default:
		TODOFATAL2("unsupported snd header encoding " << (int)encoding);
	}

	if (*stream) {
		if (loopEnd - loopStart <= 1) {
			// don't loop
		}
		else if (loopStart == 0) {
			(**stream).SetLoop(true);
		}
		else {
			TODO2("looping on a portion of the snd isn't supported yet");
			(**stream).SetLoop(true);
		}

		GetEx(chan).ApplyPitch();
		(**stream).Play();
	}
}

OSErr SndDoImmediate(SndChannelPtr chan, const SndCommand* cmd)
{
	auto& impl = GetEx(chan);

	switch (cmd->cmd & 0x7FFF) {
	case nullCmd:
		break;

	case flushCmd:
		// flushCmd is a no-op for now because we don't support queuing commands--
		// all commands are executed immediately in the current implementation.
		break;

	case quietCmd:
		if (GetMixSource(chan))
			GetMixSource(chan)->Stop();
		break;

	case soundCmd:
		ProcessSoundCmd(chan, cmd->ptr);
		break;

	case ampCmd:
	{
		double desiredAmplitude = cmd->param1 / 255.0;
		LOG << "ampCmd " << desiredAmplitude << "\n";
		if (GetEx(chan).stream)
			GetEx(chan).stream->SetGain(desiredAmplitude);
		break;
	}

	case freqCmd:
	{
		LOG << "freqCmd " << cmd->param2 << " " << GetMidiNoteName(cmd->param2) << " " << midiNoteFrequencies[cmd->param2] << "\n";
		impl.playbackNote = Byte(cmd->param2);
		impl.ApplyPitch();
		break;
	}

	case rateCmd:
	{
		// IM:S says it's a fixed-point multiplier of 22KHz, but Nanosaur uses rate "1" everywhere,
		// even for sounds sampled at 44Khz, so I'm treating it as just a pitch multiplier.
		impl.pitchMult = cmd->param2 / 65536.0;
		impl.ApplyPitch();
		break;
	}

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

	if (initBits & initMACE6)
		TODOFATAL2("MACE-6 not supported yet");

	SInt16 nCmds = f.Read<SInt16>();
	//LOG << nCmds << " commands\n";
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
		//LOG << "command " << cmd << "\n";
		if (cmd == bufferCmd || cmd == soundCmd) {
			//LOG << "offset found! " << param2 << "\n";
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

	auto& stream = Pomme::Files::GetStream(fRefNum);
	// Rewind -- the file might've been fully played already and we might just be trying to loop it
	stream.seekg(0, std::ios::beg);
	Pomme::Sound::AudioClip clip = Pomme::Sound::ReadAIFF(stream);

	auto& impl = GetEx(chan);

	impl.Recycle();

	// TODO: don't use new/delete
	// TODO: get rid of the gratuitous buffer copy
	impl.stream = new cmixer::WavStream(clip.sampleRate, 16, clip.nChannels, std::vector<char>(clip.pcmData));

	if (theCompletion)
		impl.stream->onComplete = [=]() { theCompletion(chan); };
	else
		impl.stream->onComplete = nullptr;

	impl.stream->Play();

	if (!async) {
		while (impl.stream->GetState() != cmixer::CM_STATE_STOPPED) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		theCompletion(chan);
		impl.Recycle();
		return noErr;
	}

	return noErr;
}

OSErr SndPauseFilePlay(SndChannelPtr chan)
{
	// TODO: check that chan is being used for play from disk
	auto* stream = GetEx(chan).stream;
	if (stream) stream->TogglePause();
	return noErr;
}

OSErr SndStopFilePlay(SndChannelPtr chan, Boolean quietNow)
{
	// TODO: check that chan is being used for play from disk
	if (!quietNow)
		TODO2("quietNow==false not supported yet, sound will be cut off immediately instead");
	auto* stream = GetEx(chan).stream;
	if (stream) stream->Stop();
	return noErr;
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
	InitMidiFrequencyTable();
	cmixer::InitWithSDL();
}
