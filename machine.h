#ifndef MACHINE_H_
#define MACHINE_H_

/*
 * This file specifies the hardware/software boundary, i.e. between the
 * (simulated) MIPS CPU hardware and the kernel software you must write.
 *
 * Since this interface is part of the MIPS hardware itself, you should not have
 * much reason to change anything in this file. Indeed, changing anything will
 * probably make your kernel not work on the existing simulated hardware.
 */

/* Kernel entry point has the following signature. It should never return. */
void __boot();

/* We are simulating a 1 MHz machine,
 * so 1,000,000 cycles per second,
 * or 1 cycle per usec,
 * or 1 usec per cycle.
 */
#define CPU_MHZ 1
#define CPU_CYCLES_PER_SECOND 1000000
#define CPU_CYCLES_PER_USEC 1
#define CPU_USEC_PER_CYCLES 1

/* Everything uses a 4k page size */
#define PAGE_SIZE 4096

/* Physical Address Space Layout 
 * =============================================================================
 *
 * The first page of the physical address space (physical address 0x00000000)
 * contains a block of boot parameters.  These consist of:
 *  - the device table, showing the layout of the physical address space
 *  - a count of RAM pages used by the bootloader
 *  - boot parameters passed on the simulator command line
 *
 * The boot parameters is just a list of fixed-length strings.
 *
 * The device table is a list of up to 16 entries. Each entry specifies a range
 * of physical addresses, and a type and model code indicating what lives at those
 * addresses. By default, 128 MB of RAM is simulated, but this can be changed on
 * the command line (up to 1GB). The default device table specifies a physical address space
 * layout that looks like this:
 *
 *  +--------------------+ 
 *  |                    | 0x00004000 + N * 4096 - 1
 *  |                    |
 *  |                    |
 *  |                    |
 *  |   RAM              | 0x00004000
 *  +--------------------+ 
 *  |                    | 0x00003FFF
 *  |                    |
 *  |                    | 
 *  |   network I/O      | 0x00003000
 *  +--------------------+ 
 *  |                    | 0x00002FFF
 *  |                    |
 *  |                    |
 *  |   console I/O      | 0x00002000
 *  +--------------------+ 
 *  |                    | 0x00001FFF
 *  |                    |
 *  |                    |
 *  |   keyboard I/O     | 0x00001000
 *  +--------------------+ 
 *  |                    | 0x00000FFF
 *  |   ROM (boot        |
 *  |      parameters)   | 
 *  |                    | 0x00000000
 *  +--------------------+ 
 *
 */
struct device_table_entry {
#define DEV_TYPE_EMPTY	   0x0000
#define DEV_TYPE_ROM	   0x0001
#define DEV_TYPE_RAM	   0x0003
#define DEV_TYPE_KEYBOARD  0x2210
#define DEV_TYPE_CONSOLE   0x1610
#define DEV_TYPE_NETWORK   0x3410
  unsigned int type;
  unsigned int model;
  unsigned int start; // inclusive
  unsigned int end;   // exclusive
};

struct bootparams {
  struct device_table_entry devtable[16]; // up to 16 device table entries
  unsigned int bootpages; // count of pages used by bootloader
  char argdata[16][128]; // up to 16 arguments, 127 characters each
};


/* Keyboard Device
 * =============================================================================
 *
 * Whenever the 'status' register is non-zero, keyboard data is available in the
 * 'data' register.  Whenever 'status' is non-zero, the keyboard device will
 * raise KBD_INTERRUPT on core 0. This will cause core 0 to trap so long as:
 *  - The interrupt enable (IE) bit of Core 0's Status register is 1.
 *  - The KBD_INTERRUPT bit of the interrupt mask (IM) field of Core 0's Status
 *    register is 1.
 */

struct dev_kbd {
  char status, pad1, pad2, pad3;
  char data, pad4, pad5, pad6;
};


/* Console Device
 * =============================================================================
 *
 * The console device has no interrupts, and only supports writing. Whenever a
 * character is written to 'data', it will be shown immediately on the console.
 */

struct dev_console {
  char data, pad1, pad2, pad3;
};


/* Network Device
 * =============================================================================
 *
 * The 'cmd' and 'data' registers are used together to configure and query the
 * network device.
 *
 * The 'rx_base' register specifies the physical address in RAM of the receive
 * ring. Driver should initialize this.
 *
 * The 'rx_capacity' register specifies the number of slots in the ring. It must
 * be either 1, 2, 4, 8, or 16. The size of the ring array is 8*rx_capacity
 * bytes, since each slot occupies 8 bytes.
 *
 * The 'rx_head' register specifies where the device will next look to find
 * an empty buffer. Specifically, it reads the the slot at index (rx_head mod
 * rx_capacity). The device will increment this pointer after every successful
 * packet reception. Eventually it will wrap around to zero due to arithmetic
 * overflow, but the (rx_head mod rx_capacity) calculation will work fine even
 * then.
 *
 * The device will never touch any ring entries except the one at (rx_head mod
 * rx_capacity), and then only when the ring is not full. It will modify rx_head
 * only by incrementing it, and will never modify rx_tail.
 *
 * The 'rx_tail' register specifies where the driver should look to find a
 * full buffer. Specifically, the driver should read the slot at index (rx_tail
 * mod rx_capacity). The driver should increment this pointer after it replaces
 * the full buffer in that slot with a fresh empty buffer. There is no need to
 * ever reset rx_tail back to zero, though it too will eventually and harmlessly
 * wrap around to zero due to arithmetic overflow.
 *
 * The driver should never touch any ring entries except the one at (rx_tail mod
 * rx_capacity), and then only when the ring is not empty. It should only modify
 * rx_tail by incrementing it, and should never modify rx_head except during
 * device initialization or when the device is turned off.
 *
 * The (rx_head == rx_tail), the ring is considered empty. Otherwise if the two
 * indexes point at the same slot, i.e. if (rx_head mod rx_capacity) == (rx_tail
 * mod rx_capacity)), then the ring is considered full. Note the careful
 * distinction between full and empty here: in both cases, the indexes point to
 * the same slot. To distinguish the two cases, the "non-modulo" values are
 * compared.
 *
 */

struct dev_net {
  unsigned int cmd;
  unsigned int data;
  unsigned int rx_base;
  unsigned int rx_capacity;
  unsigned int rx_head;
  unsigned int rx_tail;
  // tx ring omitted (you don't need it anyway)
};

struct dma_ring_slot {
  unsigned int dma_base;
  unsigned int dma_len;
};

#define NET_MAX_RING_CAPACITY 16 // ring capacity must be a power of 2, at most 16

#define NET_MINPKT 28   // the smallest packet contains only IP/UDP headers
#define NET_MAXPKT 4000 // the largest packet easily fits within a single page

/* These values are meant to be written into the 'cmd' register by the driver.
 * The driver should then follow that by writing into the 'data' register.
 * */
#define NET_SET_POWER      0x101 /* 1 to data turns on card, 0 turns off */
#define NET_SET_RECEIVE    0x102 /* 1 to data turns on reception DMA, 0 turns off */
//#define NET_SET_TRANSMIT   0x103 /* 1 to data turns on transmission DMA, 0 turns off */
#define NET_SET_INTERRUPTS 0x104 /* 1 to data turns on receive interrupts, 0 turns off */

/* These values are meant to be written into the 'cmd' register by the driver.
 * The driver should then follow that by reading from the 'data' register.
 * */
#define NET_GET_POWER      0x201 /* 1 from data means card is turned on */
#define NET_GET_RECEIVE    0x202 /* 1 from data means reception is turned on */
//#define NET_GET_TRANSMIT   0x203 /* 1 from data means transmission is turned on */
#define NET_GET_INTERRUPTS 0x204 /* 1 from data means receive interrupts are turned on */
#define NET_GET_DROPCOUNT  0x205 /* <n> from data means <n> received packets were dropped */


/* Virtual Address Space Layout 
 * =============================================================================
 *
 * The simulator implements a hardware virtual memory system that is a mix
 * of MIPS and a classic x86 system, with some modifications and simplifications. 
 *
 * The hardware uses a two level page table structure, as follows:
 * The Context Register (c0r4) contains a pointer to a 4096-byte page directory.
 * Each of the 4-byte page directory entries (PDEs) points to a 4096-byte page table.
 * Each of the 4-byte page table entries (PTEs) points to a 4096-byte page of data.
 * 
 * And address space is denoted by a 32-bit value suitable for writing into the
 * Context Register, which has the following format:
 * +----------------------------------------+---------------------+
 * |    physical page number                |     reserved        |
 * +----------------------------------------+---------------------+
 *    The upper 20 bits contain the physical page number of the page directory.
 *    The lower bits are reserved, and must be always be zero.
 *
 * A PDE has the following format:
 * +----------------------------------------+-----------------+---+
 * |    physical page number                |     unused      | V |
 * +----------------------------------------+-----------------+---+
 *    The upper 20 bits contain the physical page number of the page table.
 *    The low bit is 1 for "valid", or 0 for "invalid" .
 *    Other bits are ignored by the hardware, and so can be used for anything you like.
 *
 * A PTE has the following format:
 * +----------------------------------------+------+---+---+---+---+
 * |    physical page number                |unused| C | X | W | V |
 * +----------------------------------------+------+---+---+---+---+
 *    The upper 20 bits contain the physical page number of the page containing data.
 *    The low bits indicate "cache-disable", "executable", "writable", and
 *    "valid", respectively.
 *    Other bits are ignored by the hardware, and so can be used for anything you like.
 *
 * For sanity's sake, the simulator enforces a few additional restriction: all
 * pagetables must reside in RAM; the stack should reside in RAM; execution
 * should never happen on non-RAM pages; etc. The simulator will halt with an
 * error message if any of these restrictions are broken, e.g. if the Context
 * Register or any of the PDEs points at a non-RAM physical page.
 *
 * The simulator sets up some page tables for you before invoking the kernel's
 * entry point.  The default mappings are reasonable, but you can modify the
 * page tables however you like at any time (so long as you are careful about
 * modifying mappings that are being used, e.g. the ones that used to map the
 * kernel's code segment).
 *
 * The default mappings are created as follows. Every physically addressable
 * page mapped into the virtual address space starting at address 0xC0000000.
 * There is at most 1GB of physical addressable memory, so all of physical
 * memory fits in the range 0xC0000000 to 0xFFFFFFFF.  All of these pages
 * are created with W=1, X=0 permissions, except the ROM page, which has W=0,
 * X=0. Device pages have C=1, all others have C=0.
 *
 * Additional mappings are created for the kernel code, data, as specified in
 * the kernel ELF file. For instance, the kernel ELF file has a code segment
 * near virtual 0x80000000, so a few RAM pages are mapped a second time at this
 * address. 
 *
 * A few more pages are mapped somewhere below 0xC0000000 for a kernel stack
 * for each core. Each core will need to allocate its own trap stack.
 *
 * The total number of physical RAM pages used for this is written into the boot
 * parameter block. This count includes some number of pages for the kernel
 * code, data, and stack, plus additional pages for the pagetables.
 */


/* TLB Organization
 * =============================================================================
 *
 * The simulator does not implement a TLB. 
 */


/* Cache Organization
 * =============================================================================
 *
 * Both instruction fetch and data are assumed to use an infinitely large cache
 * with zero latency and perfect hardware cache coherence. Thus for this project,
 * you need not think about cache issues, with one exception: pages used for
 * memory-mapped device I/O should be marked as "cache-disable". This is done
 * for you in the existing memory maps.
 */

/*
 * These values are found in the error code (ECODE) field of the Cause register
 * whenever there is a trap.
 */
enum {
  ECODE_INT = 0,  /* external interrupt */
  ECODE_MOD,	  /* attempt to write to a non-writable page */
  ECODE_TLBL,	  /* page fault during load or instruction fetch */
  ECODE_TLBS,	  /* page fault during store */
  ECODE_ADDRL,	  /* unaligned address during load or instruction fetch */
  ECODE_ADDRS,	  /* unaligned address during store */
  ECODE_IBUS,	  /* instruction fetch bus error */
  ECODE_DBUS,	  /* data load/store bus error */
  ECODE_SYSCALL,  /* system call */
  ECODE_BKPT,	  /* breakpoint */
  ECODE_RI,	  /* reserved opcode */
  ECODE_OVF,	  /* arithmetic overflow */
  ECODE_NOEX,	  /* attempt to execute to a non-executable page */
};

/* These values define the bits of the interrupt mask (IM) field of the Status
 * register, and the bits of the interrupt pending (IP) field of the Cause
 * register. For instance, INTR_NETWORK = 5, so 5th bit of the IP field will be
 * set to 1 whenever there is a network interrupt pending. And network
 * interrupts can be ignored by setting the 5th bit of the IM field to 0.
 */
enum {
  INTR_SW0 = 0,
  INTR_SW1,
  INTR_HW0,
  INTR_HW1,
  INTR_HW2,
  INTR_NETWORK,
  INTR_KEYBOARD,
  INTR_TIMER,
};

#endif
