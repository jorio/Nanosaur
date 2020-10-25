#include "Utilities/StringUtils.h"

#include <algorithm>
#include <iterator>

std::string UppercaseCopy(const std::string& in)
{
	std::string out;
	std::transform(
		in.begin(),
		in.end(),
		std::back_inserter(out),
		[](unsigned char c) -> unsigned char
		{
			return (c >= 'a' && c <= 'z') ? ('A' + c - 'a') : c;
		});
	return out;
}

u8string AsU8(const std::string s)
{
	return u8string(s.begin(), s.end());
}

std::string FromU8(const u8string u8s)
{
	return std::string(u8s.begin(), u8s.end());
}
