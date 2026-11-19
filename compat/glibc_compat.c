/* glibc >= 2.38 headers redirect strtol & co. to C23 variants (__isoc23_*).
   This file is compiled as plain C17 (no _GNU_SOURCE), so the calls below bind to the
   classic symbols, and the C23 names are provided locally inside the plugin. */
#if defined(__linux__) && defined(__x86_64__)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#define HIDDEN __attribute__((visibility("hidden"), used))
HIDDEN long __isoc23_strtol (const char* s, char** e, int b) { return strtol (s, e, b); }
HIDDEN long long __isoc23_strtoll (const char* s, char** e, int b) { return strtoll (s, e, b); }
HIDDEN unsigned long __isoc23_strtoul (const char* s, char** e, int b) { return strtoul (s, e, b); }
HIDDEN unsigned long long __isoc23_strtoull (const char* s, char** e, int b) { return strtoull (s, e, b); }
HIDDEN long __isoc23_wcstol (const wchar_t* s, wchar_t** e, int b) { return wcstol (s, e, b); }
HIDDEN long long __isoc23_wcstoll (const wchar_t* s, wchar_t** e, int b) { return wcstoll (s, e, b); }
HIDDEN unsigned long __isoc23_wcstoul (const wchar_t* s, wchar_t** e, int b) { return wcstoul (s, e, b); }
HIDDEN unsigned long long __isoc23_wcstoull (const wchar_t* s, wchar_t** e, int b) { return wcstoull (s, e, b); }
HIDDEN int __isoc23_sscanf (const char* s, const char* f, ...) { va_list ap; va_start (ap, f); int r = vsscanf (s, f, ap); va_end (ap); return r; }
HIDDEN int __isoc23_vsscanf (const char* s, const char* f, va_list ap) { return vsscanf (s, f, ap); }
HIDDEN int __isoc23_fscanf (FILE* fp, const char* f, ...) { va_list ap; va_start (ap, f); int r = vfscanf (fp, f, ap); va_end (ap); return r; }
HIDDEN int __isoc23_vfscanf (FILE* fp, const char* f, va_list ap) { return vfscanf (fp, f, ap); }
HIDDEN int __isoc23_swscanf (const wchar_t* s, const wchar_t* f, ...) { va_list ap; va_start (ap, f); int r = vswscanf (s, f, ap); va_end (ap); return r; }
HIDDEN int __isoc23_vswscanf (const wchar_t* s, const wchar_t* f, va_list ap) { return vswscanf (s, f, ap); }

/* fmod became fmod@GLIBC_2.38 and arc4random appeared in 2.36; both are routed here
   with -Wl,--wrap (also for references coming from the static libstdc++). */
#include <sys/random.h>
#include <time.h>
HIDDEN double __wrap_fmod (double x, double y)
{
    long double r;
    __asm__ ("1: fprem\n\t"
             "fnstsw %%ax\n\t"
             "testw $0x400, %%ax\n\t"
             "jnz 1b"
             : "=t" (r) : "0" ((long double) x), "u" ((long double) y) : "ax", "cc");
    return (double) r;
}
HIDDEN float __wrap_fmodf (float x, float y) { return (float) __wrap_fmod ((double) x, (double) y); }
HIDDEN unsigned int __wrap_arc4random (void)
{
    unsigned int v = 0;
    if (getrandom (&v, sizeof v, 0) != (ssize_t) sizeof v)
        v = (unsigned int) time (NULL) * 2654435761u;
    return v;
}
#endif
