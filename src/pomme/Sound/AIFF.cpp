#include "Pomme.h"
#include "PommeSound.h"
#include "Utilities/BigEndianIStream.h"
#include <iostream>

struct AIFFCOMM {
	int numChannels;
	int numSampleFrames;
	int sampleSize;
	double sampleRate;
	OSType compressionType = 'NONE';
};

static void AIFFAssert(bool condition, const char* message)
{
	if (!condition) {
		throw std::runtime_error(message);
	}
}

Pomme::Sound::AudioClip Pomme::Sound::ReadAIFF(std::istream& theF)
{
	BigEndianIStream f(theF);

	AIFFAssert('FORM' == f.Read<OSType>(), "AIFF: invalid FORM");
	auto formSize = f.Read<SInt32>();
	auto endOfForm = f.Tell() + std::streampos(formSize);
	auto formType = f.Read<OSType>();
	bool isAiffC = formType == 'AIFC';

	AIFFAssert(formType == 'AIFF' || formType == 'AIFC', "AIFF: not an AIFF or AIFC file");

	AIFFCOMM COMM = {};

	AudioClip clip = {};

	while (f.Tell() != endOfForm) {
		auto ckID = f.Read<OSType>();
		auto ckSize = f.Read<SInt32>();
		std::streampos endOfChunk = f.Tell() + std::streampos(ckSize);

		switch (ckID) {
		case 'FVER':
		{
			auto timestamp = f.Read<UInt32>();
			AIFFAssert(timestamp == 0xA2805140, "AIFF: unrecognized FVER");
			break;
		}

		case 'COMM': // common chunk, 2-85
		{
			COMM.numChannels		= f.Read<SInt16>();
			COMM.numSampleFrames	= f.Read<SInt32>();
			COMM.sampleSize			= f.Read<SInt16>();
			COMM.sampleRate			= f.Read80BitFloat();
			COMM.compressionType = 'NONE';

			clip.nChannels			= COMM.numChannels;
			clip.bitDepth			= COMM.sampleSize;
			clip.sampleRate			= (int)COMM.sampleRate;

			if (isAiffC) {
				COMM.compressionType = f.Read<OSType>();
				f.ReadPascalString(); // This is a human-friendly compression name. Skip it.
			}
			break;
		}

		case 'SSND':
		{
			AIFFAssert(0 == f.Read<UInt64>(), "AIFF: unexpected offset/blockSize in SSND");
			// sampled sound data is here

			const int ssndSize = ckSize - 8;
			auto ssnd = std::vector<char>(ssndSize);
			f.Read(ssnd.data(), ssndSize);

			std::unique_ptr<Pomme::Sound::Codec> codec = Pomme::Sound::GetCodec(COMM.compressionType);
			clip.bitDepth = 16;  // force bitdepth to 16 (decoder output)
			clip.pcmData.resize(COMM.numSampleFrames * COMM.numChannels * codec->SamplesPerPacket() * 2);
			codec->Decode(COMM.numChannels, std::span(ssnd), std::span(clip.pcmData));
			break;
		}

		default:
			f.Goto(int(endOfChunk));
			break;
		}

		AIFFAssert(f.Tell() == endOfChunk, "AIFF: incorrect end-of-chunk position");
	}

	return clip;
}

