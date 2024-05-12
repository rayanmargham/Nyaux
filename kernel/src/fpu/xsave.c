#include "xsave.h"
#include "main.h"
#include "vmm.h"
#define CPUID_FEAT_ECX_AV (1 << 28)
uint64_t xsave_size = 0;
struct Cpuid {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

inline struct Cpuid cpuid(uint32_t eax, uint32_t ecx) {
	struct Cpuid id = {};
	asm("cpuid" : "=a"(id.eax), "=b"(id.ebx), "=c"(id.ecx), "=d"(id.edx) : "a"(eax), "c"(ecx));
	return id;
}
inline void xsetbv(uint32_t index, uint64_t value) {
    register uint32_t ecx asm("ecx") = index;
    register uint32_t eax asm("eax") = value & 0xFFFFFFFF;
    register uint32_t edx asm("edx") = value >> 32 & 0xFFFFFFFF;
    asm volatile("xsetbv" : : "r"(ecx), "r"(eax), "r"(edx) : "memory");
}

void setup_xsave()
{
    update_cr4((uint64_t)read_cr4() | (1 << 18)); // enable xsave
    xsave_size = cpuid(0xd, 0).ecx;
    bool cpu_has_avx = (cpuid(1, 0).edx & CPUID_FEAT_ECX_AV) != 0;
    if (cpu_has_avx)
    {
        kprintf("CPU HAS AVX!\n");
        xsetbv(0, 7); // ALSO ENABLE AVX
    }
    else {
        kprintf("CPU NO AVX :pensive:\n");
        xsetbv(0, 2); // DONT ENABLE AVX LOL
    }
}