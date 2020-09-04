#pragma once

#include "Files/Volume.h"

namespace Pomme::Files
{
	/**
	 * Volume implementation that lets the Mac app access files
	 * on the host system's filesystem.
	 */
	class HostVolume : public Volume
	{
		std::vector<std::filesystem::path> directories;

		std::filesystem::path ToPath(long parID, const std::string& name);
		std::filesystem::path ToPath(const FSSpec& spec);
		FSSpec ToFSSpec(const std::filesystem::path& fullPath);
		
	public:
		explicit HostVolume(short vRefNum);

		virtual ~HostVolume() = default;

		//-----------------------------------------------------------------------------
		// Utilities

		bool IsRegularFile(const FSSpec*) override;

		bool IsDirectory(const FSSpec*) override;

		bool IsDirectoryIDLegal(long dirID) const override;

		long GetDirectoryID(const std::filesystem::path& dirPath);

		//-----------------------------------------------------------------------------
		// Toolbox API Implementation

		OSErr FSMakeFSSpec(long dirID, const std::string& fileName, FSSpec* spec) override;

		OSErr OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<ForkHandle>& stream) override;

		OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) override;

		OSErr FSpDelete(const FSSpec* spec) override;

		OSErr DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID) override;
	};
}
