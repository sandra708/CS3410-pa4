#include "kernel.h"

//requests a lock
void append_list(volatile struct list_header *list, struct packet_info *packet){
    spin_lock(&(list->lock));
    if(list->length == 0){
        list->head = packet;
        list->tail = packet;
    }else{
        (list->tail)->next = packet;
        packet->prev = list->tail;
        list->tail = packet;
    }
    (list->length)++;
    unlock(&(list->lock));
}

//requests lock - remains in the method until list has an element to remove
struct packet_info* poll(volatile struct list_header* list){
    while(1){
        spin_lock(&(list->lock));
        if(list->length){
           break; //list is non-empty 
        } 
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

//polls and returns the head of the list if list nonempty
//otherwise, returns NULL
struct packet_info* get(volatile struct list_header *list){
    spin_lock(&(list->lock));
    struct packet_info* val;
    if(list->length){
        val = NULL;
    }else{
        struct packet_info* val = list->head;
        struct packet_info* next = val->next;
        list->head = next;
        (list->length)--;
    }
    unlock(&(list->lock));
    return val;
}

void test_sync(struct list_header *list, struct packet_info *arr, int size){
    while(size){
        append_list(list, &arr[size - 1]);
        
        poll(list);
        size--;
    }
}
