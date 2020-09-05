#pragma once

#include "PommeInternal.h"

namespace Pomme::Files
{
	enum ForkType {
		DataFork,
		ResourceFork
	};

	struct ForkHandle
	{
	public:
		ForkType forkType;
		char permission;

	protected:
		ForkHandle(ForkType _forkType, char _permission)
			: forkType(_forkType)
			, permission(_permission)
		{}
		
	public:
		virtual std::iostream& GetStream() = 0;
		
		virtual ~ForkHandle() = default;
	};

	/**
	 * Base class for volumes through which the Mac app is given access to files.
	 */
	class Volume
	{
	protected:
		short volumeID;

	public:
		Volume(short vid)
			: volumeID(vid)
		{}

		virtual ~Volume() = default;
		
		//-----------------------------------------------------------------------------
		// Toolbox API Implementation

		virtual OSErr FSMakeFSSpec(long dirID, const std::string& suffix, FSSpec* spec) = 0;
		
		virtual OSErr OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<ForkHandle>& handle) = 0;

		virtual OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) = 0;

		virtual OSErr FSpDelete(const FSSpec* spec) = 0;

		virtual OSErr DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID) = 0;
	};
}