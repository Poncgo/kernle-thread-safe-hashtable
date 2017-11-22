#ifndef HASH_TABLE_H_INCLUDED
#define HASH_TABLE_H_INCLUDED

#include <linux/list.h>
#include <as>

#define HASH_TB_MIN_NUM 8

#define HLIST_SIZE sizeof(struct hlist_head)

//load factor  percent ,if larger than 90 expand table size to 2 * ( size)
#define EXPAND_FACTOR 90
//else shrink to 1/2 * size
#define SHRINK_FACTOR 10

#define KEY_LEN 20

#define VAL_LEN 40

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

typedef struct hash_table_t
{
    struct hlist_head *hash_table;
    atomic_t size;
    atomic_t used;
    atomic_t rehashidx;
}hash_table;

typedef struct hash_entity_t
{
    struct hlist_node node; //use hlist_node to link entity while conflict, also by convenience of api of hlist.
    char * key;
    char * val;
}hash_entity;

int hash_add(char *key, char *val);

int hash_del(char *key, char *val);

int hash_find(char *key, char *val);

int rehash_table(void);



#endif // HASH_TABLE_H_INCLUDED
