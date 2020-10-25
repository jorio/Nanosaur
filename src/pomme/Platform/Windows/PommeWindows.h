#pragma once

#include <filesystem>

namespace Pomme::Platform::Windows
{
	std::filesystem::path GetPreferencesFolder();

	void SysBeep();
}
