#pragma once

#include <stdint.h>

#ifdef __cplusplus

#include <algorithm>

template<typename T> T ByteswapScalar(T x)
{
#if TARGET_RT_BIGENDIAN
	return x;
#else
	char* b = (char*)&x;
	std::reverse(b, b + sizeof(T));
	return x;
#endif
}

#endif


#ifdef __cplusplus
extern "C" {
#endif

int ByteswapStructs(const char* format, int structSize, int structCount, void* buffer);

int ByteswapInts(int intSize, int intCount, void* buffer);

static inline uint16_t Byteswap16(const void* p)
{
	uint16_t v;
	v =
		(*(const uint8_t*) p)
		| ((*(const uint8_t*) p + 1) << 8);
	return v;
}

static inline uint32_t Byteswap32(const void* p)
{
	uint32_t v;
	v =
		(*(const uint8_t*) p)
		| ((*(const uint8_t*) p + 1) << 8)
		| ((*(const uint8_t*) p + 2) << 16)
		| ((*(const uint8_t*) p + 3) << 24);
	return v;
}

#ifdef __cplusplus
}
#endif
