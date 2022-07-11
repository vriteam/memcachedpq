#include <stdio.h>
#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <libmemcached/memcached.h>

#define S_IP        "127.0.0.1"
#define S_PORT       11211
#define TIMES        (1<<20)
#define THREAD_NUMS      10
#define THREAD_G_NUMS    10
#define MPQ_KEY          "maxpriority"

#define BUFFER_SIZE      4096
#define KEY_SIZE         20
typedef struct {
    char *ip;
    unsigned int port;
    unsigned long long times;
    int snums;
    int gnums;
    int mode;
} settings;

typedef struct _thread{
    pthread_t t;
    uint64_t tps;
} thread;
enum {
    STANDARD,
    RAND,
};
static volatile sig_atomic_t recieved_signal;
static settings *setting = (settings[]){{0}};
static char buffer[BUFFER_SIZE];
static uint32_t flag;
static memcached_st *server;
static thread tarray[THREAD_NUMS];
static thread garray[THREAD_NUMS];

static uint32_t* pri_buffer;
/**
 * 检查数组的正序
 */
static int check_down_seq(uint32_t *p, size_t n){
	if(n <= 1)
		return 0;
	ssize_t t = -1;
	size_t i = 0;
	for(;i < n - 1;++i) {
		if(t != -1 && t < p[i]) return 1;
		t = p[i];
	}
	return 0;
}

static void sigint(int signal){
    if(signal == SIGINT)
        recieved_signal |= 1;
    else if(signal == SIGALRM)
        recieved_signal |= 2;
}
/**
 * 初始化一个到指定ip和port的实例
 */
static memcached_st *memcached_connect(const char *ip, unsigned int port){
    memcached_st *server;
    server = memcached_create(NULL);
    memcached_server_add(server, ip, port);
    return server;
}
/**
 * 写的线程
 */
static void* setter(void* arg){
    memcached_st *server = memcached_connect(setting->ip, setting->port);
    size_t id = (size_t)arg;
    char key[KEY_SIZE];
    memcached_return ret;
    unsigned long long i = 0;
    int l, rnd;
	uint32_t priority;
    for(;setting->mode == RAND || i < setting->times; ++i){
        if(recieved_signal != 0)
            return NULL;
        rnd = rand();
		priority = rnd % sizeof(buffer) + 1;
        l = snprintf(key, KEY_SIZE, "%ld-%lld_%ld", (long)id, i, priority);
        ret = memcached_add(server, key, l, buffer, rnd % sizeof(buffer), 0, priority);
        while( ret != MEMCACHED_SUCCESS ){
			l = snprintf(key, KEY_SIZE, "%ld-%lld_%ld", (long)id, i, priority);
            if(recieved_signal != 0)
                return NULL;
            ret = memcached_add(server, key, l, buffer, rnd % sizeof(buffer), 0, priority);
        }
        tarray[id].tps++;
    }
    return NULL;
}

/**
 * 读的线程
 */
static void* getter(void* arg){
    memcached_st *server = memcached_connect(setting->ip, setting->port);
    char *key = MPQ_KEY, *value;
    memcached_return ret;
    size_t id = (size_t)arg;
    size_t value_length;
    uint32_t flag;
    int l;
    for(;;){
        if(recieved_signal !=0)
            return NULL;
        flag = 0;
        value = memcached_get(server, key, sizeof(MPQ_KEY) - 1, &value_length, &flag, &ret);
        if(flag > 0){
			if(setting->mode == STANDARD){
				pri_buffer[garray[id].tps] = flag;
			}
            garray[id].tps++;
            if(setting->mode == STANDARD && garray[id].tps == setting->times){
                return NULL;
            }
        }
    }
    return NULL;
}

static buffer_init(char* buffer, size_t size){
    memset(buffer, 'a', size);
}

static void setting_init(settings* s){
    s->ip       = S_IP;
    s->port     = S_PORT;
    s->times    = TIMES;
    s->snums    = THREAD_NUMS;
    s->gnums    = THREAD_G_NUMS;
    s->mode     = STANDARD;
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    setting_init(setting);
    int c;
    while (-1 != (c = getopt(argc, argv,
                    "s:"
                    "M::"
                    "g:"
                    "t:"
                    "h:"
                    "p:") ) ){
        switch (c){
            case 's':
                setting->snums = atoi(optarg);
                break;
            case 'g':
                setting->gnums = atoi(optarg);
                break;
            case 't':
                setting->times = (unsigned long long)atoi(optarg);
                break;
            case 'M':
                setting->mode = RAND;
                break;
            case 'p':
                setting->port = atoi(optarg);
                break;
            case 'h':
                setting->ip = strdup(optarg);
                break;
        }
    }
    //初始化数据缓冲区
    buffer_init(buffer, BUFFER_SIZE);
    //捕获两个信号
    signal(SIGINT, sigint);
    signal(SIGALRM, sigint);
    static int i;
    static struct timeval b, e;
    gettimeofday(&b, NULL);
    //如果是随机模式就设置闹钟
    if(setting->mode == RAND && setting->times > 0){
        alarm((unsigned int)setting->times);
    }
    //标准模式
    if(setting->mode == STANDARD){
		pri_buffer = calloc(setting->times, sizeof(uint32_t));
        //依次创建写和读的线程
        pthread_create(&tarray[0].t, NULL, setter, (void*)i);
        pthread_join(tarray[0].t, NULL);
        pthread_create(&garray[0].t, NULL, getter, (void*)i);
        pthread_join(garray[0].t, NULL);
    } else {
        for(i = 0;i < setting->snums; ++i){
            pthread_create(&tarray[i].t, NULL, setter, (void*)i);
        }
        for(i = 0;i < setting->gnums; ++i){
            pthread_create(&garray[i].t, NULL, getter, (void*)i);
        }

        for(i = 0;setting->snums && i < setting->snums; ++i){
            pthread_join(tarray[i].t, NULL);
        }
        for(i = 0;setting->gnums && i < setting->gnums; ++i){
            pthread_join(garray[i].t, NULL);
        }
    }
    if(setting->mode == RAND) {
        while(recieved_signal == 0) {
            pause();
        }
    }
    gettimeofday(&e, NULL);
    static uint64_t sts, gts, tps;
    long sec = ((e.tv_sec * 1000 + e.tv_usec /1000) - (b.tv_sec * 1000 + b.tv_usec /1000) )/1000;
    if(setting->mode == STANDARD){
        for(i = 0;i < setting->snums; ++i){
            sts += tarray[i].tps;
        }
        for(i = 0;i < setting->gnums; ++i){
            gts += garray[i].tps;
        }
		if( 0 != check_down_seq(pri_buffer, setting->times)){
			printf("wrong seq\n");
			return 1;
		}
    } else {
        sts = tarray[0].tps;
        gts = garray[0].tps;
    }
    tps = gts + sts;
    printf("time:%ld\
            add:%"PRId64"\
            get:%"PRId64"\
            tps:%"PRId64"\n",
            sec, sts, gts, sec > 0 ? tps/sec : tps);
}

//gcc test_pq.c -L/usr/local/libmemcached/lib -lmemcached -I/usr/local/libmemcached/include/ -Wl,-rpath -Wl,/usr/local/libmemcached/lib
