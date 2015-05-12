
#ifndef ____network__
#define ____network__

#include "list.h"

#define MAX_PACKETS 500
#define RING_SIZE 16
#define secret_little_endian 0x1034

#define add_spammer 0x0101
#define add_evil_m 0x0201
#define add_vulnerable 0x0301

#define del_spammer 0x0102
#define del_evil 0x0202
#define del_vulnerable 0x0302

#define print_stats 0x0103

#define  evil_hashtable_size 40
#define  vulnerable_hashtable_size 40
#define  spam_hashtable_size 40

#define evil_bucket_size 3
#define spam_bucket_size 3
#define vulnerable_bucket_size 3

/*codes to define the packet type in check_packet_pipeline();
neutral=0   spam=1  evil=2 vulnerable=4
spam&evil=3 spam&vulnerable=5 evil&vulnerable=6
spam&evil&vulnerable=7
*/
#define is_neutral 0
#define is_spammer 1
#define is_evil 2
#define is_vulnerable 4

// Initializes the network driver, allocating the space for the ring buffer.
void network_init();

/* Initializes the network driver, allocates the space for the ring buffer.
    Allocates linked list header files, packet spaces.
    Fills the ring buffer with empty packet spaces.
    Also creates eviil, vulnerable, and spam hashtables*/
void network_init_pipeline();

// Starts receiving packets!
void network_start_receive();

unsigned long djb2(unsigned char *pkt, int n);

// Continually polls for data on the ring buffer. Loops forever!
void network_poll();

/*stats printing functions*/
void evil_print();
void vulnerable_print();
void spam_print();
void simple_stats_print();
void all_print();

// Called when a network interrupt occurs.
void network_trap();

void network_stats_print();

/* Enable network receiving. Also records the initial 
    cycle for speed statistics calculations*/
void network_start_receive();

void network_start_receive_pipeline();

void network_set_interrupts(int opt);

//hash function
unsigned long djb2(unsigned char *pkt, int n);

/*Continually polls the ring buffer until it finds a packet
   Completely and wonderfully unsyncronized*/
void network_poll();

/*Checks if the packet is a command packet and executes the command if it is*/
void execute_command_pipeline(struct honeypot_command_packet * packet);

/* Checks if the packet is evil, vulnerable, or spam
    Returns the correct code defined above and puts the packet info into
    the correct hashtable if necessary.*/
int check_packet_pipeline(struct honeypot_command_packet * packet, int hash);

//assigns each core to its pipeline stage
void core_start(int core_id);



#endif
