#pragma once

#include <vector>
#include <map>

namespace Pomme {
	class BigEndianIStream {
		std::istream& stream;

	public:
		BigEndianIStream(std::istream& theStream);
		void Read(char* dst, int n);
		void Skip(int n);
		std::streampos Tell() const;

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

	void Init();
	void InitFiles(const char* applName);
}