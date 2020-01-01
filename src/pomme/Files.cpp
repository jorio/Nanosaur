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

	auto f = Pomme::BigEndianIStream(openFiles[slot].rf);

	// ----------------
	// Detect AppleDouble
	
	if (0x0005160700020000ULL != f.Read<UInt64>())
		TODOFATAL2("Not ADF magic");
	f.Skip(16);
	auto numOfEntries = f.Read<UInt16>();
	UInt32 adfResForkLen = 0;
	UInt32 adfResForkOff = 0;
	for (int i = 0; i < numOfEntries; i++) {
		auto entryID	= f.Read<UInt32>();
		auto offset		= f.Read<UInt32>();
		auto length		= f.Read<UInt32>();
		if (entryID == 2) {
			f.Goto(offset);
			adfResForkLen = length;
			adfResForkOff = offset;
			break;
		}
	}

	// -------------------
	// Resource Header
	UInt32 dataSectionOff		= f.Read<UInt32>() + adfResForkOff;
	UInt32 mapSectionOff		= f.Read<UInt32>() + adfResForkOff;
	UInt32 dataSectionLen		= f.Read<UInt32>();
	UInt32 mapSectionLen		= f.Read<UInt32>();
	f.Skip(112 + 128); // system- (112) and app- (128) reserved data

	if (f.Tell() != dataSectionOff)
		TODOFATAL2("unexpected data offset");

	f.Goto(mapSectionOff);

	// map header
	f.Skip(16 + 4 + 2); // junk
	UInt16 fileAttr				= f.Read<UInt16>();
	UInt32 typeListOff			= f.Read<UInt16>() + mapSectionOff;
	UInt32 resNameListOff		= f.Read<UInt16>() + mapSectionOff;

	// all resource types
	int nResTypes = 1 + f.Read<UInt16>();
	for (int i = 0; i < nResTypes; i++) {
		UInt32 resType			= f.Read<OSType>();
		int    resCount			= f.Read<UInt16>() + 1;
		UInt32 resRefListOff	= f.Read<UInt16>() + typeListOff;

		// The guard will rewind the file cursor to the pos in the next iteration
		auto guard1 = f.GuardPos();

		f.Goto(resRefListOff);

		for (int j = 0; j < resCount; j++) {
			SInt16 resID				= f.Read<UInt16>();
			UInt16 resNameRelativeOff	= f.Read<UInt16>();
			UInt32 resPackedAttr		= f.Read<UInt32>();
			f.Skip(4); // junk

			// The guard will rewind the file cursor to the pos in the next iteration
			auto guard2 = f.GuardPos();

			// unpack attributes
			Byte   resFlags = (resPackedAttr & 0xFF000000) >> 24;
			UInt32 resDataOff = (resPackedAttr & 0x00FFFFFF) + dataSectionOff;

			if (resFlags & 1) // compressed
				TODOFATAL2("we don't support compressed resources yet");

			// Read name
			std::string name;
			if (resNameRelativeOff != 0xFFFF) {
				f.Goto(resNameListOff + resNameRelativeOff);
				// read pascal string
				UInt8 pascalStrLen = f.Read<UInt8>();
				char str[256];
				f.Read(str, pascalStrLen);
				str[pascalStrLen] = '\0';
				name = std::string(str);
			}

			// Read data
			f.Goto(resDataOff);
			UInt32 resLen = f.Read<UInt32>();

			std::vector<Byte> buf = f.ReadBytes(resLen);

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
