#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <sqlite3.h>

#include "sensor_db.h"
#include "errmacros.h"

static int execute_query(DBCONN* conn, char* sql, callback_t f, void* arg);
static int check_table(DBCONN* conn, callback_t f);
static int get_table(void *arg, int count, char **value, char **name);


void storagemgr_parse_sensor_data(DBCONN* conn, sbuffer_t** buffer) {

    sensor_data_t data;

	while (*buffer != NULL) {

        if ( sbuffer_check_buffer(*buffer, 1) == 0 )
            break;

        int rc = sbuffer_remove(*buffer, &data);
        SBUFFER_ERR(rc);

        if (rc == SBUFFER_NO_DATA)
            continue;

        if ( insert_sensor(conn, data.id, data.value, data.ts) != SQLITE_OK )
            break;
    }

    LOG_PRINTF("Connection to SQL server lost\n");
}

DBCONN* init_connection(char clear_up_flag) {

    DBCONN* db;
    char* sql = "";

    int rc = sqlite3_open(TO_STRING(DB_NAME), &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error code %d: %s\n", rc, sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    LOG_PRINTF("Connection to SQL server established\n");

    if ( check_table(db, &get_table) == 0) {

        ASPRINTF_ERR( asprintf(&sql, "CREATE TABLE %s ("
        "id             INTEGER PRIMARY KEY AUTOINCREMENT, "
        "sensor_id      INT, "
        "sensor_value   DECIMAL(4,2), "
        "timestamp      TIMESTAMP );", TO_STRING(TABLE_NAME)) );

        rc = execute_query(db, sql, 0, NULL);
        LOG_PRINTF("New table %s created\n", TO_STRING(TABLE_NAME));

    } else if (clear_up_flag) {

        ASPRINTF_ERR( asprintf(&sql, "DELETE FROM %s;", TO_STRING(TABLE_NAME)) );

        rc = execute_query(db, sql, 0, NULL);
        LOG_PRINTF("Table %s cleared\n", TO_STRING(TABLE_NAME));
    }

    if (rc != SQLITE_OK) {
        LOG_PRINTF("Connection to SQL server lost\n");
        return NULL;
    }

    return db;
}

void disconnect(DBCONN* conn) {

    if (conn == NULL)
        return;

    int rc = sqlite3_close(conn);
    if (rc != SQLITE_OK)
        fprintf(stderr, "SQL error code %d\n", rc);
}

int insert_sensor(DBCONN* conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {

    DEBUG_PRINTF("Inserting data from sensor %d at %ld into the SQL database...\n", id, ts);

    char* sql;
    ASPRINTF_ERR( asprintf(&sql, "INSERT INTO %s (sensor_id, sensor_value, timestamp) VALUES (%d, %lf, %ld);", TO_STRING(TABLE_NAME), id, value, ts) );
    return execute_query(conn, sql, 0, NULL);
}

int find_sensor_all(DBCONN* conn, callback_t f) {

    char* sql;
    ASPRINTF_ERR( asprintf(&sql, "SELECT * FROM %s;", TO_STRING(TABLE_NAME)) );
    return execute_query(conn, sql, f, NULL);
}

int find_sensor_by_value(DBCONN* conn, sensor_value_t value, callback_t f) {

    char* sql;
    ASPRINTF_ERR( asprintf(&sql, "SELECT * FROM %s WHERE sensor_value = %f;", TO_STRING(TABLE_NAME), value) );
    return execute_query(conn, sql, f, NULL);
}

int find_sensor_exceed_value(DBCONN* conn, sensor_value_t value, callback_t f) {

    char* sql;
    ASPRINTF_ERR( asprintf(&sql, "SELECT * FROM %s WHERE sensor_value > %f;", TO_STRING(TABLE_NAME), value) );
    return execute_query(conn, sql, f, NULL);
}

int find_sensor_by_timestamp(DBCONN* conn, sensor_ts_t ts, callback_t f) {

    char* sql;
    ASPRINTF_ERR( asprintf(&sql, "SELECT * FROM %s WHERE timestamp = %ld;", TO_STRING(TABLE_NAME), ts) );
    return execute_query(conn, sql, f, NULL);
}

int find_sensor_after_timestamp(DBCONN* conn, sensor_ts_t ts, callback_t f) {

    char* sql;
    ASPRINTF_ERR( asprintf(&sql, "SELECT * FROM %s WHERE timestamp > %ld;", TO_STRING(TABLE_NAME), ts) );
    return execute_query(conn, sql, f, NULL);
}

int execute_query(DBCONN* conn, char* sql, callback_t f, void* arg) {

    char* err_msg;
    int rc = sqlite3_exec(conn, sql, f, arg, &err_msg);
    free(sql);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error code %d: %s\n", rc, err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(conn);
        return rc;
    }

    return SQLITE_OK;
}

int check_table(DBCONN* conn, callback_t f) {

    char* sql;
    int table_exist = 0;
    ASPRINTF_ERR( asprintf(&sql, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", TO_STRING(TABLE_NAME)) );
    execute_query(conn, sql, f, &table_exist);
    return table_exist;
}

int get_table(void* table_exist, int count, char** value, char** name) {

    if (count >= 1)
        *(int*)table_exist = 1;
    return 0;
}
