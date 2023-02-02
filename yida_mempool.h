#include <stdlib.h>

#define SIZE_WORD sizeof(unsigned long)
#define MALLOC_CHECK(ptr)  \
    {                      \
        if ((ptr) == NULL) \
            return NULL;   \
    }
#define MEMORY_ALIGN(ptr, align) \
    (char *)(((unsigned long)(ptr) + ((unsigned long)align - 1)) & ~((unsigned long)align - 1))

typedef struct pool_large
{
    struct pool_large *next;
    void *alloc; // 指向大内存空间
} pool_large_t;

struct pool_data
{
    char *last; // 指向当前可用空间起始
    char *end;  // 指向当前可用空间尾
    struct Yida_mempool *next;
    unsigned int failed;    
}; // 小内存链表头结构体

struct Yida_mempool
{
    struct pool_data data;             // 小内存链表的第一块空间头
    size_t max;                   // 当前块的最大容量
    struct Yida_mempool *current; // 指向当前线程池
    pool_large_t *large;          // 大内存链表
};

typedef struct Yida_mempool Yida_mempool_t;
typedef struct pool_data pool_data_t;

Yida_mempool_t *create_mempool(size_t size)
{
    Yida_mempool_t *p;
    if (size <= sizeof(Yida_mempool_t))
        return NULL;                                 // 要创建的池还没控制块大
    MALLOC_CHECK(p = (Yida_mempool_t *)malloc(size)) // 申请内存总量为size，包含控制块

    p->data.last = (char *)p + sizeof(Yida_mempool_t);
    p->data.end = (char *)p + size;
    p->data.next = NULL;
    p->data.failed = 0;

    size = size - sizeof(Yida_mempool_t);
    p->max = size; // 可用的最大容量
    p->current = p;
    p->large = NULL; // 还没分配大内存
    return p;
}

void *mempool_alloc_block(Yida_mempool_t *pool, size_t size)
{
    char *m;
    size_t psize;
    Yida_mempool_t *p, *novel;

    psize = (size_t)(pool->data.end - (char *)pool);

    MALLOC_CHECK(m = (char *)malloc(psize))
    novel = (Yida_mempool_t *)m;

    novel->data.end = m + psize;
    novel->data.next = NULL;
    novel->data.failed = 0;

    m += sizeof(pool_data_t);
    m = MEMORY_ALIGN(m, SIZE_WORD);
    novel->data.last = m + size;

    for (p = pool->current; p->data.next; p = p->data.next)
    {
        if (p->data.failed++ > 4)
        {
            pool->current = p->data.next;
        }
    }
    p->data.next = novel;
    return m;
}

inline void *mempool_alloc_small(Yida_mempool_t *pool, size_t size, unsigned align)
{
    char *m;
    Yida_mempool_t *p;
    p = pool->current;
    do
    {
        m = p->data.last;
        if (align)
        {
            m = MEMORY_ALIGN(m, SIZE_WORD);
        }
        if ((size_t)(p->data.end - m) >= size)
        {
            p->data.last = m + size;
            return m;
        }
        p = p->data.next;
    } while (p);
    return mempool_alloc_block(pool, size);
}

void *mempool_alloc_large(Yida_mempool_t *pool, size_t size)
{
    void *p;
    unsigned n = 0;
    pool_large_t *large;

    MALLOC_CHECK(p = malloc(size))

    for (large = pool->large; large; large = large->next)
    {
        if (large->alloc == NULL)
        {
            large->alloc = p;
            return p;
        }
        if (n++ > 3)
        {
            break;
        }
    }

    large = (pool_large_t *)mempool_alloc_small(pool, sizeof(pool_large_t), 1);
    if (large == NULL)
    {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

void *mempool_alloc(Yida_mempool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max)
    {
        return mempool_alloc_small(pool, size, 1);
    }
#endif

    return mempool_alloc_large(pool, size);
}

int mempool_free(Yida_mempool_t *pool, void *p)
{
    pool_large_t *l;
    for (l = pool->large; l; l = l->next)
    {
        if (p == l->alloc)
        {
            free(l->alloc);
            l->alloc = NULL;
            return 0;
        }
    }
    return -1;
}

void *mempool_alloc_aligned(Yida_mempool_t *pool, size_t size, size_t alignment)
{
    void *p;
    pool_large_t *large;

    MALLOC_CHECK(p = malloc(size))

    large = (pool_large_t *)mempool_alloc_small(pool, sizeof(pool_large_t), 1);
    if (large == NULL)
    {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}