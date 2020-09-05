#include <filesystem>
#include <iostream>
#include <sstream>

#include "PommeInternal.h"
#include "GrowablePool.h"
#include "memstream.h"
#include "Files/Volume.h"
#include "Files/HostVolume.h"
#include "Files/ArchiveVolume.h"

#ifdef _WIN32
#include <shlobj.h>
#endif

using namespace Pomme;
using namespace Pomme::Files;

#define LOG POMME_GENLOG(POMME_DEBUG_FILES, "FILE")

constexpr int MAX_VOLUMES = 32767;  // vRefNum is a signed short

//-----------------------------------------------------------------------------
// State

static Pomme::GrowablePool<std::unique_ptr<ForkHandle>, SInt16, 0x7FFF> openFiles;

static std::vector<std::unique_ptr<Volume>> volumes;

//-----------------------------------------------------------------------------
// Utilities

bool Pomme::Files::IsRefNumLegal(short refNum)
{
	return openFiles.IsAllocated(refNum);
}

std::iostream& Pomme::Files::GetStream(short refNum)
{
	if (!IsRefNumLegal(refNum)) {
		throw std::exception("illegal refNum");
	}
	return openFiles[refNum]->GetStream();
}

void Pomme::Files::CloseStream(short refNum)
{
	if (!IsRefNumLegal(refNum)) {
		throw std::exception("illegal refNum");
	}
	openFiles[refNum].reset(nullptr);
	openFiles.Dispose(refNum);
	LOG << "Closed stream " << refNum << "\n";
}

bool Pomme::Files::IsStreamOpen(short refNum)
{
	if (!IsRefNumLegal(refNum)) {
		throw std::exception("illegal refNum");
	}
	return openFiles[refNum].get() != nullptr;
//	return openFiles[refNum].stream.is_open();
}

bool Pomme::Files::IsStreamPermissionAllowed(short refNum, char perm)
{
	if (!IsRefNumLegal(refNum)) {
		throw std::exception("illegal refNum");
	}
	return (perm & openFiles[refNum]->permission) == perm;
}

//-----------------------------------------------------------------------------
// Init

void Pomme::Files::Init()
{
	auto hostVolume = std::make_unique<HostVolume>(0);
	volumes.push_back(std::move(hostVolume));

	short systemRefNum = openFiles.Alloc();
	if (systemRefNum != 0) {
		throw std::logic_error("expecting 0 for system refnum");
	}
}

//-----------------------------------------------------------------------------
// Implementation

bool IsVolumeLegal(short vRefNum)
{
	return vRefNum >= 0 && vRefNum < volumes.size();
}

OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param pascalFileName, FSSpec* spec)
{
	if (!IsVolumeLegal(vRefNum))
		return nsvErr;
	
	return volumes.at(vRefNum)->FSMakeFSSpec(dirID, Pascal2Cpp(pascalFileName), spec);
}

static OSErr OpenFork(const FSSpec* spec, ForkType forkType, char permission, short* refNum)
{
	if (openFiles.IsFull())
		return tmfoErr;
	if (!IsVolumeLegal(spec->vRefNum))
		return nsvErr;
	short newRefNum = openFiles.Alloc();
	auto& handlePtr = openFiles[newRefNum];
	OSErr rc = volumes.at(spec->vRefNum)->OpenFork(spec, forkType, permission, handlePtr);
	if (rc != noErr) {
		openFiles.Dispose(newRefNum);
		newRefNum = -1;
	}
	if (refNum) {
		*refNum = newRefNum;
	}
	if (rc == noErr) {
		LOG << "Stream #" << newRefNum << " opened: " << Pascal2Cpp(spec->name) << ", " << (forkType==DataFork?"data":"rsrc") << "\n";
	} else {
		LOG << "Failed to open " << Pascal2Cpp(spec->name) << "\n";
	}
	
	return rc;
}

OSErr FSpOpenDF(const FSSpec* spec, char permission, short* refNum)
{
	return OpenFork(spec, ForkType::DataFork, permission, refNum);
}

OSErr FSpOpenRF(const FSSpec* spec, char permission, short* refNum)
{
	return OpenFork(spec, ForkType::ResourceFork, permission, refNum);
}

OSErr FindFolder(short vRefNum, OSType folderType, Boolean createFolder, short* foundVRefNum, long* foundDirID)
{
	std::filesystem::path path;

	switch (folderType) {
	case kPreferencesFolderType:
	{
#ifdef _WIN32
		PWSTR wpath;
		// If we ever want to port to something older than Vista, this won't work.
		SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, 0, &wpath);
		path = std::filesystem::path(wpath);
#elif defined(__APPLE__)
		// TODO: mac pref folder
		TODO2("your OS is not supported yet for folder type " << Pomme::FourCCString(folderType));
		return fnfErr;
#else
		const char *home = getenv("XDG_CONFIG_HOME");
		if (home) {
			path = std::filesystem::path(home);
		} else {
			home = getenv("HOME");
			if (!home) {
				return fnfErr;
			}
			path = std::filesystem::path(home) / ".config";
		}
#endif
		break;
	}

	default:
		TODO2("folder type '" << Pomme::FourCCString(folderType) << "' isn't supported yet");
		return fnfErr;
	}

	path = path.lexically_normal();

	bool exists = std::filesystem::exists(path);

	if (exists && !std::filesystem::is_directory(path)) {
		return dupFNErr;
	}
	if (!exists && createFolder) {
		std::filesystem::create_directories(path);
	}

	*foundVRefNum = 0;//GetVolumeID(path);
	*foundDirID = dynamic_cast<HostVolume*>(volumes.at(0).get())->GetDirectoryID(path);
	return noErr;
}

OSErr DirCreate(short vRefNum, long parentDirID, ConstStr255Param directoryName, long* createdDirID)
{
	return IsVolumeLegal(vRefNum)
		? volumes.at(vRefNum)->DirCreate(parentDirID, Pascal2Cpp(directoryName), createdDirID)
		: nsvErr;
}

OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag)
{
	return IsVolumeLegal(spec->vRefNum)
		? volumes.at(spec->vRefNum)->FSpCreate(spec, creator, fileType, scriptTag)
		: nsvErr;
}

OSErr FSpDelete(const FSSpec* spec)
{
	return IsVolumeLegal(spec->vRefNum)
		? volumes.at(spec->vRefNum)->FSpDelete(spec)
		: nsvErr;
}

OSErr ResolveAlias(const FSSpec* spec, AliasHandle alias, FSSpec* target, Boolean* wasChanged)
{
	*wasChanged = false;

	int aliasSize = GetHandleSize(alias);

	// the target FN is at offset 50, and the target FN is a Str63, so 50+64
	if (aliasSize < 50+64) {
		std::cerr << "unexpected size of alias: " << aliasSize << "\n";
		return unimpErr;
	}

	memstream istr(*alias, GetHandleSize(alias));
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

	if (!IsVolumeLegal(target->vRefNum)) return nsvErr;
	
	auto& volume = *volumes.at(target->vRefNum);

	if (!volume.IsDirectoryIDLegal(target->parID)) {
		return dirNFErr;
	}

	//auto path = ToPath(*target);
	if (kind == 0 && !volume.IsRegularFile(target)) {
		//std::cerr << "alias target file doesn't exist: " << path << "\n";
		return fnfErr;
	}
	else if (kind == 1 && !volume.IsDirectory(target)) {
		//std::cerr << "alias target dir doesn't exist: " << path << "\n";
		return dirNFErr;
	}

	LOG << "alias OK: " << Pascal2Cpp(target->name) << "\n";
	return noErr;
}

OSErr FSRead(short refNum, long* count, Ptr buffPtr)
{
	if (*count < 0) return paramErr;
	if (!IsRefNumLegal(refNum)) return rfNumErr;
	if (!IsStreamOpen(refNum)) return fnOpnErr;
	if (!IsStreamPermissionAllowed(refNum, fsRdPerm)) return ioErr;

	auto& f = GetStream(refNum);
	f.read(buffPtr, *count);
	*count = (long)f.gcount();
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

OSErr FSClose(short refNum)
{
	if (!IsRefNumLegal(refNum))
		return rfNumErr;
	if (!IsStreamOpen(refNum))
		return fnOpnErr;
	CloseStream(refNum);
	return noErr;
}

OSErr GetEOF(short refNum, long* logEOF)
{
	if (!IsRefNumLegal(refNum)) return rfNumErr;
	if (!IsStreamOpen(refNum)) return fnOpnErr;

	auto& f = GetStream(refNum);
	StreamPosGuard guard(f);
	f.seekg(0, std::ios::end);
	*logEOF = (long)f.tellg();

	return noErr;
}

OSErr SetEOF(short refNum, long logEOF)
{
	TODO();
	return unimpErr;
}

short Pomme::Files::MountArchiveAsVolume(const std::string& archivePath)
{
	if (volumes.size() >= MAX_VOLUMES) {
		throw std::out_of_range("Too many volumes mounted");
	}
	
	short vRefNum = volumes.size();
	
	auto archiveVolume = std::make_unique<ArchiveVolume>(vRefNum, archivePath);
	volumes.push_back(std::move(archiveVolume));
	
	LOG << "Archive \"" << archivePath << "\" mounted as volume " << vRefNum << ".\n";
	
	return vRefNum;
}

