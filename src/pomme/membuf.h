#pragma once

#include <streambuf>
#include <vector>

class membuf : public std::basic_streambuf<char>
{
	char* begin;
	char* end;

public:
	membuf(char* p, size_t n);
	membuf(std::vector<char>&);

	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override;

	virtual pos_type seekpos(std::streampos pos, std::ios_base::openmode mode) override;
};
