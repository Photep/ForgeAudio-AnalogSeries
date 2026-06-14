// tests/main.cpp — the ONLY translation unit that defines the doctest implementation.
// Defining DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN here provides main() and the doctest runtime.
// No other test TU may define this macro (doing so produces duplicate-symbol link errors).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
