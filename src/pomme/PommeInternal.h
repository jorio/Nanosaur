#pragma once

#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <filesystem>

namespace Pomme {
	class StreamPosGuard {
		std::istream& stream;
		const std::streampos backup;

	public:
		StreamPosGuard(std::istream& f);
		~StreamPosGuard();
	};

	class BigEndianIStream {
		std::istream& stream;

	public:
		BigEndianIStream(std::istream& theStream);
		void Read(char* dst, int n);
		void Skip(int n);
		void Goto(int absoluteOffset);
		std::streampos Tell() const;
		StreamPosGuard GuardPos();
		std::vector<Byte> ReadBytes(int n);
		std::string ReadPascalString();
		double Read80BitFloat();

		template<typename T> T Read() {
			char b[sizeof(T)];
			Read(b, sizeof(T));
#if !(TARGET_RT_BIGENDIAN)
			if (sizeof(T) > 1)
				std::reverse(b, b + sizeof(T));
#endif
			return *(T*)b;
		}
	};

	template<typename TPooledRecord, typename TId, int N>
	class Pool {
		// fixed size array so that pointers to the elements don't move around in memory
		// (unlike how a vector might move around elements on resize)
		TPooledRecord pool[N];
		std::vector<TId> freeIDs;
		int inUse, inUsePeak;

	public:
		Pool(int k) {
			inUse = 0;
			inUsePeak = 0;
			freeIDs.reserve(N);
			for (int i = N-1; i >= 0; i--)
				freeIDs.push_back(i);
		}

		TPooledRecord* Alloc() {
			if (freeIDs.empty())
				throw std::length_error("pool exhausted");
			TId id = freeIDs.back();
			freeIDs.pop_back();
			inUse++;
			if (inUse > inUsePeak)
				inUsePeak = inUse;
			return &pool[id];
		}

		void Dispose(TPooledRecord* obj) {
			long id = obj - &pool[0];
			if (id < 0 || id >= N)
				throw std::invalid_argument("obj isn't stored in pool");
			inUse--;
			freeIDs.push_back(id);
		}
	};

	struct Rez {
		ResType				fourCC;
		SInt16				id;
		Byte				flags;
		std::string			name;
		std::vector<Byte>	data;
	};

	struct RezFork {
		std::map<ResType, std::map<short, Rez> > rezMap;
	};

	struct Color {
		UInt8 a, r, g, b;
		Color(UInt8 red, UInt8 green, UInt8 blue);
		Color(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha);
		Color();
	};

	struct Pixmap {
		int width;
		int height;
		std::vector<Byte> data;

		Pixmap();
		Pixmap(int w, int h);
		void WriteTGA(const char* path);
	};


	void Init(const char* applName);
	void InitTimeManager();
	void InitFiles(const char* applName);
	void InitSoundManager();

	bool IsDirIDLegal(long dirID);
	bool IsRefNumLegal(short refNum);
	std::fstream& GetStream(short refNum);
	bool IsStreamOpen(short refNum);
	void CloseStream(short refNum);

	std::filesystem::path ToPath(short vRefNum, long parID, ConstStr255Param name);
	std::filesystem::path ToPath(const FSSpec& spec);
	
	Pixmap ReadPICT(std::istream& f, bool skip512 = true);

	void ReadAIFF(std::istream& f);
	std::vector<SInt16> DecodeMACE3(const std::vector<Byte>& input, const int nChannels);
	std::vector<SInt16> DecodeIMA4(const std::vector<Byte>& input, const int nChannels);

	std::string FourCCString(FourCharCode t, char filler = '?');
}