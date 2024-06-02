#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "../include/kvstore.h"

#if ENABLE_HASH	

kvs_hashtable global_hash;

int hash_index_compute(char *key,int size)
{
    if(!key) return -1;
    int sum = 0;
    int i = 0;
    while(key[i] != 0)
    {
        sum += key[i];
        i++;
    }
    return sum % size;
}

kvs_hashnode* hash_create_node(char *key,char *value)
{
    kvs_hashnode *node = (kvs_hashnode*)kvs_malloc(sizeof(kvs_hashnode));
    if(!node) return NULL;

    char* keycopy = kvs_malloc(strlen(key)+1);
    if (keycopy == NULL) return NULL;
	memset(keycopy, 0, strlen(key) + 1);
	strncpy(keycopy, key, strlen(key));
    node->key = keycopy;

    
    char* valuecopy = kvs_malloc(strlen(value)+1);
    if (valuecopy == NULL) {return NULL; kvs_free(keycopy);}
	memset(valuecopy, 0, strlen(value) + 1);
	strncpy(valuecopy, value, strlen(value));
    node->value = valuecopy;

    node->next = NULL;

	return node;
}

int kvs_hash_create(kvs_hashtable *inst)
{
    if(!inst) return -1;

    inst->nodes = (kvs_hashnode**)kvs_malloc(sizeof(kvs_hashnode*) * MAX_TABLE_SIZE);
    if (!inst->nodes) return -1;

    inst->max_slots = MAX_TABLE_SIZE;
	inst->count = 0; 

	return 0;
}

void kvs_hash_destory(kvs_hashtable *inst)
{
    if (!inst) return;

    for(int i = 0; i < MAX_TABLE_SIZE ;i++)
    {
        kvs_hashnode *node = inst->nodes[i];

        while(node != NULL)
        {
            kvs_hashnode *tmp = node;
			node = node->next;
			inst->nodes[i] = node;
			
			kvs_free(tmp);
        }
    }

    kvs_free(inst->nodes);
}

int kvs_hash_set(kvs_hashtable *table,char *key,char *value)
{
    if (!table || !key || !value) return -1;

    int index = hash_index_compute(key,MAX_TABLE_SIZE);
    kvs_hashnode *flag = table->nodes[index];
    while(flag != NULL)
    {
        if(strcmp(key,flag->key) == 0)
        return 1; //重复

        flag = flag->next;
    }
    
    kvs_hashnode *new_node = hash_create_node(key, value);
    new_node->next = table->nodes[index];
    table->nodes[index] = new_node;

    
    table->count++;
    return 0;
}

char *kvs_hash_get(kvs_hashtable *table,char *key)
{
    char *res = NULL;
    if (!table || !key) return res;

    
    int index = hash_index_compute(key,MAX_TABLE_SIZE);
    kvs_hashnode *flag = table->nodes[index];
    while(flag != NULL)
    {
        if(strcmp(key,flag->key) == 0)
        {
            return flag->value;
        }

        flag = flag->next;
    }
    return res;
}

int kvs_hash_mod(kvs_hashtable *table, char *key, char *value)
{
    if (!table || !key || !value) return -1;

    int index = hash_index_compute(key,MAX_TABLE_SIZE);

    kvs_hashnode *flag = table->nodes[index];

    while(flag != NULL)
    {
        if (strcmp(flag->key, key) == 0) {
			break;
		}
        flag = flag->next;
    }

    if(flag == NULL)
    {
        return 1;
    }

    kvs_free(flag->value);

    char *valuecopy = kvs_malloc(strlen(value) + 1);
	if (valuecopy == NULL) return -2;
	memset(valuecopy, 0, strlen(value) + 1);
	strncpy(valuecopy, value, strlen(value));

	flag->value = valuecopy;
	
	return 0;   
}

int kvs_hash_count(kvs_hashtable *table) {
	return table->count;
}

int kvs_hash_exist(kvs_hashtable *table, char *key) {

	char *value = kvs_hash_get(table, key);
	if (!value) return 1;

	return 0;
}

int kvs_hash_del(kvs_hashtable *table, char *key) {
	if (!table || !key) return -2;

	int idx = hash_index_compute(key, MAX_TABLE_SIZE);

	kvs_hashnode *head = table->nodes[idx];
	
    if(head == NULL)  return 1; //not exist
    
    if (strcmp(head->key, key) == 0) {
		kvs_hashnode *tmp = head->next;
		table->nodes[idx] = tmp;
		
        kvs_free(head->key);
        kvs_free(head->value);
		kvs_free(head);
		table->count --;	
		return 0;
	}
	
	while (head->next != NULL) {
		if (strcmp(head->next->key, key) == 0) break;
        
        head = head->next; // search node
	}

	if (head->next == NULL) return 1; //not exist

	kvs_hashnode *tmp = head->next;
	head->next = tmp->next;

	kvs_free(tmp->key);
	kvs_free(tmp->value);
	kvs_free(tmp);
	
	table->count --;
	return 0;
}


























#endif