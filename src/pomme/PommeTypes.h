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

//-----------------------------------------------------------------------------
// (Pascal) String types

struct Str32 {
    unsigned char length;
    char buf[32];
};

// WARNING: that's not an actual pascal string!!
typedef const char*                     Str255;
// WARNING: that's not an actual pascal string!!
typedef const char*                     ConstStr255Param;
typedef char*                           StringPtr;

//-----------------------------------------------------------------------------
// Point & Rect types

struct Point { SInt16 v, h; };
struct Rect { SInt16 top, left, bottom, right; };
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
    char name[64];
};

typedef Handle AliasHandle;

//-----------------------------------------------------------------------------
// QuickDraw types

struct RGBColor {
    UInt16 red;
    UInt16 green;
    UInt16 blue;
};

struct Picture {
    SInt16 picSize;
    Rect picFrame;
};

typedef Picture* PicPtr;
typedef PicPtr* PicHandle;

struct GWorld { int _____dummy_____; };
struct Window { int _____dummy_____; };
struct GDHandle { int _____dummy_____; };
struct PixMap { int _____dummy_____; }; // if needed, look at quickdraw.h
typedef GWorld* GWorldPtr;
typedef Window* WindowPtr;
typedef PixMap* PixMapPtr;
typedef PixMapPtr* PixMapHandle;

//-----------------------------------------------------------------------------
// Sound Manager types

struct SndCommand {
    unsigned short    cmd;
    short             param1;
    long              param2;
};

struct SCStatus {
    UnsignedFixed                   scStartTime;
    UnsignedFixed                   scEndTime;
    UnsignedFixed                   scCurrentTime;
    Boolean                         scChannelBusy;
    Boolean                         scChannelDisposed;
    Boolean                         scChannelPaused;
    Boolean                         scUnused;
    unsigned long                   scChannelAttributes;
    long                            scCPULoad;
};

typedef struct SndChannel* SndChannelPtr;

struct SndChannel {
    SndChannelPtr                   nextChan;
    Ptr                             firstMod;                   // reserved for the Sound Manager
    /*SndCallBackUPP*/void* callBack;
    long                            userInfo;
    long                            wait;                       // The following is for internal Sound Manager use only.
    SndCommand                      cmdInProgress;
    short                           flags;
    short                           qLength;
    short                           qHead;
    short                           qTail;
    SndCommand                      queue[128];
};

struct ModRef {
    unsigned short                  modNumber;
    long                            modInit;
};

struct SndListResource {
    short                           format;
    short                           numModifiers;
    ModRef                          modifierPart[1];
    short                           numCommands;
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
