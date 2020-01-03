#include <iostream>
#include "PommeInternal.h"
#include <strstream>

#ifdef POMME_DEBUG_RESOURCES
static std::ostream& LOG = std::cout;
#else
static std::ostringstream LOG;
#endif

using namespace Pomme;

//-----------------------------------------------------------------------------
// State

OSErr lastResError;
short currentResFile = -1;
RezFork curRezFork;

//-----------------------------------------------------------------------------
// Resource file management

OSErr ResError(void) {
	return lastResError;
}

short FSpOpenResFile(const FSSpec* spec, char permission) {
	short slot;

	lastResError = FSpOpenRF(spec, permission, &slot);

	if (noErr != lastResError) {
		return -1;
	}

	// ----------------
	// Load resource fork

	curRezFork.rezMap.clear();

	auto f = Pomme::BigEndianIStream(GetIStreamRF(slot));

	// ----------------
	// Detect AppleDouble

	if (0x0005160700020000ULL != f.Read<UInt64>())
		TODOFATAL2("Not ADF magic");
	f.Skip(16);
	auto numOfEntries = f.Read<UInt16>();
	UInt32 adfResForkLen = 0;
	UInt32 adfResForkOff = 0;
	for (int i = 0; i < numOfEntries; i++) {
		auto entryID = f.Read<UInt32>();
		auto offset = f.Read<UInt32>();
		auto length = f.Read<UInt32>();
		if (entryID == 2) {
			f.Goto(offset);
			adfResForkLen = length;
			adfResForkOff = offset;
			break;
		}
	}

	// -------------------
	// Resource Header
	UInt32 dataSectionOff = f.Read<UInt32>() + adfResForkOff;
	UInt32 mapSectionOff = f.Read<UInt32>() + adfResForkOff;
	UInt32 dataSectionLen = f.Read<UInt32>();
	UInt32 mapSectionLen = f.Read<UInt32>();
	f.Skip(112 + 128); // system- (112) and app- (128) reserved data

	if (f.Tell() != dataSectionOff)
		TODOFATAL2("unexpected data offset");

	f.Goto(mapSectionOff);

	// map header
	f.Skip(16 + 4 + 2); // junk
	UInt16 fileAttr = f.Read<UInt16>();
	UInt32 typeListOff = f.Read<UInt16>() + mapSectionOff;
	UInt32 resNameListOff = f.Read<UInt16>() + mapSectionOff;

	// all resource types
	int nResTypes = 1 + f.Read<UInt16>();
	for (int i = 0; i < nResTypes; i++) {
		UInt32 resType = f.Read<OSType>();
		int    resCount = f.Read<UInt16>() + 1;
		UInt32 resRefListOff = f.Read<UInt16>() + typeListOff;

		// The guard will rewind the file cursor to the pos in the next iteration
		auto guard1 = f.GuardPos();

		f.Goto(resRefListOff);

		for (int j = 0; j < resCount; j++) {
			SInt16 resID = f.Read<UInt16>();
			UInt16 resNameRelativeOff = f.Read<UInt16>();
			UInt32 resPackedAttr = f.Read<UInt32>();
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
		}
	}

	return slot;
}

void UseResFile(short refNum) {
	// See MoreMacintoshToolbox:1-69

	lastResError = unimpErr;

	if (refNum == 0)
		TODOFATAL2("using the System file's resource fork is not implemented");
	if (refNum <= 0)
		TODOFATAL2("illegal refNum " << refNum);

	if (!GetIStreamRF(refNum).is_open()) {
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
	if (!GetIStreamRF(refNum).is_open())
		TODOFATAL2("already closed res file " << refNum);
	//UpdateResFile(refNum); // MMT:1-110
	GetIStreamRF(refNum).close();
	if (refNum == currentResFile)
		currentResFile = -1;
}

short Count1Resources(ResType theType) {
	lastResError = noErr;

	try {
		return curRezFork.rezMap.at(theType).size();
	}
	catch (std::out_of_range) {
		return 0;
	}
}

Handle GetResource(ResType theType, short theID) {
	lastResError = noErr;

	try {
		const auto& data = curRezFork.rezMap.at(theType).at(theID).data;
		LOG << "GetResource " << FourCCString(theType) << " " << theID << ": " << data.size() << "\n";
		Handle h = NewHandle(data.size());
		memcpy(*h, data.data(), data.size());
		return h;
	}
	catch (std::out_of_range) {
		lastResError = resNotFound;
		return nil;
	}
}
