PAPI_HOME = /apps/riscv/papi/6.0.0

libpapi.so: papi.c
	gcc -fPIC -I$(PAPI_HOME)/include -c $^
#	gcc -DDEBUG -fPIC -I$(PAPI_HOME)/include -c $^
	gcc -shared -o lib/$@ papi.o

test: test.c
	gcc test.c -I$(PAPI_HOME)/include -Wl,-rpath,. -L. -lpapi -o $@
#	gcc test.c -I. -Wl,-rpath,. -L. -lpapi -o $@

all: libpapi.so test

clean:
	rm -f lib/libpapi.so test
