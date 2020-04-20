// Adapted from cmixer by rxi (https://github.com/rxi/cmixer)

/*
** Copyright (c) 2017 rxi
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**/

#pragma once

#include <vector>
#include <functional>

#define BUFFER_SIZE (512)

namespace cmixer {

enum {
	CM_STATE_STOPPED,
	CM_STATE_PLAYING,
	CM_STATE_PAUSED
};

struct Source {
	int16_t pcmbuf[BUFFER_SIZE];	// Internal buffer with raw stereo PCM
	int samplerate;					// Stream's native samplerate
	int length;						// Stream's length in frames
	int end;						// End index for the current play-through
	int state;						// Current state (playing|paused|stopped)
	int64_t position;				// Current playhead position (fixed point)
	int lgain, rgain;				// Left and right gain (fixed point)
	int rate;						// Playback rate (fixed point)
	int nextfill;					// Next frame idx where the buffer needs to be filled
	bool loop;						// Whether the source will loop when `end` is reached
	bool rewind;					// Whether the source will rewind before playing
	bool active;					// Whether the source is part of `sources` list
	double gain;					// Gain set by `cm_set_gain()`
	double pan;						// Pan set by `cm_set_pan()`
	std::function<void()> onComplete;		// Callback

protected:
	Source(int theSampleRate, int theLength);

	virtual void Rewind2() = 0;
	virtual void FillBuffer(int16_t* buffer, int length) = 0;

public:
	void Rewind();
	void RecalcGains();
	void FillBuffer(int offset, int length);
	void Process(int len);

public:
	~Source();
	double GetLength() const;
	double GetPosition() const;
	int GetState() const;
	void SetGain(double gain);
	void SetPan(double pan);
	void SetPitch(double pitch);
	void SetLoop(bool loop);
	void Play();
	void Pause();
	void TogglePause();
	void Stop();
};

class WavStream : public Source {
	int bitdepth;
	int channels;
	int idx;

	std::vector<char> udata;

	void Rewind2();
	void FillBuffer(int16_t* buffer, int length);

	inline uint8_t* data8() { return (uint8_t*)udata.data(); }
	inline int16_t* data16() { return (int16_t*)udata.data(); }

public:
	bool bigEndian;

	WavStream(
		int theSampleRate,
		int theBitDepth,
		int nChannels,
		std::vector<char>&& data
	);
};


void InitWithSDL();
void ShutdownWithSDL();
double GetMasterGain();
void SetMasterGain(double);
WavStream LoadWAVFromFile(const char* path);

}
