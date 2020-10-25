#include "Pomme.h"
#include "PommeFiles.h"
#include "Utilities/BigEndianIStream.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#define LOG POMME_GENLOG(POMME_DEBUG_RESOURCES, "RSRC")

using namespace Pomme;
using namespace Pomme::Files;

//-----------------------------------------------------------------------------
// State

static OSErr lastResError = noErr;

static std::vector<ResourceFork> rezSearchStack;

static int rezSearchStackIndex = 0;

//-----------------------------------------------------------------------------
// Internal

static void ResourceAssert(bool condition, const char* message)
{
	if (!condition) {
		throw std::runtime_error(message);
	}
}

static ResourceFork& GetCurRF() {
	return rezSearchStack[rezSearchStackIndex];
}

#if 0
static void PrintStack(const char* msg) {
	LOG << "------ RESOURCE SEARCH STACK " << msg << " -------\n";
	for (int i = int(rezSearchStack.size() - 1); i >= 0; i--) {
		LOG	<< (rezSearchStackIndex == i? " =====> " : "        ")
			<< " StackPos=" << i << " "
			<< " RefNum=" << rezSearchStack[i].fileRefNum << " "
//			<< Pomme::Files::GetHostFilename(rezSearchStack[i].fileRefNum)
			<< "\n";
	}
	LOG << "------------------------------------\n";
}

static void DumpResource(ResourceMetadata& meta)
{
	Handle handle = NewHandle(meta.size);
	auto& fork = Pomme::Files::GetStream(meta.forkRefNum);
	fork.seekg(meta.dataOffset, std::ios::beg);
	fork.read(*handle, meta.size);

	std::stringstream fn;
	fn << "rezdump/" << meta.id << "_" << meta.name << "." << Pomme::FourCCString(meta.type, '_');
	std::ofstream dump(fn.str(), std::ofstream::binary);
	dump.write(*handle, meta.size);
	dump.close();
	std::cout << "wrote " << fn.str() << "\n";

	DisposeHandle(handle);
}
#endif

//-----------------------------------------------------------------------------
// Resource file management

OSErr ResError(void) {
	return lastResError;
}

short FSpOpenResFile(const FSSpec* spec, char permission)
{
	short slot;

	lastResError = FSpOpenRF(spec, permission, &slot);

	if (noErr != lastResError) {
		return -1;
	}

	auto f = Pomme::BigEndianIStream(Pomme::Files::GetStream(slot));
	auto resForkOff = f.Tell();

	// ----------------
	// Load resource fork

	rezSearchStack.emplace_back();
	rezSearchStackIndex = int(rezSearchStack.size() - 1);
	GetCurRF().fileRefNum = slot;
	GetCurRF().resourceMap.clear();

	// -------------------
	// Resource Header
	UInt32 dataSectionOff = f.Read<UInt32>() + resForkOff;
	UInt32 mapSectionOff = f.Read<UInt32>() + resForkOff;
	f.Skip(4); // UInt32 dataSectionLen
	f.Skip(4); // UInt32 mapSectionLen
	f.Skip(112 + 128); // system- (112) and app- (128) reserved data

	ResourceAssert(f.Tell() == dataSectionOff, "FSpOpenResFile: Unexpected data offset");

	f.Goto(mapSectionOff);

	// map header
	f.Skip(16 + 4 + 2); // junk
	f.Skip(2); // UInt16 fileAttr
	UInt32 typeListOff = f.Read<UInt16>() + mapSectionOff;
	UInt32 resNameListOff = f.Read<UInt16>() + mapSectionOff;

	// all resource types
	int nResTypes = 1 + f.Read<UInt16>();
	for (int i = 0; i < nResTypes; i++) {
		OSType resType = f.Read<OSType>();
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

			// Check compressed flag
			ResourceAssert(!(resFlags & 1), "FSpOpenResFile: Compressed resources not supported yet");

			// Fetch name
			std::string name;
			if (resNameRelativeOff != 0xFFFF) {
				f.Goto(resNameListOff + resNameRelativeOff);
				name = f.ReadPascalString();
			}

			// Fetch size
			f.Goto(resDataOff);
			SInt32 size = f.Read<SInt32>();

			ResourceMetadata resMetadata;
			resMetadata.forkRefNum = slot;
			resMetadata.type       = resType;
			resMetadata.id         = resID;
			resMetadata.flags      = resFlags;
			resMetadata.dataOffset = resDataOff + 4;
			resMetadata.size       = size;
			resMetadata.name       = name;
			GetCurRF().resourceMap[resType][resID] = resMetadata;
		}
	}

	//PrintStack(__func__);

	return slot;
}

void UseResFile(short refNum) {
	// See MoreMacintoshToolbox:1-69

	lastResError = unimpErr;

	ResourceAssert(refNum != 0, "UseResFile: Using the System file's resource fork is not implemented.");
	ResourceAssert(refNum >= 0, "UseResFile: Illegal refNum");
	ResourceAssert(IsStreamOpen(refNum), "UseResFile: Resource stream not open");

	for (size_t i = 0; i < rezSearchStack.size(); i++) {
		if (rezSearchStack[i].fileRefNum == refNum) {
			lastResError = noErr;
			rezSearchStackIndex = i;
			return;
		}
	}

	std::cerr << "no RF open with refNum " << rfNumErr << "\n";
	lastResError = rfNumErr;
}

short CurResFile() {
	return GetCurRF().fileRefNum;
}

void CloseResFile(short refNum)
{
	ResourceAssert(refNum != 0, "CloseResFile: Closing the System file's resource fork is not implemented.");
	ResourceAssert(refNum >= 0, "CloseResFile: Illegal refNum");
	ResourceAssert(IsStreamOpen(refNum), "CloseResFile: Resource stream not open");

	//UpdateResFile(refNum); // MMT:1-110
	Pomme::Files::CloseStream(refNum);

	auto it = rezSearchStack.begin();
	while (it != rezSearchStack.end()) {
		if (it->fileRefNum == refNum)
			it = rezSearchStack.erase(it);
		else
			it++;
	}

	rezSearchStackIndex = std::min(rezSearchStackIndex, (int)rezSearchStack.size()-1);
}

short Count1Resources(ResType theType) {
	lastResError = noErr;

	try {
		return (short)GetCurRF().resourceMap.at(theType).size();
	}
	catch (std::out_of_range&) {
		return 0;
	}
}

Handle GetResource(ResType theType, short theID) {
	lastResError = noErr;

	for (int i = rezSearchStackIndex; i >= 0; i--) {
		const auto& fork = rezSearchStack[i];

		if (fork.resourceMap.end() == fork.resourceMap.find(theType))
            continue;
        
        auto& resourcesOfType = fork.resourceMap.at(theType);
        if (resourcesOfType.end() == resourcesOfType.find(theID))
			continue;
		
		const auto& meta = fork.resourceMap.at(theType).at(theID);
		auto& forkStream = Pomme::Files::GetStream(rezSearchStack[i].fileRefNum);
		Handle handle = NewHandle(meta.size);
		forkStream.seekg(meta.dataOffset, std::ios::beg);
		forkStream.read(*handle, meta.size);
		return handle;
	}

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
	lastResError = noErr;
	ONCE(TODOMINOR());
}

long GetResourceSizeOnDisk(Handle theResource) {
	TODO();
	return -1;
}

long SizeResource(Handle theResource) {
	return GetResourceSizeOnDisk(theResource);
}
