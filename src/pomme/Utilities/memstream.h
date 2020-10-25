#pragma once

#include <iostream>
#include <streambuf>
#include <vector>

class membuf : public std::basic_streambuf<char>
{
	char* begin;
	char* end;

public:
	membuf(char* p, size_t n);

	explicit membuf(std::vector<char>&);

	virtual ~membuf() override = default;

	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override;

	virtual pos_type seekpos(std::streampos pos, std::ios_base::openmode mode) override;
};

class memstream
	: membuf
	, public std::iostream
{
public:
	memstream(char* p, size_t n);

	explicit memstream(std::vector<char>& data);

	virtual ~memstream() = default;
};
