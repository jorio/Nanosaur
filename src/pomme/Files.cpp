#include "PommeInternal.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef WIN32
#include <shlobj.h>
#endif

using namespace Pomme;

constexpr auto MAX_OPEN_FILES = 32767;

#ifdef POMME_DEBUG_FILES
static std::ostream& LOG = std::cout;
#else
static std::ostringstream LOG;
#endif

//-----------------------------------------------------------------------------
// State

std::vector<std::filesystem::path> directories;
std::fstream openFiles[MAX_OPEN_FILES];
short nextFileSlot = -1;

//-----------------------------------------------------------------------------
// Utilities

std::filesystem::path GetPath(short vRefNum, long parID, ConstStr255Param name) {
	std::filesystem::path path(directories[parID]);
	path += std::filesystem::path::preferred_separator;
	path += Pascal2C(name);
	return path.lexically_normal();
}

short GetVolumeID(const std::filesystem::path& path) {
	return 0;
}

long GetDirectoryID(const std::filesystem::path& dirPath) {
	if (std::filesystem::exists(dirPath) && !std::filesystem::is_directory(dirPath)) {
		std::cerr << "Warning: GetDirID should only be used on directories! " << dirPath << "\n";
	}
	directories.emplace_back(dirPath);
	return directories.size() - 1;
}

std::fstream& Pomme::GetStream(short refNum) {
	return openFiles[refNum];
}

void Pomme::CloseStream(short refNum) {
	openFiles[refNum].close();
}

bool Pomme::IsStreamOpen(short refNum) {
	return openFiles[refNum].is_open();
}

//-----------------------------------------------------------------------------
// Init

void Pomme::InitFiles(const char* applName) {
	// default directory (ID 0)
	directories.push_back(std::filesystem::current_path());

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

FSSpec ToFSSpec(const std::filesystem::path& fullPath) {
	auto parentPath = fullPath;
	parentPath.remove_filename();

	FSSpec spec;
	spec.vRefNum = GetVolumeID(parentPath);
	spec.parID = GetDirectoryID(parentPath);
	spec.name = Str255(fullPath.filename().string().c_str());
	return spec;
}

OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param pascalFileName, FSSpec* spec)
{
	if (vRefNum != 0)
		TODOFATAL2("can't handle anything but the default volume");

	if (directories.empty())
		TODOFATAL2("did you init the fake mac?");

	auto fullPath = directories[dirID];
	auto newFN = std::string(Pascal2C(pascalFileName));
	std::replace(newFN.begin(), newFN.end(), '/', '_');
	std::replace(newFN.begin(), newFN.end(), ':', '/');
	fullPath += '/' + newFN;
	fullPath = fullPath.lexically_normal();

	LOG << "FSMakeFSSpec: " << fullPath << "\n";

	*spec = ToFSSpec(fullPath);

	if (std::filesystem::exists(fullPath))
		return noErr;
	else
		return fnfErr;
}

OSErr FSpOpenDF(const FSSpec* spec, char permission, short* refNum) {
	short slot = nextFileSlot;

	const auto path = GetPath(spec->vRefNum, spec->parID, spec->name);

	if (nextFileSlot == MAX_OPEN_FILES)
		TODOFATAL2("too many files have been opened");

	if (permission != fsRdPerm)
		TODOFATAL2("only fsRdPerm is implemented");

	if (!std::filesystem::is_regular_file(path)) {
		*refNum = -1;
		return fnfErr;
	}

	nextFileSlot++;
	openFiles[slot] = std::fstream(path, std::ios::binary | std::ios::in);
	*refNum = slot;
	LOG << "Opened DF " << path << " at slot " << *refNum << "\n";

	return noErr;
}

OSErr FSpOpenRF(const FSSpec* spec, char permission, short* refNum) {
	short slot = nextFileSlot;

	const auto path = GetPath(spec->vRefNum, spec->parID, spec->name);

	if (nextFileSlot == MAX_OPEN_FILES)
		TODOFATAL2("too many files have been opened");

	if (permission != fsRdPerm)
		TODOFATAL2("only fsRdPerm is implemented");

	if (!std::filesystem::is_regular_file(path)) {
		*refNum = -1;
		return fnfErr;
	}

	nextFileSlot++;
	openFiles[slot] = std::fstream(path, std::ios::binary | std::ios::in);
	*refNum = slot;
	LOG << "Opened RF " << path << " at slot " << *refNum << "\n";

	return noErr;
}

OSErr FindFolder(short vRefNum, OSType folderType, Boolean createFolder, short* foundVRefNum, long* foundDirID)
{
	switch (folderType) {
	case kPreferencesFolderType:
	{
#ifdef WIN32
		PWSTR wpath;
		// If we ever want to port to something older than Vista, this won't work.
		SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, 0, &wpath);
		auto path = std::filesystem::path(wpath).lexically_normal();
		*foundVRefNum = GetVolumeID(path);
		*foundDirID = GetDirectoryID(path);
		return noErr;
#else
		TODO2("your OS is not supported yet for folder type " << Pomme::fourCCstr(folderTYpe));
		break;
#endif
	}

	default:
		TODO2("folder type '" << Pomme::FourCCString(folderType) << "' isn't supported yet");
		return fnfErr;
	}
}

OSErr DirCreate(short vRefNum, long parentDirID, ConstStr255Param directoryName, long* createdDirID)
{
	const auto path = GetPath(vRefNum, parentDirID, directoryName);

	if (std::filesystem::exists(path)) {
		if (std::filesystem::is_directory(path)) {
			LOG << __func__ << ": directory already exists: " << path << "\n";
			return noErr;
		} else {
			std::cerr << __func__ << ": a file with the same name already exists: " << path << "\n";
			return bdNamErr;
		}
	}

	try {
		std::filesystem::create_directory(path);
	} catch (const std::filesystem::filesystem_error& e) {
		std::cerr << __func__ << " threw " << e.what() << "\n";
		return ioErr;
	}

	LOG << __func__ << ": created " << path << "\n";
	return noErr;
}