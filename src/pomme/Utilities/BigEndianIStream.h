#pragma once

#include <algorithm>
#include <istream>
#include <vector>

namespace Pomme
{
	class StreamPosGuard
	{
		std::istream& stream;
		const std::streampos backup;
		bool active;

	public:
		StreamPosGuard(std::istream& f);
		~StreamPosGuard();
		void Cancel();
	};

	class BigEndianIStream
	{
		std::istream& stream;

	public:
		BigEndianIStream(std::istream& theStream);
		void Read(char* dst, int n);
		void Skip(int n);
		void Goto(int absoluteOffset);
		std::streampos Tell() const;
		StreamPosGuard GuardPos();
		std::vector<unsigned char> ReadBytes(int n);
		std::string ReadPascalString();
		std::string ReadPascalString_FixedLengthRecord(const int maxChars);
		double Read80BitFloat();

		template<typename T> T Read()
		{
			char b[sizeof(T)];
			Read(b, sizeof(T));
#if !(TARGET_RT_BIGENDIAN)
			if (sizeof(T) > 1)
			{
				std::reverse(b, b + sizeof(T));
			}
#endif
			return *(T*)b;
		}
	};
}
