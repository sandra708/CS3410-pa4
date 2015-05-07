#include "kernel.h"

#define RING_SIZE 16
#define DMA_SIZE PAGE_SIZE


// Print out all of the current statistics.
// The 'data' field can be ignored.
#define HONEYPOT_PRINT      0x301



volatile struct dev_net *net_dev;
unsigned short secret_little_endian=0x1034;

unsigned short add_spammer=0x0101;
unsigned short add_evil_m=0x0201;
unsigned short add_vulnerable=0x0301;

unsigned short del_spammer=0x0102;
unsigned short del_evil=0x0202;
unsigned short del_vulnerable=0x0302;

unsigned short print_stats=0x0103;

unsigned int * vul;
unsigned int * evil;
unsigned int * spam;

int list_sizes=20;
int vul_len=0;
int spam_len=0;
int evil_len=0;
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
			printf("Network Device is on..\n");

			struct dma_ring_slot* ring = (struct dma_ring_slot*) malloc(sizeof(struct dma_ring_slot) * RING_SIZE);//malloc ring buffer
            net_dev->rx_base=virtual_to_physical(ring);
			net_dev->rx_tail=0;
			net_dev->rx_capacity=RING_SIZE;
			net_dev->rx_head=1;

			for (j=0; j< RING_SIZE; j++) { //malloc ring buffer slots
    			unsigned int* space = malloc(DMA_SIZE);
    			ring[j].dma_base = virtual_to_physical(space);
    			ring[j].dma_len = NET_MAXPKT;
			}

    	}
	}
    vul=calloc(list_sizes,sizeof(unsigned int));
    evil=calloc(list_sizes,sizeof(unsigned int));
    spam=calloc(list_sizes,sizeof(unsigned int));

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

/*  Determines if the packet is a command packet
    returns 0 if not command packet
    returns 1 if command packet*/
int is_command(struct honeypot_command_packet*  packet ){

    if(packet->secret_big_endian==secret_little_endian)
        return 1;
    else
        return 0;

}

void print_array(unsigned int *arr){
    int p;
    for (p=0;p<list_sizes;p++){
        printf("%d:%d\t",p,arr[p] );
    }
    printf("\n");

}


/* edits the vulnerable list according to the command field*/
void edit_vulnerable(unsigned short command, unsigned int data){
    if(command==add_vulnerable){
        if(vul_len<list_sizes){
            int p;
            for(p=0;p<list_sizes;p++){
                //printf("vul [%d] : %d\n", p, vul[p]);
                if(vul[p]==0){
                    vul[p]=data;
                    vul_len++;
                    //printf("vulnerable add\n");
                    //print_array(vul);
                    return;
                }
            }
        }

        else;
            //printf("vulnerable list full\n");
    }
    else if(command==del_vulnerable){
        int p;
        for(p=0;p<list_sizes;p++){
            if(vul[p]==data){
                vul[p]=0;
                vul_len--;
                printf("vulnerable delete\n");
                return;
            }
        }
    }

}

/* edits the spammer list according to the command field*/
void edit_spammer(unsigned short command, unsigned int data){
    if(command==add_spammer){
        if(spam_len<list_sizes){
            int p;
            for(p=0;p<list_sizes;p++){
                //printf("spam [%d] : %d\n", p, spam[p]);
                if(spam[p]==0){
                    spam[p]=data;
                    spam_len++;
                    //printf("spam add\n");
                    //return;
                }
            }
        }

        else;
            //printf("spam list full\n");
    }
    else if(command==del_spammer){
        int p;
        for(p=0;p<list_sizes;p++){
            if(spam[p]==data){
                spam[p]=0;
                spam_len--;
                printf("spam delete\n");
               // print_array(spam);
                return;
            }
        }
    }
}

/* edits the evil list according to the command field*/
void edit_evil(unsigned short command, unsigned int data){
    if(command==add_evil_m){
        if(evil_len<list_sizes){
            int p;
            for(p=0;p<list_sizes;p++){
                //printf("evil [%d] : %d\n", p, evil[p]);
                if(evil[p]==0){
                    evil[p]=data;
                    evil_len++;
                   // printf("evil add\n");
                    //print_array(evil);
                    return;
                }
            }
        }

        else;
            //printf("evil list full\n");
    }
    else if(command==del_evil){
        int p;
        for(p=0;p<list_sizes;p++){
            if(evil[p]==data){
                evil[p]=0;
                evil_len--;
                printf("evil delete\n");
                //print_array(evil);
                return;
            }
        }
    }
}


unsigned long djb2(unsigned char *pkt, int n)
{
  unsigned long hash = 5381;
  int i = 0;
  while (i < n-8) {
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
    hash = hash * 33 + pkt[i++];
  }
  while (i < n)
    hash = hash * 33 + pkt[i++];
  return hash;
}



void check_packet(struct honeypot_command_packet * packet){
    int p;
    //unsigned int src_addr=packet->headers.ip_source_address_big_endian;
    //unsigned int des_addr=packet->headers.udp_dest_port_big_endian;
    unsigned int hash =djb2((void *)packet , DMA_SIZE);

    /*printf("src=%d des=%d hash=%d \n", src_addr,des_addr,hash);
    printf("spam   ");
    print_array(spam);
    printf("vul   ");
    print_array(vul);*/
    printf("hash %d\n",hash );
    printf("evil   ");
    print_array(evil);

    for(p=0;p<list_sizes;p++){
        /*if(src_addr==spam[p]){
            total_spam++;
            printf("updated stats spam=%d \n",total_spam);
            printf("src %d\n",src_addr );
            print_array(spam);

        }
        if(des_addr==vul[p]){
            total_vul++;
            printf("updated stats vul=%d \n",total_vul);
           printf("des %d\n",des_addr );
            print_array(vul);
        }*/
        if(hash==evil[p]){
            total_evil++;
            printf("updated stats evil=%d \n",total_evil);
           printf("hash %d\n",hash );
            print_array(evil);
        }
    }
    



}

void execute_command(struct honeypot_command_packet * packet){
    unsigned short command = packet->cmd_big_endian;
    unsigned int data =packet->data_big_endian;

    if(command==print_stats){
        printf("printing stats vul=%d spam=%d evil=%d \n",total_vul,total_spam,total_evil);
    }
/*
    else if(command>0x0300){
        edit_vulnerable(command, data);
    }*/
    else if(command==add_evil_m || command==del_evil){
        edit_evil(command,data);

    }/*
    else if(command>0x0100){
        edit_spammer(command,data);
    }*/
    check_packet(packet);
    
}


int handle_packet(struct honeypot_command_packet*  vaddrpacket ){
    if( (net_dev->rx_head % net_dev->rx_capacity) != (net_dev->rx_tail % net_dev->rx_capacity)){//not full
        if(is_command(vaddrpacket)){
            //printf("executing command..\n");
            execute_command(vaddrpacket);
        }
        else{
            //printf("checking packet..\n");
            check_packet(vaddrpacket);
        }
		net_dev->rx_tail++;        
		return 1;
	}
	else {
        return 0;
    }

}



// Continually polls for data on the ring buffer until the
void network_poll(){
    //int i;
    unsigned int head=net_dev->rx_head;
    unsigned int tail=net_dev->rx_tail;
    int not_full=1;
    printf("polling..\n" );

    struct dma_ring_slot * base= physical_to_virtual(net_dev->rx_base);

    while(1) {//while the buffer in non-empty
        //printf("head %d tail %d capacity %d base %p\n",net_dev->rx_head,net_dev->rx_tail,net_dev->rx_capacity,physical_to_virtual(net_dev->rx_base));
        not_full=handle_packet(physical_to_virtual(base[tail%RING_SIZE].dma_base));
        head=net_dev->rx_head;
        tail=net_dev->rx_tail;        
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
