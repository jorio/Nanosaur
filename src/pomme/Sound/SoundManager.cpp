#include "Pomme.h"
#include "PommeFiles.h"
#include "Sound/cmixer.h"
#include "PommeSound.h"
#include "Utilities/BigEndianIStream.h"
#include "Utilities/memstream.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>
#include <cstring>

#define LOG POMME_GENLOG(POMME_DEBUG_SOUND, "SOUN")
#define LOG_NOPREFIX POMME_GENLOG_NOPREFIX(POMME_DEBUG_SOUND)

static struct ChannelImpl* headChan = nullptr;
static int nManagedChans = 0;
static double midiNoteFrequencies[128];

//-----------------------------------------------------------------------------
// Cookie-cutter sound command list.
// Used to generate 'snd ' resources.

static const char kSampledSoundCommandList[20] = {
	0,1,			// format
	0,1,			// modifier count
	0,5,			// modifier "sampled synth"
	0,0,0,0,		// init bits
	0,1,			// command count
	(char)0x80,soundCmd,	// command soundCmd (high bit set)
	0,0,			// param1
	0,0,0,20,		// param2 (offset)
	// Sample data follows
};

constexpr int kSampledSoundCommandListLength = sizeof(kSampledSoundCommandList);

//-----------------------------------------------------------------------------
// 'snd ' resource header

struct SampledSoundHeader
{
	UInt32	zero;
	union						// the meaning of union this is decided by the encoding type
	{
		SInt32	stdSH_nBytes;
		SInt32	cmpSH_nChannels;
		SInt32	extSH_nChannels;
		SInt32	nativeSH_nBytes;
	};
	Fixed	fixedSampleRate;
	UInt32	loopStart;
	UInt32	loopEnd;
	Byte	encoding;
	Byte	baseFrequency;		 // 0-127, see Table 2-2, IM:S:2-43

	unsigned int sampleRate() const
	{
		return (static_cast<unsigned int>(fixedSampleRate) >> 16) & 0xFFFF;
	}
};

static_assert(sizeof(SampledSoundHeader) >= 22 && sizeof(SampledSoundHeader) <= 24,
	"unexpected SampledSoundHeader size");

constexpr int kSampledSoundHeaderLength = 22;
constexpr const char* kSampledSoundHeaderPackFormat = "IiIIIbb";

enum SampledSoundEncoding
{
	stdSH				= 0x00,
	nativeSH_mono16		= 0x10,		// pomme extension
	nativeSH_stereo16	= 0x11,		// pomme extension
	cmpSH				= 0xFE,
	extSH				= 0xFF,
};

//-----------------------------------------------------------------------------
// Internal channel info

struct ChannelImpl
{
private:
	ChannelImpl* prev;
	ChannelImpl* next;

public:
	SndChannelPtr macChannel;

	bool macChannelStructAllocatedByPomme;
	cmixer::WavStream source;
	FilePlayCompletionProcPtr onComplete;

	Byte baseNote = kMiddleC;
	Byte playbackNote = kMiddleC;
	double pitchMult = 1;
	bool temporaryPause = false;
	
	ChannelImpl(SndChannelPtr _macChannel, bool transferMacChannelOwnership)
		: macChannel(_macChannel)
		, macChannelStructAllocatedByPomme(transferMacChannelOwnership)
		, source()
		, onComplete(nullptr)
		, baseNote(kMiddleC)
		, playbackNote(kMiddleC)
		, pitchMult(1.0)
	{
		macChannel->firstMod = (Ptr)this;
		
		Link();  // Link chan into our list of managed chans
	}
	
	~ChannelImpl()
	{
		Unlink();

		macChannel->firstMod = nullptr;

		if (macChannelStructAllocatedByPomme) {
			delete macChannel;
		}
	}

	void Recycle()
	{
		source.Clear();
		baseNote = kMiddleC;
		playbackNote = kMiddleC;
		pitchMult = 1;
		temporaryPause = false;
	}

	void ApplyPitch()
	{
		if (!source.active) {
			return;
		}
		double baseFreq = midiNoteFrequencies[baseNote];
		double playbackFreq = midiNoteFrequencies[playbackNote];
		source.SetPitch(pitchMult * playbackFreq / baseFreq);
	}

	ChannelImpl* GetPrev() const
	{
		return prev;
	}
	
	ChannelImpl* GetNext() const
	{
		return next;
	}
	
	void SetPrev(ChannelImpl* newPrev)
	{
		prev = newPrev;
	}
	
	void SetNext(ChannelImpl* newNext)
	{
		next = newNext;
		macChannel->nextChan = newNext ? newNext->macChannel : nullptr;
	}
	
	void Link()
	{
		if (!headChan) {
			SetNext(nullptr);
		}
		else {
			assert(nullptr == headChan->GetPrev());
			headChan->SetPrev(this);
			SetNext(headChan);
		}

		headChan = this;
		SetPrev(nullptr);

		nManagedChans++;
	}

	void Unlink()
	{
		if (headChan == this) {
			headChan = GetNext();
		}

		if (nullptr != GetPrev()) {
			GetPrev()->SetNext(GetNext());
		}

		if (nullptr != GetNext()) {
			GetNext()->SetPrev(GetPrev());
		}

		SetPrev(nullptr);
		SetNext(nullptr);

		nManagedChans--;
	}
};

//-----------------------------------------------------------------------------
// Internal utilities

static inline ChannelImpl& GetImpl(SndChannelPtr chan)
{
	return *(ChannelImpl*)chan->firstMod;
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
	unsigned short g = (unsigned short)(cmixer::GetMasterGain() * 256.0);
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
OSErr SndNewChannel(SndChannelPtr* macChanPtr, short synth, long init, SndCallBackProcPtr userRoutine)
{
	if (synth != sampledSynth) {
		TODO2("unimplemented synth type " << sampledSynth);
		return unimpErr;
	}

	//---------------------------
	// Allocate Mac channel record if needed
	
	bool transferMacChannelOwnership = false;

	if (!*macChanPtr) {
		*macChanPtr = new SndChannel;
		(**macChanPtr) = {};
		transferMacChannelOwnership = true;
	}

	//---------------------------
	// Set up
	
	(**macChanPtr).callBack = userRoutine;
	new ChannelImpl(*macChanPtr, transferMacChannelOwnership);

	//---------------------------
	// Done

	LOG << "New channel created, init = $" << std::hex << init << std::dec << ", total managed channels = " << nManagedChans << "\n";

	return noErr;
}

// IM:S:2-129
OSErr SndDisposeChannel(SndChannelPtr macChanPtr, Boolean quietNow)
{
	if (!quietNow) {
		TODO2("SndDisposeChannel: quietNow == false is not implemented");
	}
	delete &GetImpl(macChanPtr);
	return noErr;
}

OSErr SndChannelStatus(SndChannelPtr chan, short theLength, SCStatusPtr theStatus)
{
	*theStatus = {};

	auto& source = GetImpl(chan).source;

	theStatus->scChannelPaused = source.GetState() == cmixer::CM_STATE_PAUSED;
	theStatus->scChannelBusy   = source.GetState() == cmixer::CM_STATE_PLAYING;

	return noErr;
}

static void ProcessSoundCmd(SndChannelPtr chan, const Ptr sndhdr)
{
	// Install a sampled sound as a voice in a channel. If the high bit of the
	// command is set, param2 is interpreted as an offset from the beginning of
	// the 'snd ' resource containing the command to the sound header. If the
	// high bit is not set, param2 is interpreted as a pointer to the sound
	// header.

	auto& impl = GetImpl(chan);

	impl.Recycle();

	// PACKED RECORD
	memstream headerInput(sndhdr, kSampledSoundHeaderLength+42);
	Pomme::BigEndianIStream f(headerInput);

	SampledSoundHeader sh;
	f.Read(reinterpret_cast<char*>(&sh), kSampledSoundHeaderLength);
	ByteswapStructs(kSampledSoundHeaderPackFormat, kSampledSoundHeaderLength, 1, reinterpret_cast<char*>(&sh));

	if (sh.zero != 0) {
		// The first field can be a pointer to the sampled-sound data.
		// In practice it's always gonna be 0.
		TODOFATAL2("expected 0 at the beginning of an snd");
	}

	int sampleRate = sh.sampleRate();
	impl.baseNote = sh.baseFrequency;

	LOG << sampleRate << "Hz, " << GetMidiNoteName(sh.baseFrequency) << ", loop " << sh.loopStart << "->" << sh.loopEnd << ", ";

	switch (sh.encoding) {
	case 0x00: // stdSH - standard sound header - IM:S:2-104
	{
		// noncompressed sample data (8-bit mono) from this point on
		char* here = sndhdr + f.Tell();
		impl.source.Init(sampleRate, 8, 1, false, std::span<char>(here, sh.stdSH_nBytes));
		LOG_NOPREFIX << "stdSH: 8-bit mono, " << sh.stdSH_nBytes << " frames\n";
		break;
	}

	case nativeSH_mono16:
	case nativeSH_stereo16:
	{
		int nChannels = sh.encoding == nativeSH_mono16 ? 1 : 2;
		char* here = sndhdr + f.Tell();
		auto span = std::span(here, sh.nativeSH_nBytes);
		impl.source.Init(sampleRate, 16, nChannels, false, span);
		LOG_NOPREFIX << "nativeSH\n";
		break;
	}

	case 0xFF: // extSH - extended sound header - IM:S:2-106
	{
		// fields that follow baseFrequency
		SInt32 nFrames = f.Read<SInt32>();
		f.Skip(22);
		SInt16 bitDepth = f.Read<SInt16>();
		f.Skip(14);

		int nBytes = sh.extSH_nChannels * nFrames * bitDepth / 8;

		// noncompressed sample data (big endian) from this point on
		char* here = sndhdr + f.Tell();

		LOG_NOPREFIX << "extSH: " << bitDepth << "-bit " << (sh.extSH_nChannels == 1? "mono": "stereo") << ", " << nFrames << " frames\n";

		impl.source.Init(sampleRate, bitDepth, sh.extSH_nChannels, true, std::span<char>(here, nBytes));
		break;
	}

	case 0xFE: // cmpSH - compressed sound header - IM:S:2-108
	{
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
		char* here = sndhdr + f.Tell();

		std::cout << "cmpSH: " << Pomme::FourCCString(format) << " " << (sh.cmpSH_nChannels == 1 ? "mono" : "stereo") << ", " << nCompressedChunks << " ck\n";

		std::unique_ptr<Pomme::Sound::Codec> codec = Pomme::Sound::GetCodec(format);

		// Decompress
		int nBytesIn  = sh.cmpSH_nChannels * nCompressedChunks * codec->BytesPerPacket();
		int nBytesOut = sh.cmpSH_nChannels * nCompressedChunks * codec->SamplesPerPacket() * 2;

		auto spanIn = std::span(here, nBytesIn);
		auto spanOut = impl.source.GetBuffer(nBytesOut);

		codec->Decode(sh.cmpSH_nChannels, spanIn, spanOut);
		impl.source.Init(sampleRate, 16, sh.cmpSH_nChannels, false, spanOut);

		break;
	}

	default:
		TODOFATAL2("unsupported snd header encoding " << (int)sh.encoding);
	}

	if (sh.loopEnd - sh.loopStart <= 1) {
		// don't loop
	}
	else if (sh.loopStart == 0) {
		impl.source.SetLoop(true);
	}
	else {
		TODO2("looping on a portion of the snd isn't supported yet");
		impl.source.SetLoop(true);
	}

	impl.ApplyPitch();
	impl.temporaryPause = false;
	impl.source.Play();
}

OSErr SndDoImmediate(SndChannelPtr chan, const SndCommand* cmd)
{
	auto& impl = GetImpl(chan);

	// Discard the high bit of the command (it indicates whether an 'snd ' resource has associated data).
	switch (cmd->cmd & 0x7FFF) {
	case nullCmd:
		break;

	case flushCmd:
		// flushCmd is a no-op for now because we don't support queuing commands--
		// all commands are executed immediately in the current implementation.
		break;

	case quietCmd:
		impl.source.Stop();
		break;

	case soundCmd:
		ProcessSoundCmd(chan, cmd->ptr);
		break;

	case ampCmd:
		impl.source.SetGain(cmd->param1 / 255.0);
		LOG << "ampCmd " << impl.source.gain << "\n";
		break;

	case freqCmd:
		LOG << "freqCmd " << cmd->param2 << " " << GetMidiNoteName(cmd->param2) << " " << midiNoteFrequencies[cmd->param2] << "\n";
		impl.playbackNote = Byte(cmd->param2);
		impl.ApplyPitch();
		break;

	case rateCmd:
		// IM:S says it's a fixed-point multiplier of 22KHz, but Nanosaur uses rate "1" everywhere,
		// even for sounds sampled at 44Khz, so I'm treating it as just a pitch multiplier.
		impl.pitchMult = cmd->param2 / 65536.0;
		impl.ApplyPitch();
		break;

	case pommeSetLoopCmd:
		impl.source.SetLoop(cmd->param1);
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
		throw std::runtime_error(msg);
}

// IM:S:2-58 "MyGetSoundHeaderOffset"
OSErr GetSoundHeaderOffset(SndListHandle sndHandle, long* offset)
{
	memstream sndStream((Ptr)*sndHandle, GetHandleSize((Handle)sndHandle));
	Pomme::BigEndianIStream f(sndStream);

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
		UInt16 cmd = f.Read<UInt16>();
		f.Skip(2); // SInt16 param1
		SInt32 param2 = f.Read<SInt32>();
		cmd &= 0x7FFF; // See IM:S:2-75
		// When a sound command contained in an 'snd ' resource has associated sound data,
		// the high bit of the command is set. This changes the meaning of the param2 field of the
		// command from a pointer to a location in RAM to an offset value that specifies the offset
		// in bytes from the resource's beginning to the location of the associated sound data (such
		// as a sampled sound header). 
		if (cmd == bufferCmd || cmd == soundCmd) {
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

	auto& impl = GetImpl(chan);
	impl.Recycle();

	auto& stream = Pomme::Files::GetStream(fRefNum);
	// Rewind -- the file might've been fully played already and we might just be trying to loop it
	stream.seekg(0, std::ios::beg);
	Pomme::Sound::ReadAIFF(stream, impl.source);

	if (theCompletion) {
		impl.source.onComplete = [=]() { theCompletion(chan); };
	}

	impl.temporaryPause = false;
	impl.source.Play();

	if (!async) {
		while (impl.source.GetState() != cmixer::CM_STATE_STOPPED) {
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
	GetImpl(chan).source.TogglePause();
	return noErr;
}

OSErr SndStopFilePlay(SndChannelPtr chan, Boolean quietNow)
{
	// TODO: check that chan is being used for play from disk
	if (!quietNow)
		TODO2("quietNow==false not supported yet, sound will be cut off immediately instead");
	GetImpl(chan).source.Stop();
	return noErr;
}

NumVersion SndSoundManagerVersion()
{
	NumVersion v = {};
	v.majorRev = 3;
	v.minorAndBugRev = 9;
	v.stage = 0x80;
	v.nonRelRev = 0;
	return v;
}

//-----------------------------------------------------------------------------
// Extension: decompress

Boolean Pomme_DecompressSoundResource(SndListHandle* sndHandlePtr, long* offsetToHeader)
{
	// Prep the BE reader on the header.
	Ptr sndhdr = (Ptr)(**sndHandlePtr) + *offsetToHeader;
	memstream headerInput(sndhdr, kSampledSoundHeaderLength + 42);
	Pomme::BigEndianIStream f(headerInput);

	// Read in SampledSoundHeader and unpack it.
	SampledSoundHeader sh;
	f.Read(reinterpret_cast<char*>(&sh), kSampledSoundHeaderLength);
	ByteswapStructs(kSampledSoundHeaderPackFormat, kSampledSoundHeaderLength, 1, reinterpret_cast<char*>(&sh));

	// We only handle cmpSH (compressed) 'snd ' resources.
	if (sh.encoding != cmpSH) {
		return false;
	}

	// Fields that follow SampledSoundHeader when the encoding is cmpSH.
	const auto nCompressedChunks = f.Read<SInt32>();
	f.Skip(14);
	const auto format = f.Read<OSType>();
	f.Skip(20);

	// Compressed sample data in the input stream from this point on.

	const char* here = sndhdr + f.Tell();

	int outInitialSize = kSampledSoundCommandListLength + kSampledSoundHeaderLength;

	std::unique_ptr<Pomme::Sound::Codec> codec = Pomme::Sound::GetCodec(format);

	// Decompress
	const int nBytesIn  = sh.cmpSH_nChannels * nCompressedChunks * codec->BytesPerPacket();
	const int nBytesOut = sh.cmpSH_nChannels * nCompressedChunks * codec->SamplesPerPacket() * 2;
	SndListHandle outHandle = (SndListHandle)NewHandle(outInitialSize + nBytesOut);
	auto spanIn = std::span(here, nBytesIn);
	auto spanOut = std::span((char*)*outHandle + outInitialSize, nBytesOut);
	codec->Decode(sh.cmpSH_nChannels, spanIn, spanOut);

	// ------------------------------------------------------
	// Now we have the PCM data.
	// Put the output 'snd ' resource together.
	
	SampledSoundHeader shOut = sh;
	shOut.zero = 0;
	shOut.encoding = sh.cmpSH_nChannels == 2 ? nativeSH_stereo16 : nativeSH_mono16;
	shOut.nativeSH_nBytes = nBytesOut;
	ByteswapStructs(kSampledSoundHeaderPackFormat, kSampledSoundHeaderLength, 1, reinterpret_cast<char*>(&shOut));

	memcpy(*outHandle, kSampledSoundCommandList, kSampledSoundCommandListLength);
	memcpy((char*)*outHandle + kSampledSoundCommandListLength, &shOut, kSampledSoundHeaderLength);

	// Nuke compressed sound handle, replace it with the decopmressed one we've just created
	DisposeHandle((Handle)*sndHandlePtr);
	*sndHandlePtr = outHandle;
	*offsetToHeader = kSampledSoundCommandListLength;

	long offsetCheck = 0;
	OSErr err = GetSoundHeaderOffset(outHandle, &offsetCheck);
	if (err != noErr || offsetCheck != kSampledSoundCommandListLength) {
		throw std::runtime_error("Incorrect decompressed sound header offset");
	}

	return true;
}

//-----------------------------------------------------------------------------
// Extension: pause/unpause looping channels

void Pomme_PauseLoopingChannels(Boolean pause)
{
	for (auto* chan = headChan; chan; chan = chan->GetNext()) {
		auto& source = chan->source;
		if (pause && source.state == cmixer::CM_STATE_PLAYING && !chan->temporaryPause) {
			source.Pause();
			chan->temporaryPause = true;
		} else if (!pause && source.state == cmixer::CM_STATE_PAUSED && chan->temporaryPause) {
			source.Play();
			chan->temporaryPause = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Init Sound Manager

void Pomme::Sound::Init()
{
	InitMidiFrequencyTable();
	cmixer::InitWithSDL();
}

void Pomme::Sound::Shutdown()
{
	cmixer::ShutdownWithSDL();
	while (headChan) {
		SndDisposeChannel(headChan->macChannel, true);
	}
}


std::unique_ptr<Pomme::Sound::Codec> Pomme::Sound::GetCodec(uint32_t fourCC)
{
	switch (fourCC) {
		case 0: // Assume MACE-3 by default.
		case 'MAC3':
			return std::make_unique<Pomme::Sound::MACE>();
		case 'ima4':
			return std::make_unique<Pomme::Sound::IMA4>();
		case 'alaw':
		case 'ulaw':
			return std::make_unique<Pomme::Sound::xlaw>(fourCC);
		default:
			throw std::runtime_error("Unknown audio codec: " + Pomme::FourCCString(fourCC));
	}
}