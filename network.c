#include "kernel.h"
//#include "machine.h"

#define RING_SIZE 16
#define DMA_SIZE NET_MAXPKT
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

			net_dev->rx_tail=0;
			net_dev->rx_capacity=RING_SIZE;
			net_dev->rx_head=0;

			for (j=0; j< RING_SIZE; j++) { //malloc ring buffer slots
    			void* space = malloc(DMA_SIZE);
    			ring[j].dma_base = (unsigned int )space;
    			ring[j].dma_len = RING_SIZE;
			}
			net_dev->rx_base=(unsigned int )&ring[0];


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

	if( (net_dev->rx_head % net_dev->rx_capacity) != (net_dev->rx_tail % net_dev->rx_capacity)){//not full
		net_dev->rx_tail++;
		printf("New packet handled\n");
		return 1;
	}
	else return 0;

}

// Continually polls for data on the ring buffer until the
void network_poll(){
        int not_full=1;
        while(net_dev->rx_head!=net_dev->rx_tail) {//while the buffer in non-empty
                not_full=handle_packet( (unsigned int *) (net_dev->rx_head % net_dev->rx_capacity) );
        }

}

// Called when a network interrupt occurs.
void network_trap();

