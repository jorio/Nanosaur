#include "Pomme.h"
#include "PommeFiles.h"
#include "FindGameData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libproc.h>
#include <unistd.h>

bool FindEmbeddedGameData(FSSpec* dataSpec)
{
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	fs::path myPath = pathbuf;
	myPath = myPath.parent_path().parent_path() / "Resources" / APP_FILE_INSIDE_ARCHIVE;
	myPath = myPath.lexically_normal();

	auto applicationSpec = Pomme::Files::HostPathToFSSpec(myPath);
	dataSpec->vRefNum = applicationSpec.vRefNum;
	dataSpec->parID = applicationSpec.parID;
	
	// Use application resource file
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);
	UseResFile(resFileRefNum);
    return true;
}
