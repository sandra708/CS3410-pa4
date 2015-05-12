//a linked list data structure
//used as a queue of packets waiting some sort of processing

#ifndef ____list__
#define ____list__

#define NULL 0x00000000

struct packet_info{
    int lock;
    int status; //status values defined in pipeline.h
    int hash;
    unsigned int packet_length; //bytes in honeypot_command_packet
    struct packet_info* next; //packet in front of me
    struct packet_info* prev; //packet behind me
    struct honeypot_command_packet packet_start; //honeypot data start
};

struct list_header{
    int lock;
    int length; //number of elements in list -> 0 if empty
    struct packet_info* head;
    struct packet_info* tail;
};

//appends a packet to the list by modifying the list and the packet's info
//synchronized with a mutex
void append_list(volatile struct list_header *list, struct packet_info *packet);

//removes the head packet from the list, in FIFO order
//if the list is empty, spins in and out of lock until a packet is added
//synchronized with a mutex
struct packet_info* poll(volatile struct list_header *list);

//removes and returns the head packet if one exists, or NULL otherwise
//synchronized with a mutex
struct packet_info* get(volatile struct list_header *list);

//synchronization test
void test_sync(struct list_header *list, struct packet_info *arr, int size);

#endif
