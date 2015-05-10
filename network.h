
#ifndef ____network__
#define ____network__

//#include <stdio.h>

#define MAX_PACKETS 240
#define RING_SIZE 16

// Initializes the network driver, allocating the space for the ring buffer.
void network_init();

// Starts receiving packets!
void network_start_receive();

// If opt != 0, enables interrupts when a new packet arrives.
// If opt == 0, disables interrupts.
void network_set_interrupts(int opt);

unsigned int change_end(unsigned int data);

unsigned long djb2(unsigned char *pkt, int n);

// Continually polls for data on the ring buffer. Loops forever!
void network_poll();


void evil_print();
void vulnerable_print();
void spam_print();
void simple_stats_print();
void all_print();

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
    int hash;
    unsigned int packet_length;
    struct packet_info* next;
    struct honeypot_command_packet * packet_start;
};

//we want two of these, one for packets outgoing to process,
//the other for incoming garbage addresses to core 0
struct list_header{
    int lock;
    int length; //number of elements in list -> 0 if empty
    struct packet_info* head;
    struct packet_info* tail;
};


//requests a lock
void append_list(struct list_header *list, struct packet_info *packet);

//requests lock - remains in the method until list has an element to remove
struct packet_info* poll(struct list_header *list);

//synchronization test
void test_sync(struct list_header *list, struct packet_info *arr, int size);

#endif /* defined(____network__) */
