PAPI_HOME = /apps/riscv/papi/6.0.0

libpapi.so: papi.c
	clang -g -DEPI_EPAC_VPU -O3 -fPIC -I$(PAPI_HOME)/include -c $^
	#gcc -g -DEPI_EPAC_VPU -O2 -DDEBUG -fPIC -I$(PAPI_HOME)/include -c $^
	clang -g -O2 -shared -o $@ papi.o

test: test.c
	clang -g -O0 -mepi test.c -I$(PAPI_HOME)/include -Wl,-rpath,. -L. -lpapi -o $@
#	gcc test.c -I. -Wl,-rpath,. -L. -lpapi -o $@

all: libpapi.so test

clean:
	rm -rf *.prv *.pcf *.row set-* TRACE*

veryclean:
	rm -rf libpapi.so test *.prv *.pcf *.row set-* TRACE*
