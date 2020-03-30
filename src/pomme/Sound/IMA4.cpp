// Adapted from ffmpeg. Look at libavcodec/adpcm{,_data}.{c,h}

/*
 * Portions Copyright (c) 2001-2003 The FFmpeg project
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <vector>
#include <cassert>
#include "PommeInternal.h"

const int8_t ff_adpcm_index_table[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8,
};

const int16_t ff_adpcm_step_table[89] = {
		7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
	   19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
	   50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
	  130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
	  337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
	  876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
	 2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
	 5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

struct ADPCMChannelStatus {
	int predictor;
	int16_t step_index;
	int step;
};

static inline int sign_extend(int val, unsigned bits)
{
	unsigned shift = 8 * sizeof(int) - bits;
	union { unsigned u; int s; } v = { (unsigned)val << shift };
	return v.s >> shift;
}

static inline int adpcm_ima_qt_expand_nibble(ADPCMChannelStatus* c, int nibble, int shift)
{
	int step_index;
	int predictor;
	int diff, step;

	step = ff_adpcm_step_table[c->step_index];
	step_index = c->step_index + ff_adpcm_index_table[nibble];
	step_index = std::clamp(step_index, 0, 88);

	diff = step >> 3;
	if (nibble & 4) diff += step;
	if (nibble & 2) diff += step >> 1;
	if (nibble & 1) diff += step >> 2;

	if (nibble & 8)
		predictor = c->predictor - diff;
	else
		predictor = c->predictor + diff;

	c->predictor = std::clamp(predictor, -32768, 32767);
	c->step_index = step_index;

	return c->predictor;
}

// In QuickTime, IMA is encoded by chunks of 34 bytes (=64 samples). Channel data is interleaved per-chunk.
void DecodeIMA4Chunk(
	const unsigned char** input,
	SInt16** output,
	std::vector<ADPCMChannelStatus>& ctx)
{
	const int nChannels = ctx.size();
	const unsigned char* in = *input;
	SInt16* out = *output;
	
	for (int chan = 0; chan < nChannels; chan++) {
		ADPCMChannelStatus& cs = ctx[chan];

		// Bits 15-7 are the _top_ 9 bits of the 16-bit initial predictor value
		int predictor = sign_extend((in[0] << 8) | in[1], 16);
		int step_index = predictor & 0x7F;
		predictor &= ~0x7F;
		
		in += 2;

		if (cs.step_index == step_index) {
			int diff = predictor - cs.predictor;
			if (diff < 0x00) diff = -diff;
			if (diff > 0x7f) goto update;
		}
		else {
		update:
			cs.step_index = step_index;
			cs.predictor = predictor;
		}

		if (cs.step_index > 88u)
			throw std::invalid_argument("step_index[chan]>88!");

		int pos = chan;
		for (int m = 0; m < 32; m++) {
			int byte = (unsigned char)(*in++);
			out[pos] = adpcm_ima_qt_expand_nibble(&cs, byte & 0x0F, 3);
			pos += nChannels;
			out[pos] = adpcm_ima_qt_expand_nibble(&cs, byte >> 4, 3);
			pos += nChannels;
		}
	}

	*input = in;
	*output += 64 * nChannels;
}

std::vector<SInt16> Pomme::Sound::DecodeIMA4(const std::vector<Byte>& input, const int nChannels)
{
	if (input.size() % 34 != 0)
		throw std::invalid_argument("odd input buffer size");

	int nChunks = int(input.size()) / (34 * nChannels);
	int nSamples = 64 * nChunks;

	std::vector<SInt16> output(nSamples * nChannels);

	const unsigned char* in = input.data();
	SInt16* out = output.data();
	std::vector<ADPCMChannelStatus> ctx(nChannels);

	for (int chunk = 0; chunk < nChunks; chunk++)
	{
		DecodeIMA4Chunk(&in, &out, ctx);
	}

	assert(output.size() == nSamples * nChannels);
	assert(in == (input.data() + input.size()));

	return output;
}
