#include "kernel.h"

struct bootparams *bootparams;

int debug = 1; // change to 0 to stop seeing so many messages

void shutdown() {
  puts("Shutting down...");
  // this is really the "wait" instruction, which gcc doesn't seem to know about
  __asm__ __volatile__ ( ".word 0x42000020\n\t");
  while (1);
}


/* Trap handling.
 *
 * Only core 0 will get interrupts, but any core can get an exception (or
 * syscall).  So we allocate enough space for every core i to have its own place
 * to store register state (trap_save_state[i]), its own stack to use in the
 * interrupt handler (trap_stack_top[i]), and a valid $gp to use during
 * interrupt handling (trap_gp[i]).
 */

struct mips_core_data *trap_save_state[MAX_CORES]; /* one save-state per core */
void *trap_stack_top[MAX_CORES]; /* one trap stack per core */
unsigned int trap_gp[MAX_CORES]; /* one trap $gp per core */

void trap_init()
{
  int id = current_cpu_id();

  /* trap should use the same $gp as the current code */
  trap_gp[id] = current_cpu_gp();

  /* trap should use a fresh stack */
  void *bottom = alloc_pages(4);
  trap_stack_top[id] = bottom + 4*PAGE_SIZE - 4;

  /* put the trap save-state at the top of the corresponding trap stack */
  trap_stack_top[id] -= sizeof(struct mips_core_data);
  trap_save_state[id] = trap_stack_top[id];

  /* it is now safe to take interrupts on this core */
  intr_restore(1);
}

void interrupt_handler(int cause)
{
  // note: interrupts will only happen on core 0
  // diagnose the source(s) of the interrupt trap
  int pending_interrupts = (cause >> 8) & 0xff;
  int unhandled_interrupts = pending_interrupts;

  if (pending_interrupts & (1 << INTR_KEYBOARD)) {
    if (debug) printf("interrupt_handler: got a keyboard interrupt, handling it\n");
    keyboard_trap();
    unhandled_interrupts &= ~(1 << INTR_KEYBOARD);
  }

  if (pending_interrupts & (1 << INTR_TIMER)) {
    printf("interrupt_handler: got a spurious timer interrupt, ignoring it and hoping it doesn't happen again\n");
    unhandled_interrupts &= ~(1 << INTR_TIMER);
  }

  if (unhandled_interrupts != 0) {
    printf("got interrupt_handler: one or more other interrupts (0x%08x)...\n", unhandled_interrupts);
  }

}

void trap_handler(struct mips_core_data *state, unsigned int status, unsigned int cause)
{
  if (debug) printf("trap_handler: status=0x%08x cause=0x%08x on core %d\n", status, cause, current_cpu_id());
  // diagnose the cause of the trap
  int ecode = (cause & 0x7c) >> 2;
  switch (ecode) {
    case ECODE_INT:	  /* external interrupt */
      interrupt_handler(cause);
      return; /* this is the only exception we currently handle; all others cause a shutdown() */
    case ECODE_MOD:	  /* attempt to write to a non-writable page */
      printf("trap_handler: some code is trying to write to a non-writable page!\n");
      break;
    case ECODE_TLBL:	  /* page fault during load or instruction fetch */
    case ECODE_TLBS:	  /* page fault during store */
      printf("trap_handler: some code is trying to access a bad virtual address!\n");
      break;
    case ECODE_ADDRL:	  /* unaligned address during load or instruction fetch */
    case ECODE_ADDRS:	  /* unaligned address during store */
      printf("trap_handler: some code is trying to access a mis-aligned address!\n");
      break;
    case ECODE_IBUS:	  /* instruction fetch bus error */
      printf("trap_handler: some code is trying to execute non-RAM physical addresses!\n");
      break;
    case ECODE_DBUS:	  /* data load/store bus error */
      printf("trap_handler: some code is read or write physical address that can't be!\n");
      break;
    case ECODE_SYSCALL:	  /* system call */
      printf("trap_handler: who is doing a syscall? not in this project...\n");
      break;
    case ECODE_BKPT:	  /* breakpoint */
      printf("trap_handler: reached breakpoint, or maybe did a divide by zero!\n");
      break;
    case ECODE_RI:	  /* reserved opcode */
      printf("trap_handler: trying to execute something that isn't a valid instruction!\n");
      break;
    case ECODE_OVF:	  /* arithmetic overflow */
      printf("trap_handler: some code had an arithmetic overflow!\n");
      break;
    case ECODE_NOEX:	  /* attempt to execute to a non-executable page */
      printf("trap_handler: some code attempted to execute a non-executable virtual address!\n");
      break;
    default:
      printf("trap_handler: unknown error code 0x%x\n", ecode);
      break;
  }
  shutdown();
}


/* kernel entry point called at the end of the boot sequence */
void __boot() {

  if (current_cpu_id() == 0) {
    /* core 0 boots first, and does all of the initialization */

    // boot parameters are on physical page 0
    bootparams = physical_to_virtual(0x00000000);

    // initialize console early, so output works
    console_init();

    // output should now work
    printf("Welcome to my kernel!\n");
    printf("Running on a %d-way multi-core machine\n", current_cpu_exists());

    // initialize memory allocators
    mem_init();

    // prepare to handle interrupts, exceptions, etc.
    trap_init();

    // initialize keyboard late, since it isn't really used by anything else
    keyboard_init();

    // see which cores are already on
    for (int i = 0; i < 32; i++)
      printf("CPU[%d] is %s\n", i, (current_cpu_enable() & (1<<i)) ? "on" : "off");

    // turn on all other cores
    set_cpu_enable(0xFFFFFFFF);

    // see which cores got turned on
    busy_wait(0.1);
    for (int i = 0; i < 32; i++)
      printf("CPU[%d] is %s\n", i, (current_cpu_enable() & (1<<i)) ? "on" : "off");

  } else {
    /* remaining cores boot after core 0 turns them on */

    // nothing to initialize here... 
  }

  printf("Core %d of %d is alive!\n", current_cpu_id(), current_cpu_exists());

  busy_wait(current_cpu_id() * 0.1); // wait a while so messages from different cores don't get so mixed up
  int size = 64 * 1024 * 4;
  printf("about to do calloc(%d, 1)\n", size);
  unsigned int t0  = current_cpu_cycles();
  calloc(size, 1);
  unsigned int t1  = current_cpu_cycles();
  printf("DONE (%u cycles)!\n", t1 - t0);

  while (1) ;

  for (int i = 1; i < 30; i++) {
    int size = 1 << i;
    printf("about to do calloc(%d, 1)\n", size);
    calloc(size, 1);
  }



  while (1) {
    printf("Core %d is still running...\n", current_cpu_id());
    busy_wait(4.0); // wait 4 seconds
  }

  shutdown();
}

