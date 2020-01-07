#pragma once

#include <typeindex>
#include <map>

namespace structpack {
	extern std::map<std::type_index, std::string> formatDB;

	int Unpack(const std::string& format, Ptr buffer);

	template<typename T> T* UnpackObj(Ptr p, int repeat = 1)
	{
		T* obj0 = reinterpret_cast<T*>(p);

		try {
			const std::string& format = formatDB.at(std::type_index(typeid(T)));

			for (int i = 0; i < repeat; i++)
				p += Unpack(format, (Ptr)p);
		}
		catch (std::out_of_range) {
			std::stringstream ss;
			ss << typeid(T).name() << " isn't registered in structpack::formatDB";
			throw std::out_of_range(ss.str());
		}

		if (&obj0[repeat] != reinterpret_cast<T*>(p))
			throw std::exception("unexpected end pointer after unpacking");

		return obj0;
	}

	template<typename T> void Register(std::string fmtstr)
	{
		formatDB[std::type_index(typeid(T))] = fmtstr;

		int length = Unpack(fmtstr, nullptr);

		if (sizeof(T) != length) {
			std::stringstream ss;
			ss << "unexpected sizeof(" << typeid(T).name() << ") " << sizeof(T) << " doesn't match packformat length " << length << " (" << fmtstr << ")";
			throw std::invalid_argument(ss.str());
		}
	}

}
