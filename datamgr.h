#ifndef DATAMGR_H
#define DATAMGR_H

#include <time.h>
#include <stdio.h>

#include "config.h"
#include "sbuffer.h"

#ifndef RUN_AVG_LENGTH
  #define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
  #error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
  #error SET_MIN_TEMP not set
#endif


void datamgr_parse_sensor_files(FILE * fp_sensor_map, FILE * fp_sensor_data);
void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t ** buffer);
void datamgr_free();
uint16_t datamgr_get_room_id(sensor_id_t sensor_id);
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);
time_t datamgr_get_last_modified(sensor_id_t sensor_id);
int datamgr_get_total_sensors();


#endif /* DATAMGR_H */
