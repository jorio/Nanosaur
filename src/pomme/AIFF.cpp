#include <iostream>
#include "PommeInternal.h"
using namespace Pomme;

#ifdef POMME_DEBUG_SOUND
static std::ostream& LOG = std::cout;
#else
static std::ostringstream LOG;
#endif

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
	BE<UInt32> header[6] = {
		'.snd',							// magic
		24,								// offset to data
		samples.size() * sizeof(T),		// data size
		1 + sizeof(T),					// format (2,3,4,5 => 8,16,24,32-bit PCM)
		sampleRate,
		nChannels
	};

	std::ofstream f(fn, std::ofstream::binary);
	f.write((const char*)header, sizeof(header));
	for (int i = 0; i < samples.size(); i++) {
		T beSample = ToBE(samples[i]);
		f.write((char*)&beSample, sizeof(T));
	}
	f.close();

	std::cout << "Dumped AU: " << fn << "\n";
}

void Pomme::ReadAIFF(std::istream& theF) {
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
			f.Goto(endOfChunk);
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