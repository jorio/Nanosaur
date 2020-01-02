#include "PommeInternal.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace Pomme;

constexpr auto MAX_OPEN_FILES = 32767;

//-----------------------------------------------------------------------------
// Types

struct InternalMacFileHandle {
	std::ifstream rf;
};

//-----------------------------------------------------------------------------
// State

std::vector<std::filesystem::path> fsspecParentDirectories;
InternalMacFileHandle openFiles[MAX_OPEN_FILES];
short nextFileSlot = -1;

//-----------------------------------------------------------------------------
// Utilities

static const char* fourCCstr(ResType t) {
	static char b[5];
	*(ResType*)b = t;
#if !(TARGET_RT_BIGENDIAN)
	std::reverse(b, b + 4);
#endif
	// replace non-ascii with '?'
	for (int i = 0; i < 4; i++) {
		char c = b[i];
		if (c < ' ' || c > '~') b[i] = '?';
	}
	b[4] = '\0';
	return b;
}

class FSSpecEx {
public:
	const FSSpec& spec;

	FSSpecEx(const FSSpec& theSpec) :
		spec(theSpec)
	{
	}

	bool exists() const {
		return std::filesystem::exists(cppPath());
	}

	bool isFile() const {
		return std::filesystem::is_regular_file(cppPath());
	}

	std::filesystem::path cppPath() const {
		std::filesystem::path path(fsspecParentDirectories[spec.parID]);
		path += std::filesystem::path::preferred_separator;
		path += Pascal2C(spec.name);
		return path;
	}
};

//-----------------------------------------------------------------------------
// Init

void Pomme::InitFiles(const char* applName) {
	// default directory (ID 0)
	fsspecParentDirectories.push_back(std::filesystem::current_path());

	nextFileSlot = 1; // file 0 is System

	FSSpec applSpec;
	OSErr applSpecErr = FSMakeFSSpec(0, 0, applName, &applSpec);
	if (noErr != applSpecErr)
		TODOFATAL2("couldn't load applSpec, err " << applSpecErr);
	short applRefNum = FSpOpenResFile(&applSpec, fsRdPerm);
	UseResFile(applRefNum);
}

//-----------------------------------------------------------------------------
// Implementation

OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param pascalFileName, FSSpec* spec)
{
	if (vRefNum != 0)
		TODOFATAL2("can't handle anything but the default volume");

	if (fsspecParentDirectories.empty())
		TODOFATAL2("did you init the fake mac?");

	auto fullPath = fsspecParentDirectories[dirID];
	auto newFN = std::string(Pascal2C(pascalFileName));
	std::replace(newFN.begin(), newFN.end(), '/', '_');
	std::replace(newFN.begin(), newFN.end(), ':', '/');
	fullPath += '/' + newFN;
	fullPath.make_preferred();

	std::cout << "FSMakeFSSpec: " << fullPath << "\n";

	auto parentPath = fullPath;
	parentPath.remove_filename();

	fsspecParentDirectories.emplace_back(parentPath);

	spec->vRefNum = 0;
	spec->parID = fsspecParentDirectories.size() - 1; // the index of the one we've just pushed
	spec->name = Str255(fullPath.filename().string().c_str());

	if (std::filesystem::exists(fullPath))
		return noErr;
	else
		return fnfErr;
}

std::ifstream& Pomme::GetIStreamRF(short refNum) {
	return openFiles[refNum].rf;
}

OSErr FSpOpenRF(const FSSpec* spec0, char permission, short* refNum) {
	short slot = nextFileSlot;

	const FSSpecEx spec(*spec0);

	if (nextFileSlot == MAX_OPEN_FILES)
		TODOFATAL2("too many files have been opened");

	if (permission != fsRdPerm)
		TODOFATAL2("only fsRdPerm is implemented");

	if (!spec.isFile()) {
		*refNum = -1;
		return fnfErr;
	}

	nextFileSlot++;
	openFiles[slot].rf = std::ifstream(spec.cppPath(), std::ios::binary);
	*refNum = slot;
	std::cout << "Opened RF " << spec.cppPath() << " at slot " << *refNum << "\n";

	return noErr;
}
