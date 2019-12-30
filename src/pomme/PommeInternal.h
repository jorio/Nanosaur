#pragma once

#include <vector>
#include <map>

namespace Pomme {

	struct Rez {
		ResType				fourCC;
		SInt16				id;
		Byte				flags;
		std::string			name;
		std::vector<Byte>	data;
	};

	struct RezFork {
		std::map<ResType, std::map<short, Rez> > rezMap;
	};

	void Init();
	void InitFiles(const char* applName);
}