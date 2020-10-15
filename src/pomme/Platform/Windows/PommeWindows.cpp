#include "Platform/Windows/PommeWindows.h"

#include <shlobj.h>

std::filesystem::path Pomme::Platform::Windows::GetPreferencesFolder()
{
	wchar_t* wpath = nullptr;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &wpath);
	auto path = std::filesystem::path(wpath);
	CoTaskMemFree(static_cast<void*>(wpath));
	return path;
}

void Pomme::Platform::Windows::SysBeep()
{
	MessageBeep(0);
}
