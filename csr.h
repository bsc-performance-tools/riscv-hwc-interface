#ifndef CSR_H
#define CSR_H
#include <stdint.h>

#define GEN_READ_CSR(csr)\
  uint64_t read_csr_##csr(){\
    uint64_t output;\
    asm("csrr %0, " #csr : "=r"(output));\
    return output;\
  }

#define GEN_READ_CSR_BITS(csr, name, to, from)\
  uint64_t read_csr_##name(){\
    uint64_t output;\
    asm("csrr %0, " #csr : "=r"(output));\
    output = output >> from; \
    output = output & ((1 << (to-from+1))-1); \
    return output;\
  }

GEN_READ_CSR(vstart);
GEN_READ_CSR(vxsat);
GEN_READ_CSR(vxrm);

GEN_READ_CSR(vl);
GEN_READ_CSR(vtype);
GEN_READ_CSR_BITS(vtype, lmul, 1, 0);
GEN_READ_CSR_BITS(vtype, sew,  4, 2);
GEN_READ_CSR_BITS(vtype, ediv, 6, 5);
GEN_READ_CSR_BITS(vtype, vill, 63, 63);
#endif
