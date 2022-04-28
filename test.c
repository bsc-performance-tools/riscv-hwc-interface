#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <papi.h>

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



void main(int argc, char **argv)
{
	long long hwc_values[4] = {0};
	int event_set=0;
	PAPI_create_eventset(&event_set);
	fprintf(stdout, "event_set = %lld\n", event_set);
	PAPI_start(0);
	PAPI_add_event(event_set, 50);
	PAPI_add_event(event_set, 59);
	int event_completed_inst=0;
	int event_completed_inst2=0;
	PAPI_event_name_to_code("ISSUED_INST", &event_completed_inst);
	PAPI_event_name_to_code("ARITH_Q_STALL", &event_completed_inst2);
	fprintf(stdout, "Complete_Vector_Instructions = %lld\n", event_completed_inst);
	fprintf(stdout, "Cycles_VPU_Active = %lld\n", event_completed_inst2);
	PAPI_add_event(event_set, event_completed_inst);
	PAPI_add_event(event_set, event_completed_inst2);
	PAPI_reset(0);
	PAPI_accum(0, hwc_values);
	PAPI_accum(0, hwc_values);

	int i = 0;
	int a = 4;

	uint64_t tmp;
	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	uint64_t scalar_cycles = tmp;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	uint64_t scalar_instret = tmp;
	for (i=0; i < 1000; i ++)
	{
		a += a;
	}
	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	scalar_cycles = tmp - scalar_cycles;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	scalar_instret = tmp - scalar_instret;

	PAPI_accum(0, hwc_values);

	fprintf(stdout, "instret = %lld\n", scalar_instret);
	fprintf(stdout, "cycle = %lld\n", scalar_cycles);
	fprintf(stdout, "ipc = %lf\n", (double)scalar_instret/(double)scalar_cycles);

	fprintf(stdout, "instret = %lld\n", hwc_values[1]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[0]);
	fprintf(stdout, "ipc = %lf\n", (double)hwc_values[1]/(double)hwc_values[0]);
	fprintf(stdout, "vec_instret = %lld\n", hwc_values[2]);
	fprintf(stdout, "cycle_instret = %lld\n", hwc_values[3]);

	PAPI_read(0, hwc_values);
	PAPI_accum(0, hwc_values);

	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	scalar_cycles = tmp;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	scalar_instret = tmp;

	asm("vsetvli zero, zero, e64, m1");
	for (i=0; i < 1000; i ++){
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	asm("vfadd.vv v0, v0, v0");
	}

	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	scalar_cycles = tmp - scalar_cycles;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	scalar_instret = tmp - scalar_instret;


	PAPI_accum(0, hwc_values);

	fprintf(stdout, "instret = %lld\n", scalar_instret);
	fprintf(stdout, "cycle = %lld\n", scalar_cycles);
	fprintf(stdout, "ipc = %lf\n", (double)scalar_instret/(double)scalar_cycles);

	fprintf(stdout, "instret = %lld\n", hwc_values[1]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[0]);
	fprintf(stdout, "ipc = %lf\n", (double)hwc_values[1]/(double)hwc_values[0]);
	fprintf(stdout, "vec_instret = %lld\n", hwc_values[2]);
	fprintf(stdout, "cycle_instret = %lld\n", hwc_values[3]);
}
