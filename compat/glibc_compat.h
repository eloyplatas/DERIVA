// Force-included in every C++ file: bind hypot to its original glibc version (fmod and
// arc4random are handled with --wrap in glibc_compat.c)
// so the plugin also loads on distributions with glibc < 2.38 (Ubuntu 22.04, Debian 12...).
#pragma once
#if defined(__linux__) && defined(__x86_64__)
__asm__ (".symver hypot,hypot@GLIBC_2.2.5");
__asm__ (".symver hypotf,hypotf@GLIBC_2.2.5");
#endif
