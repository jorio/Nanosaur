#pragma once

#include "Files/Volume.h"
#include <map>

namespace Pomme::Files
{
	/**
	 * Read-only volume implementation that lets the Mac app access files
	 * inside an archive (such as a StuffIt file).
	 */
	class ArchiveVolume : public Volume
	{
	public:
		struct CompressedForkMetadata
		{
			UInt32 uncompressedLength;
			UInt32 compressedLength;
			UInt32 compressionMethod;
			UInt32 offsetToCompressedData;
		};

		struct CompressedFileMetadata
		{
			std::string name;
			CompressedForkMetadata dataFork;
			CompressedForkMetadata rsrcFork;
			UInt32 type, creator;
		};

	private:
		std::unique_ptr<std::istream> backingStream;

		/// Full paths of all directories in the archive.
		/// Paths are uppercase to ease case-insensitive lookups.
		std::vector<std::string> directories;

		/// Metadata for all files in the archive.
		/// Keys are full uppercase paths to the file for case-insensitive lookups.
		std::map<std::string, CompressedFileMetadata> files;

		/// Convert an FSSpec to a Mac-style path (with colons as path separators)
		/// that can be used as a key to look up a file or directory in the archive
		/// using `files` or `directories`, respectively.
		std::string FSSpecToPath(const FSSpec* spec) const;

		/// Decompress a fork from the archive.
		std::vector<char> DecompressFork(CompressedForkMetadata& forkInfo);
		
		/// Read StuffIt directory or file entry.
		/// Recursive if the entry is a directory.
		UInt32 ReadEntry(
			std::istream& input,
			int globalOffset,
			const std::string& parentPath,
			bool collapseIfFolder);

		void ReadStuffIt5();
		
	public:
		explicit ArchiveVolume(short vRefNum, const std::string& pathToArchiveOnHost);

		virtual ~ArchiveVolume() = default;

		//-----------------------------------------------------------------------------
		// Utilities

		bool IsRegularFile(const FSSpec*) override;

		bool IsDirectory(const FSSpec*) override;

		bool IsDirectoryIDLegal(long dirID) const override;

		long GetDirectoryID(const std::string& dirPath);

		//-----------------------------------------------------------------------------
		// Toolbox API Implementation

		OSErr FSMakeFSSpec(long dirID, const std::string& fileName, FSSpec* spec) override;

		OSErr OpenFork(const FSSpec* spec, ForkType forkType, char permission, std::unique_ptr<std::iostream>& stream) override;

		OSErr FSpCreate(const FSSpec* spec, OSType creator, OSType fileType, ScriptCode scriptTag) override;

		OSErr FSpDelete(const FSSpec* spec) override;

		OSErr DirCreate(long parentDirID, const std::string& directoryName, long* createdDirID) override;
	};
}
