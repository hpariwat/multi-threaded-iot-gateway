#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "sbuffer.h"
#include "errmacros.h"

int sbuffer_init(sbuffer_t** buffer) {

    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL)
        return SBUFFER_FAILURE;

    (*buffer)->head = NULL;
    (*buffer)->mid = NULL;
    (*buffer)->tail = NULL;

    (*buffer)->num.initialize = 0;
    (*buffer)->num.terminate = 0;

    PTHR_ERR( pthread_mutex_init( &(*buffer)->pthr.main_key, NULL ) );
    PTHR_ERR( pthread_mutex_init( &(*buffer)->pthr.write_key, NULL ) );
    PTHR_ERR( pthread_cond_init( &(*buffer)->pthr.buffer_not_empty, NULL ) );
    PTHR_ERR( pthread_cond_init( &(*buffer)->pthr.allow_remove, NULL ) );
    PTHR_ERR( pthread_barrier_init( &(*buffer)->pthr.barrier, NULL, 2 ) );

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t** buffer) {

    if ( buffer == NULL ||  *buffer == NULL )
        return SBUFFER_FAILURE;

    while ( (*buffer)->head ) {
        sbuffer_node_t* dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }

    PTHR_ERR( pthread_mutex_destroy( &(*buffer)->pthr.main_key ) );
    PTHR_ERR( pthread_mutex_destroy( &(*buffer)->pthr.write_key ) );
    PTHR_ERR( pthread_cond_destroy( &(*buffer)->pthr.buffer_not_empty ) );
    PTHR_ERR( pthread_cond_destroy( &(*buffer)->pthr.allow_remove ) );
    PTHR_ERR( pthread_barrier_destroy( &(*buffer)->pthr.barrier ) );

    free(*buffer);
    *buffer = NULL;

    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t* buffer, sensor_data_t* data) {

    if (buffer == NULL)
        return SBUFFER_FAILURE;

    if (buffer->head == NULL)
        return SBUFFER_NO_DATA;

    PTHR_ERR( pthread_mutex_lock( &buffer->pthr.main_key ) );
    while ( buffer->head->allow_remove == 0 )
        PTHR_ERR( pthread_cond_wait( &buffer->pthr.allow_remove, &buffer->pthr.main_key ) );

    *data = buffer->head->element.data;
    sbuffer_node_t* dummy = buffer->head;

    if (buffer->head == buffer->tail)
        buffer->head = buffer->tail = NULL;
    else
        buffer->head = buffer->head->next;

    free(dummy);
    PTHR_ERR( pthread_mutex_unlock( &buffer->pthr.main_key ) );

    return SBUFFER_SUCCESS;
}

int sbuffer_read(sbuffer_t* buffer, sensor_data_t* data) {

    if (buffer == NULL)
        return SBUFFER_FAILURE;

    if (buffer->mid == NULL)
        return SBUFFER_NO_DATA;

    PTHR_ERR( pthread_mutex_lock( &buffer->pthr.main_key ) );
    *data = buffer->mid->element.data;
    buffer->mid->allow_remove = 1;

    if (buffer->mid == buffer->tail)
        buffer->mid = NULL;
    else
        buffer->mid = buffer->mid->next;

    PTHR_ERR( pthread_cond_broadcast( &buffer->pthr.allow_remove ) );
    PTHR_ERR( pthread_mutex_unlock( &buffer->pthr.main_key ) );

    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t* buffer, sensor_data_t* data) {

    if (buffer == NULL)
        return SBUFFER_FAILURE;

    sbuffer_node_t* dummy = malloc(sizeof(sbuffer_node_t));

    if (dummy == NULL)
        return SBUFFER_FAILURE;

    dummy->element.data = *data;
    dummy->next = NULL;
    dummy->allow_remove = 0;

    PTHR_ERR( pthread_mutex_lock( &buffer->pthr.write_key ) );
    if (buffer->tail == NULL) {
        buffer->head = buffer->mid = buffer->tail = dummy;
    } else {
        buffer->tail->next = dummy;
        buffer->tail = dummy;
        if (buffer->mid == NULL)
            buffer->mid = dummy;
    }

    PTHR_ERR( pthread_cond_broadcast( &buffer->pthr.buffer_not_empty ) );
    PTHR_ERR( pthread_mutex_unlock( &buffer->pthr.write_key ) );

    return SBUFFER_SUCCESS;
}

int sbuffer_check_buffer(sbuffer_t* buffer, int check_head) {

    PTHR_ERR( pthread_mutex_lock( &buffer->pthr.main_key ) );
    while ( (buffer->head == NULL && check_head) || (buffer->mid == NULL && !check_head) ) {
        if ( buffer->num.terminate == 1 ) {
            PTHR_ERR( pthread_mutex_unlock( &buffer->pthr.main_key ) );
            return 0;
        }
        PTHR_ERR( pthread_cond_wait( &buffer->pthr.buffer_not_empty, &buffer->pthr.main_key ) );
    }
    PTHR_ERR( pthread_mutex_unlock( &buffer->pthr.main_key ) );

    return 1;
}
