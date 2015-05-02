#include "kernel.h"

int intr_level(void)
{
  int status = current_cpu_status();
  return (status & 1);
}

int intr_disable(void)
{
  int status = current_cpu_status();
  set_cpu_status(status & ~1);
  return (status & 1);
}

void intr_restore(int level)
{
  if (level)
    set_cpu_status(current_cpu_status() | 1);
}

void busy_wait(double sec)
{
  int tsc = current_cpu_cycles();
  int end = tsc + (int)(sec * CPU_CYCLES_PER_SECOND);
  while (current_cpu_cycles() < end)
    ;
}

void busy_wait_cycles(int cycles)
{
  int tsc = current_cpu_cycles();
  int end = tsc + cycles;
  while (current_cpu_cycles() < end)
    ;
}
