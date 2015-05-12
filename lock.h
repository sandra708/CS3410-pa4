
#ifndef ____lock__
#define ____lock__

//mutex-locks on the memory location m 
//spins while m is not zero, and then atomically sets m to 0
void spin_lock(volatile int* m);

//unlocks a set mutex-lock
//NEVER unlock something that this thread did not already lock 
//ALWAYS remember to unlock resources as you are done with them
volatile int* unlock(volatile int *m);

#endif
