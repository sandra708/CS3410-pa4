#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "machine.h"
#include "honeypot.h"
#include "console.h"
#include "keyboard.h"

/* This is used by the trap handler to save the CPU state
 * Note: So long as trap handlers do not touch any coprocessor state (e.g.
 * memory management stuff in coprocessor 0, or the floating point registers,
 * which are in coprocessor 1), it is safe for the trap handler to only save
 * the general registers, plus the commonly used HI and LO multiplication
 * registers.
 */
struct mips_core_data {
  unsigned int R[32];
  unsigned int LO, HI;
  unsigned int PC; /* EPC actually */
};


/* kernel.c */

#define MAX_CORES 32 // expect at most 32-way multi-core machine
extern struct bootparams *bootparams;
extern int debug;
void shutdown() __attribute__ ((noreturn));



/* machine.s */

/* The current_cpu_enable() and set_cpu_enable() functions read and write the
 * "multi-core cpu enable" register. There are 32 possible cores, and this
 * register holds a bit mask describing which cores that are turned on.
 * Initially, the register contains the value 0x00000001, indicating that only
 * core 0 is turned on. If we write all ones to this register, i.e. 0xFFFFFFFF,
 * or -1, then it will turn on every one of the other cores. If we read
 * 0x0000003F from this register, then it means six cores are turned on (cores 0
 * through 5).  If we read 0x80008003, it means four cores are turned on, (core
 * 0, core 1, core 15, and core 31).
 *
 * One can find out how many cores exist (without turning them on) by reading
 * the current_cpu_exists() register, which returns a number between 1 and 32.
 *
 * Once turned on, cores can not be turned off.
 *
 * Any of the cores can access these registers, with the same effect. E.g., core
 * 0 can turn on core 3 by writing (1<<3) to the enable register, then later
 * core 3 can turn on core 2 by writing (1<<2).
 */

unsigned int current_cpu_id(); // id number of the current core
unsigned int current_cpu_enable(); // bit mask of which cpu cores are turned on
unsigned int current_cpu_exists(); // count of number of cpu cores

unsigned int current_cpu_context(); // MIPS 'Context' register
unsigned int current_cpu_status(); // MIPS 'Status' register
unsigned int current_cpu_cause(); // MIPS 'Cause' register
unsigned int current_cpu_epc(); // MIPS 'EPC' register
unsigned int current_cpu_badvaddr(); // MIPS 'BadVaddr' register
unsigned int current_cpu_cycles(); // MIPS 'Cycles' register (number of cycles since boot)
unsigned int current_cpu_gp(); // $gp register

unsigned int set_cpu_enable(unsigned int mask); // turns on more cores by writing new 1 bits to enable register

unsigned int set_cpu_context(unsigned int context);
unsigned int set_cpu_status(unsigned int status);
unsigned int set_cpu_cause(unsigned int cause);
unsigned int set_cpu_epc(unsigned int epc);
unsigned int set_cpu_badvaddr(unsigned int badvaddr);


/* intr.c */

// do nothing, very fast, for given number of seconds
void busy_wait(double sec);

// do nothing, very fast, for given number of cycles
void busy_wait_cycles(int cycles);

// check if interrupts enabled on this core; returns 1 if interrupts are enabled, 0 otherwise
int intr_level(void);

// disable interrupts on this core; returns 1 if interrupts had been enabled, 0 otherwise
int intr_disable(void);

// if level==0, do nothing; else enable interrupts
void intr_restore(int level);

// Typically, these functions are used to implement a limited kind of critical section:
//
//  int level = intr_disable();
//  ...
//  /* critical section */
//  ...
//  intr_restore(level);
//

/* mem.c */

#define NOPAGE 0xFFFFFFFF // physical address go from 0 to at most 1 GB, so 0xFFFFFFFF can serve as a "null" physical address

void mem_init();

// note: these are not synchronized: if someone is mucking with the pagetables
// while these are running, something could go wrong.
void *physical_to_virtual(unsigned int paddr); // return some usable virtual address (not necessarily unique) for given physical address
unsigned int virtual_to_physical(void *vptr); // return physical address that given virtual address maps to, or NOPAGE if there is no mapping

// standard library helpers
void *memset(void *s, unsigned int c, unsigned int len);
void *memcpy(void *dest, const void *src, unsigned int len);

// fast, underlying page-at-a-time memory management
// note: These are not synchronized in any way. If multiple cores are going to
// call these functions, then every access to these need to be use some
// multi-core safe synchronization. And if interrupt handlers are going to call
// these functions, then every access to these needs to use interrupt-safe
// synchronization too.
void *alloc_pages(unsigned int count); // allocate count pages of physically (and virtually) contiguous memory
void *calloc_pages(unsigned int count); // allocate and clear count pages of physically (and virtually) contiguous memory
void free_pages(void *page, unsigned int count); // free count contiguous pages that came from page_alloc() and/or page_calloc()

// standard memory management
// note: These are not synchronized in any way. If multiple cores are going to
// call these functions, then every access to these need to be use some
// multi-core safe synchronization. And if interrupt handlers are going to call
// these functions, then every access to these needs to use interrupt-safe
// synchronization too.
void *malloc(unsigned int size); // allocate size bytes of memory
void *calloc(unsigned int size, unsigned int count); // allocate and clear (size * count) bytes of memory
void free(void *p); // free a pointer that came from malloc() or calloc()


/* printf.c */

int printf_u(const char *format, ...); // unsynchronized
//int printf_m(const char *format, ...); // protected by a mutex
int printf_i(const char *format, ...); // protected by a interrupt-disable
//int printf_mi(const char *format, ...); // protected by a mutex followed by interrupt-disable
int printf(const char *format, ...); // protected by something or other
int sprintf(char *out, const char *format, ...);

#endif // _KERNEL_H_

