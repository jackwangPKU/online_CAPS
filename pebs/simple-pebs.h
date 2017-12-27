#define SIMPLE_PEBS_BASE 	0x7000
#define SIMPLE_PEBS_SET_CPU    	(SIMPLE_PEBS_BASE + 1)
#define SIMPLE_PEBS_GET_SIZE   	(SIMPLE_PEBS_BASE + 2)
#define SIMPLE_PEBS_GET_OFFSET 	(SIMPLE_PEBS_BASE + 3)
#define SIMPLE_PEBS_START	(SIMPLE_PEBS_BASE + 4)
#define SIMPLE_PEBS_STOP	(SIMPLE_PEBS_BASE + 5)
#define SIMPLE_PEBS_RESET	(SIMPLE_PEBS_BASE + 6)
#define GET_ACCESS		0x8000
#define GET_CYCLES		0x8001
#define GET_INSTR		0x8002
#define GET_OCCUPANCY		0x8003
#define GET_OCCUPANCY_INIT	0x8004
#define GET_OCCUPANCY_RESET	0x8005

struct cpuid_out {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
};

void
lcpuid(const unsigned leaf,
       const unsigned subleaf,
       struct cpuid_out *out);
