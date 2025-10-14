libpapi.so: papi.c
	mkdir -p lib
	$(CC) -fPIC -Iinclude -c $^
	$(CC) -shared -o lib/$@ papi.o

libpapi.a: papi.c
	mkdir -p lib
	$(CC) -static -Iinclude -c $^
	$(AR) cru lib/$@ papi.o

test: test.c libpapi.so
	$(CC) test.c -Iinclude -Wl,-rpath,lib -Llib -lpapi -o $@

all: libpapi.so libpapi.a

clean:
	rm -rf lib papi.o test
