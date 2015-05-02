#ifndef KEYBOARD_H_
#define KEYBOARD_H_

/* Keyboard device driver interface.
 *
 * The keyboard device driver provides two functions:
 *   keyboard_init() initializes the driver
 *   keyboard_trap() must call this on every keyboard interrupt
 *
 * The driver does not implement any functions for reading characters, or any
 * shared datastructure to store previously pressed characters. Instead the
 * keyboard_trap handler just prints the character to the screen and discards
 * it.
 *
 * You could modify the keyboard driver to do some other action, or to put the
 * characters it reads into a shared datastructure of some kind. For example, it
 * could buffer characters until it sees a newline, then copy the buffer to some
 * (concurrency-safe) list of "input line" strings. Or it could just put all
 * characters into a (concurrency-safe) ring buffer.
 * 
 * Note: The (simulated, and real) keyboard device has a small internal buffer
 * implemented in hardware.  If the user presses several keys quickly, the
 * device will store at least a few of them until the driver has a chance to
 * read them. So unless the keyboard interrupt is ignored for a long time, the
 * driver should see every key the user presses.
 */

// detect the keyboard device, and initialize the keyboard driver
void keyboard_init();

// the exception handler should call this when a keyboard interrupt is detected
// note: interrupts should be off when calling this function
void keyboard_trap();

#endif
