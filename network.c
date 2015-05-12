#include "kernel.h"


#define DMA_SIZE PAGE_SIZE


volatile struct dev_net *net_dev;

volatile struct list_header *garbage_list;//list of packets spaces that can be filled with new packet
volatile struct list_header *ring_buffer_list;//list of packets spaces in the ring buffer 
volatile struct list_header *hashing_buffer_list;//list of packets waiting to be hashed
volatile struct list_header *check_packet_buffer_list; //list of packets waiting to be checked for spam/vulnerable/evil/command

//struct dma_ring_slot* ring_buffer;//pointer to the base of the ring buffer
struct dma_ring_slot* ring_buffer_pipeline;//pointer to the base of the ring buffer

volatile unsigned int rx_buff;

volatile int total_vul=0;
volatile int total_evil=0;
volatile int total_spam=0;

volatile int total_packets=0;
volatile int time_start=0;
volatile int last_print=0;
volatile int bytes_handled=0;
volatile int * rx_buff_lock;


struct hashtable evil_hashtable;
struct hashtable vulnerable_hashtable;
struct hashtable spam_hashtable;





/* Initializes the network driver, allocating the space for the ring buffer and dma slots
    also creates eviil, vulnerable, and spam hashtables*/
/*void network_init(){
	int i,j;
	for(i=0; i<16; i++){
    	if(bootparams->devtable[i].type == DEV_TYPE_NETWORK){
			net_dev = physical_to_virtual(bootparams->devtable[i].start);//find the pointer to the network device 
			net_dev->cmd=NET_SET_POWER;//turn on device
			net_dev->data=1;
			printf("Network Device is on..\n");

			ring_buffer = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            int  temp=(int)ring_buffer;
            net_dev->rx_base=virtual_to_physical((void *)temp);
			net_dev->rx_tail=0;
			net_dev->rx_capacity=RING_SIZE;
			net_dev->rx_head=0;

			for (j=0; j< RING_SIZE; j++) { //malloc ring buffer slots
    			unsigned int* space = malloc(DMA_SIZE);
    			ring_buffer[j].dma_base = virtual_to_physical(space);
    			ring_buffer[j].dma_len = NET_MAXPKT;
			}

    	}
	}
    hashtable_create(&evil_hashtable,evil_hashtable_size,evil_bucket_size);
    hashtable_create(&vulnerable_hashtable,vulnerable_hashtable_size,vulnerable_bucket_size);
    hashtable_create(&spam_hashtable,spam_hashtable_size,spam_bucket_size);
}*/

void header_space_malloc(){
    garbage_list=malloc(sizeof(struct list_header));
    garbage_list->length=0;
    garbage_list->lock=0;
    //garbage_list->lock=(int *)malloc(sizeof(int *));
    //garbage_list->lock[0]=0;
    //garbage_list->pa
    //printf("header_space_malloc:\n");
     // printf("garbage_list %p\n",garbage_list);
     // printf("garbage_list length start %p\n",&garbage_list->length);
    //  printf("lock %p\n",&garbage_list->lock);
    ring_buffer_list=malloc(sizeof(struct list_header));
    ring_buffer_list->lock=0;
    ring_buffer_list->length=0;

    hashing_buffer_list=malloc(sizeof(struct list_header));
    hashing_buffer_list->lock=0;
    hashing_buffer_list->length=0;

    check_packet_buffer_list=malloc(sizeof(struct list_header));
    check_packet_buffer_list->lock=0;
    check_packet_buffer_list->length=0;
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
        //printf("i %d self %p next %p pktstart %p\n",i,me,before,&me->packet_start );
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
   // printf("b %p\n",garbage_list->lock);
   // printf("b %d\n",*garbage_list->lock);
   // printf("b %p\n",&garbage_list->lock);
   // printf("initial_dma_ring_slot_init:\n");
    //int gl = (int) &garbage_list->lock;
    /*
    spin_lock(&garbage_list->lock);
    int i=0;
    while( i < RING_SIZE && garbage_list->length>0){
        spin_lock(&garbage_list->head->lock);
        if(garbage_list->head->status==IN_GARBAGE_LIST){
            ring_buffer_pipeline[i].dma_len=NET_MAXPKT;
            garbage_list->head->status=IN_RING_BUFFER;
            //printf("i %d glis %p\n",i,garbage_list->head );
            ring_buffer_pipeline[i].dma_base=virtual_to_physical(&garbage_list->head->packet_start);
            int *temp = &garbage_list->head->lock;
            garbage_list->head=garbage_list->head->prev;
            unlock(temp);
            garbage_list->length--;
            i++;    
        }
        else{
            unlock(&garbage_list->head->lock);
            unlock(&garbage_list->lock);
            //printf("Packet at %p has incorrect status for adding to ring buffer. ", garbage_list->head);
            //printf("Only allocated %d/%d ring slots, Sincerely, core %d\n", i,RING_SIZE, current_cpu_id());
            net_dev->rx_head=i-1;
            return i;
        }

    }
    unlock(&garbage_list->lock);*/
    
    /*
    skeleton for refactored loop?*/
    int i = 0;
    while(i < RING_SIZE){
        printf("Allocating %dth ring slot. ", i);
        struct packet_info* next = poll(garbage_list); //if we allocated less than 16 packets initially, we won't ever be able to process anything
        printf("Pointer %p.\n", next);
        next->status = IN_RING_BUFFER;
        ring_buffer_pipeline[i].dma_base=virtual_to_physical(&next->packet_start);
        i++;
    }

    //printf("Succesfully allocated %d/%d ring slots, Sincerely, core %d\n", i,RING_SIZE, current_cpu_id());
   // printf("ring_buffer %p\n",ring_buffer_pipeline );
    /*for (int j = 0; j < i; j++)
    {
        //printf("dma start %p\n",&ring_buffer_pipeline[j] );
        //printf("ring_buffer[%d]=%p\n",j,physical_to_virtual(ring_buffer_pipeline[j].dma_base));
    }*/
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
            printf("Network Device is on..\n");
            ring_buffer_pipeline = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            int temp=(int)ring_buffer_pipeline;
            net_dev->rx_base=virtual_to_physical((void *)temp);
            net_dev->rx_tail=0;
            
            net_dev->rx_head=0;
            rx_buff=3;

        }
    }

    header_space_malloc();
    printf("Space for linked list headers allocated...\n");//
    garbage_list_alloc(MAX_PACKETS);//
    printf("Packet space allocated..enough space for %d\n",MAX_PACKETS);
    int spots= initial_dma_ring_slot_init();//
    printf("Succesfully added %d spots to the ring_buffer\n", spots);
    net_dev->rx_capacity=spots;
    printf("start rx_tail %d\n",net_dev->rx_tail );
    hashtable_create(&evil_hashtable,evil_hashtable_size,evil_bucket_size);
    hashtable_create(&vulnerable_hashtable,vulnerable_hashtable_size,vulnerable_bucket_size);
    hashtable_create(&spam_hashtable,spam_hashtable_size,spam_bucket_size);
}

void network_start_receive(){
    net_dev->cmd=NET_SET_RECEIVE;//enable receive 
    net_dev->data=1;
    printf("Network receive enabled..\n");
    time_start=current_cpu_cycles();
    last_print=time_start;
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

void check_packet(struct honeypot_command_packet * packet, int n){
    unsigned int src_addr=packet->headers.ip_source_address_big_endian;
    unsigned int des_addr=packet->headers.udp_dest_port_big_endian<<16;
    unsigned int hash2=djb2((void *)packet , n);

    if(src_addr==hashtable_get(&spam_hashtable,src_addr)){
        total_spam++;
    }
    if(des_addr==hashtable_get(&vulnerable_hashtable,des_addr)){
        total_vul++;
    }
    if(hash2==hashtable_get(&evil_hashtable,hash2) ){
        total_evil++;
    }
}

int max(int x, int y, int z){
    if(x>=y && x>=z) return x;
    else if (y>=z ) return y;
    else return z;
}

void simple_stats_print(){
    int cycle=current_cpu_cycles();
    int secs1=(cycle-time_start)/CPU_CYCLES_PER_SECOND;
    //int secs2=(cycle-last_print)/CPU_CYCLES_PER_SECOND;
    //last_print=cycle;
    int r1=total_packets/secs1;
    int r2=bytes_handled/secs1;
    //int r3=total_packets/(secs2+1);
    //int r4=bytes_handled/(secs2+1);
    printf("----------totals---------\n");
    printf("Totals: %d packets %d bytes\n",total_packets,bytes_handled );
    printf("Speed: ~%d packets/sec ~%d bytes/sec\n", r1, r2 );
    //printf("Since Last Print: ~%d packets/sec ~%d bytes/sec\n", r3, r4 );
    printf("Evil %d Vulnerable %d Spam %d\n", total_evil,total_vul,total_spam);
    printf("-------------------------\n");
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
    simple_stats_print();
}

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

// Continually polls for data on the ring buffer until the

void network_poll(){
    //printf("polling..\n" );
    struct dma_ring_slot* curr;
    while(1){
        while(net_dev->rx_head != net_dev->rx_tail ){
            curr = &ring_buffer_pipeline[net_dev->rx_tail % RING_SIZE];
            execute_ringbuffer_stage(garbage_list, curr, hashing_buffer_list);
            net_dev->rx_tail++;
        }
    }
}

// Called when a network interrupt occurs.
void network_trap(){

    all_print();
}

void execute_command_pipeline(struct honeypot_command_packet * packet){
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

}

/* Checks if the packet is evil, vulnerable, or spam
    Returns the correct code defined above and puts the packet info into
    the correct hashtable if necessary.*/
int check_packet_pipeline(struct honeypot_command_packet * packet, int hash){
    unsigned int src_addr=packet->headers.ip_source_address_big_endian;
    unsigned int des_addr=packet->headers.udp_dest_port_big_endian<<16;
    int code=0;
    if(src_addr==hashtable_get(&spam_hashtable,src_addr)){
        total_spam++;
        code+=is_spammer;
    }
    if(des_addr==hashtable_get(&vulnerable_hashtable,des_addr)){
        total_vul++;
        code+=is_vulnerable;
    }
    if(hash==hashtable_get(&evil_hashtable,hash) ){
        total_evil++;
        code+=is_evil;

    }
    return code;
}

/*Updates all statistics according to that packet*/
void update_stats(struct packet_info *current_packet){
    total_packets++;
    bytes_handled+=current_packet->packet_length;
}

void core_start(int core_id){

    if(core_id==0){
        while(1);
    }
    /*else if(core_id==1){

        while(1){
            //printf("executing glts core:%d\n",current_cpu_id() );
            execute_garbage_list_transfer_stage(net_dev, garbage_list, ring_buffer_pipeline, rx_buff);
        }
    }
    else if(core_id==2){
        //busy_wait(0.1);
        //add_first_packet_hashbuffer_list(ring_buffer_pipeline, hashing_buffer_list, rx_buff,net_dev->rx_head);
        while(1){
            //aquire lock for rx_buff 
            rx_buff=execute_remove_from_ring_buffer(ring_buffer_pipeline, hashing_buffer_list, rx_buff,net_dev->rx_head);
//printf("rbl %d %d %d |%d <%d <%d \n",hashing_buffer_list->length,check_packet_buffer_list->length,garbage_list->length,net_dev->rx_head,net_dev->rx_tail,rx_buff );
        }
    }*/
    else if (core_id == 1){
        network_poll();
    }
    else if (core_id == 2){
        while(1){
            //busy_wait(0.4);
            execute_checking_stage(check_packet_buffer_list, garbage_list);
        }
    } 
    else{
        while(1)
             execute_hashing_stage(hashing_buffer_list, check_packet_buffer_list);
    }
}



