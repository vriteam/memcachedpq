#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "heap_sort.h"

/**
 * 构建一个最大堆的子过程
 * 将当前元素及其子元素符合最大堆规则,即子元素都小于其夫元素
 * ptr 堆首字节的地址
 * size 堆中当前元素的个数
 * pos 构造的启始位置
 * nmemb 一个元素的大小
 * cmp_fun 元素的比较函数
 */
static void max_heap(void *ptr, size_t size, size_t pos, size_t nmemb, heap_sort_cmp cmp_fun){
    int largest = -1;
    /**
     * 如果当前元素不是最大的
     * 交换当前元素和最大元素
     * 交换可能导致调换的子元素违反规则
     */
    while( largest != pos ){
        //找到三个元素(如果有子元素的情况)
        if( RIGHT(pos) < size && cmp_fun( GET_ELE_PTR(ptr, RIGHT(pos), nmemb), GET_ELE_PTR(ptr, pos, nmemb) ) > 0 )
            largest = cmp_fun( GET_ELE_PTR(ptr, RIGHT(pos), nmemb), GET_ELE_PTR(ptr, LEFT(pos), nmemb) ) > 0 ? RIGHT(pos) : LEFT(pos);
        else if( LEFT(pos) < size && cmp_fun( GET_ELE_PTR(ptr, LEFT(pos), nmemb), GET_ELE_PTR(ptr, pos, nmemb) ) > 0)
            largest = LEFT(pos);
        else
            largest = pos;
        //交换元素
        if( largest != pos ){
            heap_switch( GET_ELE_PTR(ptr, pos, nmemb), GET_ELE_PTR(ptr, largest, nmemb), nmemb);
            heap_switch(&pos, &largest, sizeof(largest));
        }
    }
}
/**
 * 构造一个最大堆
 * 将头一半的数据依次构造
 * d 堆首元素的地址
 * s 堆中元素的个数
 * nmemb 一个元素的大小
 * cmp_fun 元素的比较函数
 * PARENT(s - 1)为数组最后元素的父亲
 */
static void build_max_heap(void *d, size_t s, size_t nmemb, heap_sort_cmp cmp_fun){
    int i;
    for( i = PARENT(s - 1); i >= 0 ; i--)
        max_heap(d, s, i, nmemb, cmp_fun);
}

/**
 * 在最大堆上修改某个元素大小
 * ptr 堆首的地址
 * pos 要修改的元素的位置
 * v 新元素的指针
 * s 堆中元素的数量
 * nmemb 一个元素的大小
 * cmp_fun 元素的比较函数
 */
static void max_heap_increase(void *ptr, size_t pos, void* v, size_t s,
                                  size_t nmemb, heap_sort_cmp cmp_fun){
    //增加其值
    if( cmp_fun(v, GET_ELE_PTR(ptr, pos, nmemb) ) > 0 ){
        memcpy(GET_ELE_PTR(ptr, pos, nmemb), v, nmemb);
        while( pos > 0 && cmp_fun( GET_ELE_PTR(ptr, pos, nmemb), GET_ELE_PTR(ptr, PARENT(pos), nmemb) ) > 0 ){
            heap_switch(GET_ELE_PTR(ptr, pos, nmemb), GET_ELE_PTR(ptr, PARENT(pos), nmemb), nmemb);
            pos = PARENT(pos);
        }
    }else{//减小了某值,从该数据开始重新构建最大堆
        memcpy(GET_ELE_PTR(ptr, pos, nmemb), v, nmemb);
        max_heap(ptr, pos, s, nmemb, cmp_fun);
    }
}
/**
 * 在最大堆上插入数据
 * ptr 堆首的地址
 * src 要插入的源数据
 * pos 要修改的元素的位置
 * s 堆中元素的数量
 * nmemb 一个元素的大小
 * cmp_fun 元素的比较函数
 */
static void max_heap_insert(void *ptr, void* src, size_t pos, size_t s,
                                  size_t nmemb, heap_sort_cmp cmp_fun){
    bzero(GET_ELE_PTR(ptr, pos, nmemb), nmemb);
    max_heap_increase(ptr, pos, src, s, nmemb, cmp_fun);
}
/**
 * 在最大堆上取数据
 * 取出堆首元素,将最后一个元素放到首位,然后重新构造最大堆
 * dest 目标地址的指针
 * ptr 堆首的地址
 * s 堆中元素的数量
 * nmemb 一个元素的大小
 * cmp_fun 元素的比较函数
 */
static void max_heap_get(void *dest, void *ptr, size_t s, size_t nmemb, heap_sort_cmp cmp_fun){
    memcpy(dest, ptr, nmemb);
    memcpy(ptr, GET_ELE_PTR(ptr, (s - 1), nmemb), nmemb);
    max_heap(ptr, s - 1, 0, nmemb, cmp_fun);
}
/**
 * 查看堆首元素
 * dest 目标地址的指针
 * ptr 堆首的地址
 * nmemb 一个元素的大小
 */
static void max_heap_look(void *dest, void *ptr, size_t nmemb){
    memcpy(dest, ptr, nmemb);
}
/**
 * 初始化队列的计数状态
 */
static void queue_stat_init(queue_info *q) {
    q->otimes = q->ltimes = q->etimes = q->ftimes = q->itimes = q->gtimes = 0;
}
/**
 * 优先级队列的插入操作
 */
int pq_insert(queue_info* q, void* d){
    if( q->max_size == q->size ) {
        q->otimes++;
        return 1;
    }
    q->size++;
    max_heap_insert(q->mem, d, q->size - 1, q->size, q->ele_size, q->cmp);
    q->itimes++;
    return 0;
}
/**
 * 优先级队列的读取操作
 */
int pq_get(queue_info* q, void* d){
    if(q->size == 0) {
        q->etimes++;
        return 1;
    }
    max_heap_get(d, q->mem, q->size, q->ele_size, q->cmp);
    q->size--;
    q->gtimes++;
    return 0;
}
/**
 * 优先级队列的查看
 */
int pq_look(queue_info* q, void* d){
    if(q->size == 0) {
        q->etimes++;
        return 1;
    }
    max_heap_look(d, q->mem, q->ele_size);
    q->ltimes++;
    return 0;
}
/**
 * 重新构造一条优先级队列
 */
int pq_resort(queue_info* q){
    build_max_heap(q->mem, q->size, q->ele_size, q->cmp);
    return 0;
}

/**
 * 初始化优先级队列
 */
queue_info* pq_init(size_t msize, size_t esize, heap_sort_cmp cmp){
    //分配优先级队列描述符的空间
    queue_info *q = malloc( sizeof(queue_info) + msize * esize - 1);
    if(q == NULL)
        return NULL;
    queue_stat_init(q);
    q->cmp      = cmp;
    q->max_size = msize;
    q->ele_size = esize;
    q->size     = 0;
    return q;
}

/**
 * 清空队列
 */
void pq_reset(queue_info* q){
    q->size = 0;
    q->ftimes++;
}

/**
 * 重置队列状态
 */
void pq_stat_reset(queue_info* q){
    queue_stat_init(q);
}
/**
 * 队列是否满了
 */
bool pq_is_full(queue_info* q){
    return q->size >= q->max_size;
}
void pq_deinit(queue_info* q){
    if(q != NULL)
        free(q);
}

