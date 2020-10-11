#pragma once

#include <stdbool.h>
#include <stdint.h>

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
typedef struct { UInt32 hi, lo; } UnsignedWide;
#else
typedef struct { UInt32 lo, hi; } UnsignedWide;
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

typedef char                            Str32[33];
typedef char                            Str255[256];
typedef char*							StringPtr;
typedef const char*                     ConstStr255Param;

//-----------------------------------------------------------------------------
// Point & Rect types

typedef struct Point { SInt16 v, h; } Point;

typedef struct Rect
{
	union
	{
		struct { SInt16 top, left, bottom, right; };
		struct { Point topLeft, bottomRight; };
	};
} Rect;

typedef Point* PointPtr;
typedef Rect* RectPtr;

typedef struct FixedPoint { Fixed x, y; } FixedPoint;
typedef struct FixedRect { Fixed left, top, right, bottom; } FixedRect;

//-----------------------------------------------------------------------------
// FSSpec types

typedef struct FSSpec
{
	// Volume reference number of the volume containing the specified file or directory.
	short vRefNum;

	// Parent directory ID of the specified file or directory (the directory ID of the directory containing the given file or directory).
	long parID;

	// The name of the specified file or directory. In Carbon, this name must be a leaf name; the name cannot contain a semicolon.
	// WARNING: this is a C string, NOT a pascal string!
	// Mac application code using "name" (the pascal string) must be adjusted.
	Str255 cName;
} FSSpec;

typedef Handle AliasHandle;

//-----------------------------------------------------------------------------
// QuickDraw types

typedef SInt16							QDErr;

typedef struct RGBColor
{
	UInt16 red;
	UInt16 green;
	UInt16 blue;
} RGBColor;

typedef struct Picture
{
	// Version 1 size.
	// Not used for version 2 PICTs, as the size may easily exceed 16 bits.
	SInt16 picSize;

	Rect picFrame;

	// Raw raster image decoded from PICT opcodes. Shouldn't be accessed
	// directly by the Mac application as it is stored in a format internal
	// to the Pomme implementation for rendering (typically ARGB32).
	Ptr __pomme_pixelsARGB32;
} Picture;


typedef Picture* PicPtr;
typedef PicPtr* PicHandle;
typedef Handle GDHandle; // GDevice handle. Game code doesn't care about GDevice internals, so we just alias a generic Handle
// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/QuickDraw_Ref/qdref_main/data_type_41.html
typedef struct PixMap
{
	Rect bounds;
	short pixelSize;
	Ptr _impl;
} PixMap;
typedef PixMap*							PixMapPtr;
typedef PixMapPtr*						PixMapHandle;
typedef struct GrafPort
{
	Rect portRect;
	void* _impl;
} GrafPort;
typedef GrafPort*						GrafPtr;
typedef GrafPtr                         WindowPtr;
typedef GrafPort                        CGrafPort;
typedef GrafPtr							CGrafPtr;
typedef CGrafPtr						GWorldPtr;

//-----------------------------------------------------------------------------
// Sound Manager types

typedef struct SndCommand
{
	unsigned short    cmd;
	short             param1;
	union {
		long          param2;
		Ptr           ptr; // pomme addition to pass 64-bit clean pointers
	};
} SndCommand;

typedef struct SCStatus
{
	UnsignedFixed                   scStartTime;                // starting time for play from disk (based on audio selection record)
	UnsignedFixed                   scEndTime;                  // ending time for play from disk (based on audio selection record)
	UnsignedFixed                   scCurrentTime;              // current time for play from disk
	Boolean                         scChannelBusy;              // true if channel is processing commands
	Boolean                         scChannelDisposed;          // reserved
	Boolean                         scChannelPaused;            // true if channel is paused
	Boolean                         scUnused;                   // reserved
	unsigned long                   scChannelAttributes;        // attributes of this channel
	long                            scCPULoad;                  // cpu load for this channel ("obsolete")
} SCStatus;

typedef struct SndChannel* SndChannelPtr;

typedef void (*SndCallBackProcPtr)(SndChannelPtr chan, SndCommand* cmd);
// for pomme implementation purposes we don't care about the 68000/ppc specifics of universal procedure pointers
typedef SndCallBackProcPtr          SndCallbackUPP;

typedef struct SndChannel
{
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
} SndChannel;

typedef struct ModRef
{
	unsigned short                  modNumber;
	long                            modInit;
} ModRef;

typedef struct SndListResource
{
	short                           format;
	short                           numModifiers;
	// flexible array hack
	ModRef                          modifierPart[1];
	short                           numCommands;
	// flexible array hack
	SndCommand                      commandPart[1];
	UInt8                           dataPart[1];
} SndListResource;

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
typedef struct NumVersion
{
	UInt8               majorRev;               // 1st part of version number in BCD
	UInt8               minorAndBugRev;         // 2nd & 3rd part of version number share a byte
	UInt8               stage;                  // stage code: dev, alpha, beta, final
	UInt8               nonRelRev;              // revision level of non-released version
} NumVersion;
#else
typedef struct NumVersion
{
	UInt8               nonRelRev;              // revision level of non-released version
	UInt8               stage;                  // stage code: dev, alpha, beta, final
	UInt8               minorAndBugRev;         // 2nd & 3rd part of version number share a byte
	UInt8               majorRev;               // 1st part of version number in BCD
} NumVersion;
#endif
