#ifndef CONSOLE_H_
#define CONSOLE_H_

/* Console device driver interface.
 *
 * The console device driver provides two functions:
 *   console_init() initializes the driver
 *   putchar() outputs one character to the console
 *   puts() outputs a sequence of characters to the console
 * 
 * These functions are only minimally concurrency-safe: they should not crash,
 * even if called from inside or outside interrupt handlers, or from multiple
 * cores.
 *
 * However, they are also completely unsynchronized: if different cores are
 * printing strings to the console at the same time, e.g. by calling puts() or
 * by repeatedly calling putchar(), the output characters will be interleaved in
 * some random order. Similarly, if one core is mid-way through printing a
 * string, interrupts can arrive and print other characters to the console.
 *
 * If you want better synchronization, you can modify these, or have the calling
 * functions implement synchronization. For example, printf_m takes a mutex
 * before repeatedly calling putchar, so that even if multiple cores call
 * printf_m we can be sure the outputs from printf_m will not be interleaved
 * with each other. Of course, the outputs from printf_m can still be
 * interleaved with outputs from other functions that call putchar directly.
 */

// detect the console device, and initialize the console driver
void console_init();

// write one character to the console
int putchar(int c);

// write a sequence of characters to the console
int puts(char *s);

#endif
