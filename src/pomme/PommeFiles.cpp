#include "PommeInternal.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <strstream>

constexpr auto MAX_OPEN_FILES = 32767;

//-----------------------------------------------------------------------------
// Types

struct InternalMacFileHandle {
	std::ifstream rf;
};

//-----------------------------------------------------------------------------
// State

OSErr lastResError;
std::vector<std::filesystem::path> fsspecParentDirectories;
InternalMacFileHandle openFiles[MAX_OPEN_FILES];
short nextFileSlot = -1;
short currentResFile = -1;
Pomme::RezFork curRezFork;

//-----------------------------------------------------------------------------
// Utilities

template<typename T> T ReadBE(std::ifstream& i) {
	char b[sizeof(T)];
	i.read(b, sizeof(T));
#if !(TARGET_RT_BIGENDIAN)
	std::reverse(b, b + sizeof(T));
#endif
	return *(T*)b;
}

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

class FilePosGuard {
	std::ifstream& _f;
	const std::streampos _backup;

public:
	FilePosGuard(std::ifstream& f) :
		_f(f),
		_backup(_f.tellg())
	{
	}

	~FilePosGuard()
	{
		_f.seekg(_backup, std::ios_base::beg);
	}
};

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
	if (noErr != FSMakeFSSpec(0, 0, applName, &applSpec))
		TODOFATAL2("couldn't load applSpec");
	short applRefNum = FSpOpenResFile(&applSpec, fsRdPerm);
	UseResFile(applRefNum);
}

//-----------------------------------------------------------------------------
// Implementation

OSErr ResError(void) {
	return lastResError;
}

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

short FSpOpenResFile(const FSSpec* spec0, char permission) {
	short slot = nextFileSlot;

	const FSSpecEx spec(*spec0);

	if (nextFileSlot == MAX_OPEN_FILES)
		TODOFATAL2("too many files have been opened");

	if (permission != fsRdPerm)
		TODOFATAL2("only fsRdPerm is implemented");

	if (!spec.isFile()) {
		lastResError = fnfErr;
		return -1;
	}

	nextFileSlot++;

	openFiles[slot].rf = std::ifstream(spec.cppPath(), std::ios::binary);

	// ----------------
	// Load resource fork

	curRezFork.rezMap.clear();

	// ----------------
	// Detect AppleDouble
	auto& f = openFiles[slot].rf;
	if (0x0005160700020000ULL != ReadBE<UInt64>(f))
		TODOFATAL2("Not ADF magic");
	f.seekg(16, std::ios_base::cur);
	auto numOfEntries = ReadBE<UInt16>(f);
	UInt32 adfResForkLen = 0;
	UInt32 adfResForkOff = 0;
	for (int i = 0; i < numOfEntries; i++) {
		auto entryID	= ReadBE<UInt32>(f);
		auto offset		= ReadBE<UInt32>(f);
		auto length		= ReadBE<UInt32>(f);
		if (entryID == 2) {
			f.seekg(offset, std::ios_base::beg);
			adfResForkLen = length;
			adfResForkOff = offset;
			break;
		}
	}

	// -------------------
	// Resource Header
	UInt32 dataSectionOff		= ReadBE<UInt32>(f) + adfResForkOff;
	UInt32 mapSectionOff		= ReadBE<UInt32>(f) + adfResForkOff;
	UInt32 dataSectionLen		= ReadBE<UInt32>(f);
	UInt32 mapSectionLen		= ReadBE<UInt32>(f);
	f.seekg(112 + 128, std::ios_base::cur); // system- (112) and app- (128) reserved data

	if (f.tellg() != dataSectionOff)
		TODOFATAL2("unexpected data offset");

	f.seekg(mapSectionOff, std::ios_base::beg);

	// map header
	f.seekg(16 + 4 + 2, std::ios_base::cur); // junk
	UInt16 fileAttr				= ReadBE<UInt16>(f);
	UInt32 typeListOff			= ReadBE<UInt16>(f) + mapSectionOff;
	UInt32 resNameListOff		= ReadBE<UInt16>(f) + mapSectionOff;

	// all resource types
	int nResTypes = 1 + ReadBE<UInt16>(f);
	for (int i = 0; i < nResTypes; i++) {
		UInt32 resType			= ReadBE<OSType>(f);
		int    resCount			= ReadBE<UInt16>(f) + 1;
		UInt32 resRefListOff	= ReadBE<UInt16>(f) + typeListOff;

		// The guard will rewind the file cursor to the pos in the next iteration
		FilePosGuard filepos1(f);

		f.seekg(resRefListOff, std::ios_base::beg);

		for (int j = 0; j < resCount; j++) {
			SInt16 resID				= ReadBE<UInt16>(f);
			UInt16 resNameRelativeOff	= ReadBE<UInt16>(f);
			UInt32 resPackedAttr		= ReadBE<UInt32>(f);
			f.seekg(4, std::ios_base::cur); // junk

			// The guard will rewind the file cursor to the pos in the next iteration
			FilePosGuard filepos2(f);

			// unpack attributes
			Byte   resFlags = (resPackedAttr & 0xFF000000) >> 24;
			UInt32 resDataOff = (resPackedAttr & 0x00FFFFFF) + dataSectionOff;

			if (resFlags & 1) // compressed
				TODOFATAL2("we don't support compressed resources yet");

			// Read name
			std::string name;
			if (resNameRelativeOff != 0xFFFF) {
				f.seekg(resNameListOff + resNameRelativeOff, std::ios_base::beg);
				// read pascal string
				UInt8 pascalStrLen = ReadBE<UInt8>(f);
				char str[256];
				f.read(str, pascalStrLen);
				str[pascalStrLen] = '\0';
				name = std::string(str);
			}

			// Read data
			f.seekg(resDataOff, std::ios_base::beg);
			UInt32 resLen = ReadBE<UInt32>(f);

			std::vector<Byte> buf(resLen);
			f.read(reinterpret_cast<char*>(buf.data()), resLen);

			Pomme::Rez r;
			r.fourCC = resType;
			r.id = resID;
			r.flags = resFlags;
			r.name = name;
			r.data = buf;
			curRezFork.rezMap[resType][resID] = r;

			std::cout << spec.cppPath() << ": "
				<< fourCCstr(resType) << " #" << resID
				<< " " << resLen << " bytes "
				<< " '" << name << "' " << "\n";
		}
	}

	return slot;
}

//-----------------------------------------------------------------------------
// Resource file management

void UseResFile(short refNum) {
	// See MoreMacintoshToolbox:1-69

	lastResError = unimpErr;

	if (refNum == 0)
		TODOFATAL2("using the System file's resource fork is not implemented");
	if (refNum <= 0)
		TODOFATAL2("illegal refNum " << refNum);

	if (!openFiles[refNum].rf.is_open()) {
		std::cerr << "can't UseResFile on this refNum " << refNum << "\n";
		return;
	}

	lastResError = noErr;
	currentResFile = refNum;
}

short CurResFile() {
	return currentResFile;
}

void CloseResFile(short refNum) {
	if (refNum == 0)
		TODOFATAL2("closing the System file's resource fork is not implemented");
	if (refNum <= 0)
		TODOFATAL2("illegal refNum " << refNum);
	if (!openFiles[refNum].rf.is_open())
		TODOFATAL2("already closed res file " << refNum);
	//UpdateResFile(refNum); // MMT:1-110
	openFiles[refNum].rf.close();
	if (refNum == currentResFile)
		currentResFile = -1;
}
