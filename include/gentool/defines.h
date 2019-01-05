#pragma once

// containts global macro defines


#if defined(_WIN32) && defined(USE_LIB_TARGET)
#  ifdef GENTOOL_EXPORT_SYMBOLS
#    define API_EXPORT __declspec(dllexport)
#  else
#    define API_EXPORT __declspec(dllimport)
#  endif
#else
#  define API_EXPORT
#endif