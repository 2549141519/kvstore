#pragma once

#include <stdio.h>
#include <string.h>

#define NETWORK_REACTOR 	0
#define NETWORK_PROACTOR	1
#define NETWORK_NTYCO		2
#define NETWORK_SELECT		NETWORK_REACTOR
#define KVS_MAX_TOKENS		128

#define ENABLE_ARRAY		0
#define ENABLE_RBTREE		0
#define ENABLE_HASH			1




enum {
	KVS_CMD_START = 0,
	// array
	KVS_CMD_SET = KVS_CMD_START,
	KVS_CMD_GET,
	KVS_CMD_DEL,
	KVS_CMD_MOD,
	KVS_CMD_EXIST,
	
	KVS_CMD_COUNT,
};

typedef int (*msg_handler)(char *msg, int length, char *response);



void* kvs_malloc(size_t size);
void kvs_free(void* ptr);


extern int reactor_start(unsigned short port, msg_handler handler);
extern int proactor_start(unsigned short port, msg_handler handler);
extern int coroutine_start(unsigned short port, msg_handler handler);

#define KVS_ARRAY_SIZE 512

#if ENABLE_ARRAY

typedef struct kvs_array_node
{
    char *key;
    char *value;
}kvs_array_node;

typedef struct kvs_array
{
    kvs_array_node *table;
    int total;
    
}kvs_array;

extern kvs_array global_array;
int kvs_array_create(kvs_array *inst);
void kvs_array_destory(kvs_array *inst);

int kvs_array_set(kvs_array *inst, char *key, char *value);
char* kvs_array_get(kvs_array *inst, char *key);
int kvs_array_del(kvs_array *inst, char *key);
int kvs_array_mod(kvs_array *inst, char *key, char *value);
int kvs_array_exist(kvs_array *inst, char *key);
#endif

#if ENABLE_HASH	


#define MAX_TABLE_SIZE	1024

typedef struct kvs_hashnode
{
	char *key;
	char *value;

	struct kvs_hashnode *next;
}kvs_hashnode;

typedef struct kvs_hashtable
{
	kvs_hashnode **nodes;
	int max_slots;
	int count;

}kvs_hashtable;

extern kvs_hashtable global_hash;


int kvs_hash_create(kvs_hashtable *table);
void kvs_hash_destory(kvs_hashtable *table);
int kvs_hash_set(kvs_hashtable *table, char *key, char *value);
char * kvs_hash_get(kvs_hashtable *table, char *key);
int kvs_hash_mod(kvs_hashtable *table, char *key, char *value);
int kvs_hash_del(kvs_hashtable *table, char *key);
int kvs_hash_exist(kvs_hashtable *table, char *key);



#endif

