
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
        printf("Data = %x Count = %d\t",self->bucket_buffer[j].value,self->bucket_buffer[j].count);
        //printf("%d:%x   ",j,self->values[j] );
    }

    printf("\n");
}


/*Toss an input into a bucket. 
Doubles the size of the bucket if it is not currently big enough*/
int bucket_toss(struct bucket *my_bucket, struct input *new_value){
    int elements=my_bucket->num_inputs+1;
    int size=my_bucket->bucket_size;

    if(elements > size){//if adding another input will fill bucket
	   //my_bucket->bucket_buffer=(struct input *)realloc(my_bucket->bucket_buffer,2*sizeof(struct input)*size);
        /*unsigned int * temp=(unsigned int *)malloc(2*sizeof(unsigned int)*size);//bigger bucket
        memcpy(temp,my_bucket->values,elements*4);
        printf("free 1 %p\n",my_bucket->values);
        bucket_print(my_bucket);
        free(my_bucket->values);
        my_bucket->values=temp;
        printf("free 2 %p\n",temp);
        free(temp);
        my_bucket->bucket_size=2*size;

	    my_bucket->bucket_buffer[elements-1]=*new_input;//add new element to end
        my_bucket->values[elements-1]=new_value;*/
        //printf("Bucket full. replacing old value: %d with :%d\n",my_bucket->bucket_buffer[elements-1].value,new_value->value);
        //bucket_print(my_bucket);
        //printf("inserting %d\n",new_value->value);
        printf("resizing bucket\n");
        struct input * temp=my_bucket->bucket_buffer;
        //printf("bp1 if before\n");
        //bucket_print(my_bucket);
        my_bucket->bucket_buffer=(struct input *)malloc(2*sizeof(struct input)*size);
        //printf("bp2 if before\n");
        //bucket_print(my_bucket);
        memcpy(my_bucket->bucket_buffer,temp,sizeof(struct input)*size);
        //printf("bp1 if after\n");
        //bucket_print(my_bucket);
        free(temp);
        my_bucket->bucket_buffer[elements-1]=*new_value;
        my_bucket->bucket_size=2*size;
        my_bucket->num_inputs++;
        //printf("bp2 if after\n");
        //bucket_print(my_bucket);
        //printf("final size %d\n",my_bucket->num_inputs );

        return 0;

    }
    else{
	   my_bucket->bucket_buffer[elements-1]=*new_value;//add new element to end
        //my_bucket->values[elements-1]=new_value;
        my_bucket->num_inputs++;
        return 1;
    }
} 

/* initializes a bucket*/
void bucket_create(struct bucket *self, int my_bucket_size){
        self->num_inputs=0;
        self->bucket_size=my_bucket_size;
        self->bucket_buffer=(struct input *)malloc(my_bucket_size * sizeof(struct input));

        //self->values=(unsigned int *)malloc(my_bucket_size * sizeof(unsigned int));

}


/* initializes the hashtable*/
void hashtable_create(volatile struct hashtable *self, int hashtable_size, int my_bucket_size){
    spin_lock(&(self->lock));
    self->buffer = (struct bucket*)malloc(hashtable_size * sizeof(struct bucket));
    self->size = hashtable_size;
    self->length = 0;
    self->insertions = 0;

    int i;
    for(i=0;i<hashtable_size;i++){
    	struct bucket b;
    	bucket_create(&b,my_bucket_size);
    	self->buffer[i]=b;

    }
    unlock(&(self->lock));
}


// initialize a new input
void input_create(struct input *self, int value, int count, int my_hash){
	self->count=count;
	self->value=value;
    self->my_hash=my_hash;
}

void rehash(volatile struct hashtable *self,struct input *x){
    unsigned int h=x->my_hash;
    unsigned int s=self->size;
    int bucket_index=h%s;
    struct bucket * my_new_bucket= &self->buffer[bucket_index];
    //bucket_toss(my_new_bucket, &x);
    bucket_toss(my_new_bucket,x);
    
}

int bucket_get(struct bucket* self, int value){
    int i,tempv;
    for(i=0; i<self->num_inputs; i++){    //for all inputs in the bucket
       tempv= self->bucket_buffer[i].value; //get the value of the input
       if(tempv == value){    //if the value matches
           //return (self->bucket_buffer[i]).value; 
            //self->bucket_buffer[i].count++;
            return self->bucket_buffer[i].count;//return it
        }
    }
    //printf("ERROR key %d is not defined\n",key);
    return -1;
}

int hashtable_get(volatile struct hashtable *self, int value){
    spin_lock(&(self->lock));

    unsigned int h=hasher(value);
    struct bucket *b=self->buffer;
    unsigned int s=self->size;
    int bucket_index=h%s;
    int r = bucket_get( &b[bucket_index] ,value);

    unlock(&(self->lock));
    return r;
}

//increments the number of packets observed with $value, if $value is in the table
int bucket_incr(struct bucket *self, int value){
    int i;
    for(i = 0; i < self->num_inputs; i++){
        if(self->bucket_buffer[i].value == value){
            self->bucket_buffer[i].count++;
            return 1;
        }
    }
    return 0;
}

int hashtable_increment(volatile struct hashtable *self, int value){
    spin_lock(&self->lock);

    unsigned int h=hasher(value);
    struct bucket *b=self->buffer;
    unsigned int s=self->size;
    int bucket_index=h%s;
    int r = bucket_incr( &b[bucket_index] ,value);

    unlock(&self->lock);

    return r;
}

void free_buckets(struct bucket * self,int len){
    int d;
    for(d=0;d<len;d++)
        free(self[d].bucket_buffer);
    free(self);
}

void hashtable_put( volatile struct hashtable *self, int value, int initial_bucket_size){  

    if(hashtable_get(self,value)==value){
        //printf("Already in bucket: %d\n",value);
        return;
    }

    spin_lock(&(self->lock));

    struct input x;
    int h=hasher(value);
    input_create(&x, value, 0,h );
    
    unsigned int s=self->size;
    unsigned int l=self->length;
    int success;

    float current_load=(l+1)/(float)s;
    if(current_load>LOAD_FACTOR){		//if we must double the size
       printf("resizing hashtable\n");
	   unsigned int new_size=2*s;
	   struct bucket * bucket_list=self->buffer;

	   self->buffer=(struct bucket*)malloc(new_size*sizeof(struct bucket));
	   self->size=new_size;
	
       int p;
	   for(p=0;p<new_size;p++){//create new buckets
            struct bucket b;
            bucket_create(&b,initial_bucket_size);
        	self->buffer[p]=b;
	   }
	   //struct bucket current;
	   int i,j;

	   for(i=0;i<s;i++){ //for all buckets
            struct bucket *current=&bucket_list[i];
            for(j=0;j<current->num_inputs;j++){//re-input all
                struct input *in=&current->bucket_buffer[j];
                rehash(self,in);
                //hashtable_print(self);
		      }
	   }

       //free old buckets
	   free_buckets(bucket_list,s);

	   int bucket_index=x.my_hash%new_size;
	   struct bucket * my_new_bucket=&self->buffer[bucket_index];

	   success=bucket_toss(my_new_bucket, &x);
        //succes=bucket_toss(my_new_bucket,value);
	   self->insertions+=success;
	   self->length+=success;

    }

    else{
        int bucket_index=h%s;
        struct bucket * my_new_bucket=&self->buffer[bucket_index];
        //bucket_toss(my_new_bucket, &x);
        success=bucket_toss(my_new_bucket,&x);
       self->insertions+=success;
       self->length+=success;
    }
    unlock(&(self->lock));
}

void hashtable_print(volatile struct  hashtable *self){
    spin_lock(&(self->lock));
    int j;
    int len=self->size;
    for(j=0;j<len;j++){
        if(self->buffer[j].num_inputs>0){
            printf("[Bucket %d]: ",j);
	       bucket_print(&self->buffer[j]);
       }
    }
    unlock(&(self->lock));
}

void hashtable_elements_print(volatile struct  hashtable *self){
    spin_lock(&(self->lock));
    printf("Value\tCount\n");
    int i,j,k;
    struct input * temp;
    for(i=0;i<self->size;i++){
        k=self->buffer[i].num_inputs;
        temp=self->buffer[i].bucket_buffer;
        for(j=0;j<k;j++){
            printf("  %d\t%x\n",temp[j].value,temp[j].count );
        }
    }
    unlock(&(self->lock));
}

int bucket_remove(struct bucket *self, int my_key){
    int size=self->num_inputs;
    int i,tempkey;
    struct input * buff=self->bucket_buffer;
    //unsigned int * buff=self->values;
    for(i=0;i<size;i++){
	//tempkey=buff[i].key;
        tempkey=buff[i].value;
	   if(tempkey==my_key){
	       self->num_inputs--;
	       //memmove(buff+i,buff+i+1,sizeof(struct input*)*(size-i-1)); //shift everything to the left
            memcpy(buff+i,buff+i+1,sizeof(struct input)*(size-i-1));
	       //free(self->);
	       return 1;//return 1 if succesfully removed
	
        }
    }
    //printf("ERROR key %d is not defined.\n",my_key);
    return 0;//return 0 if nothing removed
}


int hashtable_remove(volatile struct hashtable *self, int key){
    spin_lock(&(self->lock));
    unsigned int h=hasher(key);
    struct bucket *buf=self->buffer;
    unsigned int s=self->size;
    int bucket_index=h%s;
    int success=bucket_remove(&buf[bucket_index],key);
    
    if(success){//if remove was successful 
    	self->length--;
    }
    unlock(&(self->lock));
    return success;
}

void hashtable_stats(volatile struct hashtable *self){
    spin_lock(&(self->lock));
	unsigned int i= self->insertions;
	unsigned int l= self->length;
	unsigned int s= self->size;
    unlock(&(self->lock));
	printf("Length = %d; Size = %d; Insertions = %d\n",l,s,i);
}

void test1(void) {
    struct hashtable a;

    hashtable_create(&a,2,1);
    hashtable_put(&a,13,1);
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

    hashtable_create(&a,2,1);
    for (i=1; i<10; i++) {
        hashtable_put(&a, i, 1);
        //printf("i: %d\n",i );
       // hashtable_print(&a);
        if (hashtable_get(&a, i) != i) {
            //hashtable_print(&a);
            printf("test2a failed. expected %d got %d at loc %d\n",i,hashtable_get(&a,i),hasher(i)%a.length);
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
    //hashtable_print(&a);

}

// Test insertions w/ deletions
void test3(void) {
    struct hashtable a;
    int i;

    hashtable_create(&a,2,1);

    for (i=0; i<100; i++) {
        hashtable_put(&a, i,1);
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

    hashtable_create(&a,2,1);
    hashtable_put(&a, 2001,1);
    for (i=0; i < 1000; i++) {
        hashtable_put(&a, i,1);
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
    //hashtable_print(&a);

}




