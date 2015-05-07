#include "kernel.h"

#define RING_SIZE 16
#define DMA_SIZE 4096
volatile struct dev_net *net_dev;




// Initializes the network driver, allocating the space for the ring buffer.
void network_init(){
	int i,j;
	for(i=0; i<16; i++){
    	if(bootparams->devtable[i].type == DEV_TYPE_NETWORK){
			net_dev = physical_to_virtual(bootparams->devtable[i].start);//find the pointer to the network device 
			net_dev->cmd=NET_SET_POWER;//turn on device
			net_dev->data=1;
			printf("Network Device is on..\n");

			struct dma_ring_slot* ring = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer

			net_dev->rx_tail=1;
			net_dev->rx_capacity=RING_SIZE;
			net_dev->rx_head=0;

			for (j=0; j< RING_SIZE; j++) { //malloc ring buffer slots
    			void* space = malloc(DMA_SIZE);
    			ring[j].dma_base = virtual_to_physical((unsigned int *)space);
    			ring[j].dma_len = NET_MAXPKT;
			}
			net_dev->rx_base=virtual_to_physical(ring);


    	}
	}

}

// Starts receiving packets!
void network_start_receive(){
		net_dev->cmd=NET_SET_RECEIVE;//enable receive 
		net_dev->data=1;


}

// If opt != 0, enables interrupts when a new packet arrives.
// If opt == 0, disables interrupts.
void network_set_interrupts(int opt){
	net_dev->cmd=NET_SET_INTERRUPTS;//enable interuppts
	net_dev->data=1;

}


int handle_packet(unsigned int * base ){
    //printf("head mod capacity %d tail mod capacity %d \n", net_dev->rx_head % net_dev->rx_capacity, net_dev->rx_tail % net_dev->rx_capacity);
	if( (net_dev->rx_head % net_dev->rx_capacity) != (net_dev->rx_tail % net_dev->rx_capacity)){//not full
		net_dev->rx_tail++;
		printf("New packet handled\n");
        
		return 1;
	}
	else return 0;

}

// Continually polls for data on the ring buffer until the
void network_poll(){
    unsigned int head=net_dev->rx_head;
    unsigned int tail=net_dev->rx_tail;
        int not_full=1;
        printf("head %d tail %d capacity %d base %x\n",net_dev->rx_head,net_dev->rx_tail,net_dev->rx_capacity,net_dev->rx_base);
        printf("polling..\n" );
        unsigned int * base=(unsigned int *) net_dev->rx_base;
        while(head!=tail) {//while the buffer in non-empty
            printf("head %d tail %d capacity %d base %x\n",net_dev->rx_head,net_dev->rx_tail,net_dev->rx_capacity,net_dev->rx_base);

            head=net_dev->rx_head;
            tail=net_dev->rx_tail;
                not_full=handle_packet( &base[tail%RING_SIZE]);
        }

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
