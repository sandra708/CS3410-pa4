#include "kernel.h"

#define IN_HASING_LIST 20
#define BEING_HASHED 25
#define IN_CHECKING_LIST 30
#define BEIN_CHECKED 35
#define IN_GARBAGE_LIST 40
#define IN_RING_BUFFER 50

/* executes transfer from hashing list then hashing then transfer to checking stage*/
void execute_hashing_stage(struct list_header * hashing_list, struct list_header * checking_list);

/* executes transfer from checking list then checking then transfer to garbage list stage*/

void execute_checking_stage(struct list_header * checking_list, struct list_header * garbage_list);

/* executes transfer from garbage list to ring buffer */
void execute_garbage_list_transfer_stage(struct list_header * garbage_list, struct list_header * ring_buff_list);

/* gets the page base from the vaddr of the honeypot_command_packet pointer */
void * get_page_base(int dma_base);

int execute_remove_from_ring_buffer(struct dma_ring_slot* ring_buffer, struct list_header * hashing_buffer_list, int rx_buff);