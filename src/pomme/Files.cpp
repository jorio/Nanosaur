#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <strstream>

#include "PommeInternal.h"
#include "GrowablePool.h"

#ifdef WIN32
#include <shlobj.h>
#endif

using namespace Pomme;
using namespace Pomme::Files;

#define LOG POMME_GENLOG(POMME_DEBUG_FILES, "FILE")

enum ForkType {
	DataFork,
	ResourceFork
};

//-----------------------------------------------------------------------------
// Internal structs

struct InternalFileHandle
{
	std::fstream stream;
	std::filesystem::path debugPath;
	int forkType;
	char permission;
};

//-----------------------------------------------------------------------------
// State

static std::vector<std::filesystem::path> directories;

static Pomme::GrowablePool<InternalFileHandle, SInt16, 0x7FFF> openFiles;

//-----------------------------------------------------------------------------
// Utilities

std::filesystem::path Pomme::Files::ToPath(short vRefNum, long parID, ConstStr255Param name)
{
	std::filesystem::path path(directories[parID]);
	path += std::filesystem::path::preferred_separator;
	path += Pascal2C(name);
	return path.lexically_normal();
}

std::filesystem::path Pomme::Files::ToPath(const FSSpec& spec)
{
	return Pomme::Files::ToPath(spec.vRefNum, spec.parID, spec.name);
}

bool Pomme::Files::IsDirIDLegal(long dirID)
{
	return dirID >= 0 && dirID < directories.size();
}

bool Pomme::Files::IsRefNumLegal(short refNum)
{
	return openFiles.IsAllocated(refNum);
}

std::fstream& Pomme::Files::GetStream(short refNum)
{
	return openFiles[refNum].stream;
}

void Pomme::Files::CloseStream(short refNum)
{
	openFiles[refNum].stream.close();
	openFiles.Dispose(refNum);
}

bool Pomme::Files::IsStreamOpen(short refNum)
{
	return openFiles[refNum].stream.is_open();
}

bool Pomme::Files::IsStreamPermissionAllowed(short refNum, char perm)
{
	return (perm & openFiles[refNum].permission) == perm;
}

std::string Pomme::Files::GetHostFilename(short refNum)
{
	return openFiles[refNum].debugPath.string();
}

//-----------------------------------------------------------------------------
// Internal Utilities

static short GetVolumeID(const std::filesystem::path& path)
{
	return 0;
}

static long GetDirectoryID(const std::filesystem::path& dirPath)
{
	if (std::filesystem::exists(dirPath) && !std::filesystem::is_directory(dirPath)) {
		std::cerr << "Warning: GetDirID should only be used on directories! " << dirPath << "\n";
	}
	directories.emplace_back(dirPath);
	return directories.size() - 1;
}

static FSSpec ToFSSpec(const std::filesystem::path& fullPath)
{
	auto parentPath = fullPath;
	parentPath.remove_filename();

	FSSpec spec;
	spec.vRefNum = GetVolumeID(parentPath);
	spec.parID = GetDirectoryID(parentPath);
	spec.name = Str255(fullPath.filename().string().c_str());
	return spec;
}

static OSErr FSpOpenXF(const std::filesystem::path& path, ForkType forkType, char permission, short* refNum)
{
	if (openFiles.IsFull())
		return tmfoErr;

	if (permission == fsCurPerm) {
		TODO2("fsCurPerm not implemented yet");
		return unimpErr;
	}

	if ((permission & fsWrPerm) && forkType != ForkType::DataFork) {
		TODO2("opening resource fork for writing isn't implemented yet");
		return unimpErr;
	}

	if (!std::filesystem::is_regular_file(path)) {
		*refNum = -1;
		return fnfErr;
	}

	std::ios::openmode openmode = std::ios::binary;
	if (permission & fsWrPerm) openmode |= std::ios::out;
	if (permission & fsRdPerm) openmode |= std::ios::in;

	short newRefNum = openFiles.Alloc();
	auto& of = openFiles[newRefNum];
	of.stream = std::fstream(path, openmode);
	of.debugPath = path;
	of.forkType = forkType;
	of.permission = permission;

	if (refNum)
		*refNum = newRefNum;

	LOG << "Opened " << path << " at slot " << newRefNum << "\n";

	return noErr;
}

//-----------------------------------------------------------------------------
// Init

void Pomme::Files::Init(const char* applName)
{
	// default directory (ID 0)
	directories.push_back(std::filesystem::current_path());

	short systemRefNum = openFiles.Alloc();
	if (systemRefNum != 0) throw "expecting 0 for system refnum";

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

OSErr FSpOpenDF(const FSSpec* spec, char permission, short* refNum)
{
	const auto path = ToPath(*spec);
	return FSpOpenXF(path, ForkType::DataFork, permission, refNum);
}

OSErr FSpOpenRF(const FSSpec* spec, char permission, short* refNum)
{
	auto path = ToPath(*spec);

	// ADF filename
	std::stringstream adfFilename;
	adfFilename << "._" << Pascal2C(spec->name);
	// TODO: on osx, we could try {name}/..namedfork/rsrc
	path.replace_filename(adfFilename.str());

	return FSpOpenXF(path, ForkType::ResourceFork, permission, refNum);
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

OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag)
{
	std::ofstream df(ToPath(*spec));
	df.close();
	// TODO: we could write an AppleDouble file to save the creator/filetype.
	return noErr;
}

OSErr FSpDelete(const FSSpec* spec)
{
	auto path = ToPath(*spec);

	std::cout << "FSpDelete " << path << "\n";
	/*
	std::stringstream ss;
	ss << "The Mac application wants to delete \"" << path << "\".\nAllow?";
	if (IDYES != MessageBoxA(nullptr, ss.str().c_str(), "FSpDelete", MB_ICONQUESTION | MB_YESNO)) {
		return fLckdErr;
	}
	*/

	if (std::filesystem::remove(path))
		return noErr;
	else
		return fnfErr;
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

	auto path = ToPath(*target);
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
	if (!IsStreamPermissionAllowed(refNum, fsRdPerm)) return ioErr;

	auto& f = GetStream(refNum);
	f.read(buffPtr, *count);
	*count = f.gcount();
	if (f.eof()) return eofErr;

	return noErr;
}

OSErr FSWrite(short refNum, long* count, Ptr buffPtr)
{
	if (*count < 0) return paramErr;
	if (!IsRefNumLegal(refNum)) return rfNumErr;
	if (!IsStreamOpen(refNum)) return fnOpnErr;
	if (!IsStreamPermissionAllowed(refNum, fsWrPerm)) return wrPermErr;

	auto& f = GetStream(refNum);
	f.write(buffPtr, *count);

	return noErr;
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
