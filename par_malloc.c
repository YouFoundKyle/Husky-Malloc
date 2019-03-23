#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "xmalloc.h"

typedef struct node_st{
	size_t block_size;
	struct node_st* next;
	struct node_st* prev;
	
} node;


const size_t PAGE_SIZE = 4096;
size_t NODE_SIZE = sizeof(node);
size_t MIN_NODE_SIZE = sizeof(node) * 2;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 


__thread node* bin[7];


//8 16 32 64 128 256 512 1024 4096
int
pick_bin(size_t size){
	int ind = 3;
	
	while (size > pow(2,ind)){
		ind += 1;
	}
	return ind-3;	
}




//initialize empty free_list
__thread node* freels = NULL;

static
size_t
div_up(size_t xx, size_t yy)
{
	// This is useful to calculate # of pages
	// for large allocations.
	size_t zz = xx / yy;
	
	if (zz * yy == xx) {
		return zz;
	}
	else {
		return zz + 1;
	}
}

/*
 * First Fit 
 * 
 * then go to best fit -> find block that will lead to least left over bytes
 */

// node*
// pick_prev_node(node* n){
// 	node* check = freels;
// 	while(check != NULL && check->next != NULL){
// 		if (check->next == n){
// 			return check;
// 		}
// 		check = check->next;   
// 	}
// 	return freels;
// }



void
insert_node(node* n){
	
	n->next = NULL;
	
	
	if(freels == NULL || freels->next == NULL){
		freels = n;
	}
	else{
		node* check = freels;
		while(check != NULL && check->next != NULL){
			if (check->next < n &&
				((check->next->next == NULL || check->next->next > n))){
				n->next = check->next;
			check->next = n;
			n->prev = check;
			return;
				}
				check = check->next; 
		}
	}
}


node*
add_node(void* n, size_t b_size ){
	
	node* n_node = n;
	
	
	n_node->block_size = b_size - NODE_SIZE;
	n_node->next = NULL;
	n_node->prev = NULL;
	
	insert_node(n_node);
	
	return n_node;
}

//remove a node from the free_list
void
remove_node(node* n){
	
	if(freels == n){
		freels = NULL;
	}
	else {
		n->prev->next = n->next;
	}
}

//extract free space from the node if it exists
node*
split_node(node* n, size_t size ){
	
	size_t free_space = n->block_size - size ;
	
	//find the increment in terms of sizeof(node*)
	long p_inc = (size + NODE_SIZE)/ NODE_SIZE;
	
	//check if there is enough space to split the node
	if ( free_space > MIN_NODE_SIZE){
		
		remove_node(n);
		node* a_node = add_node((n + p_inc ), free_space);
		
		n->block_size = size;
	}
	else {
		remove_node(n);
	}
	return n;
}

node*
pick_free_node(size_t size){
	
	node* check = freels;
	
	while(check != NULL){
		if (check->block_size > size){ //is node size large enough
			return split_node(check, size);
		}
		check = check->next;
	}
	return NULL;
}


//allocate memory
void*
xmalloc(size_t size)
{
	//increase size to take into account the size var
	size += sizeof(size_t);
	
	size_t pages = div_up(size, PAGE_SIZE);
	// Actually allocate memory with mmap and a free list.
	
	
	size_t b_size = pages * PAGE_SIZE;
	if (pages > 1){
		node* p =  mmap(0,b_size, PROT_READ|PROT_WRITE|PROT_EXEC,MAP_ANON|MAP_PRIVATE, -1,0 );
		p->block_size = b_size;
		
		return (void*) p + sizeof(size_t);
		
	}
	
	pthread_mutex_lock(&mutex);
	
	node* free_n = pick_free_node(size);
	pthread_mutex_unlock(&mutex);
	
	//found free block
	if (free_n != NULL){
		return (void*) free_n + sizeof(size_t);
	}
	
	pthread_mutex_lock(&mutex);
	
	node* p =  mmap(0,b_size, PROT_READ|PROT_WRITE|PROT_EXEC,MAP_ANON|MAP_PRIVATE, -1,0 );
	p->block_size = size;
	size_t free_space = b_size - size;
	
	long p_inc = (size + NODE_SIZE)/ NODE_SIZE;
	
	if (free_space > MIN_NODE_SIZE ){
		add_node(p + p_inc, free_space );
	}
	
	pthread_mutex_unlock(&mutex);
	return (void*) p + sizeof(size_t);
	
	
}

//free an alllocated node
void
xfree(void* item)
{
	pthread_mutex_lock(&mutex);
	void* node_start = item- sizeof(size_t);
	
	//get the size of the node
	size_t bl_size = *(size_t *) node_start;
	
	size_t page_num = bl_size/PAGE_SIZE;
	
	if (page_num > 1){
		munmap(node_start, bl_size );
	}
	else
	{
		insert_node(node_start);
	}
	
	pthread_mutex_unlock(&mutex);
	
}

//allocate a node to a new address
void*
xrealloc(void* prev, size_t bytes)
{
	void* new = xmalloc(bytes);
	memcpy(new, prev, bytes);
	xfree(prev);
	return new;
	
}



