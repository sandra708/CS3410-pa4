
#include "kernel.h"


#define  INITIAL_NUM_BUCKETS 5
#define  LOAD_FACTOR 0.75
#define  INITIAL_BUCKET_SIZE 3

struct hashtable {
    struct bucket *buffer;		// pointer to buckets
    unsigned int size;		// the max number of buckets the hashtable can hold
    unsigned int length;	// the number of inputs stored in the hashtable
    unsigned int insertions;
};

struct bucket {
    unsigned int num_inputs;	//number of elements in the bucket
    unsigned int bucket_size;	//size of the bucket
    //struct input * bucket_buffer;	//array of all the inputs in the bucket
    unsigned int * values;//array of values in the bucket
};


/*struct input{
    int value;
    int key;
};*/

int hasher (int a);


void bucket_print(struct bucket *self);


/*Toss an input into a bucket. 
Doubles the size of the bucket if it is not currently big enough*/
void bucket_toss(struct bucket *my_bucket, unsigned int new_value);

/* initializes a bucket*/
void bucket_create(struct bucket *self);


/* initializes the hashtable*/
void hashtable_create( struct hashtable *self);


/* initialize a new input
void input_create(struct input *self, int value, int key);
*/

void rehash( struct hashtable *self,int key, int value);

void hashtable_put( struct hashtable *self, int key, int value);
  

void hashtable_print( struct hashtable *self);


/* return value with that key in the bucket*/

int bucket_get(struct bucket *self, int key);

int hashtable_get( struct hashtable *self, int key);


/* removes item with that key from the bucket*/

int bucket_remove(struct bucket *self, int my_key);



int hashtable_remove( struct hashtable *self, int key);

void hashtable_stats( struct hashtable *self);

void test1();

void test2();

void test3();

void test4();


