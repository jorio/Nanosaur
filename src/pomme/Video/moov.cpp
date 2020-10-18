#include "Video/Cinepak.h"
#include "PommeVideo.h"
#include "PommeSound.h"
#include "Utilities/BigEndianIStream.h"
#include "PommeDebug.h"

#include <iostream>
#include <sstream>

using namespace Pomme::Video;

class MoovException: public std::runtime_error {
public:
	MoovException(const std::string m) : std::runtime_error(m) {}
};

//-----------------------------------------------------------------------------
// Utilities

struct ChunkInfo
{
	UInt32 offset;
	UInt32 samplesPerChunk;
};

static void MoovAssert(bool condition, const std::string msg)
{
	if (!condition) {
		throw MoovException(msg);
	}
}

template<typename T>
static void Expect(Pomme::BigEndianIStream& f, const T value, const std::string msg)
{
	T found = f.Read<T>();
	if (value != found) {
		std::stringstream ss;
		ss << "moov parser: " << msg << ": incorrect value: expected " << value << ", found " << found;
		throw MoovException(ss.str());
	}
}

struct AtomGuard
{
	Pomme::BigEndianIStream& f;
	FourCharCode fourCC;
	std::streampos end;
	
	AtomGuard(Pomme::BigEndianIStream& f, FourCharCode requiredAtomType)
		: f(f)
		, fourCC(requiredAtomType)
	{
		auto start = f.Tell();
		auto atomSize = f.Read<UInt32>();
		Expect<FourCharCode>(f, requiredAtomType, "expected atom");
		end = start + (std::streampos)atomSize;
	}
	
	~AtomGuard()
	{
		if (f.Tell() != end)
		{
			std::cerr << "WARNING: "
				<< (f.Tell() < end ? "didn't reach " : "read past ")
				<< "end of atom " << Pomme::FourCCString(fourCC) << "\n";
		}
	}
};

static void RequireAtomAndSkip(Pomme::BigEndianIStream& f, FourCharCode requiredAtomType)
{
	AtomGuard atom(f, requiredAtomType);
	f.Goto(atom.end);
}

static void SkipAtomIfPresent(Pomme::BigEndianIStream& f, const FourCharCode fourCCToSkip)
{
	auto posGuard = f.GuardPos();

	auto atomSize = f.Read<UInt32>();
	auto atomType = f.Read<FourCharCode>();

	if (atomType == fourCCToSkip) {
		f.Skip(atomSize-8);
		posGuard.Cancel();
	}
}

//-----------------------------------------------------------------------------
// Atom parsers

// Sample-to-Chunk
static std::vector<ChunkInfo> Parse_stsc(Pomme::BigEndianIStream& f)
{
	std::vector<ChunkInfo> chunkInfos;
	AtomGuard stsc(f, 'stsc');
	Expect<UInt32>(f, 0, "bad stsc version + flags");
	const auto numberOfEntries = f.Read<UInt32>();
	for (UInt32 i = 0; i < numberOfEntries; i++) {
		ChunkInfo ci = {};
		ci.offset = 0xFFFFFFFF;

		const auto firstChunk = f.Read<UInt32>();
		ci.samplesPerChunk = f.Read<UInt32>();
		Expect<UInt32>(f, 1, "sample description ID");

		// duplicate last chunk
		for (UInt32 j = chunkInfos.size(); j < firstChunk - 1; j++) {
			auto lastChunk = chunkInfos[chunkInfos.size()-1];
			chunkInfos.push_back(lastChunk);
		}

		chunkInfos.push_back(ci);
	}

	return chunkInfos;
}

// Sample Sizes
static std::vector<UInt32> Parse_stsz(Pomme::BigEndianIStream& f)
{
	std::vector<UInt32> sampleSizes;
	AtomGuard stsz(f, 'stsz');
	Expect<UInt32>(f, 0, "stsz version + flags");
	auto globalSampleSize = f.Read<UInt32>();
	auto numberOfEntries = f.Read<UInt32>();
	if (globalSampleSize == 0) {
		for (UInt32 i = 0; i < numberOfEntries; i++) {
			sampleSizes.push_back(f.Read<UInt32>());
		}
	} else {
		sampleSizes.push_back(globalSampleSize);
	}
	return sampleSizes;
}

// Chunk Offsets
static void Parse_stco(Pomme::BigEndianIStream& f, std::vector<ChunkInfo>& chunkList)
{
	AtomGuard stco(f, 'stco');
	Expect<UInt32>(f, 0, "stco version + flags");
	auto numberOfEntries = f.Read<UInt32>();
	for (UInt32 i = 0; i < numberOfEntries; i++) {
		auto chunkOffset = f.Read<UInt32>();
		chunkList.at(i).offset = chunkOffset;
	}
}

static void Parse_mdia_vide(Pomme::BigEndianIStream& f, Movie& movie, UInt32 timeScale)
{
	std::vector<ChunkInfo> chunkList;
	std::vector<UInt32> frameSizes;
	
	{
		AtomGuard minf(f, 'minf');
		RequireAtomAndSkip(f, 'vmhd');
		RequireAtomAndSkip(f, 'hdlr');
		RequireAtomAndSkip(f, 'dinf');
		{
			AtomGuard stbl(f, 'stbl');
			{
				AtomGuard stsd(f, 'stsd');
				Expect<UInt32>(f, 0, "vide stsd version + flags");
				Expect<UInt32>(f, 1, "vide stsd number of entries");
				f.Skip(4); // UInt32 sampleDescriptionSize
				movie.videoFormat = f.Read<FourCharCode>();
				f.Skip(6); // reserved
				f.Skip(2); // data reference index
				Expect<UInt16>(f, 1, "vide stsd version");
				Expect<UInt16>(f, 1, "vide stsd revision level");  // docs say it should be 0, but in practice it's 1
				f.Skip(4); // vendor
				f.Skip(4); // temporal quality
				f.Skip(4); // spatial quality
				movie.width = f.Read<UInt16>();
				movie.height = f.Read<UInt16>();
				f.Skip(4); // horizontal resolution (ppi)
				f.Skip(4); // vertical resolution (ppi)
				Expect<UInt32>(f, 0, "vide stsd data size");
				Expect<UInt16>(f, 1, "vide stsd frame count per sample");
				f.Skip(32); // compressor name
				Expect<UInt16>(f, 24, "pixel depth");
				f.Skip(2); // color table ID
			}
			{
				AtomGuard stts(f, 'stts');
				Expect<UInt32>(f, 0, "stts version + flags");
				Expect<UInt32>(f, 1, "stts number of entries");
				f.Skip(4); // UInt32 sampleCount
				auto sampleDuration = f.Read<UInt32>();
				movie.videoFrameRate = (float)timeScale / sampleDuration;
			}
			SkipAtomIfPresent(f, 'stss');
			chunkList = Parse_stsc(f);
			frameSizes = Parse_stsz(f);
			Parse_stco(f, chunkList);
			SkipAtomIfPresent(f, 'stsh');
		}
	}

	std::cout << "vide: " << Pomme::FourCCString(movie.videoFormat) << ", " << movie.width << "x" << movie.height << ", " << movie.videoFrameRate << "fps\n";

	// ------------------------------------
	// EXTRACT VIDEO FRAMES

	// Set up guard 
	auto guard = f.GuardPos();

	int frameCounter = 0;
	for (const auto& chunk : chunkList)
	{
		f.Goto(chunk.offset);
		for (int s = 0; s < chunk.samplesPerChunk; s++, frameCounter++) {
			const int frameSize = frameSizes[frameCounter];
			movie.videoFrames.push(f.ReadBytes(frameSize));
		}
	}
}

static void Parse_mdia_soun(Pomme::BigEndianIStream& f, Movie& movie)
{
	std::vector<ChunkInfo> chunkList;

	{
		AtomGuard minf(f, 'minf');
		SkipAtomIfPresent(f, 'smhd');
		SkipAtomIfPresent(f, 'hdlr');
		SkipAtomIfPresent(f, 'dinf');
		{
			AtomGuard stbl(f, 'stbl');
			{
				AtomGuard stsd(f, 'stsd');
				Expect<UInt32>(f, 0, "soun stsd version + flags");
				Expect<UInt32>(f, 1, "soun stsd number of entries");
				f.Skip(4); // UInt32 sampleDescriptionSize
				movie.audioFormat = f.Read<FourCharCode>();
				f.Skip(6); // reserved
				f.Skip(2); // data reference index
				Expect<UInt16>(f, 0, "soun stsd version");
				Expect<UInt16>(f, 0, "soun stsd revision level");
				f.Skip(4); // vendor
				movie.audioNChannels = f.Read<UInt16>();
				movie.audioBitDepth = f.Read<UInt16>();
				Expect<UInt16>(f, 0, "soun stsd compression ID");
				Expect<UInt16>(f, 0, "soun stsd packet size");
				Fixed fixedSampleRate = f.Read<Fixed>();
				movie.audioSampleRate = (static_cast<unsigned int>(fixedSampleRate) >> 16) & 0xFFFF;
			}
			{
				AtomGuard stts(f, 'stts');
				Expect<UInt32>(f, 0, "stts version + flags");
				Expect<UInt32>(f, 1, "stts number of entries");
				auto sampleCount = f.Read<UInt32>();
				Expect<UInt32>(f, 1, "soun stts: sample duration");
				movie.audioSampleCount = sampleCount;
			}
			SkipAtomIfPresent(f, 'stss');
			chunkList = Parse_stsc(f); //SkipAtomIfPresent(f, 'stsc');
			auto sampleSize = Parse_stsz(f); //SkipAtomIfPresent(f, 'stsz');
			MoovAssert(1 == sampleSize.size(), "in the sound track, all samples are expected to be of size 1");
			MoovAssert(1 == sampleSize[0], "in the sound track, all samples are expected to be of size 1");
			Parse_stco(f, chunkList);
			SkipAtomIfPresent(f, 'stsh');
		}
	}

	std::cout << "soun: " << Pomme::FourCCString(movie.audioFormat) << ", " << movie.audioNChannels << "ch, " << movie.audioBitDepth << "bit, " << movie.audioSampleRate << "Hz\n";

	// ------------------------------------
	// EXTRACT AUDIO

	// Set up position guard for rest of function
	auto guard = f.GuardPos();
	
	// Unfortunately, Nanosaur's movies use version 0 of the 'stsd' atom for sound tracks.
	// This means that the "bytes per packet" count is not encoded into the file. (QTFF-2001, pp. 100-101)
	// We have to deduce it from the audio format.
	std::vector<int> chunkLengths;
	UInt32 compressedLength = 0;

	for (const auto& chunk : chunkList) {
		int chunkBytes;
		switch (movie.audioFormat)
		{
		case 'twos':
		case 'swot':
			chunkBytes = chunk.samplesPerChunk * movie.audioNChannels * (movie.audioBitDepth / 8);
			break;
		case 'ima4':
			chunkBytes = Pomme::Sound::IMA4::GetInputSize(chunk.samplesPerChunk, movie.audioNChannels);
			break;
		default:
			throw MoovException("Unknown moov audio format: " + Pomme::FourCCString(movie.audioFormat));
		}
		chunkLengths.push_back(chunkBytes);
		compressedLength += chunkBytes;
	}
	
	std::vector<char> compressedSoundData;
	compressedSoundData.reserve(compressedLength);
	char* out = compressedSoundData.data();

	for (int i = 0; i < chunkList.size(); i++) {
		f.Goto(chunkList[i].offset);
		f.Read(out, chunkLengths[i]);
		out += chunkLengths[i];
	}

	MoovAssert(out == compressedSoundData.data() + compressedLength, "csd length != total length");

	switch (movie.audioFormat)
	{
		case 'twos':
		case 'swot':
			movie.audioStream.SetBuffer(std::move(compressedSoundData));
			movie.audioStream.Init(
				movie.audioSampleRate,
				movie.audioBitDepth,
				movie.audioNChannels,
				movie.audioFormat == 'twos',
				movie.audioStream.GetBuffer(compressedLength));
			break;

		case 'ima4':
		{
			auto outBytes = Pomme::Sound::IMA4::GetOutputSize(compressedLength, movie.audioNChannels);
			auto outSpan = movie.audioStream.GetBuffer(outBytes);
			auto inSpan = std::span(compressedSoundData.data(), compressedLength);
			Pomme::Sound::IMA4::Decode(movie.audioNChannels, inSpan, outSpan);
			movie.audioStream.Init(movie.audioSampleRate, 16, movie.audioNChannels, false, outSpan);
			break;
		}
		
		default:
			throw MoovException("Unknown moov audio format: " + Pomme::FourCCString(movie.audioFormat));
	}
}

static void Parse_mdia(Pomme::BigEndianIStream& f, Movie& movie)
{
	AtomGuard mdia(f, 'mdia');
	
	FourCharCode componentType;
	UInt32 timeScale;

	{
		AtomGuard mdhd(f, 'mdhd');
		Expect<UInt32>(f, 0, "mdhd version + flags");
		f.Skip(4); // ctime
		f.Skip(4); // mtime
		timeScale = f.Read<UInt32>();
		f.Skip(4); // UInt32 duration
		f.Skip(2); // language
		f.Skip(2); // quality
		//std::cout << "mdhd: timeScale: " << timeScale << " units per second; duration: " << duration << " units\n";
	}

	{
		AtomGuard hdlr(f, 'hdlr');
		f.Skip(4);
		Expect<FourCharCode>(f, 'mhlr', "mhlr required here");
		componentType = f.Read<FourCharCode>();
		f.Goto(hdlr.end);
	}
	
	if ('vide' == componentType) {
		Parse_mdia_vide(f, movie, timeScale);
	}
	else if ('soun' == componentType) {
		Parse_mdia_soun(f, movie);
	}
	else {
		throw MoovException("hdlr component type should be either vide or soun");
	}
}

static void Parse_trak(Pomme::BigEndianIStream& f, Movie& movie)
{
	AtomGuard trak(f, 'trak');

	RequireAtomAndSkip(f, 'tkhd');

	while (f.Tell() < trak.end) {
		auto start = f.Tell();
		auto atomSize = f.Read<UInt32>();
		auto atomType = f.Read<FourCharCode>();
		f.Goto(start);
		if (atomType != 'mdia') {
			f.Skip(atomSize);
		} else {
			Parse_mdia(f, movie);
		}
	}
}

//-----------------------------------------------------------------------------
// Moov parser

Movie Pomme::Video::ReadMoov(std::istream& theF)
{
	Pomme::BigEndianIStream f(theF);
	Movie movie;
	AtomGuard moov(f, 'moov');
	RequireAtomAndSkip(f, 'mvhd');
	Parse_trak(f, movie);		// Parse first track(video)
	Parse_trak(f, movie);		// Parse second track (audio)
	SkipAtomIfPresent(f, 'udta');
	return movie;
}
