
#include "kernel.h"
//#include "hashtable.h"
#define LOAD_FACTOR 0.75
int hasher (int a);



struct input
{
    int value;
    int count;
    unsigned int my_hash;
};

struct bucket
{
    struct input* bucket_buffer;
    int num_inputs;
    int bucket_size;
};

struct hashtable
{
    struct bucket* buffer;
    int size;
    int length;
    int insertions;
    int lock;
};

//this is not independently sychronizable under the current mutex
//void bucket_print(struct bucket *self);

/* initializes the hashtable*/
void hashtable_create( struct hashtable *self, int hashtable_size, int my_bucket_size);

//returns 1 if value is in the table; 0 otherwise
int hashtable_increment(struct hashtable *self, int value);

int hashtable_get( struct hashtable *self, int value);

void hashtable_put( struct hashtable *self, int value, int initial_bucket_size);

void hashtable_print( struct  hashtable *self);

void hashtable_elements_print(struct  hashtable *self);

int hashtable_remove( struct hashtable *self, int key);

//never implemented
//void hashtable_stats( struct hashtable *self);

void stats_print();

void test1(void);

// Test insertions
void test2(void);

// Test insertions w/ deletions
void test3(void);

void test4(void);



