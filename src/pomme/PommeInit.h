#pragma once

namespace Pomme
{
	// Throw this exception to interrupt the game's main loop
	class QuitRequest : public std::exception
	{
		public: virtual const char* what() const noexcept;
	};

	void Init(const char* applName);
	void Shutdown();
}

