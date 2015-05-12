#include "kernel.h"


void execute_hashing_stage(volatile struct list_header* hashing_buffer_list, volatile struct list_header* checking_buffer_list){
  struct packet_info* current_packet = poll(hashing_buffer_list);

  current_packet->status=BEING_HASHED;

  current_packet->hash=djb2((unsigned char *)&current_packet->packet_start,current_packet->packet_length);
  
  current_packet->status = IN_CHECKING_LIST;
  append_list(checking_buffer_list, current_packet);
}

void execute_checking_stage(volatile struct list_header* checking_buffer_list,volatile  struct list_header* garbage_list, volatile struct global_stats* stats){
  //printf("Checking list: %p, Garbage list: %p, Stats: %p \n", checking_buffer_list, garbage_list, stats);
  struct packet_info* current_packet = poll(checking_buffer_list);
  current_packet->status=BEING_CHECKED;
  //checks if it is a command packet and executes this command
  //printf("Executing command pipeline.\n");
  execute_command_pipeline(&current_packet->packet_start);
  //printf("Checking packet.\n");
  //checks if packet is evil/vulnerable/spam and increments hashtable
  int code = check_packet_pipeline(&current_packet->packet_start, current_packet->hash);
  //updates stats
  update_stats(stats, current_packet, code);

  current_packet->status = IN_GARBAGE_LIST;
  append_list(garbage_list, current_packet);
}

void * get_page_base(void* dma_base_vaddr){
  int total=sizeof(struct packet_info);
  return dma_base_vaddr-total;
}

void execute_ringbuffer_stage(volatile struct list_header* garbage_list, volatile struct dma_ring_slot* current, volatile struct list_header * hashing_buffer_list){ 
  struct packet_info* current_packet =physical_to_virtual(current->dma_base)-0x18;
 // printf("base %p packet %p\n",physical_to_virtual(current->dma_base),current_packet );
  current_packet->packet_length = current->dma_len;
  current_packet->status = IN_HASHING_LIST;
  //printf("%p\n",current_packet );
  append_list(hashing_buffer_list, current_packet);

  struct packet_info* next_packet = poll(garbage_list);

  next_packet->status = IN_RING_BUFFER;
  current->dma_len = NET_MAXPKT;
  current->dma_base = virtual_to_physical(&next_packet->packet_start);

}


/*
// executes transfer from garbage list to ring buffer list stage
 //   Assumes that only one core performs this job and is therefore 
 //   partially unsycronized.
void execute_garbage_list_transfer_stage(volatile struct dev_net * net_dev,volatile  struct list_header * garbage_list,volatile  struct dma_ring_slot * ring_buffer, unsigned int rx_buff){ 
//printf("execute_garbage_list_transfer_stage 0\n");
  struct packet_info * current_packet;
  if(net_dev->rx_tail < rx_buff){//if there is space in the ring buffer 
    spin_lock(&garbage_list->lock);
    if(garbage_list->length>0){//if there are packets in the garbage list
//printf("\n");
      current_packet=garbage_list->head;
      garbage_list->head=current_packet->prev;
      garbage_list->length--;
      unlock(&garbage_list->lock);
    }
    else {//garbage_list length is 0;
      unlock(&garbage_list->lock);
      //printf("%d\n",current_cpu_id() );
      //printf("No packets spaces free to add to ring buffer. Sincerely, core %d\n", current_cpu_id());
      return;
    }

    spin_lock(&current_packet->lock);
    if(current_packet->status==IN_GARBAGE_LIST){ //double check that the packet is in the garbage list
//printf("execute_garbage_list_transfer_stage 3\n");
      update_stats(current_packet);
      current_packet->status=IN_RING_BUFFER;
      ring_buffer[(net_dev->rx_tail+1)%RING_SIZE].dma_len=NET_MAXPKT;
      ring_buffer[(net_dev->rx_tail+1)%RING_SIZE].dma_base=virtual_to_physical(&current_packet->packet_start);
      net_dev->rx_tail++; //tells the network device that there is a free page to put a new packet into
      //printf("%d\n",current_cpu_id() );
      //printf("incrementing head %d buffer %d\n",net_dev->rx_head, rx_buff );

      unlock(&current_packet->lock);
      return;
    }
    else{
      unlock(&current_packet->lock);
      //printf("%d\n",current_cpu_id() );
      //printf("Packet at %p has incorrect status for transfer to ring buffer. Sincerely, core %d\n", current_packet, current_cpu_id());
      return;
    }

  }

  else{
    //printf("%d\n", rx_buff);
    //printf("Waiting for stuff to be removed from the buffer.....Sincerely, core %d\n",current_cpu_id());
  }
}
*/


//removes a packet from the ring buffer into hashing_buffer_list
//assumes that there is a packet to remove
//returns new rx_buff if succesful and old one if unsucessful
//
/*int execute_remove_from_ring_buffer(volatile struct dma_ring_slot* ring_buffer,volatile struct list_header * hashing_buffer_list, int rx_buff,int rx_head){
//printf("buff%d head %d \n",rx_buff,rx_head );

if(rx_buff%RING_SIZE!=rx_head%RING_SIZE+1){
  volatile struct dma_ring_slot *curr =&ring_buffer[rx_buff%RING_SIZE];
  struct packet_info *current_packet=get_page_base(physical_to_virtual(curr->dma_base));
  
  //printf("execute_remove_from_ring_buffer dma_basev %p pckt_stt %p \n",physical_to_virtual(curr->dma_base),current_packet);
  //printf("&current_packet->lock %p\n",&current_packet->lock);
//printf("%p",current_packet);

  spin_lock(&current_packet->lock);

  if(current_packet->status==IN_RING_BUFFER){ //double check that the packet is in the ring buffer
    current_packet->status=IN_HASHING_LIST;
         // printf("Status %d\n", current_packet->status);

    current_packet->packet_length=curr->dma_len;
//printf("- ");

    spin_lock(&hashing_buffer_list->lock);
    current_packet->next=hashing_buffer_list->tail;
    hashing_buffer_list->tail=current_packet;
    if(hashing_buffer_list->length==0){
      hashing_buffer_list->head=current_packet;
    }
    hashing_buffer_list->length++;
    unlock(&current_packet->lock);
    unlock(&hashing_buffer_list->lock);
  }
  else{
         // printf("Status %d\n", current_packet->status);

      unlock(&current_packet->lock);
      unlock(&hashing_buffer_list->lock);
      //printf("%d\n",current_cpu_id() );
      printf("Packet at %p has incorrect status for out of the ring buffer. Sincerely, core %d\n", current_packet, current_cpu_id());
      return rx_buff;
  }

  //printf("execute_remove_from_ring_buffer 0\n");
  //spin_lock(rx_buff_lock);
  int j=rx_buff;
  //unlock(rx_buff_lock);
  return j+1;
}
else{
  return rx_buff;
}
}*/
