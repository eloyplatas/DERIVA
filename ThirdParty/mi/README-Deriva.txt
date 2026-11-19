Beads DSP (Mutable Instruments, Emilie Gillet) and stmlib, MIT License.
Source: https://github.com/pichenettes/eurorack (beads/dsp, beads/resources.*)
        https://github.com/pichenettes/stmlib (dsp, utils)
Local changes, marked [Deriva]:
  - ENABLE_HARDWARE_24K disabled (always 48 kHz inside the plugin).
  - stmlib::Random state is thread_local.
  - Variable-length arrays replaced by fixed arrays of kMaxBlockSize (64).
Compiled with -DTEST (desktop build of stmlib helpers).
  - Empty lookup tables in resources.cc get a nullptr entry (standard C++).
  - 12-bit delay line items use explicit 3-byte packing (Packed12x2) instead of GCC
    packed bitfields, so the memory layout is identical with MSVC, Clang and GCC.
  - stmlib/utils/dsp.h: always_inline attribute wrapped in a portable macro.
  - Built with _USE_MATH_DEFINES (M_PI on MSVC).
