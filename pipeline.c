#include "kernel.h"

// executes transfer from hashing list then hashing then transfer to checking stage
void execute_hashing_stage(volatile struct list_header * hashing_buffer_list,volatile  struct list_header * checking_buffer_list){
//printf("execute_hashing_stage 0*\n");

  struct packet_info * current_packet;
//printf("execute_hashing_stage 1*\n");
  spin_lock(&hashing_buffer_list->lock);

  if(hashing_buffer_list->length>0){
      printf("execute_hashing_stage 2*\n");

    current_packet=hashing_buffer_list->head;
          printf("execute_hashing_stage %p\n", hashing_buffer_list->head);

    spin_lock(&current_packet->lock);
    if(current_packet->status==IN_HASHING_LIST){
      printf("execute_hashing_stage 3*\n");

      hashing_buffer_list->head=current_packet->prev;
      hashing_buffer_list->length--;
      unlock(&hashing_buffer_list->lock);

      current_packet->status=BEING_HASHED;
      current_packet->hash=djb2((unsigned char *)&current_packet->packet_start,current_packet->packet_length);
         printf("hash %d\n",current_packet->hash );

      current_packet->status=IN_CHECKING_LIST;
      spin_lock(&checking_buffer_list->lock);
      current_packet->next=checking_buffer_list->tail;
      checking_buffer_list->tail=current_packet;
      checking_buffer_list->length++;
      unlock(&checking_buffer_list->lock);
      unlock(&current_packet->lock);
    }
    else{
      printf("execute_hashing_stage 4*\n");
      unlock(&current_packet->lock);
      unlock(&hashing_buffer_list->lock);
      //printf("%d\n",current_cpu_id() );
      //printf("Packet at %p has incorrect status for hashing. Sincerely, core %d\n",current_packet,current_cpu_id() );
      return;
    }
  }
  else {//hashing_buffer_list length is 0;
    unlock(&hashing_buffer_list->lock);
      //printf("execute_hashing_stage 5*\n");
    //printf("%d\n",current_cpu_id() );
    //printf("No packets waiting to be hashed Sincerely, core %d\n",current_cpu_id());
    return;
  }


printf("execute_hashing_stage 6*\n");

}
/*
// executes transfer from checking list then checking then transfer to garbage list stage
void execute_checking_stage(volatile struct list_header * checking_buffer_list,volatile  struct list_header * garbage_list){

  struct packet_info * current_packet;
  int cl=checking_buffer_list->lock;
  spin_lock(&cl);
  if(checking_buffer_list->length>0){
    current_packet=checking_buffer_list->head;
    checking_buffer_list->head=current_packet->next;
    checking_buffer_list->length--;
    unlock(&cl);
  }
  else {//checking_buffer_list length is 0;
    unlock(&cl);
    //printf("%d\n",current_cpu_id() );
    //printf("No packets waiting to be checked. Sincerely, core %d\n", current_cpu_id());
    return;
  }

  spin_lock(&cl);
  if(current_packet->status==IN_CHECKING_LIST){
    current_packet->status=BEING_CHECKED;
    execute_command_pipeline(current_packet->packet_start);//checks if it is a command packet and executes this command
    check_packet_pipeline(current_packet->packet_start,current_packet->hash);//checks if packet is evil/vulnerable/spam
    current_packet->status=IN_GARBAGE_LIST;
    unlock(&cl);
  }
  else{
    unlock(&current_packet->lock);
    //printf("%d\n",current_cpu_id() );
    ////printf("Packet at %p has incorrect status for checking. Sincerely, core %d\n", current_packet, current_cpu_id());
    return;
  }
  int gl=garbage_list->lock;
  spin_lock(&gl);
  garbage_list->tail=current_packet;
  garbage_list->length++;
  unlock(&gl);
}

// executes transfer from garbage list to ring buffer list stage
 //   Assumes that only one core performs this job and is therefore 
 //   partially unsycronized.
void execute_garbage_list_transfer_stage(volatile struct dev_net * net_dev,volatile  struct list_header * garbage_list,volatile  struct dma_ring_slot * ring_buffer, unsigned int rx_buff){ 

  struct packet_info * current_packet;
  int *gl=garbage_list->lock;
  if(net_dev->rx_head%RING_SIZE < rx_buff%RING_SIZE){//if there is space in the ring buffer 
    spin_lock(gl);
    if(garbage_list->length>0){//if there are packets in the garbage list
      current_packet=garbage_list->head;
      garbage_list->head=current_packet->next;
      garbage_list->length--;
      unlock(gl);
    }
    else {//garbage_list length is 0;
      unlock(gl);
      //printf("%d\n",current_cpu_id() );
      //printf("No packets spaces free to add to ring buffer. Sincerely, core %d\n", current_cpu_id());
      return;
    }
    int cl=current_packet->lock;
    spin_lock(&current_packet->lock);
    if(current_packet->status==IN_GARBAGE_LIST){ //double check that the packet is in the garbage list
      update_stats(current_packet);
      current_packet->status=IN_RING_BUFFER;
      ring_buffer[net_dev->rx_tail%RING_SIZE].dma_len=NET_MAXPKT;
      ring_buffer[net_dev->rx_tail%RING_SIZE].dma_base=virtual_to_physical(current_packet->packet_start);
      net_dev->rx_head++; //tells the network device that there is a free page to put a new packet into
      //printf("%d\n",current_cpu_id() );
      //printf("incrementing head %d\n",net_dev->rx_head );

      unlock(&cl);
      return;
    }
    else{
      unlock(&cl);
      //printf("%d\n",current_cpu_id() );
      //printf("Packet at %p has incorrect status for transfer to ring buffer. Sincerely, core %d\n", current_packet, current_cpu_id());
      return;
    }

  }

  else{
    //printf("%d\n", rx_buff);
    //printf("Waiting for stuff to be removed from the buffer.....Sincerely, core %d\n",current_cpu_id());
  }//remember to change next of the packet to tail of list******************
}
*/
//gets the page base from the vaddr of the honeypot_command_packet pointer 
void * get_page_base(void* dma_base_vaddr){
          //int lock   + int status+ int hash +unsign int packet_len+    struct info * next      +    struct info * prev
  int total=sizeof(int)+sizeof(int)+sizeof(int)+sizeof(unsigned int)+sizeof(struct packet_info *)+sizeof(struct packet_info *);
  return dma_base_vaddr-total;
}

//removes a packet from the ring buffer into hashing_buffer_list
//assumes that there is a packet to remove
//returns new rx_buff if succesful and old one if unsucessful
//
int execute_remove_from_ring_buffer(volatile struct dma_ring_slot* ring_buffer,volatile struct list_header * hashing_buffer_list, int rx_buff,int rx_head){
//printf("buff%d head %d \n",rx_buff,rx_head );

if(rx_buff%RING_SIZE < rx_head%RING_SIZE){
  volatile struct dma_ring_slot *curr =&ring_buffer[rx_buff%RING_SIZE];
  struct packet_info *current_packet=get_page_base(physical_to_virtual(curr->dma_base));
  
  //printf("execute_remove_from_ring_buffer dma_basev %p pckt_stt %p \n",physical_to_virtual(curr->dma_base),current_packet);
  //printf("&current_packet->lock %p\n",&current_packet->lock);
  spin_lock(&current_packet->lock);
  if(current_packet->status==IN_RING_BUFFER){ //double check that the packet is in the ring buffer
    current_packet->status=IN_HASHING_LIST;
    current_packet->packet_length=curr->dma_len;
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
      unlock(&current_packet->lock);
      unlock(&hashing_buffer_list->lock);
      //printf("%d\n",current_cpu_id() );
      //printf("Packet at %p has incorrect status for out of the ring buffer. Sincerely, core %d\n", current_packet, current_cpu_id());
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
}

