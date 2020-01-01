#include "PommeInternal.h"

Pomme::StreamPosGuard::StreamPosGuard(std::istream& theStream) :
	stream(theStream),
	backup(theStream.tellg())
{
}

Pomme::StreamPosGuard::~StreamPosGuard() {
	stream.seekg(backup, std::ios_base::beg);
}

Pomme::BigEndianIStream::BigEndianIStream(std::istream& theStream) :
	stream(theStream) {
}

void Pomme::BigEndianIStream::Read(char* dst, int n) {
	stream.read(dst, n);
	if (stream.eof()) {
		throw "Read past end of stream!";
	}
}

std::vector<Byte> Pomme::BigEndianIStream::ReadBytes(int n) {
	std::vector<Byte> buf(n);
	Read(reinterpret_cast<char*>(buf.data()), n);
	return buf;
}

void Pomme::BigEndianIStream::Goto(int absoluteOffset) {
	stream.seekg(absoluteOffset, std::ios_base::beg);
}

void Pomme::BigEndianIStream::Skip(int n) {
	stream.seekg(n, std::ios_base::cur);
}

std::streampos Pomme::BigEndianIStream::Tell() const {
	return stream.tellg();
}

Pomme::StreamPosGuard Pomme::BigEndianIStream::GuardPos() {
	return Pomme::StreamPosGuard(stream);
}