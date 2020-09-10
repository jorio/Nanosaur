#include <iostream>
#include "PommeInternal.h"
#include "memstream.h"
#include "Files/ArchiveVolume.h"
#include "pomme/maconv/stuffit/methods/arsenic.h"

#ifdef _WIN32
#include <shlobj.h>
#endif

#define LOG POMME_GENLOG(POMME_DEBUG_FILES, "ARCH")

using namespace Pomme;
using namespace Pomme::Files;

struct ArchiveForkHandle : public ForkHandle
{
	std::vector<char> rawData;
	memstream stream;

public:
	ArchiveForkHandle(ForkType _forkType, char _perm, std::vector<char>&& _rawData)
		: ForkHandle(_forkType, _perm)
		, rawData(_rawData)
		, stream(rawData)
	{
	}

	virtual ~ArchiveForkHandle() override = default;

	virtual std::iostream& GetStream() override
	{
		return stream;
	}
};

ArchiveVolume::ArchiveVolume(short vRefNum, const std::string& pathToArchiveOnHost)
	: Volume(vRefNum)
{
	backingStream = std::make_unique<std::ifstream>(pathToArchiveOnHost, std::ios::binary | std::ios::in);
	ReadStuffIt5();
}

//-----------------------------------------------------------------------------
// Public utilities

long ArchiveVolume::GetDirectoryID(const std::string& dirPath)
{
	for (int i = 0; i < directories.size(); i++) {
		if (dirPath == directories[i]) {
			//LOG << "directory [ID already allocated] " << i << ": " << dirPath << "\n";
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Internal utilities

class ArchiveVolumeException : public std::runtime_error
{
public:
	ArchiveVolumeException(const char* m) : std::runtime_error(m) {}
};

static void ArchiveAssert(bool condition, const char* message)
{
	if (!condition)
		throw ArchiveVolumeException(message);
}

static std::string ProcessEntryName(std::vector<Byte>& nameBytes)
{
	std::string name;

	for (int i = 0; i < nameBytes.size(); ) {
		if ((unsigned int)nameBytes[i] == 0xE2 && (unsigned int)nameBytes[i + 1] == 0x84) {
			i += 2;
		}
		else {
			if ((unsigned int)nameBytes[i] < 0x20 || (unsigned int)nameBytes[i] > 127) {
				name += '#';
			}
			else {
				name += (char)nameBytes[i];
			}
			i++;
		}
	}

	return name;
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

std::string ArchiveVolume::FSSpecToPath(const FSSpec* spec) const
{
	return UppercaseCopy(directories[spec->parID] + ":" + Pascal2Cpp(spec->name));
}

OSErr ArchiveVolume::OpenFork(
	const FSSpec* spec,
	ForkType forkType,
	char permission,
	std::unique_ptr<ForkHandle>& handle)
{
	if (permission == fsCurPerm) {
		TODO2("fsCurPerm not implemented yet");
		return unimpErr;
	}

	if (permission & fsWrPerm) {
		return wPrErr;  // archives are read-only
	}

	std::string fullPath = FSSpecToPath(spec); //UppercaseCopy(directories[spec->parID] + ":" + Pascal2Cpp(spec->name));

	if (!files.contains(fullPath)) {
		return fnfErr;
	}

	auto& cfmd = files.at(fullPath);
	auto& fork = forkType == DataFork ? cfmd.dataFork : cfmd.rsrcFork;
	backingStream->seekg(fork.offsetToCompressedData, std::ios_base::beg);
	auto decompressedData0 = DecompressFork(fork);
	handle = std::make_unique<ArchiveForkHandle>(forkType, permission, std::move(decompressedData0));

	return noErr;
}

//-----------------------------------------------------------------------------
// Implementation

OSErr ArchiveVolume::FSMakeFSSpec(long dirID, const std::string& fileName, FSSpec* spec)
{
	if (dirID < 0 || dirID >= directories.size()) {
		throw std::runtime_error("ArchiveVolume::FSMakeFSSpec: directory ID not registered.");
	}

	auto path = directories[dirID];
	std::string suffix = UppercaseCopy(fileName);

	// Case-insensitive sanitization
	std::string::size_type begin = (suffix.at(0) == ':') ? 1 : 0;

	// Iterate on path elements between colons
	while (begin < suffix.length()) {
		auto end = suffix.find(":", begin);

		bool isLeaf = end == std::string::npos; // no ':' found => end of path
		if (isLeaf) end = suffix.length();

		if (end == begin) { // "::" => parent directory
			auto lastColon = path.rfind(':');
			if (lastColon == std::string::npos) {
				path = ":";
			} else {
				path = path.substr(0, lastColon);
			}
		}
		else {
			std::string element = suffix.substr(begin, end - begin);
			path += ":" + UppercaseCopy(element);
//			exists = CaseInsensitiveAppendToPath(path, element, !isLeaf);
		}

		// +1: jump over current colon
		begin = end + 1;
	}

	auto lastColon = path.rfind(':');
	std::string dirName = path.substr(0, lastColon);
	std::string trimmedFileName = lastColon==std::string::npos? path: path.substr(lastColon + 1);

	spec->parID = GetDirectoryID(dirName);
	spec->vRefNum = volumeID;
	spec->name = Str255(trimmedFileName.c_str());//fileName.c_str());

	auto pathKey = FSSpecToPath(spec);

	return files.contains(pathKey) || GetDirectoryID(pathKey) != -1
		? noErr
		: fnfErr;
}

OSErr ArchiveVolume::DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID)
{
	return wPrErr;  // volume is write-protected
}

OSErr ArchiveVolume::FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag)
{
	return wPrErr;  // volume is write-protected
}

OSErr ArchiveVolume::FSpDelete(const FSSpec* spec)
{
	return wPrErr;  // volume is write-protected
}

//-----------------------------------------------------------------------------
// StuffIt archive structure

std::vector<char> ArchiveVolume::DecompressFork(ArchiveVolume::CompressedForkMetadata& forkInfo)
{
	std::vector<char> outputBuffer(forkInfo.uncompressedLength);

	backingStream->seekg(forkInfo.offsetToCompressedData, std::ios_base::beg);

	switch (forkInfo.compressionMethod)
	{
	case 0:
	{
		ArchiveAssert(forkInfo.uncompressedLength == forkInfo.compressedLength, "Method 0: uncompressed/compressed length mismatch");
		backingStream->read(outputBuffer.data(), outputBuffer.size());
		break;
	}

	case 15:
	{
		std::vector<char> compressedData(forkInfo.compressedLength);
		backingStream->read(compressedData.data(), compressedData.size());
		maconv::stuffit::ArsenicMethod arsenic{};
		arsenic.data = reinterpret_cast<unsigned char*>(compressedData.data());
		arsenic.end = arsenic.data + forkInfo.compressedLength;
		arsenic.Initialize();
		arsenic.ReadBytes(reinterpret_cast<unsigned char*>(outputBuffer.data()), outputBuffer.size());
		break;
	}

	default:
		throw ArchiveVolumeException("Unsupported compression method");
		break;
	}

	LOG << "Decompressed a fork\n";

	return outputBuffer;
}

UInt32 ArchiveVolume::ReadEntry(
	std::istream& input,
	int globalOffset,
	const std::string& parentPath,
	bool collapseIfFolder)
{
	BigEndianIStream f(input);

	ArchiveAssert(0xA5A5A5A5u == f.Read<UInt32>(), "Bad entry magic number");
	auto version = f.Read<UInt8>();
	ArchiveAssert(0 == f.Read<UInt8>(), "Expected 0");
	auto headerSize = f.Read<UInt16>();
	f.Skip(1);
	auto entryFlags = f.Read<UInt8>();
	f.Skip(4 + 4); // cdate, mdate
	f.Skip(4); // offset of previous entry
	auto offsetOfNextEntry = f.Read<UInt32>();
	auto offsetOfDirectoryEntry = f.Read<UInt32>(); // parent folder
	auto nameSize = f.Read<UInt16>();
	f.Skip(2); // crc-16

	if (entryFlags & 0x40) {
		// Folder
		auto offsetOfFirstEntryInFolder = f.Read<UInt32>();
		auto sizeOfCompleteDirectory = f.Read<UInt32>();
		f.Skip(4);
		auto nFilesInFolder = f.Read<UInt16>();
		auto folderNameBytes = f.ReadBytes(nameSize);
		std::string folderName = ProcessEntryName(folderNameBytes);

		auto pp2 = parentPath;
		if (!collapseIfFolder)
			pp2 += ":" + folderName;
		pp2 = UppercaseCopy(pp2);
		directories.push_back(pp2);

		f.Goto(globalOffset + offsetOfFirstEntryInFolder);
		for (int i = 0; i < nFilesInFolder; i++) {
			auto nextE = ReadEntry(input, globalOffset, pp2, false);
			f.Goto(globalOffset + nextE);
		}
	}
	else {
		CompressedFileMetadata cfmd = {};
//		cfmd.parent = parent;

		cfmd.dataFork.uncompressedLength = f.Read<UInt32>();
		cfmd.dataFork.compressedLength = f.Read<UInt32>();
		auto dataForkCRC16 = f.Read<UInt16>();
		f.Skip(2);
		cfmd.dataFork.compressionMethod = f.Read<UInt8>();

		auto passwordDataLength = f.Read<UInt8>();
		f.Skip(passwordDataLength);

		auto fileNameBytes = f.ReadBytes(nameSize);
		cfmd.name = ProcessEntryName(fileNameBytes);

		auto commentLength = f.Read<UInt8>();
		f.Skip(commentLength);

		auto hasResourceFork = f.Read<UInt8>() & 1;
		f.Skip(2);
		cfmd.type = f.Read<UInt32>();
		cfmd.creator = f.Read<UInt32>();

		f.Skip(2); // finder flags
		f.Skip(2 + 4 + 12);
		if (version == 1) f.Skip(4);

		cfmd.rsrcFork = {};

		if (hasResourceFork) {
			cfmd.rsrcFork.uncompressedLength = f.Read<UInt32>();
			cfmd.rsrcFork.compressedLength = f.Read<UInt32>();
			auto resourceForkCRC16 = f.Read<UInt16>();
			f.Skip(2);
			cfmd.rsrcFork.compressionMethod = f.Read<UInt8>();
			ArchiveAssert(0 == f.Read<UInt8>(), "resource fork password flag?");
		}

		cfmd.rsrcFork.offsetToCompressedData = f.Tell();
		cfmd.dataFork.offsetToCompressedData = cfmd.rsrcFork.offsetToCompressedData + cfmd.rsrcFork.compressedLength;

		files.emplace(UppercaseCopy(parentPath + ":" + cfmd.name), cfmd);
	}

	return offsetOfNextEntry;
}

static bool ReadStuffItMagic(std::istream& input)
{
	char magic[81];
	input.read(magic, 80);
	magic[80] = '\0';
	return 0 == strncmp("StuffIt (c)1997-", magic, 16);
}

void ArchiveVolume::ReadStuffIt5()
{
	auto offset = backingStream->tellg();

	bool foundMagic = ReadStuffItMagic(*backingStream);

	// It might be a SIT wrapped inside MacBinary
	if (!foundMagic) {
		offset += 128;
		backingStream->seekg(offset, std::ios::beg);
		foundMagic = ReadStuffItMagic(*backingStream);
	}

	if (!foundMagic) {
		throw ArchiveVolumeException("Didn't find StuffIt magic string");
	}

	BigEndianIStream f(*backingStream);
	f.Skip(2);
	ArchiveAssert(5 == f.Read<UInt8>(), "Unknown StuffIt version");
	f.Skip(1); // flags
	auto totalSize = f.Read<UInt32>();
	f.Skip(4);
	auto nEntriesInRootDirectory = f.Read<UInt16>();
	auto offsetOfFirstEntryInRootDirectory = f.Read<UInt32>();
	f.Skip(2);

	f.Goto((int)offset + offsetOfFirstEntryInRootDirectory);

	ArchiveAssert(nEntriesInRootDirectory == 1, "cannot collapse root folder because there are more than one root entries");

	ReadEntry(*backingStream, offset, "", true);
}
