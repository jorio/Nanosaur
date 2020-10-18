#pragma once

#if __cplusplus

#include <string>
#include <sstream>

#define POMME_DEBUG_MEMORY		false
#define POMME_DEBUG_SOUND		false
#define POMME_DEBUG_PICT		false
#define POMME_DEBUG_FILES		false
#define POMME_DEBUG_RESOURCES	false
#define POMME_DEBUG_INPUT		false

#define POMME_GENLOG(define, prefix) if (!define) {} else std::cout << "[" << prefix << "] " << __func__ << ":\t"
#define POMME_GENLOG_NOPREFIX(define) if (!define) {} else std::cout

namespace Pomme
{
	std::string FourCCString(uint32_t t, char filler = '?');
}

//-----------------------------------------------------------------------------
// My todos

void ImplementMe(const char* fn, std::string msg, int severity);

#define TODOCUSTOM(message, severity) { \
	std::stringstream ss; \
	ss << message; \
	ImplementMe(__func__, ss.str(), severity); \
}

#define TODOMINOR()      TODOCUSTOM("", 0)
#define TODOMINOR2(x)    TODOCUSTOM(x, 0)
#define TODO()           TODOCUSTOM("", 1)
#define TODO2(x)         TODOCUSTOM(x, 1)
#define TODOFATAL()      TODOCUSTOM("", 2)
#define TODOFATAL2(x)    TODOCUSTOM(x, 2)

#define ONCE(x)			{ \
	static bool once = false; \
	if (!once) { \
		once = true; \
		{x} \
		printf("  \x1b[90m\\__ this todo won't be shown again\x1b[0m\n"); \
	} \
}


#endif
