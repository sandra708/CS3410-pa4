
#include "kernel.h"

#define IN_HASHING_LIST 0
#define BEING_HASHED 0
#define IN_CHECKING_LIST 0
#define BEING_CHECKED 0
#define IN_GARBAGE_LIST 0
#define IN_RING_BUFFER 0

/* executes transfer from hashing list then hashing then transfer to checking stage*/
void execute_hashing_stage(volatile struct list_header * hashing_list, volatile struct list_header * checking_list);

/* executes transfer from checking list then checking then transfer to garbage list stage*/
void execute_checking_stage(volatile struct list_header * checking_list,volatile  struct list_header * garbage_list);

//ring_buffer is the specific slot for the packet we are removing to process
void execute_ringbuffer_stage(volatile struct list_header* garbage_list, volatile struct dma_ring_slot* ring_buffer, volatile struct list_header * hashing_buffer_list, int index);

/* executes transfer from garbage list to ring buffer */
void execute_garbage_list_transfer_stage(volatile struct dev_net * net_dev,volatile  struct list_header * garbage_list,volatile  struct dma_ring_slot * ring_buffer, unsigned int rx_buff);

/* executes transfer from ring buffer to hashing_buffer_list*/
int execute_remove_from_ring_buffer(volatile struct dma_ring_slot* ring_buffer,volatile  struct list_header * hashing_buffer_list, int rx_buff, int rx_head);
