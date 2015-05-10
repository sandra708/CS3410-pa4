#include "kernel.h"


/* executes transfer from ring buffer list to hasing list stage*/
void execute_ring_buffer_transfer_stage(struct list_header * ring_buff_list, struct list_header * hashing_list);
/* executes transfer from hashing list then hashing then transfer to checking stage*/
void execute_hashing_stage(struct list_header * hashing_list, struct list_header * checking_list);

/* executes transfer from checking list then checking then transfer to garbage list stage*/

void execute_checking_stage(struct list_header * checking_list, struct list_header * garbage_list);

/* executes transfer from garbage list to ring buffer list stage*/
void execute_garbage_list_transfer_stage(struct list_header * garbage_list, struct list_header * ring_buff_list);