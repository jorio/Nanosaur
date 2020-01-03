#include <iostream>

#include "PommeInternal.h"
#include "QuesaStorage.h"

//-----------------------------------------------------------------------------
// QuickDraw 3D

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec) {
	std::filesystem::path path = Pomme::ToPath(*spec);
	std::cout << __func__ << ": " << path << "\n";
	return Q3PathStorage_New(path.string().c_str());
}


