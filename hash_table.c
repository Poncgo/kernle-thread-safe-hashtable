
#include <linux/init.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define HASH_TB_MIN_NUM 8

#define HLIST_SIZE sizeof(struct hlist_head)

//load factor  percent ,if larger than 90 expand table size to 2 * ( size)
#define EXPAND_FACTOR 90
//else shrink to 1/2 * size
#define SHRINK_FACTOR 10

typedef struct hash_table_t
{
    struct hlist_head *hash_table;
    unsigned int size;
    unsigned int used;
    unsigned int rehashidx;
}hash_table;

typedef struct hash_entity_t
{
    struct hlist_node node; //use hlist_node to link entity while conflict, also by convenience of api of hlist.
    char * key;
    char * val;
}hash_entity;

static hash_table ht;

static inline void INIT_HASH_ENTITY(hash_entity *n, char *k, char *v)
{
    INIT_HLIST_NODE(&(n->node));
    n->key = k;
    n->val = v;
}

/*
    hash code generate
*/
static inline unsigned int hash_code(char* str, unsigned int len)
{
    unsigned int hash = len;

    while (*str != "\n")
    {
        hash = ((hash << 5) ^ (hash >> 27)) ^ (*str++);
    }
    return hash;
}

unsigned int DJBHash(char *str)
{
    unsigned int hash = 5381;
 
    while (*str)
    {
        hash = ((hash << 5) + hash) + (*str++); /* times 33 */
    }
    hash &= ~(1 << 31); /* strip the highest bit */
    return hash;
}


/*
    get the idx of hash table by mode tabel size
*/
static inline unsigned int hash_idx(char * key)
{

    unsigned int hash = 0;
    if (NULL == key)
    {
        printk("key is NULL\n");
        return -1;
    }

    hash = DJBHash(key);// better one

    return hash % ht.size;
}

/*
   judge if a key exist, and return hash_table index, else -1
*/
static inline hash_entity *hash_is_exist(char *key)
{

    if (NULL == key)
    {
        printk(KERN_DEBUG "key is  null\n");
        return NULL;
    }

    unsigned int idx      = hash_idx(key);

    struct hlist_node *he = ht.hash_table[idx].first;
    hash_entity *p_entity = NULL;

    while(he)
    {
        p_entity = (hash_entity *) he;
        if (0 == strcmp(p_entity->key, key))
        {
            return p_entity;
        }
        he = he->next;
    }
    return NULL;
}

int hash_add(char *key, char *val)
{
    int idx = 0;
    int l_k = strlen(key);
    int l_v = strlen(val);
    char *k = NULL;
    char *v = NULL;
    struct hlist_head *he = NULL;
    hash_entity *p_entity = NULL;

    if  ((NULL == val) || (NULL == key))
    {
        printk( "NULL POINTER of key or val\n");
        return -1;
    }

    if ( 0 > (idx = hash_idx(key)))
    {
        printk("idx get failed\ns");
        return -1;
    }

    if (NULL != (p_entity = hash_is_exist(key)))
    {
        printk("key exist already\n");
        return -1;
    }

    p_entity = kmalloc(sizeof(hash_entity), GFP_KERNEL);
    k = kmalloc(l_k * sizeof(char), GFP_KERNEL);
    v = kmalloc(l_v * sizeof(char), GFP_KERNEL);

    copy_from_user(k, key, l_k * sizeof(char));
    copy_from_user(v, val, l_v * sizeof(char));

    INIT_HASH_ENTITY(p_entity, k, v);
    he = (ht.hash_table + idx);
    hlist_add_head(&(p_entity->node), he);
    ht.used++;
    return idx;
}


int hash_del(char *key, char *val)
{
    int idx = hash_idx(key);
    hash_entity *p_entity = NULL;
    struct hlist_node *node = NULL;
    if (0 > idx)
    {
        printk( "key error\n");
        return -1;
    }

    p_entity = hash_is_exist(key);
    if (NULL == p_entity)
    {
        printk( "no key found while del\n");
        return -1;
    }

    node = &(p_entity->node);
    hlist_del(node);
    ht.used--;

    if (NULL != val)
    {
        copy_to_user(val, p_entity->val, strlen(p_entity->val));
    }

    kfree(p_entity->key);
    kfree(p_entity->val);
    kfree(p_entity);

    return 0;
}

int hash_find(char *key, char *val)
{
    int idx = hash_idx(key);
    hash_entity *p_entity = NULL;

    if (0 > idx)
    {
        printk( "key error\n");
        return -1;
    }

    p_entity = hash_is_exist(key);
    if (NULL == p_entity)
    {
        printk( "no key found while del\n");
        return -1;
    }

    if (NULL != val)
    {
        copy_to_user(val, p_entity->val, strlen(p_entity->val));
    }
    return 0;
}

MODULE_LICENSE("Dual BSD/GPL");

/*
    init hash table by min size HASH_TB_MIN_NUM
*/
static __init int HASH_INIT(void)
{
    struct hlist_head *head = NULL;
    ht.size = 0;
    ht.rehashidx = 0;
    int i = 0;

    if (NULL != ht.hash_table)
    {
        printk("hash table not empty\n");
        return;
    }
    head = (struct hlist_head *)kcalloc(HASH_TB_MIN_NUM, HLIST_SIZE, GFP_KERNEL);
    if (NULL == head)
    {
        printk( "kcalloc failed while init\n");
        return;
    }
    ht.size = HASH_TB_MIN_NUM;
    ht.hash_table = head;
    for (i = 0; i < ht.size; ++i)
    {
        INIT_HLIST_HEAD(head + i);
    }

}


void HASH_DEL(void)
{
    int i = 0;
    struct hlist_head *he = NULL;
    struct hlist_node *node = NULL;
    hash_entity *p_entity = NULL;
    hash_entity *p_tmp = NULL;

    if ((0 == ht.size) && ( NULL == ht.hash_table))
    {
        return;
    }
    for (i =0; i < ht.size; ++i)
    {
        he = (ht.hash_table + i);
        hlist_for_each(node, he)
        {
            if (p_tmp)
            {
                kfree(p_tmp);
                p_tmp = NULL;
            }
            p_entity = (hash_entity *)node;
            p_tmp = p_entity;
            kfree(p_entity->key);
            kfree(p_entity->val);
        }
        if (p_tmp)
        {

        }

    }
    
    //del element
}
module_init(HASH_INIT);  
module_exit(HASH_DEL);  