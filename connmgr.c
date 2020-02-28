#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>
#include <inttypes.h>

#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include "config.h"
#include "connmgr.h"
#include "errmacros.h"

typedef struct pollfd poll_fd_t;

typedef struct var {
    dplist_t* list;
    tcpsock_t* server;
    poll_fd_t* poll_fd;
    int poll_max;
} var_t;

typedef struct node {
    tcpsock_t* client_socket;
    int socket_fd;
    sensor_data_t data;
} node_t;

static void handle_socket(sbuffer_t* buffer);
static void open_new_connection();
static void collect_data_from_socket(int* poll_idx, sbuffer_t* buffer);
static void close_connection(node_t* node, int* poll_idx);
static void insert_into_list(tcpsock_t* client, int* socket_fd);
static node_t* find_node_from_poll_index(int* poll_idx);
static int receive_data(tcpsock_t* client, sensor_data_t* data, int* bytes);
static void node_free(void** node);
static int node_compare(void* x, void* y);
static var_t* get_var();


void connmgr_listen(int port_number, sbuffer_t** buffer) {

    var_t* var = get_var();
    int socket_fd;

    TCP_ERR( tcp_passive_open(&var->server, port_number) );
    TCP_ERR( tcp_get_sd(var->server, &socket_fd) );

	var->poll_fd = calloc(2, sizeof(poll_fd_t));
    ALLOC_ERR(var->poll_fd);
    var->poll_fd->fd = socket_fd;
    var->poll_fd->events = POLLIN;
    var->poll_max = 1;

    var->list = dpl_create(NULL, &node_free, &node_compare);

    while (1){
        
        int rc = poll(var->poll_fd, var->poll_max, TIMEOUT * 1000);
        SYS_ERR(rc);
        if (rc == 0 && dpl_size(var->list) == 0)
            break;

        handle_socket(*buffer);
    }
    printf("\nYour session has expired\n");
}

void connmgr_free() {

    var_t* var = get_var();

	REALLOC_CHECK( var->poll_fd, sizeof(poll_fd_t) );
    dpl_free(&var->list, true);
	free(var->poll_fd);
    TCP_ERR( tcp_close(&var->server) );
    free(var);
}

void handle_socket(sbuffer_t* buffer) {

    var_t* var = get_var();

    if (var->poll_fd[0].revents & POLLIN) {
        open_new_connection();
        return;
    }

    for (int poll_idx = 1; poll_idx < var->poll_max; poll_idx++) {

        if (var->poll_fd[poll_idx].fd > 0) {
            node_t* node = find_node_from_poll_index(&poll_idx);
            if (time(NULL) - node->data.ts >= TIMEOUT) {
                close_connection(node, &poll_idx);
                continue;
            }
        }

        if (var->poll_fd[poll_idx].revents & POLLIN && dpl_size(var->list) > 0)
            collect_data_from_socket(&poll_idx, buffer);
    }
}

void open_new_connection() {

    var_t* var = get_var();
	tcpsock_t* client;
	int socket_fd, poll_idx = 0;

	TCP_ERR( tcp_wait_for_connection(var->server, &client) );
	TCP_ERR( tcp_get_sd(client, &socket_fd) );

	insert_into_list(client, &socket_fd);

	while (var->poll_fd[poll_idx].fd != -1 && poll_idx != var->poll_max)
		poll_idx++;

	if (poll_idx == var->poll_max) {
		REALLOC_CHECK( var->poll_fd, (var->poll_max + 2) * sizeof(poll_fd_t) );
		var->poll_fd[var->poll_max + 1].fd = 0;
		(var->poll_max)++;
	}

	var->poll_fd[poll_idx].fd = socket_fd;
	var->poll_fd[poll_idx].events = POLLIN;

	DEBUG_PRINTF("Poll index %d with socket fd = %d has opened the socket\n", poll_idx, socket_fd);
}

void collect_data_from_socket(int* poll_idx, sbuffer_t* buffer) {

    sensor_data_t data;
	int data_size;

	node_t* node = find_node_from_poll_index(poll_idx);
    tcpsock_t* client = node->client_socket;

    int rc = receive_data(client, &data, &data_size);

    if (rc == TCP_NO_ERROR && data_size != 0) {

        if (node->data.id == 0)
            LOG_PRINTF("A sensor node with %d has opened a new connection\n", data.id);

        node->data.id = data.id;
        node->data.value = data.value;
        node->data.ts = data.ts;

        printf("\tSensor id = %" PRIu16 "\tTemperature = %g\tTimestamp = %ld\n", data.id, data.value, (long int)data.ts);

        sbuffer_insert(buffer, &data);
    }
    else if (rc == TCP_CONNECTION_CLOSED)
		close_connection(node, poll_idx);
    else
		TCP_ERR(rc);
}

void close_connection(node_t* node, int* poll_idx) {

    var_t* var = get_var();

    LOG_PRINTF("The sensor node with %d has closed the connection\n", node->data.id);
    DEBUG_PRINTF("Poll index %d with socket fd = %d has closed the socket\n", *poll_idx, node->socket_fd);

	TCP_ERR( tcp_close( &(node->client_socket) ) );
	dpl_remove_at_index(var->list, dpl_get_index_of_element(var->list, node), true);

	var->poll_fd[*poll_idx].fd = -1;
	if (*poll_idx == (var->poll_max) - 1)
		(var->poll_max)--;

}

void insert_into_list(tcpsock_t* client, int* socket_fd) {

    var_t* var = get_var();

	node_t* node = calloc(1, sizeof(node_t));
    ALLOC_ERR(node);
	node->client_socket = client;
	node->socket_fd = *socket_fd;
    node->data.ts = time(NULL);

	dpl_insert_at_index(var->list, node, -1, false);

}

node_t* find_node_from_poll_index(int* poll_idx) {

    var_t* var = get_var();

	node_t* dummy = malloc(sizeof(node_t));
    ALLOC_ERR(dummy);
	dummy->socket_fd = var->poll_fd[*poll_idx].fd;

	int node_idx = dpl_get_index_of_element(var->list, dummy);
	node_t* node = dpl_get_element_at_index(var->list, node_idx);
	free(dummy);

	return node;
}

int receive_data(tcpsock_t* client, sensor_data_t* data, int* bytes) {

	*bytes = sizeof(data->id);
    tcp_receive(client, &(data->id), bytes);
    *bytes = sizeof(data->value);
	tcp_receive(client, &(data->value), bytes);
    *bytes = sizeof(data->ts);

	return tcp_receive(client, &(data->ts), bytes);
}

void node_free(void** node) {
	free(*node);
}

int node_compare(void* node_1, void* node_2) {

	int fd_1 = ((node_t*)node_1)->socket_fd;
	int fd_2 = ((node_t*)node_2)->socket_fd;
	if (fd_1 < fd_2) return -1;
	else if (fd_1 == fd_2) return 0;
	else return 1;
}

var_t* get_var() {

    static var_t* var;
    if (var == NULL) {
        var = malloc(sizeof(var_t));
        ALLOC_ERR(var);
    }
    return var;
}
