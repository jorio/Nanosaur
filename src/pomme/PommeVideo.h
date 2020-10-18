#pragma once

#include "PommeTypes.h"
#include "Sound/cmixer.h"

#include <istream>
#include <queue>
#include <vector>

namespace Pomme::Video
{
	struct Movie
	{
		int				width;
		int				height;
		FourCharCode	videoFormat;
		float			videoFrameRate;
		std::queue<std::vector<unsigned char>> videoFrames;

		FourCharCode	audioFormat;
		int				audioSampleRate;
		int				audioBitDepth;
		int				audioNChannels;
		cmixer::WavStream	audioStream;
		unsigned audioSampleCount;
	};

	Movie ReadMoov(std::istream& f);
}
