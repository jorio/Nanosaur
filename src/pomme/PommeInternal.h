#pragma once

#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <span>
#include <queue>
#include "Sound/cmixer.h"

namespace Pomme {
	class StreamPosGuard {
		std::istream& stream;
		const std::streampos backup;
		bool active;

	public:
		StreamPosGuard(std::istream& f);
		~StreamPosGuard();
		void Cancel();
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
		std::string ReadPascalString_FixedLengthRecord(const int maxChars);
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

	struct ResourceMetadata
	{
		short			forkRefNum;
		OSType			type;
		SInt16			id;
		Byte			flags;
		SInt32			size;
		UInt32			dataOffset;
		std::string		name;
	};

	struct ResourceFork
	{
		SInt16 fileRefNum;
		std::map<ResType, std::map<SInt16, ResourceMetadata> > resourceMap;
	};

	// Throw this exception to interrupt the game's main loop
	class QuitRequest : public std::exception
	{
		public: virtual const char* what() const noexcept;
	};

	void Init(const char* applName);
	void Shutdown();

	namespace Time
	{
		void Init();
	}
	
	namespace Files
	{
		void Init();
		bool IsRefNumLegal(short refNum);
		bool IsStreamOpen(short refNum);
		bool IsStreamPermissionAllowed(short refNum, char perm);
		std::iostream& GetStream(short refNum);
		void CloseStream(short refNum);
		short MountArchiveAsVolume(const std::string& archivePath);
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
		void Shutdown();
		AudioClip ReadAIFF(std::istream& f);

		namespace MACE
		{
			int GetOutputSize(const int inputByteCount, const int nChannels);
			void Decode(const int nChannels, const std::span<const char>& input, const std::span<char> output);
		}
		namespace IMA4
		{
			int GetInputSize(const int nSamples, const int nChannels);
			int GetOutputSize(const int inputByteCount, const int nChannels);
			void Decode(const int nChannels, const std::span<const char>& input, const std::span<char> output);
		}
		namespace ulaw
		{
			int GetOutputSize(const int inputByteCount, const int nChannels);
			void Decode(const int nChannels, const std::span<const char>& input, const std::span<char> output);
		}
	}

	namespace Video
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

	namespace Input
	{
		void Init();
	}

	namespace Graphics
	{
		struct Color
		{
			UInt8 a, r, g, b;
			Color(UInt8 red, UInt8 green, UInt8 blue);
			Color(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha);
			Color();
		};

		struct ARGBPixmap
		{
			int width;
			int height;
			std::vector<Byte> data;

			ARGBPixmap();
			ARGBPixmap(int w, int h);
			ARGBPixmap(ARGBPixmap&& other) noexcept;
			ARGBPixmap& operator=(ARGBPixmap&& other) noexcept;
			void Fill(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha = 0xFF);
			void Plot(int x, int y, UInt32 color);
			void WriteTGA(const char* path) const;
			inline UInt32* GetPtr(int x, int y) { return (UInt32*)&data.data()[4 * (y * width + x)]; }
		};

		void Init(const char* windowTitle, int windowWidth, int windowHeight);
		void Shutdown();

		ARGBPixmap ReadPICT(std::istream& f, bool skip512 = true);
		void DumpTGA(const char* path, short width, short height, const char* argbData);
		void DrawARGBPixmap(int left, int top, ARGBPixmap& p);
		CGrafPtr GetScreenPort(void);
		void SetWindowIconFromIcl8Resource(short i);

		extern const uint32_t clut8[256];
		extern const uint32_t clut4[16];
	}

	inline int Width(const Rect& r) { return r.right - r.left; }
	inline int Height(const Rect& r) { return r.bottom - r.top; }
	
	std::string FourCCString(FourCharCode t, char filler = '?');

}

