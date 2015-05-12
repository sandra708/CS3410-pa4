#include "kernel.h"


#define DMA_SIZE PAGE_SIZE


volatile struct dev_net *net_dev;

volatile struct list_header *garbage_list;//list of packets spaces that can be filled with new packet 
volatile struct list_header *hashing_buffer_list;//list of packets waiting to be hashed
volatile struct list_header *check_packet_buffer_list; //list of packets waiting to be checked for spam/vulnerable/evil/command

//struct dma_ring_slot* ring_buffer;//pointer to the base of the ring buffer
volatile struct dma_ring_slot* ring_buffer_pipeline;//pointer to the base of the ring buffer

volatile struct global_stats* stats;

volatile struct hashtable evil_hashtable;
volatile struct hashtable vulnerable_hashtable;
volatile struct hashtable spam_hashtable;

void header_space_malloc(){
    garbage_list=malloc(sizeof(struct list_header));
    garbage_list->length=0;
    garbage_list->lock=0;

    hashing_buffer_list=malloc(sizeof(struct list_header));
    hashing_buffer_list->lock=0;
    hashing_buffer_list->length=0;

    check_packet_buffer_list=malloc(sizeof(struct list_header));
    check_packet_buffer_list->lock=0;
    check_packet_buffer_list->length=0;

    //all stats should start off 0
    stats = (struct global_stats*) calloc(sizeof(struct global_stats), 1);
}

/* Allocates pages for linked list of num_packets packets assumes at least
    space for 1 packet is needed.
    Also finishes instantiating garbage_list with all of these packets*/
void garbage_list_alloc(int num_packets){
    struct packet_info * first=alloc_pages(1);
    first->lock=0;
    first->status=IN_GARBAGE_LIST;
    struct packet_info * before=first;
    struct packet_info * me;
    for (int i = 1; i < num_packets; i++){
        me=alloc_pages(1);
        me->lock=0;
        me->status=IN_GARBAGE_LIST;
        me->packet_length=i;//set to 0 later
        before->prev=me; //packet before me has me as its previous
        me->next=before; //my next is the packet before me
        //printf("i %d self %x v %p\n",i,virtual_to_physical(me),me );
        before=me; 
    }

    garbage_list->head=before;
    garbage_list->tail=first;
    garbage_list->length=num_packets;
}

/* assumes you have malloced a garbage list and the ring buffer
    returns the number of ring slots it was able to succesfully
    allocate.
*/ 
int initial_dma_ring_slot_init(){
   
    
    /*
    skeleton for refactored loop?*/
    int i = 0;
    while(i < RING_SIZE){

        struct packet_info* next = poll(garbage_list); //if we allocated less than 16 packets initially, we won't ever be able to process anything
        next->status = IN_RING_BUFFER;
        //printf("Put in rb %d th ring slot. %x , %p ,", i,virtual_to_physical(next),next);
        ring_buffer_pipeline[i].dma_base=virtual_to_physical(&next->packet_start);
        i++;
    }
    return i;    
}

/* Initializes the network driver, allocates the space for the ring buffer
    also creates eviil, vulnerable, and spam hashtables*/
void network_init_pipeline(){
    int i;
    for(i=0; i<16; i++){
        if(bootparams->devtable[i].type == DEV_TYPE_NETWORK){
            net_dev = physical_to_virtual(bootparams->devtable[i].start);//find the pointer to the network device 
            net_dev->cmd=NET_SET_POWER;//turn on device
            net_dev->data=1;
            //printf("Network Device is on..\n");
            ring_buffer_pipeline = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            int temp=(int)ring_buffer_pipeline;
            net_dev->rx_base=virtual_to_physical((void*)temp);
            net_dev->rx_tail=0;
            
            net_dev->rx_head=0;

        }
    }

    header_space_malloc();
    printf("Space for linked list headers allocated...\n");//
    garbage_list_alloc(MAX_PACKETS);//
    printf("Packet space allocated..enough space for %d\n",MAX_PACKETS);
    int spots= initial_dma_ring_slot_init();//
    printf("Succesfully added %d spots to the ring_buffer\n", spots);
    net_dev->rx_capacity=spots;
    hashtable_create(&evil_hashtable,evil_hashtable_size,evil_bucket_size);
    hashtable_create(&vulnerable_hashtable,vulnerable_hashtable_size,vulnerable_bucket_size);
    hashtable_create(&spam_hashtable,spam_hashtable_size,spam_bucket_size);
}

void network_start_receive(){
    net_dev->cmd=NET_SET_RECEIVE;//enable receive 
    net_dev->data=1;
    printf("Network receive enabled..\n");
    stats->time_start=current_cpu_cycles();
    stats->last_print=stats->time_start;
}

void network_set_interrupts(int opt){
	net_dev->cmd=NET_SET_INTERRUPTS;//enable interuppts
	net_dev->data=1;
}

/*  Determines if the packet is a command packet
    returns 0 if not command packet
    returns 1 if command packet*/
int is_command(struct honeypot_command_packet*  packet){
    if(packet->secret_big_endian==secret_little_endian)
        return 1;
    else
        return 0;
}

/*change endianess of an int
taken from stackoverflow*/
unsigned int change_end(unsigned int data){

    return( ( data >> 24 ) | (( data << 8) & 0x00ff0000 )| ((data >> 8) & 0x0000ff00) | ( data << 24)  );
}

unsigned long djb2(unsigned char *pkt, int n){
  unsigned long hash = 5381;
  int c;
  for (int i = 0; i < n; i++) {
    c = pkt[i];
    hash = hash * 33 + c;
    //hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

int max(int x, int y, int z){
    if(x>=y && x>=z) return x;
    else if (y>=z ) return y;
    else return z;
}



void evil_print(){
    printf("------------------\n");
    printf("-------evil-------\n");
    printf("------------------\n");
    hashtable_elements_print(&evil_hashtable);
}

void vulnerable_print(){
    printf("------------------\n");
    printf("----vulnerable----\n");
    printf("------------------\n");
    hashtable_elements_print(&vulnerable_hashtable);
}

void spam_print(){
    printf("------------------\n");
    printf("-------spam-------\n");
    printf("------------------\n");
    hashtable_elements_print(&spam_hashtable);
}

void all_print(){

    spam_print();
    vulnerable_print();
    evil_print();
    simple_stats_print(stats);
}
/*
void execute_command(struct honeypot_command_packet * packet, int n){
    unsigned short command = packet->cmd_big_endian;
    unsigned int data =packet->data_big_endian;

    if(command==print_stats){
        network_trap();
    }
    else if(command==add_spammer){
        hashtable_put(&spam_hashtable,data,spam_bucket_size);
    }
    else if(command==add_vulnerable){
        hashtable_put(&vulnerable_hashtable,data,vulnerable_bucket_size);
    }
    else if(command==add_evil_m){
        hashtable_put(&evil_hashtable,change_end(data),evil_bucket_size);
    }
    else if(command==del_spammer){
        hashtable_remove(&spam_hashtable,data);
    }
    else if(command==del_vulnerable){
        hashtable_remove(&vulnerable_hashtable,data);
    }
    else if(command==del_evil){
        hashtable_remove(&evil_hashtable,change_end(data));
    }

    check_packet(packet,n);
}
*/
// Continually polls for data on the ring buffer until the
void network_poll(){

    //printf("polling..\n" );
   volatile struct dma_ring_slot *curr;
    while(1){
        while(net_dev->rx_head != net_dev->rx_tail ){
            //printf("Removing a packet.\n");
            curr=&ring_buffer_pipeline[net_dev->rx_tail%RING_SIZE];
            execute_ringbuffer_stage(garbage_list, curr, hashing_buffer_list);
            net_dev->rx_tail++;   
        }
    }
}

// Called when a network interrupt occurs.
void network_trap(){

   // all_print();
}

void network_stats_print(){
    simple_stats_print(stats);
}

void execute_command_pipeline(struct honeypot_command_packet *packet){
    unsigned short command = packet->cmd_big_endian;
    unsigned int data =packet->data_big_endian;

    if(command==print_stats){
        //network_trap();
        all_print();
    }
    else if(command==add_spammer){
        hashtable_put(&spam_hashtable,data,spam_bucket_size);
    }
    else if(command==add_vulnerable){
        hashtable_put(&vulnerable_hashtable,data,vulnerable_bucket_size);
    }
    else if(command==add_vulnerable){
        hashtable_put(&vulnerable_hashtable,data,vulnerable_bucket_size);
    }
    else if(command==add_evil_m){
        hashtable_put(&evil_hashtable,change_end(data),evil_bucket_size);
    }
    else if(command==del_spammer){
        hashtable_remove(&spam_hashtable,data);
    }
    else if(command==del_vulnerable){
        hashtable_remove(&vulnerable_hashtable,data);
    }
    else if(command==del_vulnerable){
       hashtable_remove(&vulnerable_hashtable,data);
    }
    else if(command==del_evil){
        hashtable_remove(&evil_hashtable,change_end(data));
    }
  
}


int check_packet_pipeline(struct honeypot_command_packet* packet, int hash){
    unsigned int src_addr = packet->headers.ip_source_address_big_endian;
    unsigned int des_addr = packet->headers.udp_dest_port_big_endian<<16;
    int code=0;
    if(hashtable_increment(&spam_hashtable,src_addr)){
        code = code | is_spammer;
    }
    if(hashtable_increment(&vulnerable_hashtable,des_addr)){
        code = code | is_vulnerable;
    }
    if(hashtable_increment(&evil_hashtable,hash)){
        code = code | is_evil;
    }
    return code;
}

void core_start(int core_id){

    if(core_id==0){
        while(1);
    }
    else if (core_id == 1){
        busy_wait(1);
        network_poll();
    }
    else if (core_id == 2){
            busy_wait(1);

        while(1){

            execute_checking_stage(check_packet_buffer_list, garbage_list, stats);
        }
    } 
    else{
        busy_wait(1);
        while(1)
             execute_hashing_stage(hashing_buffer_list, check_packet_buffer_list);
    }
}



