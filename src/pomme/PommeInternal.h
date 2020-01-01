#pragma once

#include <vector>
#include <map>

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

	struct Pixmap {
		int width;
		int height;
		std::vector<Byte> data;

		Pixmap();
		Pixmap(int w, int h);
		void WriteTGA(const char* path);
	};


	void Init();
	void InitFiles(const char* applName);

	Pixmap ReadPICT(std::istream& f, bool skip512 = true);
}