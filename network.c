#include "kernel.h"


#define DMA_SIZE PAGE_SIZE


volatile struct dev_net *net_dev;

volatile struct list_header *garbage_list;//list of packets spaces that can be filled with new packet
volatile struct list_header *ring_buffer_list;//list of packets spaces in the ring buffer 
volatile struct list_header *hashing_buffer_list;//list of packets waiting to be hashed
volatile struct list_header *check_packet_buffer_list; //list of packets waiting to be checked for spam/vulnerable/evil/command

volatile struct dma_ring_slot* ring_buffer;//pointer to the base of the ring buffer
volatile struct dma_ring_slot* ring_buffer_pipeline;//pointer to the base of the ring buffer

unsigned short secret_little_endian=0x1034;

unsigned short add_spammer=0x0101;
unsigned short add_evil_m=0x0201;
unsigned short add_vulnerable=0x0301;

unsigned short del_spammer=0x0102;
unsigned short del_evil=0x0202;
unsigned short del_vulnerable=0x0302;

unsigned short print_stats=0x0103;

//int list_sizes=20;

struct hashtable evil_hashtable;
struct hashtable vulnerable_hashtable;
struct hashtable spam_hashtable;

int evil_hashtable_size=40;
int vulnerable_hashtable_size=40;
int spam_hashtable_size=40;

int evil_bucket_size=3;
int spam_bucket_size=3;
int vulnerable_bucket_size=3;

int total_vul=0;
int total_evil=0;
int total_spam=0;

int total_packets=0;
int time_start=0;
int last_print=0;
int bytes_handled=0;

// Initializes the network driver, allocating the space for the ring buffer.
void network_init(){
	int i,j;
	for(i=0; i<16; i++){
    	if(bootparams->devtable[i].type == DEV_TYPE_NETWORK){
			net_dev = physical_to_virtual(bootparams->devtable[i].start);//find the pointer to the network device 
			net_dev->cmd=NET_SET_POWER;//turn on device
			net_dev->data=1;
			printf("Network Device is on..\n");

			ring_buffer = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            net_dev->rx_base=virtual_to_physical(ring_buffer);
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
}

void network_init_pipeline(){
    int i,j;
    for(i=0; i<16; i++){
        if(bootparams->devtable[i].type == DEV_TYPE_NETWORK){
            net_dev = physical_to_virtual(bootparams->devtable[i].start);//find the pointer to the network device 
            net_dev->cmd=NET_SET_POWER;//turn on device
            net_dev->data=1;
            printf("Network Device is on..\n");
            ring_buffer_pipeline = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            net_dev->rx_base=virtual_to_physical(ring_buffer_pipeline);
            net_dev->rx_tail=0;
            net_dev->rx_capacity=RING_SIZE;
            net_dev->rx_head=0;
            net_dev->rx_buff=0;

        }
    }
    hashtable_create(&evil_hashtable,evil_hashtable_size,evil_bucket_size);
    hashtable_create(&vulnerable_hashtable,vulnerable_hashtable_size,vulnerable_bucket_size);
    hashtable_create(&spam_hashtable,spam_hashtable_size,spam_bucket_size);
}

/* assumes you have malloced a garbage list and the ring buffer
    returns the number of ring slots it was able to succesfully
    allocate.
*/
int initial_dma_ring_slot_init(){
    spin_lock(&garbage_list->lock);
    int i=0;
    while( i < RING_SIZE && garbage_list->length>0){
        struct packet_info pckt=garbage_list->head;
        spin_lock(&pckt->lock);
        if(pckt.status==IN_GARBAGE_LIST){
            ring_buffer_pipeline[i].dma_len=NET_MAXPKT;
            pckt.status=IN_RING_BUFFER;
            ring_buffer_pipeline[i].dma_base=pckt.packet_start;
            unlock(&pckt->lock);
            garbage_list->length--;
            garbage_list->head=pckt.next;
            i++;    
        }
        
        else{
            unlock(&pckt->lock);
            printf("Packet at %p has incorrect status for adding to ring buffer. ", pckt);
            printf("Only allocated %d ring slots, Sincerely, core %d\n", i, current_cpu_id());
            net_dev->rx_head=i;
            return i;
        }
    }
    unlock(&garbage_list->lock);
    printf("Succesfully allocated %d ring slots, Sincerely, core %d\n", RING_SIZE, current_cpu_id());
    net_dev->rx_head=RING_SIZE;
    return RING_SIZE;    
}

/* allocates pages for linked list of packets assumes at least
    space for 1 packet is needed
    also finishes instantiating garbage_list
*/
void garbage_list_alloc(int num_packets){
    struct packet_info * first=alloc_pages(1);;
    first->lock=0;
    first->status=IN_GARBAGE_LIST;
    struct packet_info * old=firstpacket;
    garbage_list->head=first

   for (int i = 0; i < num_packets-1; i++){
       struct packet_info * new=alloc_pages(1);
        pi->lock=0;
        pi->status=IN_GARBAGE_LIST;
        pi->next=old; //set the next packet as the old packet
        old=new; //set self as the old packet
   }

   garbage_list->tail=old;
   garbage_list->length=num_packets;
}

void header_space_malloc(){
    struct list_header *garbage_list=malloc(sizeof(struct list_header));
    garbage_list->lock=0;
    garbage_list->length=0;
    struct list_header *ring_buffer_list=malloc(sizeof(struct list_header));
    ring_buffer_list->lock=0;
    ring_buffer_list->length=0;
    struct list_header *hashing_list=malloc(sizeof(struct list_header));
    hashing_buffer_list->lock=0;
    hashing_buffer_list->lock=0;
    struct list_header *check_packet_list=malloc(sizeof(struct list_header));
    check_packet_buffer_list->lock=0;
    check_packet_buffer_list->length=0;
}

void network_start_receive(){
        net_dev->cmd=NET_SET_RECEIVE;//enable receive 
        net_dev->data=1;
        printf("Network receive enabled..\n");
        time_start=current_cpu_cycles();
        last_print=time_start;
}

// Starts receiving packets!
void network_start_receive_pipeline(){
		net_dev->cmd=NET_SET_RECEIVE;//enable receive 
		net_dev->data=1;
        time_start=current_cpu_cycles();
        last_print=time_start;
        printf("Network receive enabled..\n");

        header_space_malloc();
        printf("Space for linked list headers allocated...\n");

        garbage_list_alloc(MAX_PACKETS);
        printf("Packet space allocated..space for %d\n",MAX_PACKETS);
}

void network_set_interrupts(int opt){
	net_dev->cmd=NET_SET_INTERRUPTS;//enable interuppts
	net_dev->data=1;
}

/*  Determines if the packet is a command packet
    returns 0 if not command packet
    returns 1 if command packet*/
int is_command(struct honeypot_command_packet*  packet ){
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

    printf("polling..\n" );
    struct dma_ring_slot *curr;
    while(1){
        while(net_dev->rx_head!=net_dev->rx_tail ){
            curr=&ring_buffer[net_dev->rx_tail%RING_SIZE];
            bytes_handled+=curr->dma_len;
            total_packets++;
            execute_command(physical_to_virtual(curr->dma_base),curr->dma_len);

            curr->dma_len=NET_MAXPKT;
            net_dev->rx_tail++;   
        }
    }
}

// Called when a network interrupt occurs.
void network_trap(){
    all_print();
}



inline void spin_lock(int* m){
    register int *lockaddr asm("t0") = m;
    register int cond asm("t1");
    asm __volatile__ (
        ".set mips2"
        "try:"
        "ll $t1, 0($t0)"
        "bne $t1, $0, try"
        "addiu $t1, $0, 1"
        "sc $t1, 0($t0)"
        "bne $t1, $0, try"
                      );
    //printf_m("Core %d acquired lock.\n", current_cpu_id());
    printf("Core %d acquired lock.\n", current_cpu_id() );
}

inline void unlock(int* m){
    m = 0;
}


//requests a lock
void append_list(struct list_header *list, struct packet_info *packet){
    spin_lock(&(list->lock));
    
    //(list->tail)->next = packet;
    list->tail = packet;
    (list->length)++;
    //printf_m("Core %d added packet at %d.\n", current_cpu_id(), packet);
    //printf_m("After appending, list is %d elements long.\n", list->length);
    printf("Core %d added packet at %d.\n", current_cpu_id(), packet);
    printf("After appending, list is %d elements long.\n", list->length);
    unlock(&(list->lock));
}

//requests lock - remains in the method until list has an element to remove
struct packet_info* poll(struct list_header *list){
    while(1){
        spin_lock(&(list->lock));
        if(list->length) break; //list is non-empty
        unlock(&(list->lock));
        //printf_m("List empty: waiting for an element to remove.\n");
        busy_wait_cycles(0x00001000); //random value - apply testing
    }
    
    struct packet_info* poll = list->head;
    //struct packet_info* next = poll->next;
    //list->head = next;
    (list->length)--;
    //printf_m("Core %d removed packet at %d.\n", current_cpu_id(), poll);
    //printf_m("After polling, list is %d elements long.\n", list->length);    

    unlock(&(list->lock));
    
    return poll;
}

void test_sync(struct list_header *list, struct packet_info *arr, int size){
    while(size){
        append_list(list, &arr[size - 1]);
        
        poll(list);
    }
    //printf_m("Core %d has added all of its packets.\n", current_cpu_id());
}

