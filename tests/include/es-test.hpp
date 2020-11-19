// Enable the BENCHMARK macro sets, unless told not to
#ifndef NO_BENCHMARKING
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#endif

#ifdef _WIN32
// We require SEH for Windows builds, so we test with SEH support too.
#define CATCH_CONFIG_WINDOWS_SEH
// Check for memory leaks from test code.
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif

#include "catch.hpp"
