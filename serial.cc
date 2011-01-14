#include "serial.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include "serial_win32.cc"
#else
#include "serial_posix.cc"
#endif

