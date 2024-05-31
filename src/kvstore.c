#include "../include/kvstore.h"

#include <string.h>
#include<stdlib.h>




void *kvs_malloc(size_t size)
{
	return malloc(size);
}

void kvs_free(void *ptr)
{
	return free(ptr);
}

const char *command[] = {
	"SET", "GET", "DEL", "MOD", "EXIST",
	"RSET", "RGET", "RDEL", "RMOD", "REXIST",
	"HSET", "HGET", "HDEL", "HMOD", "HEXIST"
};

int kvs_split_token(char *msg,char *tokens[])
{
	if(msg == NULL || tokens == NULL)
		return -1;
	
	int index = 0;
	char* rest = msg;
	char* token;


    while ((token = strtok_r(rest, " ", &rest))) {
        tokens[index++] = token;
    }

	printf("tokens[0] %s",tokens[0]);
	printf("tokens[1] %s",tokens[1]);
	return index;

}

int kvs_filter_protocol(char **tokens, int count, char *response) 
{
	if (tokens[0] == NULL || count == 0 || response == NULL) return -1;

	int cmd = KVS_CMD_START;
	for (cmd = KVS_CMD_START;cmd < KVS_CMD_COUNT;cmd ++) {
		if (strcmp(tokens[0], command[cmd]) == 0) {
			break;
		} 
	}

	int length = 0;
	int ret = 0;
	char *key = tokens[1];
	char *value = tokens[2];

	switch(cmd) {
#if ENABLE_ARRAY
	case KVS_CMD_SET:
		ret = kvs_array_set(&global_array ,key, value);
		if (ret < 0) {
			length = sprintf(response, "ERROR\r\n");
		} else if (ret == 0) {
			length = sprintf(response, "OK\r\n");
		} else {
			length = sprintf(response, "EXIST\r\n");
		} 
		
		break;
	case KVS_CMD_GET: {
		char *result = kvs_array_get(&global_array, key);
		if (result == NULL) {
			length = sprintf(response, "NO EXIST\r\n");
		} else {
			length = sprintf(response, "%s\r\n", result);
		}
		break;
	}
	case KVS_CMD_DEL:
		ret = kvs_array_del(&global_array ,key);
		if (ret < 0) {
			length = sprintf(response, "ERROR\r\n");
 		} else if (ret == 0) {
			length = sprintf(response, "OK\r\n");
		} else {
			length = sprintf(response, "NO EXIST\r\n");
		}
		break;
	case KVS_CMD_MOD:
		ret = kvs_array_mod(&global_array ,key, value);
		if (ret < 0) {
			length = sprintf(response, "ERROR\r\n");
 		} else if (ret == 0) {
			length = sprintf(response, "OK\r\n");
		} else {
			length = sprintf(response, "NO EXIST\r\n");
		}
		break;
	case KVS_CMD_EXIST:
		ret = kvs_array_exist(&global_array ,key);
		if (ret == 0) {
			length = sprintf(response, "EXIST\r\n");
		} else {
			length = sprintf(response, "NO EXIST\r\n");
		}
		break;
#endif

	}
	return length;
}



int kvs_protocol(char *msg, int length, char *response) {  //
	
// SET Key Value
// GET Key
// DEL Key
	if (msg == NULL || length <= 0 || response == NULL) return -1;

	//printf("recv %d : %s\n", length, msg);

	char *tokens[KVS_MAX_TOKENS] = {0};

	int count = kvs_split_token(msg, tokens);
	if (count == -1) return -1;

	//memcpy(response, msg, length);

	return kvs_filter_protocol(tokens, count, response);
}















int kv_store(char *msg, int length, char *response)
{
    sprintf(response,msg,length);
    return length;
}

void dest_(void) {
#if ENABLE_ARRAY
	kvs_array_destory(&global_array);
#endif
#if ENABLE_RBTREE
	kvs_rbtree_destory(&global_rbtree);
#endif
#if ENABLE_HASH
	kvs_hash_destory(&global_hash);
#endif

}

int init_()
{	
#if ENABLE_ARRAY
	memset(&global_array, 0, sizeof(kvs_array));
	kvs_array_create(&global_array);
#endif
}

int main()
{
	init_();
    int port = 9999;

    reactor_start(port, kvs_protocol);

	

#if (NETWORK_SELECT == NETWORK_REACTOR)
	reactor_start(port, kvs_protocol);  //
#elif (NETWORK_SELECT == NETWORK_PROACTOR)
	ntyco_start(port, kvs_protocol);
#elif (NETWORK_SELECT == NETWORK_NTYCO)
	proactor_start(port, kvs_protocol);
#endif
	dest_();
}