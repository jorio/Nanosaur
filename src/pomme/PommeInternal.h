#pragma once

#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <filesystem>
#include <span>

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

	struct ResourceOnDisk {
		Byte	flags;
		UInt32	dataOffset;
		UInt32	nameOffset;
	};

	struct ResourceFork {
		SInt16 fileRefNum;
		std::map<ResType, std::map<SInt16, ResourceOnDisk> > rezMap;
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
			int GetOutputSize(const int inputByteCount, const int nChannels);
			void Decode(const int nChannels, const std::span<const char>& input, const std::span<char> output);
		}
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
		ARGBPixmap ReadPICT(std::istream& f, bool skip512 = true);
		void DumpTGA(const char* path, short width, short height, const char* argbData);
		void DrawARGBPixmap(int left, int top, ARGBPixmap& p);
		CGrafPtr GetScreenPort(void);
	}

	inline int Width(const Rect& r) { return r.right - r.left; }
	inline int Height(const Rect& r) { return r.bottom - r.top; }
	
	std::string FourCCString(FourCharCode t, char filler = '?');
	std::string Pascal2Cpp(const char* pstr);

}

