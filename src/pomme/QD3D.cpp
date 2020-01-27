#include "PommeInternal.h"
#include "QuesaStorage.h"

//-----------------------------------------------------------------------------
// Pomme extensions for Quesa

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec)
{
	auto path = Pomme::Files::ToPath(*spec);
	return Q3PathStorage_New(path.string().c_str());
}
