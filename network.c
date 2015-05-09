#include "kernel.h"

#define RING_SIZE 16
#define DMA_SIZE PAGE_SIZE


// Print out all of the current statistics.
// The 'data' field can be ignored.

volatile struct dev_net *net_dev;
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


// Initializes the network driver, allocating the space for the ring buffer.
void network_init(){
	int i,j;
	for(i=0; i<16; i++){
    	if(bootparams->devtable[i].type == DEV_TYPE_NETWORK){
			net_dev = physical_to_virtual(bootparams->devtable[i].start);//find the pointer to the network device 
			net_dev->cmd=NET_SET_POWER;//turn on device
			net_dev->data=1;
			//printf("Network Device is on..\n");

			struct dma_ring_slot* ring = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            net_dev->rx_base=virtual_to_physical(ring);
			net_dev->rx_tail=0;
			net_dev->rx_capacity=RING_SIZE;
			net_dev->rx_head=0;

			for (j=0; j< RING_SIZE; j++) { //malloc ring buffer slots
    			unsigned int* space = malloc(DMA_SIZE);
    			ring[j].dma_base = virtual_to_physical(space);
    			ring[j].dma_len = NET_MAXPKT;
			}

    	}
	}
    hashtable_create(&evil_hashtable,evil_hashtable_size,evil_bucket_size);
    hashtable_create(&vulnerable_hashtable,vulnerable_hashtable_size,vulnerable_bucket_size);
    hashtable_create(&spam_hashtable,spam_hashtable_size,spam_bucket_size);

}

// Starts receiving packets!
void network_start_receive(){
		net_dev->cmd=NET_SET_RECEIVE;//enable receive 
		net_dev->data=1;
        printf("Network receive enabled..\n");
}

// If opt != 0, enables interrupts when a new packet arrives.
// If opt == 0, disables interrupts.
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



/* edits the vulnerable list according to the command field*/
void edit_vulnerable(unsigned short command, unsigned int data){
    if(command==add_vulnerable){
        ////printf("vulnerable add %x\n",data);
        hashtable_put(&vulnerable_hashtable,data,vulnerable_bucket_size);
        ////printf("Vulnerable size %d\n",vulnerable_hashtable.length );
        //hashtable_print(&vulnerable_hashtable);
    }
    else if(command==del_vulnerable){
        hashtable_remove(&vulnerable_hashtable,data);
    }

}

/* edits the spammer list according to the command field*/
void edit_spammer(unsigned short command, unsigned int data){
    if(command==add_spammer){
        ////printf("spammer add %x\n",data);
        hashtable_put(&spam_hashtable,data,spam_bucket_size);
    }
    else if(command==del_spammer){
        hashtable_remove(&spam_hashtable,data);
    }
}

/*change endianess of an int
taken from stackoverflow*/
unsigned int change_end(unsigned int data){
    
     return( ( data >> 24 ) | (( data << 8) & 0x00ff0000 )| ((data >> 8) & 0x0000ff00) | ( data << 24)  );

}

/* edits the evil list according to the command field*/
void edit_evil(unsigned short command, unsigned int data){
    if(command==add_evil_m){
        //printf("evil add %x\n",data);
        hashtable_put(&evil_hashtable,change_end(data),evil_bucket_size);
        //printf("Evil size %d\n",evil_hashtable.length );
       // hashtable_print(&evil_hashtable);
    }
    else if(command==del_evil){
        hashtable_remove(&evil_hashtable,change_end(data));
    }
}




unsigned long djb2(unsigned char *pkt, int n)
{
  unsigned long hash = 5381;
  int c;
  //printf("normal\n");
  for (int i = 0; i < n; i++) {
    c = pkt[i];
    hash = hash * 33 + c;
    //hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    //printf("%x ",c);// , i);
  }
  //printf("\n");
  return hash;
}

unsigned long djb2_li(unsigned char *pkt, int n)
{
  unsigned long hash = 5381;
  int c;
  //printf("new\n");
  for (int i = n-1; i >= 0; i--) {
    c = pkt[i];
    hash = hash * 33 + c;
    //hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    //printf("%x ",c);// , i);
  }
  //printf("\n");
  return hash;
}

void check_packet(struct honeypot_command_packet * packet, int n){
    unsigned int src_addr=packet->headers.ip_source_address_big_endian;
    unsigned int des_addr=packet->headers.udp_dest_port_big_endian<<16;
    unsigned int hash2=djb2((void *)packet , n);
    //unsigned long hash =djb2((void *)packet , n);
    ////printf("src=%x %x  des=%x %x hash=%x %x \n", src_addr,hashtable_get(&spam_hashtable,src_addr),des_addr,hashtable_get(&vulnerable_hashtable,des_addr),hash,hashtable_get(&evil_hashtable,hash));
    //printf("spam%x\n",src_addr);
    //hashtable_print(&spam_hashtable);
    //printf("\n");
    //printf("vul %x %x\n",des_addr,hashtable_get(&vulnerable_hashtable,des_addr));
    //hashtable_print(&vulnerable_hashtable);
    //printf("\n");
    printf("hash %x %d\n",hash2,total_evil );
    //printf("evil\n");
    //hashtable_print(&evil_hashtable);
    //printf("\n");

    if(src_addr==hashtable_get(&spam_hashtable,src_addr)){
        total_spam++;
        //printf("updated stats spam=%d \n",total_spam);
        //printf("src %d hash %d des %d\n",src_addr,hash,des_addr );
        //hashtable_print(&spam_hashtable);
        }
    if(des_addr==hashtable_get(&vulnerable_hashtable,des_addr)){
        total_vul++;
        //printf("updated stats vul=%d \n",total_vul);
        //printf("des %d\n",des_addr );
        //hashtable_print(&vulnerable_hashtable);
    }
    if(hash2==hashtable_get(&evil_hashtable,hash2) ){//|| hash2==hashtable_get(&evil_hashtable,hash)){
        total_evil++;
        printf("updated stats evil=%d \n",total_evil);
        //printf("hash %d\n",hash );
        //hashtable_print(&evil_hashtable);
    }

}

int max(int x, int y, int z){
    if(x>=y && x>=z) return x;
    else if (y>=z ) return y;
    else return z;
}

void stats_print(){
       //printf("count spam_source\t|\tcount  evil_hash\t|\tcount vuln_port");
       //int j;
    printf("evil %d vulnerable %d spam %d\n", total_evil,total_vul,total_spam);
    hashtable_print(&evil_hashtable);
       //int len=max(spam_hashtable.length,vulnerable_hashtable.length,evil_hashtable.length);
       //for(j=0;j<len;j++){
        
       //}

}

void execute_command(struct honeypot_command_packet * packet, int n){
    unsigned short command = packet->cmd_big_endian;
    unsigned int data =packet->data_big_endian;

    if(command==print_stats){
        stats_print();
    }
    else if(command==add_vulnerable || command==del_vulnerable){
        edit_vulnerable(command, data);
    }
    else if(command==add_evil_m || command==del_evil){
        edit_evil(command,data);

    }
    else if(command==add_spammer || command==del_spammer){
        edit_spammer(command,data);
    }
    check_packet(packet,n);
}

void handle_packet(struct honeypot_command_packet*  vaddrpacket, int n ){
    if(is_command(vaddrpacket)){
        execute_command(vaddrpacket,n);
    }
    else{
        check_packet(vaddrpacket,n);
    }
}



// Continually polls for data on the ring buffer until the
void network_poll(){

    printf("polling..\n" );

    struct dma_ring_slot * base= physical_to_virtual(net_dev->rx_base);
    struct dma_ring_slot *curr;
    while(1){
    while(net_dev->rx_head!=net_dev->rx_tail ){
        curr=&base[net_dev->rx_tail%RING_SIZE];
        handle_packet(physical_to_virtual(curr->dma_base),curr->dma_len);
        curr->dma_len=NET_MAXPKT;
        net_dev->rx_tail++;   
    }}


}

// Called when a network interrupt occurs.
void network_trap();



inline void spin_lock(int* m){
    //TODO - asm
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
    
    unlock(&(list->lock));
}

//requests lock - remains in the method until list has an element to remove
struct packet_info* poll(struct list_header *list){
    while(1){
        spin_lock(&(list->lock));
        if(list->length) break; //list is non-empty
        unlock(&(list->lock));
        busy_wait_cycles(0x00001000); //random value - apply testing
    }
    
    struct packet_info* poll = list->head;
    //struct packet_info* next = poll->next;
    //list->head = next;
    (list->length)--;
    
    unlock(&(list->lock));
    
    return poll;
}

//adds port to the list of stuff kept track of (locks entry)
void add_port(struct port_table_entry *port){
    //TODO
}

//removes port from list (locks entry)
void remove_port(struct port_table_entry *port){
    //TODO
}

//increments access list at this entry only if it is on the list of ports (locks entry)
void increment_port(struct port_table_entry *port){
    //TODO
}

//adds hash to the list of evils (locks entry)
//table should be the root address of the whole block
void add_evil(struct evil_table_entry *table, unsigned int hash){
    //TODO
}

//removes hash from list (locks entry)
void remove_evil(struct evil_table_entry *table, unsigned int hash){
    //TODO
}

//increments access list at this entry only if it is on the list of evils (locks entry)
void increment_evil(struct evil_table_entry *table){
        //TODO
}
