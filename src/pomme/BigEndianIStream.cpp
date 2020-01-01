#include "PommeInternal.h"

Pomme::BigEndianIStream::BigEndianIStream(std::istream& theStream) :
	stream(theStream) {
}

void Pomme::BigEndianIStream::Read(char* dst, int n) {
	stream.read(dst, n);
	if (stream.eof()) {
		throw "Read past end of stream!";
	}
}

void Pomme::BigEndianIStream::Skip(int n) {
	stream.seekg(n, std::ios_base::cur);
}

std::streampos Pomme::BigEndianIStream::Tell() const {
	return stream.tellg();
}
