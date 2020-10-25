#pragma once

#include <string>

#if defined(__cplusplus) && __cplusplus >= 201703L
    typedef std::u8string u8string;
#else
    typedef std::string u8string;
#endif

std::string UppercaseCopy(const std::string&);

u8string AsU8(const std::string);

std::string FromU8(const u8string);
