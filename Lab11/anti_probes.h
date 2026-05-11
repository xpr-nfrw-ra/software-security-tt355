// anti_probes.h — minimal anti-debug / anti-VM probes.
// Each probe returns 0 (clean) or 1 (detected).
// probe_mask() combines them into a single value that is 0 only when ALL are clean.

#pragma once
#include <cstdint>

#ifdef _WIN32
  #include <windows.h>
  #include <intrin.h>
#else
  #include <cpuid.h>
  #include <ctime>
#endif

static inline uint32_t probe_debugger() {
#ifdef _WIN32
    return IsDebuggerPresent() ? 1u : 0u;
#else
    return 0u;
#endif
}

// CPUID leaf 1, ECX bit 31 is the "hypervisor present" bit.
static inline uint32_t probe_hypervisor() {
    uint32_t eax, ebx, ecx, edx;
#ifdef _WIN32
    int regs[4];
    __cpuid(regs, 1);
    eax = regs[0]; ebx = regs[1]; ecx = regs[2]; edx = regs[3];
#else
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
#endif
    return (ecx >> 31) & 1u;
}

// rdtsc timing: a single instruction pair should take very few cycles.
// If a debugger single-steps or instruments, the delta balloons.
static inline uint32_t probe_timing() {
#if defined(_WIN32) || defined(__x86_64__) || defined(__i386__)
    uint64_t t1 = __rdtsc();
    uint64_t t2 = __rdtsc();
    return (t2 - t1 > 100000ull) ? 1u : 0u;
#else
    return 0u;
#endif
}

// Combined mask: 0 iff all clean. Bits spread so any single detection still corrupts state.
// The hypervisor probe is opt-in: many modern Windows hosts run with Hyper-V / VBS enabled,
// which sets the hypervisor-present bit on bare metal. Build with -DENABLE_HYPERVISOR_PROBE=1
// to include it.
static inline uint32_t probe_mask() {
    uint32_t m = 0;
    m |= probe_debugger() ? 0xA3u : 0u;
    m |= probe_timing()   ? 0x71u : 0u;
#if defined(ENABLE_HYPERVISOR_PROBE) && ENABLE_HYPERVISOR_PROBE
    m |= probe_hypervisor() ? 0x5Cu : 0u;
#endif
    return m;
}
