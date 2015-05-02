#include "kernel.h"

// Console device driver.

// a pointer to the memory-mapped I/O region for the console
volatile struct dev_console *dev_console;

void console_init()
{
  /* Find out where the I/O region is in memory. */
  for (int i = 0; i < 16; i++) {
    if (bootparams->devtable[i].type == DEV_TYPE_CONSOLE) {
      // find a virtual address that maps to this I/O region
      dev_console = physical_to_virtual(bootparams->devtable[i].start);
      puts("Detected console device...");
      puts("...console driver is ready.");
      return;
    }
  }

  // Note: the below error message won't ever be printed (since the console
  // driver is needed for printing). If we had a simple audio device, we could
  // have it beep out an error code (e.g. in morse code).
  puts("Did not detect a console device!");
  shutdown();
}


int putchar(int c)
{
  // check if console_init has happened yet
  if (!dev_console) return -1;
  // then just send the character to the device's data register
  dev_console->data = c;
  return c;
}

int puts(char *s)
{
  // similar to putchar, but for a sequence of characters
  if (!dev_console)
    return -1;
  int i;
  for (i = 0; s[i] != '\0'; i++)
    dev_console->data = s[i];
  dev_console->data = '\n';
  return i;
}

