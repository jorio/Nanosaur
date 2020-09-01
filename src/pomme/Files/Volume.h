#pragma once

#include "PommeInternal.h"

namespace Pomme::Files
{
	enum ForkType {
		DataFork,
		ResourceFork
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
		// Utilities

		virtual bool IsRegularFile(const FSSpec*) = 0;

		virtual bool IsDirectory(const FSSpec*) = 0;

		virtual bool IsDirectoryIDLegal(long dirID) const = 0;
		
		//-----------------------------------------------------------------------------
		// Toolbox API Implementation

		virtual OSErr FSMakeFSSpec(long dirID, const std::string& suffix, FSSpec* spec) = 0;
		
		virtual OSErr OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<std::iostream>& stream) = 0;

		virtual OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) = 0;

		virtual OSErr FSpDelete(const FSSpec* spec) = 0;

		virtual OSErr DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID) = 0;
	};
}