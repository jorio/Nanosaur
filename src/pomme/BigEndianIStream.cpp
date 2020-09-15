#include "PommeInternal.h"
#include "IEEEExtended.h"

Pomme::StreamPosGuard::StreamPosGuard(std::istream& theStream) :
	stream(theStream),
	backup(theStream.tellg()),
	active(true)
{
}

Pomme::StreamPosGuard::~StreamPosGuard()
{
	if (active) {
		stream.seekg(backup, std::ios_base::beg);
	}
}

void Pomme::StreamPosGuard::Cancel()
{
	active = false;
}

Pomme::BigEndianIStream::BigEndianIStream(std::istream& theStream) :
	stream(theStream) {
}

void Pomme::BigEndianIStream::Read(char* dst, int n) {
	stream.read(dst, n);
	if (stream.eof()) {
		throw std::out_of_range("Read past end of stream!");
	}
}

std::vector<Byte> Pomme::BigEndianIStream::ReadBytes(int n) {
	std::vector<Byte> buf(n);
	Read(reinterpret_cast<char*>(buf.data()), n);
	return buf;
}

std::string Pomme::BigEndianIStream::ReadPascalString() {
	int length = Read<UInt8>();
	auto bytes = ReadBytes(length);
	bytes.push_back('\0');
	return std::string((const char*)&bytes.data()[0]);
}

std::string Pomme::BigEndianIStream::ReadPascalString_FixedLengthRecord(const int maxChars) {
	int length = Read<UInt8>();
	char buf[256];
	stream.read(buf, maxChars);
	return std::string(buf, length);
}

double Pomme::BigEndianIStream::Read80BitFloat() {
	auto bytes = ReadBytes(10);
	return ConvertFromIeeeExtended(bytes.data());
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