#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <string>
#include "CompilerSupport/filesystem.h"
#include "pomme/Utilities/StringUtils.h"

fs::path DoOpenDialog(const char* expectedArchiveName)
{
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg setLevel:CGShieldingWindowLevel()];

	[openDlg setCanChooseFiles:TRUE];
	[openDlg setTitle:@"Locate Nanosaur Game Data"];
	[openDlg setMessage:[NSString stringWithFormat:@"Please locate either “%s”,\nor the “Nanosaur™” Classic app from the game‘s disk image.", expectedArchiveName]];
	[openDlg setCanChooseDirectories:FALSE];
	[openDlg setAllowsMultipleSelection:FALSE];
	[openDlg setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:expectedArchiveName ] ] ];

	if ([openDlg runModal] == NSModalResponseOK) {
		NSString* selectedFileName = [[openDlg URL] path];
		return u8string([selectedFileName UTF8String]);
	}

	return "";
}
