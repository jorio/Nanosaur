#pragma once

#include "CompilerSupport/span.h"
#include <vector>
#include <istream>
#include <memory>
#include "Sound/cmixer.h"

namespace Pomme::Sound
{
	struct AudioClip
	{
		int nChannels;
		int bitDepth;
		int sampleRate;
		std::vector<char> pcmData;
	};

	void Init();
	void Shutdown();
	AudioClip ReadAIFF(std::istream& f);

	class Codec
	{
	public:
        virtual ~Codec() {}
		virtual int SamplesPerPacket() = 0;
		virtual int BytesPerPacket() = 0;
		virtual void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output) = 0;
	};

	class MACE: public Codec
	{
	public:
		int SamplesPerPacket() override { return 6; }
		int BytesPerPacket() override { return 2; }
		void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output) override;
	};

	class IMA4: public Codec
	{
	public:
		int SamplesPerPacket() override { return 64; }
		int BytesPerPacket() override { return 34; }
		void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output) override;
	};

	class ulaw: public Codec
	{
	public:
		int SamplesPerPacket() override { return 1; }
		int BytesPerPacket() override { return 1; }
		void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output) override;
	};

	std::unique_ptr<Pomme::Sound::Codec> GetCodec(uint32_t fourCC);
}
