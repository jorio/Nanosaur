#pragma once

#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <deque>

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

	template<typename TPooledRecord, typename TId>
	class Pool {
	public:
		std::vector<TPooledRecord> pool;
		std::deque<TId> freeIDs;

	public:
		TPooledRecord& Alloc(TId* outId) {
			TId id;
			if (freeIDs.empty()) {
				id = pool.size();
				pool.emplace_back();
			}
			else {
				id = freeIDs.front();
				freeIDs.pop_front();
			}
			if (outId)
				*outId = id;
			return pool[id];
		}

		void Dispose(TId id) {
			freeIDs.push_back(id);

			// compact end
			while (freeIDs.size() > 0 && freeIDs.back() == pool.size() - 1) {
				freeIDs.pop_back();
				pool.pop_back();
			}
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

	std::fstream& GetStream(short refNum);
	bool IsStreamOpen(short refNum);
	void CloseStream(short refNum);
	
	Pixmap ReadPICT(std::istream& f, bool skip512 = true);

	std::string FourCCString(FourCharCode t);
}