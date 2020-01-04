#include "PommeInternal.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <strstream>

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

std::filesystem::path Pomme::ToPath(short vRefNum, long parID, ConstStr255Param name) {
	std::filesystem::path path(directories[parID]);
	path += std::filesystem::path::preferred_separator;
	path += Pascal2C(name);
	return path.lexically_normal();
}

std::filesystem::path Pomme::ToPath(const FSSpec& spec) {
	return Pomme::ToPath(spec.vRefNum, spec.parID, spec.name);
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

bool Pomme::IsDirIDLegal(long dirID) {
	return dirID >= 0 && dirID < directories.size();
}

bool Pomme::IsRefNumLegal(short refNum) {
	return refNum > 0 && refNum < MAX_OPEN_FILES && refNum < nextFileSlot;
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

	const auto path = ToPath(*spec);

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

	if (nextFileSlot == MAX_OPEN_FILES)
		TODOFATAL2("too many files have been opened");

	if (permission != fsRdPerm)
		TODOFATAL2("only fsRdPerm is implemented");

	auto path = ToPath(*spec);

	// ADF filename
	std::stringstream adfFilename;
	adfFilename << "._" << Pascal2C(spec->name);
	// TODO: on osx, we could try {name}/..namedfork/rsrc
	path.replace_filename(adfFilename.str());

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
	const auto path = ToPath(vRefNum, parentDirID, directoryName);

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

OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) {
	TODO();
	return unimpErr;
}

OSErr FSpDelete(const FSSpec* spec) {
	TODO();
	return unimpErr;
}

OSErr ResolveAlias(const FSSpec* spec, AliasHandle alias, FSSpec* target, Boolean* wasChanged) {
	*wasChanged = false;

	int aliasSize = GetHandleSize(alias);

	// the target FN is at offset 50, and the target FN is a Str63, so 50+64
	if (aliasSize < 50+64) {
		std::cerr << "unexpected size of alias: " << aliasSize << "\n";
		return unimpErr;
	}

	std::istrstream istr(*alias, GetHandleSize(alias));
	Pomme::BigEndianIStream f(istr);

	f.Skip(4); // application signature

	if (f.Read<UInt16>() != aliasSize) {
		std::cerr << "unexpected size field in alias\n";
		return unimpErr;
	}

	if (f.Read<UInt16>() != 2) {
		std::cerr << "unexpected alias version number\n";
		return unimpErr;
	}

	auto kind = f.Read<UInt16>();
	if (kind > 2) {
		std::cerr << "unsupported alias kind " << kind << "\n";
		return unimpErr;
	}

	f.Skip(28); // volume name pascal string
	f.Skip(4); // volume creation date
	f.Skip(2); // volume signature (RW, BD, H+)
	f.Skip(2); // drive type (0=HD, 1=network, 2=400K FD, 3=800K FD, 4=1.4M FD, 5=misc.ejectable)
	f.Skip(4); // parent directory ID

	auto targetFilenameStr63 = f.ReadBytes(64);

	memcpy((char*)target->name, targetFilenameStr63.data(), 64);
	target->parID = spec->parID;
	target->vRefNum = spec->vRefNum;

	if (!IsDirIDLegal(target->parID)) {
		TODO2("illegal alis parID " << target->parID);
		return dirNFErr;
	}

	auto path = Pomme::ToPath(*target);
	if (kind == 0 && !std::filesystem::is_regular_file(path)) {
		std::cerr << "alias target file doesn't exist: " << path << "\n";
		return fnfErr;
	}
	else if (kind == 1 && !std::filesystem::is_directory(path)) {
		std::cerr << "alias target dir doesn't exist: " << path << "\n";
		return dirNFErr;
	}

	LOG << "alias OK: " << Pascal2C(target->name) << "\n";
	return noErr;
}

OSErr FSRead(short refNum, long* count, Ptr buffPtr) {
	if (*count < 0) return paramErr;
	if (!IsRefNumLegal(refNum)) return rfNumErr;
	if (!IsStreamOpen(refNum)) return fnOpnErr;

	auto& f = GetStream(refNum);
	f.read(buffPtr, *count);
	*count = f.gcount();
	if (f.eof()) return eofErr;

	return noErr;
}

OSErr FSWrite(short refNum, long* count, Ptr buffPtr) {
	TODO();
	return unimpErr;
}

OSErr FSClose(short refNum) {
	if (!IsRefNumLegal(refNum))
		return rfNumErr;
	if (!IsStreamOpen(refNum))
		return fnOpnErr;
	CloseStream(refNum);
	return noErr;
}

OSErr GetEOF(short refNum, long* logEOF) {
	if (!IsRefNumLegal(refNum)) return rfNumErr;
	if (!IsStreamOpen(refNum)) return fnOpnErr;

	auto& f = GetStream(refNum);
	StreamPosGuard guard(f);
	f.seekg(0, std::ios::end);
	*logEOF = f.tellg();

	return noErr;
}

OSErr SetEOF(short refNum, long logEOF) {
	TODO();
	return unimpErr;
}
