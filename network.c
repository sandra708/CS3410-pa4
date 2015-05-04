#include "kernel.h"

inline void spin_lock(int* m){
    //TODO - asm
}

inline void unlock(int* m){
    m = 0;
}

//requests a lock
void append_list(struct list_header *list, struct packet_info *packet){
    spin_lock(&(list->lock));
    
    (list->tail)->next = packet;
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
    struct packet_info* next = poll->next;
    list->head = next;
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
