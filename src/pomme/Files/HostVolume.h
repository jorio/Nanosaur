#pragma once

#include "Files/Volume.h"
#include "CompilerSupport/filesystem.h"

namespace Pomme::Files
{
	/**
	 * Volume implementation that lets the Mac app access files
	 * on the host system's filesystem.
	 */
	class HostVolume : public Volume
	{
		std::vector<fs::path> directories;

		fs::path ToPath(long parID, const std::string& name);

	public:
		explicit HostVolume(short vRefNum);

		virtual ~HostVolume() = default;

		//-----------------------------------------------------------------------------
		// Utilities

		long GetDirectoryID(const fs::path& dirPath);

		FSSpec ToFSSpec(const fs::path& fullPath);

		//-----------------------------------------------------------------------------
		// Toolbox API Implementation

		OSErr FSMakeFSSpec(long dirID, const std::string& fileName, FSSpec* spec) override;

		OSErr OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<ForkHandle>& stream) override;

		OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) override;

		OSErr FSpDelete(const FSSpec* spec) override;

		OSErr DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID) override;
	};
}
