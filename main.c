#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "errmacros.h"

typedef struct var {
    sbuffer_t* buffer;
    int port_number;
} var_t;

typedef struct fifo {
    FILE* fp;
    pthread_mutex_t key;
} fifo_t;

static void run_main_process(int* port_number, int* pipe_fd);
static void run_log_process(int* pipe_fd);
static void* connmgr(void* port_number);
static void* datamgr(void* null);
static void* strmgr(void* null);
static void try_connect(DBCONN* db, sbuffer_t* buffer);
static void start_gateway(sbuffer_t* buffer);
static void kill_gateway(sbuffer_t* buffer);
static void print_help();
static fifo_t* get_fifo();


int main( int argc, char *argv[] ) {

    int port_number, pipe_fd[2];

    if (argc != 2)
        print_help();

    port_number = atoi(argv[1]);

    remove(FIFO_NAME);
    MKFIFO_ERR( mkfifo(FIFO_NAME, 0666) );
    SYS_ERR( pipe(pipe_fd) );

    pid_t log_pid = fork();
    SYS_ERR(log_pid);

    if (log_pid != 0)
        run_main_process(&port_number, pipe_fd);
    else
        run_log_process(pipe_fd);

    return 0;
}

void run_main_process(int* port_number, int* pipe_fd) {

    DEBUG_PRINTF("Main process is starting...\n");

    sbuffer_t* buffer;
    pthread_t connmgr_id, datamgr_id, strmgr_id;

    close(pipe_fd[0]);

    fifo_t* fifo = get_fifo();
    fifo->fp = fopen(FIFO_NAME, "w");
    FILE_OPEN_ERR(fifo->fp, FIFO_NAME);
    PTHR_ERR( pthread_mutex_init( &fifo->key, NULL ) );

    SBUFFER_ERR( sbuffer_init(&buffer) );
    PTHR_ERR( pthread_create(&strmgr_id, NULL, &strmgr, buffer) );
    BARRIER_ERR( pthread_barrier_wait( &buffer->pthr.barrier) );

    if ( buffer->num.initialize ) {

        var_t* var = malloc(sizeof(var_t));
        ALLOC_ERR(var);
        var->buffer = buffer;
        var->port_number = *port_number;

        PTHR_ERR( pthread_create(&connmgr_id, NULL, &connmgr, var) );
        PTHR_ERR( pthread_create(&datamgr_id, NULL, &datamgr, buffer) );

        PTHR_ERR( pthread_join(connmgr_id, NULL) );
        kill_gateway(buffer);
        PTHR_ERR( pthread_join(datamgr_id, NULL) );

        free(var);
    }

    PTHR_ERR( pthread_join(strmgr_id, NULL) );
    SBUFFER_ERR( sbuffer_free(&buffer) );

    fclose(fifo->fp);
    FILE_CLOSE_ERR(fifo->fp , FIFO_NAME);
    PTHR_ERR( pthread_mutex_destroy( &fifo->key ) );
    free(fifo);

    int kill_log = 1;
    write(pipe_fd[1], &kill_log, sizeof(kill_log));
    close(pipe_fd[1]);

    SYS_ERR( wait(NULL) );

    DEBUG_PRINTF("Main process is exiting...\n");
}

void run_log_process(int* pipe_fd) {

    DEBUG_PRINTF("Log process is starting...\n");

    char log_buf[LOG_LENGTH], time_buf[20];
    int sequence = 0, kill_log = 0;

    close(pipe_fd[1]);

    FILE* fp_fifo = fopen(FIFO_NAME, "r");
    FILE_OPEN_ERR(fp_fifo, FIFO_NAME);

    FILE* fp_log = fopen(LOG_NAME, "w");
    FILE_OPEN_ERR(fp_log, LOG_NAME);

    while (kill_log == 0) {
        if (fgets(log_buf, LOG_LENGTH, fp_fifo) != NULL) {
            sequence++;
            time_t cur_time = time(NULL);
            strftime(time_buf, 20, "%Y-%m-%d %X", localtime(&cur_time));
            fprintf(fp_log, "%d %s %s", sequence, time_buf, log_buf);
        } else {
            read(pipe_fd[0], &kill_log, sizeof(kill_log));
        }
    }

    close(pipe_fd[0]);

    fclose(fp_log);
    FILE_CLOSE_ERR(fp_log, LOG_NAME);

    fclose(fp_fifo);
    FILE_CLOSE_ERR(fp_fifo, FIFO_NAME);

    DEBUG_PRINTF("Log process is exiting...\n");
}

void* connmgr(void* ptr) {

    DEBUG_PRINTF("Connmgr thread is starting...\n");

    var_t* var = (var_t*)ptr;

    connmgr_listen(var->port_number, &var->buffer);
    connmgr_free();

    DEBUG_PRINTF("Connmgr thread is exiting...\n");
    pthread_exit(NULL);
}

void* datamgr(void* ptr) {

    DEBUG_PRINTF("Datamgr thread is starting...\n");

    sbuffer_t* buffer = (sbuffer_t*)ptr;

    FILE* fp_map = fopen(MAP_NAME, "r");
    FILE_OPEN_ERR(fp_map, MAP_NAME);

    datamgr_parse_sensor_data(fp_map, &buffer);
    datamgr_free();

    fclose(fp_map);
    FILE_CLOSE_ERR(fp_map, MAP_NAME);

    DEBUG_PRINTF("Datamgr thread is exiting...\n");
    pthread_exit(NULL);
}

void* strmgr(void* ptr) {

    DEBUG_PRINTF("Strmgr thread is starting...\n");

    sbuffer_t* buffer = (sbuffer_t*)ptr;

    DBCONN* db = NULL;
    for (int attempt = 0; attempt < SQL_ATTEMPT; attempt++) {
        try_connect(db, buffer);
        LOG_PRINTF("Trying to connect to SQL server... Attempt %d\n", attempt + 1);
    }

    LOG_PRINTF("Unable to connect to SQL server\n");

    BARRIER_ERR( pthread_barrier_wait( &buffer->pthr.barrier) );

    DEBUG_PRINTF("Strmgr thread is exiting...\n");
    pthread_exit(NULL);
}

void try_connect(DBCONN* db, sbuffer_t* buffer) {

    time_t start_time = time(NULL);
    time_t current_time = start_time;

    while (current_time - start_time <= TIMEOUT) {
        db = init_connection(CLEAR_DATABASE);
        if (db != NULL) {
            start_gateway(buffer);
            storagemgr_parse_sensor_data(db, &buffer);
            disconnect(db);
            DEBUG_PRINTF("Strmgr thread exiting...\n");
            pthread_exit(NULL);
        }
        current_time = time(NULL);
    }
}

void start_gateway(sbuffer_t* buffer) {

    buffer->num.initialize = 1;
    DEBUG_PRINTF("Signal to start all threads sent...\n");

    BARRIER_ERR( pthread_barrier_wait( &buffer->pthr.barrier) );
}

void kill_gateway(sbuffer_t* buffer) {

    PTHR_ERR( pthread_mutex_lock( &buffer->pthr.main_key ) );

    buffer->num.terminate = 1;
    DEBUG_PRINTF("Signal to kill all threads sent...\n");
    PTHR_ERR( pthread_cond_broadcast( &buffer->pthr.buffer_not_empty ) );

    PTHR_ERR( pthread_mutex_unlock ( &buffer->pthr.main_key ) );
}

fifo_t* get_fifo() {

    static fifo_t* fifo;
    if (fifo == NULL) {
        fifo = malloc(sizeof(fifo_t));
        ALLOC_ERR(fifo);
    }
    return fifo;
}

void log_write(char* log) {

    fifo_t* fifo = get_fifo();

    PTHR_ERR( pthread_mutex_lock( &fifo->key ) );

    printf("\n%s\n", log);
    fputs(log, fifo->fp);
    fflush(fifo->fp);

    PTHR_ERR( pthread_mutex_unlock( &fifo->key ) );
}

void print_help() {
    printf("Use this program with 2 command line options: \n");
    printf("\t%-15s : a unique port number\n", "\'PORT\'");
    exit(EXIT_SUCCESS);
}
