#pragma once

#include <cstring>

template<size_t N> class PascalString {
	char buf[N + 1];

public:
	PascalString()
	{
		buf[0] = 0;
		memset(&buf[1], '~', N);
	}

	PascalString(const char* src)
	{
		size_t len = strlen(src);
		if (len > N) len = N;
		if (len < 0) len = 0;
		memcpy(&buf[1], src, len);
		buf[0] = len;
	}

	operator char* ()
	{
		return &buf[0];
	}

	operator const char* () const
	{
		return &buf[0];
	}
};
