#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <papi.h>

#define MAX_HWC 4
#define FIXED_HWC 2
#define MAX_CONF_HWC MAX_HWC - FIXED_HWC

#if defined DEBUG
# define PRINT_DEBUG fprintf(stderr, "[%s]\n" , __func__);
#else
# define PRINT_DEBUG
#endif

int   fake_PAPI_count_eventsets = 0;
int **fake_PAPI_EventSets;

// p. 20 https://sifive.cdn.prismic.io/sifive%2F834354f0-08e6-423c-bf1f-0cb58ef14061_fu540-c000-v1.0.pdf
struct RISCV_counter_def                                                                                                                                   
{                                                                                                                                                          
	unsigned int code;
	char  name[32];
	char  description[256];
};                                                                                                                                                         

static enum riscv_hwc_map
{
	INST_RETIRED = 50,
	CYCLES = 59,
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
	{ ((1 << 9)  | (2 & 0xFF)),  "MEM_MAP_IO_ADDR", "Memory-mapped I/O access" }
};

#define CSR_reset(reg) asm volatile("csrc " #reg ", %0" :: "r" (-1))

#define CSR_write(reg, val) asm volatile("csrw " #reg ", %0" :: "r" (val))

#define CSR_read(reg) \
({ \
	uint64_t __tmp; \
	asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
	__tmp; \
})

#define CSR_configure(vals) \
({ \
	CSR_write(mhpmevent3, vals[0]); \
	CSR_write(mhpmevent4, vals[1]); \
})

static int rv_hwc_get_info(char *hwc, struct RISCV_counter_def *hwc_info)
{
	int i = 0;
	for (i = 0; i<NUM_RISCV_HWC; i++)
	{
		if (strncmp(hwc, riscv_hwc[i].name, 32) == 0)
		{
			memcpy(hwc_info, &riscv_hwc[i], sizeof(struct RISCV_counter_def));
			return 0;
		}
	}

return -1;
} 

void CSR_Configure(char **cnt)
{
	uint64_t hwc_events[MAX_CONF_HWC];
	int i = 0;
	for (i=0;i<MAX_CONF_HWC;i++)
	{
		struct RISCV_counter_def hwc_info;
		if (rv_hwc_get_info(cnt[i], &hwc_info) == 0)
		{
			hwc_events[i] = hwc_info.code;
		}
	}

	CSR_configure(hwc_events);
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

static long long i_hwc_values[FIXED_HWC] = { 0, 0 };

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

	int i = 0;
	for (i = 0; i<FIXED_HWC; i++)
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
	if (EventCode == INST_RETIRED) strncpy(EventName, "Instructions", strlen("Instructions")+1);
	if (EventCode == CYCLES) strncpy(EventName, "Cycles", strlen("Cycles")+1);

	return PAPI_OK;
}

int
PAPI_create_eventset(int *EventSet)
{
	PRINT_DEBUG

	*EventSet = fake_PAPI_count_eventsets;

	fake_PAPI_count_eventsets++;
	fake_PAPI_EventSets = realloc(fake_PAPI_EventSets, fake_PAPI_count_eventsets*sizeof(int));

	fake_PAPI_EventSets[*EventSet] = calloc(FIXED_HWC, sizeof(int));

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
	if (strcmp(in, "Instructions") == 0) *out = INST_RETIRED;
	if (strcmp(in, "Cycles") == 0) *out = CYCLES;

	return PAPI_OK;
}

int
PAPI_get_event_info(int EventCode, PAPI_event_info_t *info)
{
	PRINT_DEBUG
	info->event_code = EventCode;
	info->count = 1;

	if (EventCode == INST_RETIRED)
	{
		strncpy(info->symbol, "Instructions", strlen("Instructions")+1);
		strncpy(info->short_descr, "Instructions", strlen("Instructions")+1);
		strncpy(info->long_descr, "Instructions", strlen("Instructions")+1);
	}
	if (EventCode == CYCLES)
	{
		strncpy(info->symbol, "Cycles", strlen("Cycles")+1);
		strncpy(info->short_descr, "Cycles", strlen("Cycles")+1);
		strncpy(info->long_descr, "Cycles", strlen("Cycles")+1);
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
	for (i = 0; i<FIXED_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == INST_RETIRED)
		{
			//i_hwc_values[0] = CSR_read(instret);
			i_hwc_values[i] = read_instret();
		}
		else if (fake_PAPI_EventSets[EventSet][i] == CYCLES)
		{
			//i_hwc_values[1] = CSR_read(cycle);
			i_hwc_values[i] = read_cycles();
		}
	}

	return PAPI_OK;
}

int
PAPI_read(int EventSet, long long *hwc_values)
{
	PRINT_DEBUG

	int i = 0;
	for (i = 0; i<FIXED_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == INST_RETIRED)
		{
			//hwc_values[0] = CSR_read(instret) - i_hwc_values[0];
			hwc_values[i] = read_instret() - i_hwc_values[i];
		}
		else if (fake_PAPI_EventSets[EventSet][i] == CYCLES)
		{
			//hwc_values[1] = CSR_read(cycle) - i_hwc_values[1];
			hwc_values[i] = read_cycles() - i_hwc_values[i];
		}
	}

	return PAPI_OK;
}

int
PAPI_accum(int EventSet, long long *hwc_values)
{
	PRINT_DEBUG
	long long t_hwc_values[FIXED_HWC] = { 0, 0 };

	int i = 0;
	for (i = 0; i<FIXED_HWC; i++)
	{
		if (fake_PAPI_EventSets[EventSet][i] == INST_RETIRED)
		{
			//t_hwc_values[0] = CSR_read(instret);
			t_hwc_values[i] = read_instret();
			hwc_values[i] = t_hwc_values[i] - i_hwc_values[i];
		}
		else if (fake_PAPI_EventSets[EventSet][i] == CYCLES)
		{
			//t_hwc_values[1] = CSR_read(cycle);
			t_hwc_values[i] = read_cycles();
			hwc_values[i] = t_hwc_values[i] - i_hwc_values[i];
		}
	}

	i_hwc_values[0] = t_hwc_values[0];
	i_hwc_values[1] = t_hwc_values[1];

	return PAPI_OK;
}

int
PAPI_overflow(int EventSet, int EventCode, int threshold, int flags, PAPI_overflow_handler_t handler)
{
	PRINT_DEBUG
	return PAPI_OK;
}
