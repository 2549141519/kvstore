#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include"../include/kvstore.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_LEVEL 6
kvs_skiptable global_skiptable;

int randomLevel()
{
    int level = 0;
    while(rand() < RAND_MAX/2 && level < MAX_LEVEL)
    {
        level++;
    }
    return level;
}

kvs_skipnode* createSkipNode(int level,char *key,char *value)
{
    kvs_skipnode *newnode = (kvs_skipnode*)kvs_malloc(sizeof(kvs_skipnode));
    if(!newnode) return NULL;

    newnode->key = (char *)kvs_malloc(strlen(key)+1);
    if (newnode->key == NULL) return NULL;
	memset(newnode->key, 0, strlen(key) + 1);
	strncpy(newnode->key, key, strlen(key));
   
    
    newnode->value = (char *)kvs_malloc(strlen(value)+1);
    if (newnode->value == NULL) return NULL;
	memset(newnode->value, 0, strlen(value) + 1);
	strncpy(newnode->value, value, strlen(value));

    newnode->next = (kvs_skipnode**)kvs_malloc((level + 1) * sizeof(kvs_skipnode*));
    if(!newnode->next)   return NULL;

    for(int i = 0;i <= level;i++)
    {
        newnode->next[i] = NULL;
    }

    return newnode;
}

int kvs_skip_create(kvs_skiptable* table)
{
    if(!table) return -1;

    

    table->level = 0;
    
    table->head = createSkipNode(MAX_LEVEL,"","");
    if(!table->head) return -1;
    
    return 0;
}

void kvs_skip_destory(kvs_skiptable* table)
{
    
        kvs_skipnode *root = table->head->next[0];
      
        while(root != NULL)
        {
            kvs_skipnode *tem = root;
            root = root->next[0];
            kvs_free(tem->key);
            kvs_free(tem->value);
            kvs_free(tem->next);
            kvs_free(tem);
        }

        kvs_free(table);   
}

char* kvs_skip_get(kvs_skiptable*table,char *key)
{
    if(!key || !table) return NULL;

    kvs_skipnode *currentnode = table->head;

    for(int i = table->level; i>=0 ;i--)
    {
        while(currentnode->next[i] != NULL 
            && strcmp(currentnode->next[i]->key,key) < 0)
        {
            currentnode = currentnode->next[i];
        }
    }

    currentnode = currentnode->next[0];
    
    if(currentnode && strcmp(key,currentnode->key) == 0)
        return currentnode->value;
    
    return NULL;

}

int kvs_skip_set(kvs_skiptable *table,char *key,char *value)
{
    if(!table || !key || !value) return -1;

    kvs_skipnode* updatenode[MAX_LEVEL + 1];

    kvs_skipnode *currentnode = table->head;

    for(int i = table->level; i>=0 ;i--)
    {
        while(currentnode->next[i] != NULL 
            && strcmp(currentnode->next[i]->key,key) < 0)
        {
            currentnode = currentnode->next[i];
        }
        updatenode[i] = currentnode;
    }

    currentnode = currentnode->next[0];
    if(currentnode == NULL || strcmp(currentnode->key,key) != 0)
    {
        int setlevel = randomLevel();

        if(setlevel > table->level)
        {
            for (int i = table->level + 1; i <= setlevel; ++i)
            {
                updatenode[i] = table->head;
            }//update[i] 为null
            table->level = setlevel;
        }

        kvs_skipnode* newnode = createSkipNode(setlevel, key, value);
        if(!newnode) return -1;

        for(int i = 0; i<= setlevel ;i++)
        {
            newnode->next[i] = updatenode[i]->next[i];
            updatenode[i]->next[i] = newnode;
        }

        return 0;
    }
    return 1; 
}

int kvs_skip_exist(kvs_skiptable *table,char *key)
{
    if(!table || !key)
        return -1;
    
    if(kvs_skip_get(table,key) == NULL)
        return 1;
    else
        return 0; 
}

int kvs_skip_mod(kvs_skiptable *table,char *key,char *value)
{
    if(!table || !key || !value) return -1;

    kvs_skipnode *currentnode = table->head;

    for(int i = table->level; i >= 0 ;i--)
    {
        while(currentnode->next[i] != NULL 
            && strcmp(currentnode->next[i]->key,key) < 0)
        {
            currentnode = currentnode->next[i];
        }
    }

    currentnode = currentnode->next[0];
    
    if(currentnode && strcmp(key,currentnode->key) == 0)
    {
        kvs_free(currentnode->value);

        currentnode->value = (char *)kvs_malloc(strlen(value) + 1);
        if (currentnode->value == NULL) return -1;
        
        memset(currentnode->value, 0, strlen(value) + 1);
        strncpy(currentnode->value, value, strlen(value));

        return 0;
    }
    
    return 1;
}

int kvs_skip_del(kvs_skiptable *table,char *key)
{
    if(!table || !key) return -1;

    kvs_skipnode* updatenode[table->level + 1];

    kvs_skipnode *currentnode = table->head;

    for(int i = table->level; i>=0 ;i--)
    {
        while(currentnode->next[i] != NULL 
            && strcmp(currentnode->next[i]->key,key) < 0)
        {
            currentnode = currentnode->next[i];
        }
        updatenode[i] = currentnode;
    }

    kvs_skipnode *tem = currentnode->next[0];
    if(!tem || strcmp(key,tem->key) != 0)
        return 1;
    for(int i = 0; i <= table->level; i++)
    {
        if(updatenode[i]->next[i] && strcmp(key,updatenode[i]->next[i]->key) == 0)
            {
                updatenode[i]->next[i] = updatenode[i]->next[i]->next[i];
            }
        else
            break;
    }

    for(int i = 0;i<table->level;i++)
    {
        if(table->head->next[i] == NULL)
        {
            table->level = MAX(0,i-1);
        
        break;}
    }
    
    kvs_free(tem->key);
    kvs_free(tem->value);
    kvs_free(tem->next);
    kvs_free(tem);
    return 0;
}

