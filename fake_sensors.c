#include "logger.h"
#include "fake_sensors.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_SENSORS 4

static int sensors[NUM_SENSORS];


void update_sensors_from_file()
{
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    FILE *fp = fopen(FAKE_SENSORS_FILE, "r");
    assert(fp);
    int value;
    for (int i = 0; i < NUM_SENSORS; ++i) {
        fscanf(fp, "%d", &value);
        sensors[i] = value;
    }
    fclose(fp);
    pthread_mutex_destroy(&mutex);
}

void update_file_from_sensors()
{
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    FILE *fp = fopen(FAKE_SENSORS_FILE, "w");
    assert(fp);
    for (int i = 0; i < NUM_SENSORS; ++i)
        fprintf(fp, "%d ", sensors[i]);
    fclose(fp);
    pthread_mutex_destroy(&mutex);
}

int get_sensor_value(sensor_t sensor)
{
    update_sensors_from_file();
    return sensors[sensor];
}

void set_sensor_value(sensor_t sensor, int value)
{
    update_sensors_from_file();
    sensors[sensor] = value;
    update_file_from_sensors();
}

void *handle_sensors(void *args)
{
    while (1) {
        update_sensors_from_file();
        if ((!sensors[R_BOTTOM] || (sensors[T_TOP] && sensors[T_BOTTOM])) && sensors[RELAY]) {
            sensors[RELAY] = 0;
            write_log(INFO, "Pump turned OFF.");
        }
        else if ((sensors[R_BOTTOM] && !(sensors[T_TOP] || sensors[T_BOTTOM])) && !sensors[RELAY]) {
            sensors[RELAY] = 1;
            write_log(INFO, "Pump turned ON.");
        }
        update_file_from_sensors();
        sleep(1);
    }
    return NULL;
}
