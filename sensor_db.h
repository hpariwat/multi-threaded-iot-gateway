#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "config.h"
#include "sbuffer.h"

#define REAL_TO_STRING(s) #s
#define TO_STRING(s) REAL_TO_STRING(s)

#ifndef DB_NAME
  #define DB_NAME Sensor.db
#endif

#ifndef TABLE_NAME
  #define TABLE_NAME SensorData
#endif

#define DBCONN sqlite3

typedef int (*callback_t)(void *, int, char **, char **);

void storagemgr_parse_sensor_data(DBCONN * conn, sbuffer_t ** buffer);
DBCONN * init_connection(char clear_up_flag);
void disconnect(DBCONN *conn);
int insert_sensor(DBCONN * conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts);
int find_sensor_all(DBCONN * conn, callback_t f);
int find_sensor_by_value(DBCONN * conn, sensor_value_t value, callback_t f);
int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f);
int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f);
int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f);

#endif /* _SENSOR_DB_H_ */
