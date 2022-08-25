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
	long long hwc_values[5] = {0};
	int event_set=0;
	int event_set1=1;
	PAPI_create_eventset(&event_set);
	PAPI_create_eventset(&event_set1);
	fprintf(stdout, "event_set = %lld\n", event_set);
	PAPI_start(0);
	int event_inst=0;
	int event_cycle=0;
	int event_vl=0;
	int event_completed_inst=0;
	int event_completed_inst2=0;
	PAPI_event_name_to_code("PAPI_TOT_INS", &event_inst);
	PAPI_event_name_to_code("CSR_VL", &event_vl);
	PAPI_event_name_to_code("PAPI_TOT_CYC", &event_cycle);
	PAPI_event_name_to_code("VPU_COMPLETED_INST", &event_completed_inst);
	PAPI_event_name_to_code("VPU_ACTIVE", &event_completed_inst2);
	fprintf(stdout, "PAPI_TOT_INS = %lld\n", event_inst);
	fprintf(stdout, "PAPI_TOT_CYC = %lld\n", event_cycle);
	fprintf(stdout, "CSR_VL = %lld\n", event_vl);
	fprintf(stdout, "VPU_COMPLETED_INST = %lld\n", event_completed_inst);
	fprintf(stdout, "VPU_ACTIVE = %lld\n", event_completed_inst2);
	PAPI_add_event(event_set, event_inst);
	PAPI_add_event(event_set, event_cycle);
	PAPI_add_event(event_set, event_vl);
	PAPI_add_event(event_set, event_completed_inst);
	PAPI_add_event(event_set, event_completed_inst2);
	PAPI_add_event(        1, event_inst);
	PAPI_add_event(        1, event_cycle);
	PAPI_add_event(        1, event_vl);
	PAPI_add_event(        1, event_completed_inst);
	PAPI_add_event(        1, event_completed_inst2);
	PAPI_start(event_set);

	int i = 0;
	int a = 8928372983;

	uint64_t tmp;
	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	uint64_t scalar_cycles = tmp;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	uint64_t scalar_instret = tmp;
	for (i=0; i < 100000; i ++)
	{
		a += a/5 +8 + i/7;
		a += a/5 +8 + i/7;
		a += a/5 +8 + i/7;
		a += a/5 +8 + i/7;
	}
	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	scalar_cycles = tmp - scalar_cycles;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	scalar_instret = tmp - scalar_instret;

	fprintf(stdout, "a = %d\n", a);
	PAPI_read(event_set, hwc_values);

	fprintf(stdout, "------------------\n");
	fprintf(stdout, "RAW counters\n");
	fprintf(stdout, "instret = %lld\n", scalar_instret);
	fprintf(stdout, "cycle = %lld\n", scalar_cycles);
	fprintf(stdout, "ipc = %lf\n", (double)scalar_instret/(double)scalar_cycles);
	fprintf(stdout, "------------------\n");
	fprintf(stdout, "PAPI Counters\n");
	fprintf(stdout, "instret = %lld\n", hwc_values[0]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[1]);
	fprintf(stdout, "ipc = %lf\n", (double)hwc_values[0]/(double)hwc_values[1]);
	fprintf(stdout, "vec_instret = %lld\n", hwc_values[3]);
	fprintf(stdout, "cycle_instret = %lld\n", hwc_values[4]);
	fprintf(stdout, "vl = %lld\n", hwc_values[2]);

	PAPI_reset(1);

	__asm__ __volatile__("rdcycle %0" : "=r"(tmp));
	scalar_cycles = tmp;
	__asm__ __volatile__("rdinstret %0" : "=r"(tmp));
	scalar_instret = tmp;

	asm("vsetvli zero, %0, e64, m1"::"r"(100));
	//asm("vsetvli zero, zero, e64, m1");
	for (i=0; i < 10000; i ++){
	asm("vsetvli zero, %0, e64, m1"::"r"(i%256));
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


	PAPI_read(1, hwc_values);

	fprintf(stdout, "------------------\n");
	fprintf(stdout, "RAW counters\n");
	fprintf(stdout, "instret = %lld\n", scalar_instret);
	fprintf(stdout, "cycle = %lld\n", scalar_cycles);
	fprintf(stdout, "ipc = %lf\n", (double)scalar_instret/(double)scalar_cycles);
	fprintf(stdout, "------------------\n");
	fprintf(stdout, "PAPI Counters\n");
	fprintf(stdout, "instret = %lld\n", hwc_values[0]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[1]);
	fprintf(stdout, "ipc = %lf\n", (double)hwc_values[0]/(double)hwc_values[1]);
	fprintf(stdout, "vec_instret = %lld\n", hwc_values[3]);
	fprintf(stdout, "cycle_instret = %lld\n", hwc_values[4]);
	fprintf(stdout, "vl = %lld\n", hwc_values[2]);
}
