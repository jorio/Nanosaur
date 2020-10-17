#pragma once

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include) && __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#else
    #include "CompilerSupport/filesystem_implementation.hpp"
    namespace fs = ghc::filesystem;
#endif
