#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define LOG_LENGTH 500 // log file size
#define SQL_ATTEMPT 3 // number of attempts to try to join the SQL server
#define CLEAR_DATABASE 1 // set to 1 to clear the database

#define FIFO_NAME "logFifo"
#define MAP_NAME "room_sensor.map"
#define LOG_NAME "gateway.log"

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;

typedef struct {
  sensor_id_t id;
  sensor_value_t value;
  sensor_ts_t ts;
} sensor_data_t;


void log_write(char* log);


#endif /* _CONFIG_H_ */
