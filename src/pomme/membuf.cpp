#include "membuf.h"

membuf::membuf(char* p, size_t n)
{
	begin = p;
	end = p + n;
	setg(p, p, p + n);
	setp(p, p + n);
}

membuf::membuf(std::vector<char>& vector)
	:membuf(vector.data(), vector.size())
{
}

std::streambuf::pos_type membuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
{
	pos_type ret = 0;

	if ((which & std::ios_base::in) > 0) {
		if (dir == std::ios_base::cur) {
			gbump((int32_t)off);
		}
		else if (dir == std::ios_base::end) {
			setg(begin, end + off, end);
		}
		else if (dir == std::ios_base::beg) {
			setg(begin, begin + off, end);
		}

		ret = gptr() - eback();
	}

	if ((which & std::ios_base::out) > 0) {
		if (dir == std::ios_base::cur) {
			pbump((int32_t)off);
		}
		else if (dir == std::ios_base::end) {
			setp(begin, end + off, end);
		}
		else if (dir == std::ios_base::beg) {
			setp(begin, begin + off, end);
		}

		ret = pptr() - pbase();
	}

	return ret;
}

std::streambuf::pos_type membuf::seekpos(std::streampos pos, std::ios_base::openmode mode)
{
	return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, mode);
}
