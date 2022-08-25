libpapi.so: papi.c
	clang -g -DEPI_EPAC_VPU -O3 -fPIC -I/apps/riscv/papi/6.0.0/include -c $^
	#gcc -g -DEPI_EPAC_VPU -O2 -DDEBUG -fPIC -I/apps/riscv/papi/6.0.0/include -c $^
	clang -g -O2 -shared -o $@ papi.o

test: test.c
	clang -g -O0 -mepi test.c -I/apps/riscv/papi/6.0.0/include -Wl,-rpath,. -L. -lpapi -o $@
#	gcc test.c -I. -Wl,-rpath,. -L. -lpapi -o $@

all: libpapi.so test

clean:
	rm -f libpapi.so test
