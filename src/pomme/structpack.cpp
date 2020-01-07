#include <map>
#include <strstream>

#include "structpack.h"

std::map<std::type_index, std::string> structpack::formatDB;

int structpack::Pack(const std::string& format, Ptr buffer)
{
	// it's the same thing as unpack -- just swapping bytes.
	// if we ever have different sizes for input & output (e.g. 32 vs 64 bit pointer placeholders), it'll be different.
	return Unpack(format, buffer);
}

int structpack::Unpack(const std::string& format, Ptr buffer)
{
	int totalBytes = 0;
	int repeat = 0;

	for (char c : format) {
		int fieldLength = -1;

		switch (c) {
		case '>': // big endian indicator (for compat with python's struct) - OK just ignore
			continue;

		case ' ':
		case '\r':
		case '\n':
		case '\t':
			continue;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (repeat) repeat *= 10;
			repeat += c - '0';
			continue;

		case 'x': // pad byte
		case 'c': // char
		case 'b': // signed char
		case 'B': // unsigned char
		case '?': // bool
			fieldLength = 1;
			break;

		case 'h': // short
		case 'H': // unsigned short
			fieldLength = 2;
			break;

		case 'i': // int
		case 'I': // unsigned int
		case 'l': // long
		case 'L': // unsigned long
		case 'f': // float
			fieldLength = 4;
			break;

		case 'q': // long long
		case 'Q': // unsigned long long
		case 'd': // double
			fieldLength = 8;
			break;

		default:
			throw std::invalid_argument("unknown packfmt char");
		}

		if (totalBytes % fieldLength != 0) {
			throw std::invalid_argument("WORD ALIGNMENT ERROR IN PACKFMT!!!");
		}

		if (!repeat)
			repeat = 1;

		bool doSwap = fieldLength > 1;

		if (buffer) {
			if (doSwap) {
				for (int i = 0; i < repeat; i++) {
					std::reverse(buffer, buffer + fieldLength);
					buffer += fieldLength;
				}
			}
			else {
				buffer += repeat * fieldLength;
			}
		}

		totalBytes += repeat * fieldLength;
		repeat = 0;
	}

	return totalBytes;
}
