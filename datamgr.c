#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>

#include "lib/dplist.h"
#include "datamgr.h"
#include "errmacros.h"

typedef uint16_t room_id_t;

typedef struct var {
    dplist_t* list;
} var_t;

typedef struct node {
	sensor_id_t sensor_id;
	room_id_t room_id;
	sensor_ts_t last_modified;
	sensor_value_t running_avg;
	sensor_value_t running_buffer[RUN_AVG_LENGTH];
	int running_index;
  	char buffer_is_full;
} node_t;

static void node_free(void** element);
static int node_compare(void* x, void* y);
static void create_node(int* room_id, int* sensor_id);
static node_t* get_node_from_sensor_id(sensor_id_t sensor_id);
static void calculate_running_average(node_t* node);
static void process_data(node_t* node, sensor_data_t data);
static var_t* get_var();


void datamgr_parse_sensor_files(FILE* fp_sensor_map, FILE* fp_sensor_data) {

	var_t* var = get_var();
	sensor_data_t data;
	int room_id, sensor_id;

	var->list = dpl_create(NULL, &node_free, &node_compare);

	while (fscanf(fp_sensor_map, "%" PRIu16 " %" PRIu16 "\n", &room_id, &sensor_id) == 2)
		create_node(&room_id, &sensor_id);

	while (fread(&data.id, sizeof(sensor_id_t), 1, fp_sensor_data) == 1) {
		fread(&data.value, sizeof(sensor_value_t), 1, fp_sensor_data);
		fread(&data.ts, sizeof(sensor_ts_t), 1, fp_sensor_data);

        node_t* node = get_node_from_sensor_id(data.id);
		if (node)
            process_data(node, data);
	}
}

void datamgr_parse_sensor_data(FILE* fp_sensor_map, sbuffer_t** buffer) {

	var_t* var = get_var();
    sensor_data_t data;
	int room_id, sensor_id;

	var->list = dpl_create(NULL, &node_free, &node_compare);

	while (fscanf(fp_sensor_map, "%" PRIu16 " %" PRIu16 "\n", &room_id, &sensor_id) == 2)
        create_node(&room_id, &sensor_id);

	while (*buffer != NULL) {

        if ( sbuffer_check_buffer(*buffer, 0) == 0 )
            break;

        int rc = sbuffer_read(*buffer, &data);
        SBUFFER_ERR(rc);

        if (rc == SBUFFER_NO_DATA)
            continue;

		node_t* node = get_node_from_sensor_id(data.id);
		if (node)
            process_data(node, data);
	}

}

void datamgr_free() {

	var_t* var = get_var();
	dpl_free(&var->list, true);
	free(var);
}

void process_data(node_t* node, sensor_data_t data) {

    node->last_modified = data.ts;
    node->running_buffer[node->running_index] = data.value;

    calculate_running_average(node);

    if (node->running_avg < SET_MIN_TEMP)
        LOG_PRINTF("The sensor node with %d reports it’s too cold (running avg temperature = %.3f)\n", node->sensor_id, node->running_avg);
    if (node->running_avg > SET_MAX_TEMP)
        LOG_PRINTF("The sensor node with %d reports it’s too hot (running avg temperature = %.3f)\n", node->sensor_id, node->running_avg);

    node->running_index++;

    if (node->running_index == RUN_AVG_LENGTH) {
        node->running_index = 0;
        node->buffer_is_full = 1;
    }
}

void create_node(int* room_id, int* sensor_id) {

	var_t* var = get_var();
	node_t* node = calloc(1, sizeof(node_t));
	ALLOC_ERR(node);

	node->sensor_id = *sensor_id;
	node->room_id = *room_id;
	dpl_insert_at_index(var->list, node, -1, false);
}

node_t* get_node_from_sensor_id(sensor_id_t sensor_id) {

	var_t* var = get_var();
	node_t* dummy = malloc(sizeof(node_t));
	ALLOC_ERR(dummy);

	dummy->sensor_id = sensor_id;
	int index = dpl_get_index_of_element(var->list, dummy);
	free(dummy);

	if (index == -1) {
		LOG_PRINTF("Received sensor data with invalid sensor node ID %d\n", sensor_id);
		return NULL;
	}

	return dpl_get_element_at_index(var->list, index);
}

void calculate_running_average(node_t* node) {

	sensor_value_t sum = 0;

	for (int i = 0; i < RUN_AVG_LENGTH; i++)
		sum += node->running_buffer[i];

	if (node->buffer_is_full == 0)
		node->running_avg = sum / (node->running_index + 1);
	else
		node->running_avg = sum / RUN_AVG_LENGTH;
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id) {

	node_t* node = get_node_from_sensor_id(sensor_id);
	return node->room_id;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id) {

	node_t* node = get_node_from_sensor_id(sensor_id);
	sensor_value_t sum = 0;

	for (int i = 0; i < RUN_AVG_LENGTH; i++)
		sum += node->running_buffer[i];

	return sum / RUN_AVG_LENGTH;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id) {

	node_t* node = get_node_from_sensor_id(sensor_id);
	return node->last_modified;
}

int datamgr_get_total_sensors() {
	var_t* var = get_var();
	return dpl_size(var->list);
}

void node_free(void** element) {
	free(*element);
}

int node_compare(void* node_1, void* node_2) {

	sensor_id_t id_1 = ((node_t*)node_1)->sensor_id;
	sensor_id_t id_2 = ((node_t*)node_2)->sensor_id;
	if (id_1 < id_2) return -1;
	else if (id_1 == id_2) return 0;
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
