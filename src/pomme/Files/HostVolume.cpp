#include "PommeEnums.h"
#include "PommeDebug.h"
#include "PommeFiles.h"
#include "Files/HostVolume.h"
#include "Utilities/BigEndianIStream.h"
#include "Utilities/StringUtils.h"

#include <fstream>
#include <iostream>

#define LOG POMME_GENLOG(POMME_DEBUG_FILES, "HOST")

using namespace Pomme;
using namespace Pomme::Files;

struct HostForkHandle : public ForkHandle
{
	std::fstream backingStream;

public:
	HostForkHandle(ForkType forkType, char perm, fs::path& path)
		: ForkHandle(forkType, perm)
	{
		std::ios::openmode openmode = std::ios::binary;
		if (permission & fsWrPerm) openmode |= std::ios::out;
		if (permission & fsRdPerm) openmode |= std::ios::in;
		
		backingStream = std::fstream(path, openmode);
	}

	virtual ~HostForkHandle() = default;

	virtual std::iostream& GetStream() override
	{
		return backingStream;
	}
};


HostVolume::HostVolume(short vRefNum)
	: Volume(vRefNum)
{
	// default directory (ID 0)
	directories.push_back(fs::current_path());
}

//-----------------------------------------------------------------------------
// Public utilities

long HostVolume::GetDirectoryID(const fs::path& dirPath)
{
	if (fs::exists(dirPath) && !fs::is_directory(dirPath)) {
		std::cerr << "Warning: GetDirectoryID should only be used on directories! " << dirPath << "\n";
	}

	auto it = std::find(directories.begin(), directories.end(), dirPath);
	if (it != directories.end()) {
		return std::distance(directories.begin(), it);
	}

	directories.emplace_back(dirPath);
	LOG << "directory " << directories.size() - 1 << ": " << dirPath << "\n";
	return (long)directories.size() - 1;
}

//-----------------------------------------------------------------------------
// Internal utilities

fs::path HostVolume::ToPath(long parID, const std::string& name)
{
	fs::path path = directories[parID] / AsU8(name);
	return path.lexically_normal();
}

FSSpec HostVolume::ToFSSpec(const fs::path& fullPath)
{
	auto parentPath = fullPath;
	parentPath.remove_filename();

	FSSpec spec;
	spec.vRefNum = volumeID;
	spec.parID = GetDirectoryID(parentPath);
	snprintf(spec.cName, 256, "%s", (const char*)fullPath.filename().u8string().c_str());
	return spec;
}

static void ADFJumpToResourceFork(std::istream& stream)
{
	auto f = Pomme::BigEndianIStream(stream);

	if (0x0005160700020000ULL != f.Read<UInt64>()) {
		throw std::runtime_error("No ADF magic");
	}
	f.Skip(16);
	auto numOfEntries = f.Read<UInt16>();

	for (int i = 0; i < numOfEntries; i++) {
		auto entryID = f.Read<UInt32>();
		auto offset = f.Read<UInt32>();
		f.Skip(4); // length
		if (entryID == 2) {
			// Found entry ID 2 (resource fork)
			f.Goto(offset);
			return;
		}
	}

	throw std::runtime_error("Didn't find entry ID=2 in ADF");
}

OSErr HostVolume::OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<ForkHandle>& handle)
{
	if (permission == fsCurPerm) {
		TODO2("fsCurPerm not implemented yet");
		return unimpErr;
	}

	if ((permission & fsWrPerm) && forkType != ForkType::DataFork) {
		TODO2("opening resource fork for writing isn't implemented yet");
		return unimpErr;
	}

	auto path = ToPath(spec->parID, spec->cName);
	
	if (forkType == DataFork)
	{
		if (!fs::is_regular_file(path)) {
			return fnfErr;
		}
		handle = std::make_unique<HostForkHandle>(DataFork, permission, path);
		return noErr;
	}
	else
	{
		// We want to open a resource fork on the host volume.
		// It is likely stored under one of the following names: ._NAME, NAME.rsrc, or NAME/..namedfork/rsrc

		auto specName = path.filename().u8string();

		struct {
			u8string filename;
			bool isAppleDoubleFile;
		} candidates[] =
		{
			// "._NAME": ADF contained in zips created by macOS's built-in archiver
			{ u8"._" + specName, true },

			// "NAME.rsrc": ADF extracted from StuffIt/CompactPro archives by unar
			{ specName + u8".rsrc", true },

#if __APPLE__
			// "NAME/..namedfork/rsrc": macOS-specific way to access true resource forks (not ADF)
			{ specName + u8"/..namedfork/rsrc", false },
#endif
		};

		path.remove_filename();

		for (auto c : candidates) {
			auto candidatePath = path / c.filename;
			if (!fs::is_regular_file(candidatePath)) {
				continue;
			}
			path = candidatePath;
			handle = std::make_unique<HostForkHandle>(ResourceFork, permission, path);
			if (c.isAppleDoubleFile) {
				ADFJumpToResourceFork(handle->GetStream());
			}
			return noErr;
		}
	}
	
	return fnfErr;
}

static bool CaseInsensitiveAppendToPath(
	fs::path& path,
	const std::string& element,
	bool skipFiles = false)
{
	fs::path naiveConcat = path / AsU8(element);

	if (!fs::exists(path)) {
		path = naiveConcat;
		return false;
	}

	if (fs::exists(naiveConcat)) {
		path = naiveConcat;
		return true;
	}

	const auto ELEMENT = UppercaseCopy(element);

	for (const auto& candidate : fs::directory_iterator(path)) {
		if (skipFiles && !candidate.is_directory()) {
			continue;
		}

		auto f = FromU8(candidate.path().filename().u8string());

		// It might be an AppleDouble resource fork ("._file" or "file.rsrc")
		if (candidate.is_regular_file()) {
			if (f.starts_with("._")) {
				f = f.substr(2);
			}
			else if (f.ends_with(".rsrc")) {
				f = f.substr(0, f.length() - 5);
			}
		}

		if (ELEMENT == UppercaseCopy(f)) {
			path /= AsU8(f);
			return true;
		}
	}

	path = naiveConcat;
	return false;
}

//-----------------------------------------------------------------------------
// Implementation

OSErr HostVolume::FSMakeFSSpec(long dirID, const std::string& fileName, FSSpec* spec)
{
	if (dirID < 0 || (unsigned long)dirID >= directories.size()) {
		throw std::runtime_error("HostVolume::FSMakeFSSpec: directory ID not registered.");
	}

	auto path = directories[dirID];
	auto suffix = fileName;

	// Case-insensitive sanitization
	bool exists = fs::exists(path);
	std::string::size_type begin = (suffix.at(0) == ':') ? 1 : 0;

	// Iterate on path elements between colons
	while (begin < suffix.length()) {
		auto end = suffix.find(":", begin);

		bool isLeaf = end == std::string::npos; // no ':' found => end of path
		if (isLeaf) end = suffix.length();

		if (end == begin) { // "::" => parent directory
			path = path.parent_path();
		}
		else {
			auto element = suffix.substr(begin, end - begin);
			exists = CaseInsensitiveAppendToPath(path, element, !isLeaf);
		}

		// +1: jump over current colon
		begin = end + 1;
	}

	path = path.lexically_normal();

	LOG << path << "\n";

	*spec = ToFSSpec(path);

	return exists ? noErr : fnfErr;
}

OSErr HostVolume::DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID)
{
	const auto path = ToPath(parentDirID, directoryName);

	if (fs::exists(path)) {
		if (fs::is_directory(path)) {
			LOG << __func__ << ": directory already exists: " << path << "\n";
			return noErr;
		}
		else {
			std::cerr << __func__ << ": a file with the same name already exists: " << path << "\n";
			return bdNamErr;
		}
	}

	try {
		fs::create_directory(path);
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << __func__ << " threw " << e.what() << "\n";
		return ioErr;
	}

	if (createdDirID)
	{
		*createdDirID = GetDirectoryID(path);
	}

	LOG << __func__ << ": created " << path << "\n";
	return noErr;
}

OSErr HostVolume::FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag)
{
	std::ofstream df(ToPath(spec->parID, spec->cName));
	df.close();
	// TODO: we could write an AppleDouble file to save the creator/filetype.
	return noErr;
}

OSErr HostVolume::FSpDelete(const FSSpec* spec)
{
	auto path = ToPath(spec->parID, spec->cName);

	if (fs::remove(path))
		return noErr;
	else
		return fnfErr;
}
