#pragma once

#include <vector>

template<typename T>
void DumpAU(const char* fn, const std::vector<T>& samples, int nChannels, int sampleRate, bool alreadyBE = false)
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
	if (alreadyBE) {
		f.write((char*)samples.data(), samples.size() * sizeof(T));
	}
	else {
		for (int i = 0; i < samples.size(); i++) {
			T beSample = ToBE(samples[i]);
			f.write((char*)&beSample, sizeof(T));
		}
	}
	f.close();

	std::cout << "Dumped AU: " << fn << "\n";
}
