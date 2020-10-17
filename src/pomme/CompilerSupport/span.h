#pragma once

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include) && __has_include(<span>)
    #include <span>
#else
    #include "CompilerSupport/span_implementation.hpp"
#endif

