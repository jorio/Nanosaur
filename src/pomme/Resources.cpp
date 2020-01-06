#include <iostream>
#include "PommeInternal.h"
#include <strstream>

#define LOG POMME_GENLOG(POMME_DEBUG_RESOURCES, "RSRC")

using namespace Pomme;

//-----------------------------------------------------------------------------
// State

static OSErr lastResError = noErr;

static std::vector<ResourceFork> rezSearchStack;

static int rezSearchStackIndex = 0;

//-----------------------------------------------------------------------------
// Internal

static ResourceFork& GetCurRF() {
	return rezSearchStack[rezSearchStackIndex];
}

static void PrintStack(const char* msg) {
	LOG << "------ RESOURCE SEARCH STACK " << msg << " -------\n";
	for (int i = rezSearchStack.size() - 1; i >= 0; i--) {
		LOG	<< (rezSearchStackIndex == i? " =====> " : "        ")
			<< " StackPos=" << i << " "
			<< " RefNum=" << rezSearchStack[i].fileRefNum << " "
			<< GetFilenameFromRefNum__debug(rezSearchStack[i].fileRefNum)
			<< "\n";
	}
	LOG << "------------------------------------\n";
}

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

	auto f = Pomme::BigEndianIStream(GetStream(slot));

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

	// ----------------
	// Load resource fork

	rezSearchStack.emplace_back();
	rezSearchStackIndex = rezSearchStack.size() - 1;
	GetCurRF().fileRefNum = slot;
	GetCurRF().rezMap.clear();

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

			Pomme::ResourceOnDisk r;
			r.flags			= resFlags;
			r.dataOffset	= resDataOff;
			r.nameOffset	= resNameRelativeOff == 0xFFFF ? -1 : (resNameListOff + resNameRelativeOff);
			GetCurRF().rezMap[resType][resID] = r;
		}
	}

	PrintStack("FSPOPENRESFILE");

	return slot;
}

void UseResFile(short refNum) {
	// See MoreMacintoshToolbox:1-69

	lastResError = unimpErr;

	if (refNum == 0)
		TODOFATAL2("using the System file's resource fork is not implemented");
	if (refNum <= 0)
		TODOFATAL2("illegal refNum " << refNum);

	if (!IsStreamOpen(refNum)) {
		std::cerr << "can't UseResFile on this refNum " << refNum << "\n";
		return;
	}

	for (int i = 0; i < rezSearchStack.size(); i++) {
		if (rezSearchStack[i].fileRefNum == refNum) {
			lastResError = noErr;
			rezSearchStackIndex = i;
			PrintStack("AFTER USERESFILE");
			return;
		}
	}

	std::cerr << "no RF open with refNum " << rfNumErr << "\n";
	lastResError = rfNumErr;
}

short CurResFile() {
	return GetCurRF().fileRefNum;
}

void CloseResFile(short refNum) {
	if (refNum == 0)
		TODOFATAL2("closing the System file's resource fork is not implemented");
	if (refNum <= 0)
		TODOFATAL2("illegal refNum " << refNum);
	if (!IsStreamOpen(refNum))
		TODOFATAL2("already closed res file " << refNum);
	//UpdateResFile(refNum); // MMT:1-110
	CloseStream(refNum);

	auto it = rezSearchStack.begin();
	while (it != rezSearchStack.end()) {
		if (it->fileRefNum == refNum)
			it = rezSearchStack.erase(it);
		else
			it++;
	}

	rezSearchStackIndex = min(rezSearchStackIndex, rezSearchStack.size()-1);

	PrintStack("CLOSERESFILE");
}

short Count1Resources(ResType theType) {
	lastResError = noErr;

	try {
		return GetCurRF().rezMap.at(theType).size();
	}
	catch (std::out_of_range) {
		return 0;
	}
}

Handle GetResource(ResType theType, short theID) {
	lastResError = noErr;

	for (int i = rezSearchStackIndex; i >= 0; i--) {
		PrintStack("GetResource");

		try {
			const auto& rez = rezSearchStack[i].rezMap.at(theType).at(theID);

			auto f = BigEndianIStream(GetStream(rezSearchStack[i].fileRefNum));

			f.Goto(rez.dataOffset);
			auto len = f.Read<UInt32>();

			Handle h = NewHandle(len);
			f.Read(*h, len);

			LOG << FourCCString(theType) << " " << theID << ": " << len << "\n";

			/*
			std::stringstream fn;
			fn << "b:\\rez_" << theID << "." << Pomme::FourCCString(theType, '_');
			std::ofstream dump(fn.str(), std::ofstream::binary);
			dump.write(*h, len);
			dump.close();
			std::cout << "wrote " << fn.str() << "\n";
			*/

			return h;
		}
		catch (std::out_of_range) {
			LOG << "Resource not found, go deeper in stack of open resource forks\n";
			continue;
		}
	}

	std::cerr << "Couldn't get resource " << FourCCString(theType) << " #" << theID << "\n";
	lastResError = resNotFound;
	return nil;
}

void ReleaseResource(Handle theResource) {
	DisposeHandle(theResource);
}

void RemoveResource(Handle theResource) {
	DisposeHandle(theResource);
	TODO();
}

void AddResource(Handle theData, ResType theType, short theID, const char* name) {
	TODO();
}

void WriteResource(Handle theResource) {
	TODO();
}

void DetachResource(Handle theResource) {
	TODOMINOR();
}

long GetResourceSizeOnDisk(Handle theResource) {
	TODO();
	return -1;
}

long SizeResource(Handle theResource) {
	return GetResourceSizeOnDisk(theResource);
}
