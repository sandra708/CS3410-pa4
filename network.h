
#ifndef ____network__
#define ____network__

//#include <stdio.h>

// Initializes the network driver, allocating the space for the ring buffer.
void network_init();

// Starts receiving packets!
void network_start_receive();

// If opt != 0, enables interrupts when a new packet arrives.
// If opt == 0, disables interrupts.
void network_set_interrupts(int opt);

// Continually polls for data on the ring buffer. Loops forever!
void network_poll();

// Called when a network interrupt occurs.
void network_trap();
//one per memory slot allocated for a packet
//status 0 means (packet) is in the garbage train
//status 1 means it is in the ringbuffer (empty or full)
//status 2 means it is being processed by core 0
//status 4 means it is waiting for a worker
//status 8 means it is being processed by a worker
struct packet_info{
    int lock;
    int status;
    int* packet;
    int* next;
};

//we want two of these, one for packets outgoing to process,
//the other for incoming garbage addresses to core 0
struct list_header{
    int lock;
    int length; //number of elements in list -> 0 if empty
    struct packet_info* head;
    struct packet_info* tail;
};

//we simply allocate enough memory (1 MB total) so that the source/dest ports don't need to be hashed
struct port_table_entry{
    int lock; //also indicates if this port is in the list to keep track of
    unsigned int num_accesses; //atomically incremented
};

//linear probe hashtable, no expansion (would chaining perform better? many DNE accesses)
struct evil_table_entry{
    int lock;
    int hash_val; //the djb hash of the packet
    unsigned int num_accesses;
};

//requests a lock
void append_list(struct list_header *list, struct packet_info *packet);

//requests lock - remains in the method until list has an element to remove
struct packet_info* poll(struct list_header *list);

//synchronization test
void test_sync(struct list_header *list, struct packet_info *arr, int size);

//adds port to the list of stuff kept track of (locks entry)
void add_port(struct port_table_entry *port);

//removes port from list (locks entry)
void remove_port(struct port_table_entry *port);

//increments access list at this entry only if it is on the list of ports (locks entry)
void increment_port(struct port_table_entry *port);

//adds hash to the list of evils (locks entry)
//table should be the root address of the whole block
void add_evil(struct evil_table_entry *table, unsigned int hash);

//removes hash from list (locks entry)
void remove_evil(struct evil_table_entry *table, unsigned int hash);

//increments access list at this entry only if it is on the list of evils (locks entry)
void increment_evil(struct evil_table_entry *table);

#endif /* defined(____network__) */
//>>>>>>> 10ef9773c9a202cd26b82c44a341082a07fc8e76
