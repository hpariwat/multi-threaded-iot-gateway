#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include <pthread.h>
#include "config.h"

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1

typedef struct sbuffer sbuffer_t;
typedef struct sbuffer_data sbuffer_data_t;
typedef struct sbuffer_node sbuffer_node_t;
typedef struct sbuffer_pthread sbuffer_pthread_t;
typedef struct sbuffer_num sbuffer_num_t;

struct sbuffer_pthread {
    pthread_mutex_t main_key;
    pthread_mutex_t write_key;
    pthread_cond_t buffer_not_empty;
    pthread_cond_t allow_remove;
    pthread_barrier_t barrier;
};

struct sbuffer_num {
    int initialize;
    int terminate;
};

struct sbuffer_data {
    sensor_data_t data;
};

struct sbuffer_node {
    struct sbuffer_node * next;
    sbuffer_data_t element;
    int allow_remove;
};

struct sbuffer {
    sbuffer_node_t * head;
    sbuffer_node_t * mid;
    sbuffer_node_t * tail;
    sbuffer_pthread_t pthr;
    sbuffer_num_t num;
};

int sbuffer_init(sbuffer_t ** buffer);
int sbuffer_free(sbuffer_t ** buffer);
int sbuffer_remove(sbuffer_t * buffer, sensor_data_t * data);
int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data);
int sbuffer_check_buffer(sbuffer_t* buffer, int check_head);
int sbuffer_read(sbuffer_t* buffer, sensor_data_t* data);

#endif  //_SBUFFER_H_
