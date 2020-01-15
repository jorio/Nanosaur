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

template<typename T>
void DumpAU(const char* fn, const std::vector<T>& samples, int nChannels, int sampleRate)
{
	int header[6] = {
		'.snd',							// magic
		24,								// offset to data
		samples.size() * sizeof(T),		// data size
		1 + sizeof(T),					// format (2,3,4,5 => 8,16,24,32-bit PCM)
		sampleRate,
		nChannels
	};
	structpack::Pack(">6l", (Ptr)header);

	std::ofstream f(fn, std::ofstream::binary);
	f.write((const char*)header, sizeof(header));
	for (int i = 0; i < samples.size(); i++) {
		T beSample = ToBE(samples[i]);
		f.write((char*)&beSample, sizeof(T));
	}
	f.close();

	std::cout << "Dumped AU: " << fn << "\n";
}

void Pomme::Sound::ReadAIFF(std::istream& theF)
{
	BigEndianIStream f(theF);

	if ('FORM' != f.Read<OSType>()) throw AIFFException("invalid FORM");
	auto formSize = f.Read<SInt32>();
	auto endOfForm = f.Tell() + std::streampos(formSize);
	auto formType = f.Read<OSType>();
	bool isAiffC = formType == 'AIFC';

	if (formType != 'AIFF' && formType != 'AIFC') throw AIFFException("invalid file type");

	AIFFCOMM COMM;
	memset(&COMM, 0, sizeof(COMM));

	while (f.Tell() != endOfForm) {
		auto ckID = f.Read<OSType>();
		auto ckSize = f.Read<SInt32>();
		std::streampos endOfChunk = f.Tell() + std::streampos(ckSize);

		LOG << f.Tell() << "\n";
		LOG << "Chunk " << FourCCString(ckID) << ", " << ckSize << ", " << endOfChunk << "\n";

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
			std::string compressionName = "Not compressed";
			if (isAiffC) {
				COMM.compressionType = f.Read<OSType>();
				compressionName = f.ReadPascalString();
			}
			LOG
				<< "---- " << FourCCString(formType) << " ----"
				<< "\n\tnumChannels      " << COMM.numChannels
				<< "\n\tnumSampleFrames  " << COMM.numSampleFrames
				<< "\n\tsampleSize       " << COMM.sampleSize
				<< "\n\tsampleRate       " << COMM.sampleRate
				<< "\n\tcompressionType  " << FourCCString(COMM.compressionType) << " \"" << compressionName << "\""
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
				auto decomp = Pomme::Sound::DecodeMACE3(ssnd, COMM.numChannels);
				DumpAU("AIFCMAC3.AU", decomp, COMM.numChannels, COMM.sampleRate);
				break;
			}
			case 'ima4':
			{
				auto decomp = Pomme::Sound::DecodeIMA4(ssnd, COMM.numChannels);
				DumpAU("AIFCIMA4.AU", decomp, COMM.numChannels, COMM.sampleRate);
				break;
			}
			default:
				TODO2("unknown compression type " << FourCCString(COMM.compressionType));
				break;
			}

			break;
		}

		default:
			LOG << __func__ << ": skipping unknown chunk " << FourCCString(ckID) << "\n";
			LOG << "EOC " << endOfChunk << "\n";
			f.Goto(endOfChunk);
			break;
		}

		if (f.Tell() != endOfChunk) throw AIFFException("end of chunk pos???");
	}

	LOG << "End Of Aiff\n";
}