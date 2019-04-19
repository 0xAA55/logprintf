#pragma once

typedef void(*log_on_output_fp)(char *str, void *userdata);

void log_push_cb(log_on_output_fp on_output, void *userdata);
void log_pop_cb();

void logprintf_type(const char *type, const char *fmt, ...);
void logprintf(const char *fmt, ...);
void logprintf_w(const char *fmt, ...);
void logprintf_e(const char *fmt, ...);

#if defined(_MSC_VER)
#  include<logprintf/msc_inttypes.h>
#  if _MSC_VER >= 1800
#    define PRIsize_t "zu"
#  else
#    define PRIsize_t "Iu"
#  endif
#else
#  include<inttypes.h>
#  define __USE_MINGW_ANSI_STDIO 1
#  define PRIsize_t "zu"
#endif

#ifdef _WIN32
//=============================================================================
// Func: str_GetLastError
// Desc: A debug function that formats GetLastError() value into a fixed length
//   static string and it is thread-local. Can be used for printf() in
//   the same way as strerrno(errno). The returned pointer doesn't need to be
//   freed.
//-----------------------------------------------------------------------------
char *str_GetLastError();
#endif

#define onetimelog(fmt,...) { static int fired = 0; if(!fired) {logprintf(fmt, __VA_ARGS__); fired = 1;}}
