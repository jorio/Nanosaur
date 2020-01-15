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

	struct ResourceOnDisk {
		Byte	flags;
		UInt32	dataOffset;
		UInt32	nameOffset;
	};

	struct ResourceFork {
		SInt16 fileRefNum;
		std::map<ResType, std::map<SInt16, ResourceOnDisk> > rezMap;
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

	namespace Time
	{
		void Init();
	}
	
	namespace Files
	{
		void Init(const char* applName);

		bool IsDirIDLegal(long dirID);
		bool IsRefNumLegal(short refNum);
		std::fstream& GetStream(short refNum);

		bool IsStreamOpen(short refNum);
		bool IsStreamPermissionAllowed(short refNum, char perm);
		void CloseStream(short refNum);

		std::filesystem::path ToPath(short vRefNum, long parID, ConstStr255Param name);
		std::filesystem::path ToPath(const FSSpec& spec);
		std::string GetHostFilename(short refNum);
	}

	namespace Sound
	{
		struct AudioClip
		{
			int nChannels;
			int bitDepth;
			int sampleRate;
			std::vector<char> pcmData;
		};

		void Init();
		AudioClip ReadAIFF(std::istream& f);
		std::vector<SInt16> DecodeMACE3(const std::vector<Byte>& input, const int nChannels);
		std::vector<SInt16> DecodeIMA4(const std::vector<Byte>& input, const int nChannels);
	}

	Pixmap ReadPICT(std::istream& f, bool skip512 = true);
	std::string FourCCString(FourCharCode t, char filler = '?');
}