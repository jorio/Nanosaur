#include "Pomme.h"
#include "Files/HostVolume.h"
#include "Utilities/BigEndianIStream.h"

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
	fs::path path(directories[parID]);
	path += fs::path::preferred_separator;
	path += name;
	return path.lexically_normal();
}

FSSpec HostVolume::ToFSSpec(const fs::path& fullPath)
{
	auto parentPath = fullPath;
	parentPath.remove_filename();

	FSSpec spec;
	spec.vRefNum = volumeID;
	spec.parID = GetDirectoryID(parentPath);
	snprintf(spec.cName, 256, "%s", fullPath.filename().string().c_str());
	return spec;
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

	if (forkType == ResourceFork)
	{
		auto specName = path.filename().string();
		path.remove_filename();

		auto candidates = {
				"._" + specName,
				specName + ".rsrc",
#if __APPLE__
				//specName + "/..namedfork/rsrc" // TODO: check that this works in OSX land
#endif
		};

		bool foundRF = false;
		for (auto c : candidates) {
			auto candidatePath = path / c;
			if (fs::exists(candidatePath)) {
				path = candidatePath;
				foundRF = true;
				break;
			}
		}

		if (!foundRF) {
			return fnfErr;
		}
	}

	if (!fs::is_regular_file(path)) {
		return fnfErr;
	}

	handle = std::make_unique<HostForkHandle>(forkType, permission, path);

	if (forkType == ResourceFork)
	{
		// ----------------
		// Detect AppleDouble

		auto f = Pomme::BigEndianIStream(handle->GetStream());
		if (0x0005160700020000ULL != f.Read<UInt64>()) {
			throw std::runtime_error("No ADF magic found in " + path.string());
		}
		f.Skip(16);
		auto numOfEntries = f.Read<UInt16>();
		bool foundEntryID2 = false;
		for (int i = 0; i < numOfEntries; i++) {
			auto entryID = f.Read<UInt32>();
			auto offset = f.Read<UInt32>();
			auto length = f.Read<UInt32>();
			if (entryID == 2) {
				foundEntryID2 = true;
				f.Goto(offset);
				break;
			}
		}
		if (!foundEntryID2) {
			throw std::runtime_error("Didn't find entry ID=2 in ADF");
		}
	}

	return noErr;
}

static std::string UppercaseCopy(const std::string& in)
{
	std::string out;
	std::transform(
		in.begin(),
		in.end(),
		std::back_inserter(out),
		[](unsigned char c) -> unsigned char { return std::toupper(c); });
	return out;
}

static bool CaseInsensitiveAppendToPath(
	fs::path& path,
	const std::string& element,
	bool skipFiles = false)
{
	fs::path naiveConcat = path / element;

	if (!fs::exists(path)) {
		path = naiveConcat;
		return false;
	}

	if (fs::exists(naiveConcat)) {
		path = naiveConcat;
		return true;
	}

	const std::string ELEMENT = UppercaseCopy(element);

	for (const auto& candidate : fs::directory_iterator(path)) {
		if (skipFiles && !candidate.is_directory()) {
			continue;
		}

		std::string f = candidate.path().filename().string();

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
			path /= f;
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
	std::string suffix = fileName;

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
			std::string element = suffix.substr(begin, end - begin);
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

	std::cout << "FSpDelete " << path << "\n";
	/*
	std::stringstream ss;
	ss << "The Mac application wants to delete \"" << path << "\".\nAllow?";
	if (IDYES != MessageBoxA(nullptr, ss.str().c_str(), "FSpDelete", MB_ICONQUESTION | MB_YESNO)) {
		return fLckdErr;
	}
	*/

	if (fs::remove(path))
		return noErr;
	else
		return fnfErr;
}
