#include "lock.h"

void spin_lock(volatile int* m){
    asm(".set mips2");
    asm("try:");
    asm("ll $8, 0($4)");
    asm("bne $8, $0, try");
    asm("addiu $8, $0, 1");
    asm("sc $8, 0($4)");
    asm("beq $8, $0, try");
     //printf("Core %d acquired lock. at %p\n", current_cpu_id() ,m);
}

volatile int* unlock(volatile int *m){
	//printf("Core %d releasing lock.\n", current_cpu_id());
    register volatile int* addr asm("t0") = m;
    asm(".set mips2");
    asm("sw $0, 0($8)");
    return addr;
}
