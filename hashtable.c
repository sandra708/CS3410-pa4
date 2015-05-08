
#include "kernel.h"
//#include "hashtable.h"


int hasher (int a){
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

void bucket_print(struct bucket *self){
    int nume=self->num_inputs;
    int j;
    for(j=0;j<nume;j++){
        //printf("Key = %d Value = %d\t",self->bucket_buffer[j].key,self->bucket_buffer[j].value);
        printf("%d:%d   ",j,self->values[j] );
    }

    printf("\n");
}


/*Toss an input into a bucket. 
Doubles the size of the bucket if it is not currently big enough*/
void bucket_toss(struct bucket *my_bucket, unsigned int new_value){
    int elements=my_bucket->num_inputs+1;
    int size=my_bucket->bucket_size;

    if(elements >= size){//if adding another input will fill bucket
	   //my_bucket->bucket_buffer=(struct input *)realloc(my_bucket->bucket_buffer,2*sizeof(struct input)*size);
        unsigned int * temp=(unsigned int *)malloc(2*sizeof(unsigned int)*size);//bigger bucket
        memcpy(temp,my_bucket->values,elements*4);
        printf("free 1 %p\n",my_bucket->values);
        bucket_print(my_bucket);
        free(my_bucket->values);
        my_bucket->values=temp;
        printf("free 2 %p\n",temp);
        free(temp);
        my_bucket->bucket_size=2*size;

	   //my_bucket->bucket_buffer[elements-1]=*new_input;//add new element to end
        my_bucket->values[elements-1]=new_value;

    }
    else{
	   //my_bucket->bucket_buffer[elements-1]=new_value;//add new element to end
        my_bucket->values[elements-1]=new_value;
    }
} 

/* initializes a bucket*/
void bucket_create(struct bucket *self){
        self->num_inputs=0;
        self->bucket_size=INITIAL_BUCKET_SIZE;
        //self->bucket_buffer=(struct input *)malloc(INITIAL_BUCKET_SIZE * sizeof(struct input));

        self->values=(unsigned int *)malloc(INITIAL_BUCKET_SIZE * sizeof(unsigned int));

}


/* initializes the hashtable*/
void hashtable_create( struct hashtable *self){
    self->buffer = (struct bucket*)malloc(INITIAL_NUM_BUCKETS * sizeof(struct bucket));
    self->size = INITIAL_NUM_BUCKETS;
    self->length = 0;
    self->insertions = 0;
    int i;
    for(i=0;i<INITIAL_NUM_BUCKETS;i++){
    	struct bucket b;
    	bucket_create(&b);
    	self->buffer[i]=b;

    }
}


/* initialize a new input
void input_create(struct input *self, int value, int key){
	self->key=key;
	self->value=value;
}*/

void rehash( struct hashtable *self,int key, int value){
    unsigned int h=hasher(key);
    unsigned int s=self->size;
    //struct input x;
    //input_create(&x, value, key);

    int bucket_index=h%s;
    struct bucket * my_new_bucket= &self->buffer[bucket_index];
    //bucket_toss(my_new_bucket, &x);
    bucket_toss(my_new_bucket,value);
    my_new_bucket->num_inputs++;
}

void hashtable_put( struct hashtable *self, int key, int value){
    unsigned int h=hasher(key);
    unsigned int s=self->size;
    unsigned int l=self->length;
    //struct input x;
    //input_create(&x, value, key);
    float current_load=(l+1)/(float)s;
    ;
    if(current_load>LOAD_FACTOR){		//if we must double the size
	   unsigned int new_size=2*s;
        //hashtable_print(self);
	   struct bucket * bucket_list=self->buffer;
	   self->buffer=(struct bucket*)malloc(new_size*sizeof(struct bucket));
	   self->size=new_size;
	
        int p;
	   for(p=0;p<new_size;p++){//create new buckets
            struct bucket b;
            bucket_create(&b);
        	self->buffer[p]=b;
	   }
	   struct bucket current;
	   int i,j;

	   for(i=0;i<s;i++){ //for all buckets
            current=bucket_list[i];
            for(j=0;j<current.num_inputs;j++){//re-input all
                //struct input in=current.bucket_buffer[j];
                //rehash(self, in.key, in.value);
                
                rehash(self,current.values[j],current.values[j]);
                //hashtable_print(self);
		      }
	   }
       printf("free 3 %p\n",bucket_list);
	   free(bucket_list);
	   int bucket_index=hasher(key)%new_size;
	   struct bucket * my_new_bucket=&self->buffer[bucket_index];
	   //bucket_toss(my_new_bucket, &x);
       bucket_toss(my_new_bucket,value);
	   my_new_bucket->num_inputs++;
	   self->insertions++;
	   self->length++;
    }

    else{
        int bucket_index=h%s;
        struct bucket * my_new_bucket=&self->buffer[bucket_index];
        //bucket_toss(my_new_bucket, &x);
        bucket_toss(my_new_bucket,value);
        my_new_bucket->num_inputs++;
        self->insertions++;
        self->length++;
    }

}

void hashtable_print( struct  hashtable *self){
    int j;
    int len=self->size;
    for(j=0;j<len;j++){
        printf("[Bucket %d]: ",j);
	bucket_print(&self->buffer[j]);
    }

}


/* return value with that key in the bucket*/

int bucket_get(struct bucket *self, int key){
    int i;
    unsigned int tempkey;
    for(i=0;i<self->num_inputs;i++){	//for all inputs in the bucket
	   //tempkey=(self->bucket_buffef[i]).key; //get the key of the input
        tempkey=self->values[i];
	   if(tempkey==key){ 	//if the key matches
	       //return (self->bucket_buffer[i]).value; //return the value
            return self->values[i];
        }
    }
    //printf("ERROR key %d is not defined\n",key);
    return 0;
}

int hashtable_get( struct hashtable *self, int key){
    unsigned int h=hasher(key);
    struct bucket *b=self->buffer;
    unsigned int s=self->size;
    int bucket_index=h%s;
    return bucket_get( &b[bucket_index] ,key);
}


/* removes item with that key from the bucket*/

int bucket_remove(struct bucket *self, int my_key){
    int size=self->num_inputs;
    int i,tempkey;
    //struct input * buff=self->bucket_buffer;
    unsigned int * buff=self->values;
    for(i=0;i<size;i++){
	//tempkey=buff[i].key;
        tempkey=buff[i];
	   if(tempkey==my_key){
	       self->num_inputs--;
	       //memmove(buff+i,buff+i+1,sizeof(struct input*)*(size-i-1)); //shift everything to the left
            memcpy(buff+i,buff+i+1,4*(size-i-1));
	       //free(self->);
	       return 1;//return 1 if succesfully removed
	
        }
    }
    //printf("ERROR key %d is not defined.\n",my_key);
    return 0;//return 0 if nothing removed
}


int hashtable_remove( struct hashtable *self, int key){
    unsigned int h=hasher(key);
    struct bucket *buf=self->buffer;
    unsigned int s=self->size;
    int bucket_index=h%s;
    int success=bucket_remove(&buf[bucket_index],key);
    
    if(success){//if remove was successful 
    	self->length--;
    }
    return success;
}

void hashtable_stats( struct hashtable *self){
	unsigned int i= self->insertions;
	unsigned int l= self->length;
	unsigned int s= self->size;
	printf("Length = %d; Size = %d; Insertions = %d\n",l,s,i);
}

void test1(void) {
    struct hashtable a;

    hashtable_create(&a);
    hashtable_put(&a,13,13);
    if(hashtable_get(&a,13)!=13) {
        printf("test1 failed.\n");
        return;
    }
    printf("test1 passed.\n");
    hashtable_print(&a);
}

// Test insertions
void test2(void) {
    struct hashtable a;
    int i;

    hashtable_create(&a);
    for (i=0; i<10; i++) {
        hashtable_put(&a, i, i);
        if (hashtable_get(&a, i) != i) {
            printf("test2 afailed.\n");
            return;
        }
    }
    for (i=0; i<10; i++) {
        if (hashtable_get(&a, i) != i) {
            printf("test2 bfailed.\n");
            return;
        }
    }
    printf("test2 passed.\n");
    hashtable_print(&a);

}

// Test insertions w/ deletions
void test3(void) {
    struct hashtable a;
    int i;

    hashtable_create(&a);

    for (i=0; i<100; i++) {
        hashtable_put(&a, i, i);
        if (hashtable_get(&a, i) != i) {
            printf("test3 failed.\n");
            return;
        }
    if (i%2) {
            hashtable_remove(&a, i);
        }
    }
    for (i=0; i<100; i++) {
        if ((i%2)==0) {
            if (hashtable_get(&a, i) != i) {
                printf("test3 failed.\n");
                return;
            }
    }
    }

    printf("test3 passed.\n");
    hashtable_print(&a);

}


void test4(void) {
    struct hashtable a;
    int i;

    hashtable_create(&a);
    hashtable_put(&a, 2001, 2001);
    for (i=0; i < 1000; i++) {
        hashtable_put(&a, i, i);
        if (hashtable_get(&a, i) != i) {
            printf("test4 faileda.\n");
            return;
        }
    hashtable_remove(&a, i);
    }
    if (hashtable_get(&a, 2001) != 2001) {
        printf("test4 failedb.\n");
        return;
    }
    printf("test4 passed.\n");
    hashtable_print(&a);

}




