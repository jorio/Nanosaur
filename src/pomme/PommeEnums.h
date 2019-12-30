#pragma once

//-----------------------------------------------------------------------------
// Filesystem permission char

enum EFSPermissions {
    fsCurPerm = 0,
    fsRdPerm = 1,
    fsWrPerm = 2,
    fsRdWrPerm = 3,
    fsRdWrShPerm = 4,
};

//-----------------------------------------------------------------------------
// Error codes (OSErr)

enum EErrors {
    noErr           = 0,

    unimpErr        = -4, // unimplemented core routine

    controlErr      = -17, // I/O System Errors
    statusErr       = -18, // I/O System Errors
    readErr         = -19, // I/O System Errors
    writErr         = -20, // I/O System Errors
    badUnitErr      = -21, // I/O System Errors
    unitEmptyErr    = -22, // I/O System Errors
    openErr         = -23, // I/O System Errors
    closErr         = -24, // I/O System Errors

    abortErr        = -27, // IO call aborted by KillIO
    notOpenErr      = -28, // Couldn't rd/wr/ctl/sts cause driver not opened
    dirFulErr       = -33, // File directory full
    dskFulErr       = -34, // Disk or volume full
    nsvErr          = -35, // Volume doesn't exist
    ioErr           = -36, // I/O error
    bdNamErr        = -37, // Bad file or volume name
    fnOpnErr        = -38, // File not open
    eofErr          = -39, // End-of-file reached
    posErr          = -40, // Attempt to position mark before start of file
    mFulErr         = -41, // Memory full (open) or file won't fit (load)
    tmfoErr         = -42, // Too many files open
    fnfErr          = -43, // File not found (FSSpec is still valid)
    wPrErr          = -44, // Volume is hardware-locked
    fLckdErr        = -45, // File is locked
    vLckdErr        = -46, // Volume is software-locked
    fBsyErr         = -47, // File is busy
    dupFNErr        = -48, // Duplicate filename
    opWrErr         = -49, // File already open for writing
    paramErr        = -50, // Invalid value passed in parameter
    rfNumErr        = -51, // Invalid reference number
    gfpErr          = -52, // Error during a GetFPos family function
    volOffLinErr    = -53, // Volume is offline
    permErr         = -54, // Permission error
    volOnLinErr     = -55, // Volume already online
    nsDrvErr        = -56, // No such Drive
    noMacDskErr     = -57, // Foreign disk
    extFSErr        = -58, // Volume belongs to external file system
    fsRnErr         = -59, // Couldn't rename
    badMDBErr       = -60, // Bad master directory block
    wrPermErr       = -61, // Read/write permission doesn't allow writing
    
    memROZWarn      = -99,  // soft error in ROZ
    memROZError     = -99,  // hard error in ROZ
    memROZErr       = -99,  // hard error in ROZ
    memFullErr      = -108, // Not enough room in heap zone
    nilHandleErr    = -109, // Master Pointer was NIL in HandleZone or other
    memWZErr        = -111, // WhichZone failed (applied to free block)
    memPurErr       = -112, // trying to purge a locked or non-purgeable block
    memAdrErr       = -110, // address was odd; or out of range
    memAZErr        = -113, // Address in zone check failed
    memPCErr        = -114, // Pointer Check failed
    memBCErr        = -115, // Block Check failed
    memSCErr        = -116, // Size Check failed
    memLockedErr    = -117, // trying to move a locked block (MoveHHi)

    dirNFErr        = -120, // Directory not found
    tmwdoErr        = -121, // Too many working directories open
    badMovErr       = -122, // Couldn't move
    wrgVolTypErr    = -123, // Unrecognized volume (not HFS)
    volGoneErr      = -124, // Server volume disconnected
    fsDSIntErr      = -127, // Non-hardware internal file system error

    userCanceledErr = -128,

    badExtResource  = -185, // extended resource has a bad format
    CantDecompress  = -186, // resource bent ("the bends") - can't decompress a compressed resource
    resourceInMemory= -188, // Resource already in memory
    writingPastEnd  = -189, // Writing past end of file
    inputOutOfBounds= -190, // Offset of Count out of bounds
    resNotFound     = -192, // Resource not found
    resFNotFound    = -193, // Resource file not found
    addResFailed    = -194, // AddResource failed
    addRefFailed    = -195, // AddReference failed
    rmvResFailed    = -196, // RmveResource failed
    rmvRefFailed    = -197, // RmveReference failed
    resAttrErr      = -198, // attribute inconsistent with operation
    mapReadErr      = -199, // map inconsistent with operation


};


//-----------------------------------------------------------------------------
// Script Manager enums

enum EScriptManager {
    smSystemScript = -1,
    smCurrentScript = -2,
    smAllScripts = -3
};

//-----------------------------------------------------------------------------

enum EEvents {
    everyEvent = ~0
};

//-----------------------------------------------------------------------------
// Memory enums

enum EMemory {
    maxSize = 0x7FFFFFF0 // the largest block possible
};

//-----------------------------------------------------------------------------
// Resource types

enum EResTypes {
    rAliasType = 'alis',
};

//-----------------------------------------------------------------------------
// Sound Manager enums

enum ESndPitch {
    kMiddleC = 60L,
};

enum ESndSynth {
    squareWaveSynth = 1,
    waveTableSynth = 3,
    sampledSynth = 5
};

enum ESndInit {
    initChanLeft = 0x0002,    // left stereo channel
    initChanRight = 0x0003,    // right stereo channel
    initMono = 0x0080,    // monophonic channel
    initStereo = 0x00C0,    // stereo channel
    initMACE3 = 0x0300,    // 3:1 compression
    initMACE6 = 0x0400,    // 6:1 compression
    initNoInterp = 0x0004,    // no linear interpolation
    initNoDrop = 0x0008,    // no drop-sample conversion
};

// Sound commands
enum ESndCmds {
    nullCmd = 0,
    initCmd = 1,
    freeCmd = 2,
    quietCmd = 3,
    flushCmd = 4,
    reInitCmd = 5,
    waitCmd = 10,
    pauseCmd = 11,
    resumeCmd = 12,
    callBackCmd = 13,
    syncCmd = 14,
    availableCmd = 24,
    versionCmd = 25,
    totalLoadCmd = 26,
    loadCmd = 27,
    freqDurationCmd = 40,
    restCmd = 41,
    freqCmd = 42,
    ampCmd = 43,
    timbreCmd = 44,
    getAmpCmd = 45,
    volumeCmd = 46,       // sound manager 3.0 or later only
    getVolumeCmd = 47,       // sound manager 3.0 or later only
    clockComponentCmd = 50,       // sound manager 3.2.1 or later only
    getClockComponentCmd = 51,       // sound manager 3.2.1 or later only
    scheduledSoundCmd = 52,       // sound manager 3.3 or later only
    linkSoundComponentsCmd = 53,       // sound manager 3.3 or later only
    waveTableCmd = 60,
    phaseCmd = 61,
    soundCmd = 80,
    bufferCmd = 81,
    rateCmd = 82,
    continueCmd = 83,
    doubleBufferCmd = 84,
    getRateCmd = 85,
    rateMultiplierCmd = 86,
    getRateMultiplierCmd = 87,
    sizeCmd = 90,       // obsolete command
    convertCmd = 91        // obsolete MACE command
};
