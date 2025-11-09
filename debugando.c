#include <stdio.h>
#include <stdint.h>

int main(void) {
    // mul t0,sp,s11 t0 = 0x80007ff0 * 0x00000003 = 0x80017fd0
    int32_t rs1 = (int32_t)0x80007ff0;  
    int32_t rs2 = 0x00000003;          

    printf("rs1 = 0x%08x (%d)\n", (uint32_t)rs1, rs1);
    printf("rs2 = 0x%08x (%u)\n", rs2, rs2);
    printf("prod  = 0x%08x (%d)\n", (uint32_t)(rs1*rs2), (rs1*rs2)); // espera 0xfffffe9d

    return 0;
}
