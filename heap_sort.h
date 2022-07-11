#ifndef _HEAP_SORT_H
typedef int (*heap_sort_cmp)(void*, void*);
#include <stdint.h>
#include <stdbool.h>
/**
 * 定义队列结构体
 */
typedef struct _queue_info{
    size_t    size;            //队列大小
    size_t    ele_size;        //元素大小
    size_t    max_size;        //容量
    heap_sort_cmp cmp;         //排序时候的比较函数
    uint64_t    itimes;        //插入数据的次数
    uint64_t    gtimes;        //取数据的次数
    uint64_t    ftimes;        //清空队列的次数
    uint64_t    etimes;        //读取时队列为空的次数
    uint64_t    ltimes;        //look的次数
    uint64_t    otimes;        //溢出的次数(队列满的次数)
    char	    mem[1];             //内存地址
} queue_info;
extern queue_info* memcache_pq;
#define GET_ELE_PTR(p, pos, nmemb)        (void*)( (char*)p + pos * nmemb )

/**
 * 交换任意类型数据的宏
 */
#define    heap_switch(a, b, nmemb){          \
    char t[nmemb];                            \
    memcpy((void*)&t,(void*)a, nmemb);        \
    memcpy((void*)a, (void*)b, nmemb);        \
    memcpy((void*)b, (void*)&t,nmemb);        \
}
#define PARENT(i)    ( (i - 1) >> 1 )
#define LEFT(i)      ( (i << 1) + 1 )
#define RIGHT(i)     ( (i << 1) + 2 )
queue_info* pq_init(size_t, size_t, heap_sort_cmp);
void pq_reset(queue_info*);
int  pq_resort(queue_info*);
int  pq_get(queue_info*, void*);
int  pq_look(queue_info*, void*);
int  pq_insert(queue_info*, void*);
void pq_stat_reset(queue_info*);
bool pq_is_full(queue_info*);
void pq_deinit(queue_info*);
#endif
