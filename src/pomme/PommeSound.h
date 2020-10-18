#pragma once

#include "CompilerSupport/span.h"
#include <vector>
#include <istream>

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

	namespace MACE
	{
		int GetOutputSize(const int inputByteCount, const int nChannels);
		void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output);
	}
	namespace IMA4
	{
		int GetInputSize(const int nSamples, const int nChannels);
		int GetOutputSize(const int inputByteCount, const int nChannels);
		void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output);
	}
	namespace ulaw
	{
		int GetOutputSize(const int inputByteCount, const int nChannels);
		void Decode(const int nChannels, const std::span<const char> input, const std::span<char> output);
	}
}
