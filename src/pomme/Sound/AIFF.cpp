#include <iostream>
#include "PommeInternal.h"
using namespace Pomme;

#define LOG POMME_GENLOG(POMME_DEBUG_SOUND, "AIFF")

class AIFFException: std::exception{
public:
	AIFFException(const char* m) : std::exception(m) {}
};

struct AIFFCOMM {
	int numChannels;
	int numSampleFrames;
	int sampleSize;
	double sampleRate;
	OSType compressionType = 'NONE';
};

Pomme::Sound::AudioClip Pomme::Sound::ReadAIFF(std::istream& theF)
{
	BigEndianIStream f(theF);

	if ('FORM' != f.Read<OSType>()) throw AIFFException("invalid FORM");
	auto formSize = f.Read<SInt32>();
	auto endOfForm = f.Tell() + std::streampos(formSize);
	auto formType = f.Read<OSType>();
	bool isAiffC = formType == 'AIFC';

	if (formType != 'AIFF' && formType != 'AIFC') throw AIFFException("invalid file type");

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
			if (timestamp != 0xA2805140)
				throw AIFFException("unrecognized FVER");
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
			//clip.pcmData			= std::vector<char>(COMM.numChannels * COMM.numSampleFrames * COMM.sampleSize / 8);

			std::string compressionName = "Not compressed";
			if (isAiffC) {
				COMM.compressionType = f.Read<OSType>();
				compressionName = f.ReadPascalString();
			}
			LOG << FourCCString(formType) << ": "
				<< COMM.sampleSize << "-bit, "
				<< COMM.sampleRate << " Hz, "
				<< COMM.numChannels << " chans, "
				<< COMM.numSampleFrames << " frames, "
				<< FourCCString(COMM.compressionType) << " \"" << compressionName << "\""
				<< "\n";
			break;
		}

		case 'SSND':
		{
			if (f.Read<UInt64>() != 0) throw AIFFException("unexpected offset/blockSize in SSND");
			// sampled sound data is here

			auto ssnd = f.ReadBytes(ckSize - 8);
			LOG << "SSND bytes " << ssnd.size() << "\n";

			switch (COMM.compressionType) {
			case 'MAC3':
			{
				auto pcm = Pomme::Sound::DecodeMACE3(ssnd, COMM.numChannels);
				//DumpAU("Decoded from AIFF-C MACE-3.au", pcm, COMM.numChannels, COMM.sampleRate);
				clip.bitDepth = 16; // force bitdepth to 16 (decoder output)
				// TODO: get rid of the gratuitous buffer copy
				clip.pcmData = std::vector<char>((char*)pcm.data(), (char*)(pcm.data() + pcm.size()));
				break;
			}
			case 'ima4':
			{
				auto pcm = Pomme::Sound::DecodeIMA4(ssnd, COMM.numChannels);
				//DumpAU("Decoded from AIFF-C IMA-4.au", pcm, COMM.numChannels, COMM.sampleRate);
				clip.bitDepth = 16; // force bitdepth to 16 (decoder output)
				// TODO: get rid of the gratuitous buffer copy
				clip.pcmData = std::vector<char>((char*)pcm.data(), (char*)(pcm.data() + pcm.size()));
				break;
			}
			default:
				TODO2("unknown compression type " << FourCCString(COMM.compressionType));
				break;
			}

			break;
		}

		default:
			LOG << "Skipping chunk " << FourCCString(ckID) << "\n";
			f.Goto(int(endOfChunk));
			break;
		}

		if (f.Tell() != endOfChunk) throw AIFFException("end of chunk pos???");
	}

	return clip;
}