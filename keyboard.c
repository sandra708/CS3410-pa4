#include "kernel.h"
//#include "keyboard.h"
// Keyboard device driver.

// a pointer to the memory-mapped I/O region for the keyboard
volatile struct dev_kbd *dev_kbd;

void keyboard_init()
{
  /* Find out where I/O region is in memory. */
  for (int i = 0; i < 16; i++) {
    if (bootparams->devtable[i].type == DEV_TYPE_KEYBOARD) {
      puts("Detected keyboard device...");
      // find a virtual address that maps to this I/O region
      dev_kbd = physical_to_virtual(bootparams->devtable[i].start);
      // also allow keyboard interrupts
      set_cpu_status(current_cpu_status() | (1 << (8+INTR_KEYBOARD)));
      puts("...keyboard driver is ready.");
      return;
    }
  }
}

void keyboard_trap() {
  // note: interrupts should be off already
  // so long as there is a character to be read

  //we disable interrupts anyways, for more safety
  

  while (dev_kbd->status) {
    // read the character
    char c = dev_kbd->data;
    // then just print it
    if(c=='t'){
      int level = intr_disable();
      network_stats_print();
      intr_restore(level);
    }  
    else if(c=='s'){
      int level = intr_disable();
      spam_print();
      intr_restore(level);
    }
    else if(c=='v'){
      int level = intr_disable();
      vulnerable_print();
      intr_restore(level);
    }
    else if(c=='e'){
      int level = intr_disable();
      evil_print();
      intr_restore(level);
    }
    else if(c=='a'){
      int level = intr_disable();
      all_print();
      intr_restore(level);
    }
  }
}


