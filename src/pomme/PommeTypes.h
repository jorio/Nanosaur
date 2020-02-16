#pragma once

//-----------------------------------------------------------------------------
// Integer types

typedef char                            SignedByte; 
typedef char                            SInt8;
typedef short                           SInt16;
typedef int                             SInt32;
typedef long long                       SInt64;

typedef unsigned char                   Byte; 
typedef unsigned char                   UInt8;
typedef unsigned char                   Boolean;
typedef unsigned short                  UInt16;
typedef unsigned int                    UInt32;
typedef unsigned long long              UInt64;

#if TARGET_RT_BIGENDIAN
struct UnsignedWide { UInt32 hi, lo; };
#else
struct UnsignedWide { UInt32 lo, hi; };
#endif

//-----------------------------------------------------------------------------
// Fixed/fract types

typedef SInt32                          Fixed;
typedef SInt32                          Fract;
typedef UInt32                          UnsignedFixed;
typedef SInt16                          ShortFixed;
typedef Fixed* FixedPtr;
typedef Fract* FractPtr;
typedef UnsignedFixed* UnsignedFixedPtr;
typedef ShortFixed* ShortFixedPtr;

//-----------------------------------------------------------------------------
// Basic system types

typedef SInt16                          OSErr;
typedef SInt32                          OSStatus;
typedef void* LogicalAddress;
typedef const void* ConstLogicalAddress;
typedef void* PhysicalAddress;
typedef UInt8* BytePtr;
typedef unsigned long                   ByteCount;
typedef unsigned long                   ByteOffset;
typedef SInt32                          Duration;
typedef UnsignedWide                    AbsoluteTime;
typedef UInt32                          OptionBits;
typedef unsigned long                   ItemCount;
typedef UInt32                          PBVersion;
typedef SInt16                          ScriptCode;
typedef SInt16                          LangCode;
typedef SInt16                          RegionCode;
typedef UInt32                          FourCharCode;
typedef FourCharCode                    OSType;
typedef FourCharCode                    ResType;
typedef OSType*                         OSTypePtr;
typedef ResType*                        ResTypePtr;
typedef char*                           Ptr;            // Pointer to a non-relocatable block
typedef Ptr*                            Handle;         // Pointer to a master pointer to a relocatable block 
typedef long                            Size;           // Number of bytes in a block (signed for historical reasons)
typedef void                            (*ProcPtr);

//-----------------------------------------------------------------------------
// (Pascal) String types

char* Pascal2C(const char* pstr);

#include "PascalStringHack.h"

typedef PascalString<32>				Str32;
typedef PascalString<255>				Str255;
typedef const Str255&					ConstStr255Param;
typedef char*							StringPtr;

//-----------------------------------------------------------------------------
// Point & Rect types

struct Point { SInt16 v, h; };

struct Rect
{
	union
	{
		struct { SInt16 top, left, bottom, right; };
		struct { Point topLeft, bottomRight; };
	};
};

typedef Point* PointPtr;
typedef Rect* RectPtr;

struct FixedPoint { Fixed x, y; };
struct FixedRect { Fixed left, top, right, bottom; };

//-----------------------------------------------------------------------------
// FSSpec types

struct FSSpec {
	// Volume reference number of the volume containing the specified file or directory.
	short vRefNum;

	// Parent directory ID of the specified file or directory (the directory ID of the directory containing the given file or directory).
	long parID;

	// The name of the specified file or directory. In Carbon, this name must be a leaf name; the name cannot contain a semicolon.
	Str255 name;
};

typedef Handle AliasHandle;

//-----------------------------------------------------------------------------
// QuickDraw types

typedef SInt16							QDErr;

struct RGBColor {
	UInt16 red;
	UInt16 green;
	UInt16 blue;
};

struct Picture {
	// Version 1 size.
	// Not used for version 2 PICTs, as the size may easily exceed 16 bits.
	SInt16 picSize;

	Rect picFrame;

	// Raw raster image decoded from PICT opcodes. Shouldn't be accessed
	// directly by the Mac application as it is stored in a format internal
	// to the Pomme implementation for rendering (typically ARGB32).
	Ptr __pomme_pixelsARGB32;
};


typedef Picture* PicPtr;
typedef PicPtr* PicHandle;
typedef Handle GDHandle; // GDevice handle. Game code doesn't care about GDevice internals, so we just alias a generic Handle
// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/QuickDraw_Ref/qdref_main/data_type_41.html
struct PixMap
{
	Rect bounds;
	short pixelSize;
	Ptr _impl;
};
typedef PixMap*							PixMapPtr;
typedef PixMapPtr*						PixMapHandle;
struct GrafPort {
	Rect portRect;
	void* _impl;
};
typedef GrafPort*						GrafPtr;
typedef GrafPtr                         WindowPtr;
typedef GrafPort                        CGrafPort;
typedef GrafPtr							CGrafPtr;
typedef CGrafPtr						GWorldPtr;

//-----------------------------------------------------------------------------
// Sound Manager types

struct SndCommand {
	unsigned short    cmd;
	short             param1;
	union {
		long          param2;
		Ptr           ptr; // pomme addition to pass 64-bit clean pointers
	};
};

struct SCStatus {
	UnsignedFixed                   scStartTime;                // starting time for play from disk (based on audio selection record)
	UnsignedFixed                   scEndTime;                  // ending time for play from disk (based on audio selection record)
	UnsignedFixed                   scCurrentTime;              // current time for play from disk
	Boolean                         scChannelBusy;              // true if channel is processing commands
	Boolean                         scChannelDisposed;          // reserved
	Boolean                         scChannelPaused;            // true if channel is paused
	Boolean                         scUnused;                   // reserved
	unsigned long                   scChannelAttributes;        // attributes of this channel
	long                            scCPULoad;                  // cpu load for this channel ("obsolete")
};

typedef struct SndChannel* SndChannelPtr;

typedef void (*SndCallBackProcPtr)(SndChannelPtr chan, SndCommand* cmd);
// for pomme implementation purposes we don't care about the 68000/ppc specifics of universal procedure pointers
typedef SndCallBackProcPtr          SndCallbackUPP;

struct SndChannel {
	SndChannelPtr                   nextChan;
	Ptr                             firstMod;                   // reserved for the Sound Manager (Pomme: used as internal ptr)
	SndCallBackProcPtr              callBack;
	long long                       userInfo;                   // free for application's use (Pomme: made it 64 bit so app can store ptrs)
#if 0
	long                            wait;                       // The following is for internal Sound Manager use only.
	SndCommand                      cmdInProgress;
	short                           flags;
	short                           qLength;
	short                           qHead;
	short                           qTail;
	SndCommand                      queue[128];
#endif
};

struct ModRef {
	unsigned short                  modNumber;
	long                            modInit;
};

struct SndListResource {
	short                           format;
	short                           numModifiers;
	// flexible array hack
	ModRef                          modifierPart[1];
	short                           numCommands;
	// flexible array hack
	SndCommand                      commandPart[1];
	UInt8                           dataPart[1];
};

typedef SCStatus* SCStatusPtr;
typedef SndListResource* SndListPtr;
typedef SndListPtr* SndListHandle;
typedef SndListHandle SndListHndl;
typedef void(*FilePlayCompletionProcPtr)(SndChannelPtr chan);
typedef FilePlayCompletionProcPtr FilePlayCompletionUPP;
#define NewFilePlayCompletionProc(userRoutine)                  (userRoutine)

//-----------------------------------------------------------------------------
// Keyboard input types

typedef UInt32 KeyMap[4];

//-----------------------------------------------------------------------------
// 'vers' resource

#if TARGET_RT_BIG_ENDIAN
//BCD encoded, e.g. "4.2.1a3" is 0x04214003
struct NumVersion {
	UInt8               majorRev;               // 1st part of version number in BCD
	UInt8               minorAndBugRev;         // 2nd & 3rd part of version number share a byte
	UInt8               stage;                  // stage code: dev, alpha, beta, final
	UInt8               nonRelRev;              // revision level of non-released version
};
#else
struct NumVersion {
	UInt8               nonRelRev;              // revision level of non-released version
	UInt8               stage;                  // stage code: dev, alpha, beta, final
	UInt8               minorAndBugRev;         // 2nd & 3rd part of version number share a byte
	UInt8               majorRev;               // 1st part of version number in BCD
};
#endif
