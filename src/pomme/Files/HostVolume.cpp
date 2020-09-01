#include <filesystem>
#include <iostream>

#include "PommeInternal.h"

#include "Files/HostVolume.h"

#ifdef _WIN32
#include <shlobj.h>
#endif

#define LOG POMME_GENLOG(POMME_DEBUG_FILES, "HOST")

using namespace Pomme;
using namespace Pomme::Files;


HostVolume::HostVolume(short vRefNum)
	: Volume(vRefNum)
{
	// default directory (ID 0)
	directories.push_back(std::filesystem::current_path());
}

//-----------------------------------------------------------------------------
// Public utilities

bool HostVolume::IsRegularFile(const FSSpec* spec)
{
	return std::filesystem::is_regular_file(ToPath(*spec));
}

bool HostVolume::IsDirectory(const FSSpec* spec)
{
	return std::filesystem::is_directory(ToPath(*spec));
}

bool HostVolume::IsDirectoryIDLegal(long dirID) const
{
	return dirID >= 0 && dirID < directories.size();
}

long HostVolume::GetDirectoryID(const std::filesystem::path& dirPath)
{
	if (std::filesystem::exists(dirPath) && !std::filesystem::is_directory(dirPath)) {
		std::cerr << "Warning: GetDirectoryID should only be used on directories! " << dirPath << "\n";
	}
	for (int i = 0; i < directories.size(); i++) {
		if (dirPath == directories[i]) {
			//LOG << "directory [ID already allocated] " << i << ": " << dirPath << "\n";
			return i;
		}
	}
	directories.emplace_back(dirPath);
	LOG << "directory " << directories.size() - 1 << ": " << dirPath << "\n";
	return (long)directories.size() - 1;
}

//-----------------------------------------------------------------------------
// Internal utilities

std::filesystem::path HostVolume::ToPath(long parID, const std::string& name)
{
	std::filesystem::path path(directories[parID]);
	path += std::filesystem::path::preferred_separator;
	path += name;//Pascal2Cpp(name);
	return path.lexically_normal();
}

std::filesystem::path HostVolume::ToPath(const FSSpec& spec)
{
	return ToPath(spec.parID, Pascal2Cpp(spec.name));
}

FSSpec HostVolume::ToFSSpec(const std::filesystem::path& fullPath)
{
	auto parentPath = fullPath;
	parentPath.remove_filename();

	FSSpec spec;
	spec.vRefNum = volumeID;
	spec.parID = GetDirectoryID(parentPath);
	spec.name = Str255(fullPath.filename().string().c_str());
	return spec;
}

OSErr HostVolume::OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<std::iostream>& stream)
{
	if (permission == fsCurPerm) {
		TODO2("fsCurPerm not implemented yet");
		return unimpErr;
	}

	if ((permission & fsWrPerm) && forkType != ForkType::DataFork) {
		TODO2("opening resource fork for writing isn't implemented yet");
		return unimpErr;
	}

	auto path = ToPath(*spec);

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
			if (std::filesystem::exists(candidatePath)) {
				path = candidatePath;
				foundRF = true;
				break;
			}
		}

		if (!foundRF) {
			fnfErr;
		}
	}

	if (!std::filesystem::is_regular_file(path)) {
		return fnfErr;
	}

	std::ios::openmode openmode = std::ios::binary;
	if (permission & fsWrPerm) openmode |= std::ios::out;
	if (permission & fsRdPerm) openmode |= std::ios::in;

	stream = std::make_unique<std::fstream>(path, openmode);

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
	std::filesystem::path& path,
	const std::string& element,
	bool skipFiles = false)
{
	std::filesystem::path naiveConcat = path / element;

	if (!std::filesystem::exists(path)) {
		path = naiveConcat;
		return false;
	}

	if (std::filesystem::exists(naiveConcat)) {
		path = naiveConcat;
		return true;
	}

	const std::string ELEMENT = UppercaseCopy(element);

	for (const auto& candidate : std::filesystem::directory_iterator(path)) {
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
	if (directories.empty())
		throw std::exception("HostVolume::FSMakeFSSpec: No directories registered.");

	if (!IsDirectoryIDLegal(dirID))
		throw std::exception("HostVolume::FSMakeFSSpec: Illegal dirID.");

	auto path = directories[dirID];
	std::string suffix = fileName;

	// Case-insensitive sanitization
	bool exists = std::filesystem::exists(path);
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

	if (std::filesystem::exists(path)) {
		if (std::filesystem::is_directory(path)) {
			LOG << __func__ << ": directory already exists: " << path << "\n";
			return noErr;
		}
		else {
			std::cerr << __func__ << ": a file with the same name already exists: " << path << "\n";
			return bdNamErr;
		}
	}

	try {
		std::filesystem::create_directory(path);
	}
	catch (const std::filesystem::filesystem_error& e) {
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
	std::ofstream df(ToPath(*spec));
	df.close();
	// TODO: we could write an AppleDouble file to save the creator/filetype.
	return noErr;
}

OSErr HostVolume::FSpDelete(const FSSpec* spec)
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
