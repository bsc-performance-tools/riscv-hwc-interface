#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/syscall.h>

//#include "papi.h"
#include <papi.h>

#define MAX_HWC 5
#define FIXED_HWC 3
#define MAX_CONF_HWC MAX_HWC - FIXED_HWC

#if defined DEBUG
# define PRINT_DEBUG fprintf(stderr, "[%s]\n" , __func__);
#else
# define PRINT_DEBUG
#endif

int   fake_PAPI_count_eventsets = 0;
uint64_t **fake_PAPI_EventSets;

// p. 20 https://sifive.cdn.prismic.io/sifive%2F834354f0-08e6-423c-bf1f-0cb58ef14061_fu540-c000-v1.0.pdf
struct RISCV_counter_def
{
	unsigned int code;
	char  name[32];
	char  description[256];
};

enum riscv_hwc_map
{
	INST_RETIRED = 59,
	CYCLES = 50,
	EXC_TAKEN = 1,
	INT_LD,
	INT_ST,
	ATOMIC_MEMOP,
	SYS_INST,
	INT_ARITH_INST,
	COND_BR,
	JAL_INST,
	JALR_INST,
	INT_MUL_INST,
	INT_DIV_INST,
	LD_USE_INTERLOCK,
	LNG_LAT_INTERLOCK,
	CSR_RD_INTERLOCK,
	ITIM_BUSY,
	DTIM_BUSY,
	BR_DIR_MISPREDICT,
	BR_TGT_MISPREDICT,
	PIPELINE_FLUSH_CSR_WR,
	PIPELINE_FLUSH_OTHER,
	INT_MUL_INTERLOCK,
	INST_CACHE_MISS,
	MEM_MAP_IO_ADDR,
	NUM_RISCV_HWC
} RV_HWC_MAP;

static struct RISCV_counter_def riscv_hwc[] =
{
	// Instruction Commit Events
	{ ( 1 << 8 ),  "EXC_TAKEN",      "Exception taken" },                                      /* 0 */
	{ ( 1 << 9 ),  "INT_LD",         "Integer load instruction retired" },
	{ ( 1 << 10 ), "INT_ST",         "Integer store instruction retired" },
	{ ( 1 << 11 ), "ATOMIC_MEMOP",   "Atomic memory operation retired" },
	{ ( 1 << 12 ), "SYS_INST",       "System instruction retired" },
	{ ( 1 << 13 ), "INT_ARITH_INST", "Integer arithmetic instruction retired" },               /* 5 */
	{ ( 1 << 14 ), "COND_BR",        "Conditional branch retired" },
	{ ( 1 << 15 ), "JAL_INST",       "JAL instruction retired" },
	{ ( 1 << 16 ), "JALR_INST",      "JALR instruction retired" },
	{ ( 1 << 17 ), "INT_MUL_INST",   "Integer multiplication instruction retired" },
	{ ( 1 << 18 ), "INT_DIV_INST",   "Integer division instruction retired" },                 /* 10 */

	// Microarchitectural Events
	{ ((1 << 8)  | (1 & 0xFF)),  "LD_USE_INTERLOCK",      "Load-use interlock" },
	{ ((1 << 9)  | (1 & 0xFF)),  "LNG_LAT_INTERLOCK",     "Long-latency interlock" },
	{ ((1 << 10) | (1 & 0xFF)),  "CSR_RD_INTERLOCK",      "CSR read interlock" },
	{ ((1 << 11) | (1 & 0xFF)),  "ITIM_BUSY",             "Instruction cache/ITIM busy" },
	{ ((1 << 12) | (1 & 0xFF)),  "DTIM_BUSY",             "Data cache/DTIM busy" },            /* 15 */
	{ ((1 << 13) | (1 & 0xFF)),  "BR_DIR_MISPREDICT",     "Branch direction misprediction" },
	{ ((1 << 14) | (1 & 0xFF)),  "BR_TGT_MISPREDICT",     "Branch/jump target misprediction" },
	{ ((1 << 15) | (1 & 0xFF)),  "PIPELINE_FLUSH_CSR_WR", "Pipeline flush from CSR write" },
	{ ((1 << 16) | (1 & 0xFF)),  "PIPELINE_FLUSH_OTHER",  "Pipeline flush from other event" },
	{ ((1 << 17) | (1 & 0xFF)),  "INT_MUL_INTERLOCK",     "Integer multiplication interlock" }, /* 20 */

	// Memory System Events
	{ ((1 << 8)  | (2 & 0xFF)),  "INST_CACHE_MISS", "Instruction cache miss" },
	{ ((1 << 9)  | (2 & 0xFF)),  "MEM_MAP_IO_ADDR", "Memory-mapped I/O access" },

	//EPAC Old VPU Faltan los eventos por lane!!!
	{0x020, "Load_Retries",                 "Number of Loads Retries" },
	{0x040, "Complete_Vector_Instructions", "Number of Vector instructions finished at the VPU" },
	{0x060, "Cycles_Stalled_VPU",           "Counts cycles that the VPU was stalled" },
	{0x080, "Cycles_Stalled_Renaming",      "Counts cycles stalled at renaming due to lack of physical registers" },
	{0x0A0, "Cycles_Stalled_Masked",        "Counts cycles stalled at the renaming due to the lack of physical registers for mask" },
	{0x0C0, "Cycles_Stalled_Pre_Issue_Q",   "Counts cycles stalled at the Pre-Issue Queue" },
	{0x0E0, "Cycles_Stalled_Unpacker",      "Counts cycles stalled at the Unpacker" },
	{0x100, "Cycles_Stalled_ROB",           "Counts cycles stalled at the Re-Order Buffer" },
	{0x120, "Cycles_Stalled_Arith_Q",       "Counts cycles stalled because Arithmetic Queue is full" },
	{0x140, "Cycles_Stalled_Mem_Q",         "Counts cycles stalled because Memory Queue is full" },
	{0x160, "Arithmetic_Instructions",      "Number of Arithmetic Instructions Issued" },
	{0x180, "Memory_Instructions",          "Number of Memory Instructions Issued" },
	{0x1A0, "Kill_Instructions",            "Number of Killed Instructions" },
	{0x1C0, "Cycles_VPU_Active",            "Counts Cycles that the VPU was Active" },
	{0x1E0, "Cycles_Empty_Pre_Issue_Q",     "Counts Cycles Pre-Issue Queue was Empty" },
	{0x200, "Cycles_Empty_Unpacker",        "Counts Cycles Unpacker was Empty" },
	{0x220, "Cycles_Empty_ROB",             "Counts Cycles Reorder Buffer was Empty" },
	{0x240, "Cycles_Empty_Arithmetic_Q",    "Counts Cycles Arithmetic Queue was Empty" },
	{0x260, "Cycles_Empty_Memory_Q",        "Counts Cycles Memory Queue was Empty" },

	//EPAC New VPU
	{ 0x020,  "COMPLETED_INST",  "Number of finished intructions at the VPU" },
	{ 0x040,  "ISSUED_INST",     "Number of issued instructions at the VPU" },
	{ 0x060,  "KILLS_COUNT",     "Number of kill events (not instructions) at the VPU" },
	{ 0x080,  "LOAD_RETRIES",    "Number of load instructions with retries" },
	{ 0x0A0,  "OVERLAPS_TAKEN",  "Number of instructions taking advantage of overlapping" },
	{ 0x0C0,  "MEM_DEPEN",       "Number of memory instructions with dependencies (index or mask)" },
	{ 0x0E0,  "VPU_ACTIVE",      "Number of cycles the VPU has been active" },
	{ 0x100,  "VPU_STALLED",     "Number of cycles the VPU has been stalled" },
	{ 0x120,  "RENAMING_STALL",  "Number of cycles stalled at renaming due to lack of physical registers" },
	{ 0x140,  "MASKED_STALL",    "Number of cycles stalled at the renaming due to the lack of physical registers for mask" },
	{ 0x160,  "PRE_ISSUE_STALL", "Number of cycles stalled at the Pre-Issue Queue" },
	{ 0x180,  "UNPACKER_STALL",  "Number of cycles stalled at the Unpacker" },
	{ 0x1A0,  "ROB_STALL",       "Number of cycles stalled at the Reorder Buffer" },
	{ 0x1C0,  "ARITH_Q_STALL",   "Number of cycles stalled because the Arithmetic Queue is full" },
	{ 0x1E0,  "MEMORY_Q_STALL",  "Number of cycles stalled because the Memory Queue is full" },
	{ 0x200,  "PRE_ISSUE_EMPTY", "Number of cycles the Pre-Issue Queue has been empty" },
	{ 0x220,  "UNPACKER_EMPTY",  "Number of cycles the Unpacker has been empty" },
	{ 0x240,  "ROB_EMPTY",       "Number of cycles the Reorder Buffer has been empty" },
	{ 0x260,  "ARITH_Q_EMPTY",   "Number of cycles the Arithmetic Queue has been empty" },
	{ 0x280,  "MEMORY_Q_EMPTY",  "Number of cycles the Memory Queue has been empty" },
	{ 0x2A0,  "ARITH_INST",      "Number of arithmetic instructions issued" },
	{ 0x2C0,  "MEM_INST",        "Number of memory instructions issued" },
	{ 0x2E0,  "OVERLAP_INST",    "Number of overlappable instructions issued" },
	{ 0x300,  "FP_INST",         "Number of Floating-Point instructions issued" },
	{ 0x320,  "FP_FMA_INST",     "Number of Floating-Point FMA instructions issued" }
};

#define CSR_reset(reg) asm volatile("csrc " #reg ", %0" :: "r" (-1))

#define CSR_write(reg, val) asm volatile("csrw " #reg ", %0" :: "r" (val))

#define CSR_read(reg) \
({ \
	uint64_t __tmp; \
	asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
	__tmp; \
})

#define CSR_configure(num, val) \
({ \
	int ret = syscall(__NR_arch_specific_syscall+20, 0x323+(num-3), val); \
})

#define CSR_read_hpmcounter_case(num) \
 case num:\
	return CSR_read(hpmcounter##num);

	//return CSR_read(hpmcounter##num);
uint64_t CSR_read_hpmcounter(int counter_num){
	switch (counter_num){
		CSR_read_hpmcounter_case(3);
		CSR_read_hpmcounter_case(4);
		CSR_read_hpmcounter_case(5);
		CSR_read_hpmcounter_case(6);
		CSR_read_hpmcounter_case(7);
		CSR_read_hpmcounter_case(8);
		CSR_read_hpmcounter_case(9);
	}
}

volatile
static inline uint64_t
read_cycles()
{
	uint64_t tmp;

	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));

	return tmp;
}

volatile
static inline uint64_t
read_instret()
{
	uint64_t tmp;

	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));

	return tmp;
}

static unsigned long long i_hwc_values[MAX_HWC] = { 0, 0, 0, 0, 0};

int
PAPI_start(int EventSet)
{
	PRINT_DEBUG
	PAPI_reset(0);
	return PAPI_OK;
}

int
PAPI_add_event(int EventSet, int EventCode)
{
	PRINT_DEBUG
	if (EventCode == CYCLES){fake_PAPI_EventSets[EventSet][0] = CYCLES; return PAPI_OK;}
	if (EventCode == INST_RETIRED){fake_PAPI_EventSets[EventSet][2] = INST_RETIRED; return PAPI_OK;}
	for (int i = 3; i < MAX_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == 0)
		{
			fake_PAPI_EventSets[EventSet][i] = EventCode;
			return PAPI_OK;
		}
	}

	return PAPI_OK;
}

int
PAPI_cleanup_eventset(int EventSet)
{
	PRINT_DEBUG
	return PAPI_OK;
}

int
PAPI_destroy_eventset(int *EventSet)
{
	PRINT_DEBUG
	return PAPI_OK;
}

int
PAPI_event_code_to_name(int EventCode, char *EventName)
{
	PRINT_DEBUG
	if (EventCode == INST_RETIRED){ strncpy(EventName, "Instructions", strlen("Instructions")+1); return PAPI_OK;}
	if (EventCode == CYCLES){ strncpy(EventName, "Cycles", strlen("Cycles")+1); return PAPI_OK;}
	const int n = sizeof(riscv_hwc)/sizeof(riscv_hwc[0]);
	for(int i = 0; i < n; i++){
		if (EventCode == riscv_hwc[i].code){
			strncpy(EventName, riscv_hwc[i].name, strlen(riscv_hwc[i].name)+1);
			return PAPI_OK;
		}
 }

	return PAPI_ENOEVNT;

}

int
PAPI_create_eventset(int *EventSet)
{
	PRINT_DEBUG

	*EventSet = fake_PAPI_count_eventsets;

	fake_PAPI_count_eventsets++;
	fake_PAPI_EventSets = realloc(fake_PAPI_EventSets, fake_PAPI_count_eventsets*sizeof(uint64_t));

	fake_PAPI_EventSets[*EventSet] = calloc(MAX_HWC, sizeof(uint64_t));

	return PAPI_OK;
}

int
PAPI_library_init(int version)
{
	PRINT_DEBUG
	return PAPI_VER_CURRENT;
}

int
PAPI_event_name_to_code(const char *in, int *out)
{
	PRINT_DEBUG
	if (strcmp(in, "Instructions") == 0){ *out = INST_RETIRED; return PAPI_OK;}
	if (strcmp(in, "Cycles") == 0){ *out = CYCLES; return PAPI_OK;}
	const int n = sizeof(riscv_hwc)/sizeof(riscv_hwc[0]);;
	for(int i = 0; i < n; i++){
		if (strcmp(in, riscv_hwc[i].name) == 0){
			*out = riscv_hwc[i].code; 
			return PAPI_OK;
		}
 }

	return PAPI_ENOEVNT;
}

int
PAPI_get_event_info(int EventCode, PAPI_event_info_t *info)
{
	PRINT_DEBUG
	info->event_code = EventCode;
	info->count = 1;

	if (EventCode == INST_RETIRED) {
		strncpy(info->symbol, "Instructions", strlen("Instructions")+1);
		strncpy(info->short_descr, "Instructions", strlen("Instructions")+1);
		strncpy(info->long_descr, "Instructions", strlen("Instructions")+1);
		return PAPI_OK;
	}else if (EventCode == CYCLES) {
		strncpy(info->symbol, "Cycles", strlen("Cycles")+1);
		strncpy(info->short_descr, "Cycles", strlen("Cycles")+1);
		strncpy(info->long_descr, "Cycles", strlen("Cycles")+1);
		return PAPI_OK;
	}
	const int n = sizeof(riscv_hwc)/sizeof(riscv_hwc[0]);
	for(int i = 0; i < n; i++){
		if (EventCode == riscv_hwc[i].code){
			strncpy(info->symbol, riscv_hwc[i].name, strlen(riscv_hwc[i].name)+1);
			strncpy(info->short_descr, riscv_hwc[i].name, strlen(riscv_hwc[i].name)+1);
			strncpy(info->long_descr, riscv_hwc[i].description, strlen(riscv_hwc[i].description)+1);
			return PAPI_OK;
		}
 }
	return PAPI_OK;
}

int
PAPI_add_named_event(int EventSet, const char *EventName)
{
	PRINT_DEBUG
	return PAPI_OK;
}

int
PAPI_stop(int EventSet, long long *values)
{
	PRINT_DEBUG
	return PAPI_OK;
}

char *
PAPI_strerror(int errorCode)
{
	PRINT_DEBUG
	return "RVPAPI: UNKNOWN ERROR";
}

int
PAPI_thread_init(unsigned long int (*id_fn) (void))
{
	PRINT_DEBUG
	return PAPI_OK;
}

int
PAPI_state(int EventSet, int *status)
{
	PRINT_DEBUG
	return PAPI_OK;
}

void
PAPI_shutdown()
{
	PRINT_DEBUG
}

int
PAPI_set_opt(int option, PAPI_option_t *ptr)
{
	PRINT_DEBUG
	return PAPI_OK;
}

int
PAPI_is_initialized()
{
	PRINT_DEBUG
	return PAPI_LOW_LEVEL_INITED;
}

int
PAPI_reset(int EventSet)
{
	PRINT_DEBUG
	
	int i = 0;
	for (i = 0; i<MAX_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == INST_RETIRED){
			i_hwc_values[i] = read_instret();
		}else if (fake_PAPI_EventSets[EventSet][i] == CYCLES){
			i_hwc_values[i] = read_cycles();
		}else if (fake_PAPI_EventSets[EventSet][i] != 0){
			CSR_configure(i, fake_PAPI_EventSets[EventSet][i]);
			i_hwc_values[i] = CSR_read_hpmcounter(i);
		}
	}

	return PAPI_OK;
}

int
PAPI_read(int EventSet, long long *hwc_values)
{
	PRINT_DEBUG
	int count = 0;
	int i = 0;
	for (i = 0; i<MAX_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == INST_RETIRED){
			hwc_values[count++] = read_instret() - i_hwc_values[i];
		}else if (fake_PAPI_EventSets[EventSet][i] == CYCLES){
			hwc_values[count++] = read_cycles() - i_hwc_values[i];
		}else if(fake_PAPI_EventSets[EventSet][i] != 0){
			hwc_values[count++] = CSR_read_hpmcounter(i) - i_hwc_values[i];
		}
	}

	return PAPI_OK;
}

int
PAPI_accum(int EventSet, long long *hwc_values)
{
	PRINT_DEBUG
	unsigned long long t_hwc_values[MAX_HWC] = { 0, 0, 0, 0, 0 };
	int count = 0;
	int i = 0;
	for (i = 0; i<MAX_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == INST_RETIRED){
			t_hwc_values[i] = read_instret();
			hwc_values[count++] = (long long)(t_hwc_values[i] - i_hwc_values[i]);
			i_hwc_values[i] = t_hwc_values[i];
		}else if (fake_PAPI_EventSets[EventSet][i] == CYCLES){
			t_hwc_values[i] = read_cycles();
			hwc_values[count++] = (long long)(t_hwc_values[i] - i_hwc_values[i]);
			i_hwc_values[i] = t_hwc_values[i];
		}else if(fake_PAPI_EventSets[EventSet][i] != 0){
			t_hwc_values[i] = CSR_read_hpmcounter(i);
			hwc_values[count++] = (long long)(t_hwc_values[i] - i_hwc_values[i]);
			i_hwc_values[i] = t_hwc_values[i];
		}
	}

	return PAPI_OK;
}
