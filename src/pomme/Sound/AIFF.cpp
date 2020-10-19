#include "PommeSound.h"
#include "Utilities/BigEndianIStream.h"
#include <cstdint>

static void AIFFAssert(bool condition, const char* message)
{
	if (!condition) {
		throw std::runtime_error(message);
	}
}

void Pomme::Sound::ReadAIFF(std::istream& input, cmixer::WavStream& output)
{
	BigEndianIStream f(input);

	AIFFAssert('FORM' == f.Read<uint32_t>(), "AIFF: invalid FORM");
	auto formSize = f.Read<uint32_t>();
	auto endOfForm = f.Tell() + std::streampos(formSize);
	auto formType = f.Read<uint32_t>();
	AIFFAssert(formType == 'AIFF' || formType == 'AIFC', "AIFF: not an AIFF or AIFC file");

	// COMM chunk contents
	int nChannels  = 0;
	int nPackets   = 0;
	int sampleRate = 0;
	uint32_t compressionType = 'NONE';

	bool gotCOMM = false;

	while (f.Tell() != endOfForm) {
		auto ckID = f.Read<uint32_t>();
		auto ckSize = f.Read<uint32_t>();
		std::streampos endOfChunk = f.Tell() + std::streampos(ckSize);

		switch (ckID) {
		case 'FVER':
		{
			auto timestamp = f.Read<uint32_t>();
			AIFFAssert(timestamp == 0xA2805140u, "AIFF: unrecognized FVER");
			break;
		}

		case 'COMM': // common chunk, 2-85
		{
			nChannels  = f.Read<uint16_t>();
			nPackets   = f.Read<uint32_t>();
			f.Skip(2); // sample bit depth (UInt16)
			sampleRate = (int)f.Read80BitFloat();
			if (formType == 'AIFC') {
				compressionType = f.Read<uint32_t>();
				f.ReadPascalString(); // This is a human-friendly compression name. Skip it.
			}
			gotCOMM = true;
			break;
		}

		case 'SSND':
		{
			AIFFAssert(gotCOMM, "AIFF: reached SSND before COMM");
			AIFFAssert(0 == f.Read<uint64_t>(), "AIFF: unexpected offset/blockSize in SSND");
			// sampled sound data is here

			const int ssndSize = ckSize - 8;
			auto ssnd = std::vector<char>(ssndSize);
			f.Read(ssnd.data(), ssndSize);

			// TODO: if the compression type is 'NONE' (raw PCM), just init the WavStream without decoding (not needed for Nanosaur though)
			auto codec   = Pomme::Sound::GetCodec(compressionType);
			auto spanIn  = std::span(ssnd);
			auto spanOut = output.GetBuffer(nChannels * nPackets * codec->SamplesPerPacket() * 2);
			codec->Decode(nChannels, spanIn, spanOut);
			output.Init(sampleRate, 16, nChannels, false, spanOut);
			break;
		}

		default:
			f.Goto(int(endOfChunk));
			break;
		}

		AIFFAssert(f.Tell() == endOfChunk, "AIFF: incorrect end-of-chunk position");
	}
}

